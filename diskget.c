/* 
 * Implementation of diskget. Searches whole filesystem for file
*/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <regex.h>
#include <ctype.h>

#include "FATheaders.h"
#include "ADTlinkedlist.h"
#include "utils.h"


/* Check next file entry for file, if matches sets entry to result
 * Returns 0 if end of disk, 1 otherwise */
int search_next_entry(FILE * disk, ADTlinkedlist * subdirs, FATdircompare * expected_dir, FATdirectory ** entry) {

    uint8_t dir_buff[FAT_DIRECTORY_SIZE];
    FATdirectory dir_entry;

    xfread(dir_buff, FAT_DIRECTORY_SIZE,1, disk);
    fatUnpackDirectory(&dir_entry,dir_buff);

    if( dir_entry.filename[0] == 0x00 ) return 0; //end of directory case

    if( dir_entry.filename[0] !=  0xEF
            && dir_entry.filename[0] != '.'
            && dir_entry.first_logical_cluster > 1
            && dir_entry.attributes != 0x0F //not all bits est
            && !(dir_entry.attributes & 0x08)) { //not system

        if( dir_entry.attributes & 0x10) { //save directory for recurse
            ADTlinkednode * node = xmalloc(sizeof(ADTlinkednode));
            FATdirectory * subdir = xmalloc(sizeof(FATdirectory));
            memcpy(subdir,&dir_entry,sizeof(FATdirectory));
            adtInitiateLinkedNode(node, subdir);
            adtAddEndLinkedNode(subdirs, node);
        } else if( fatCompareEntries(&dir_entry,expected_dir) == 0) {
            *entry = xmalloc(sizeof(FATdirectory));
            memcpy(*entry,&dir_entry,sizeof(FATdirectory));
        }
    }

    return 1;
}


int main(int argc, char * argv[]) {

    if( argc != 3 ) {
        printf("Usage: ./diskget <disk> <filename> \n");
        return 1;
    }

    FILE * disk = fopen(argv[1],"r");
    if( !disk) {
        perror("Opening disk failed:");
        return 3;
    }

    regex_t preg;
    regmatch_t matches[3];

    char * pattern = "^([[:alpha:][:digit:]]{1,10}).?([[:alpha:][:digit:]]{0,3})$";
    if( regcomp(&preg, pattern,REG_EXTENDED) ) {
        printf("Compiling regex failed!");
        return 5;
    }

    if( regexec(&preg,argv[2],3, matches,0) ) {
        printf("Invalid name\nThe name of file was %s\n",argv[2]);
        return 4;
    }

    regfree(&preg);

    FATdircompare expected_dir;

    int i;
    for( i = 0; i < matches[1].rm_eo - matches[1].rm_so; i++ ) expected_dir.filename[i] = toupper(argv[2][matches[1].rm_so + i]);
    for( i = matches[1].rm_eo - matches[1].rm_so; i < 8; i++) expected_dir.filename[i] = 0x20; //pad with 0x20

    for( i = 0; i < matches[2].rm_eo - matches[2].rm_so; i++ ) expected_dir.extention[i] = toupper(argv[2][matches[2].rm_so + i]);
    for( i = matches[2].rm_eo - matches[2].rm_so; i < 3; i++) expected_dir.extention[i] = 0x20; //pad with 0x20


    FATboot * boot = fatGetBootInfo(disk);

    ADTlinkedlist subdirs;
    adtInitiateLinkedList(&subdirs); //for directories to recurse... in order traversal


    xfseek(disk, fatGetRootStart(boot),SEEK_SET); //start of root

    FATdirectory * entry = NULL;

    /* Perform a traversal of filesystem untill file is found or all places are searched */

    /* Root directory search */
    int entries_read;
    for( entries_read=0; entries_read <  boot->max_root_entries; entries_read++) {
        if( search_next_entry(disk,&subdirs,&expected_dir,&entry) != 1 || entry ) {
            break;
        }
    }

    /* Subdirectory search */
    while( subdirs.num > 0 && !entry ) {

        ADTlinkednode * node = adtPopLinkedNode(&subdirs,0);
        FATdirectory * curr_dir = node->val;
        uint16_t curr_logical_cluster = curr_dir->first_logical_cluster;

        while( curr_logical_cluster <= 0xFF0 && curr_logical_cluster > 0) { //iterate through all FAT entries

            xfseek(disk, fatGetDataspaceLocation(boot,curr_logical_cluster),SEEK_SET); //go to logical cluster

            for( entries_read=0; entries_read < boot->bytes_per_sector/FAT_DIRECTORY_SIZE ; entries_read++) { //read all entries in cluster
                if( search_next_entry(disk,&subdirs,&expected_dir,&entry) != 1 || entry ) {
                    goto break_dir_search;
                }
            }

            curr_logical_cluster = fatGetFatEntry(disk,boot,curr_logical_cluster); //update entry to next value

        }


break_dir_search:
        xfree(node);
        xfree(curr_dir);

    }

    while(subdirs.num > 0 ) { //cleanup queue in case file is found
        ADTlinkednode * node = adtPopLinkedNode(&subdirs,0);
        xfree(node->val);
        xfree(node);
    }

    if( entry ) {
        FILE * out = fopen(argv[2],"w"); //name same as input, with whatever the case
        if(!out){
            perror("Aborting: Failed to open output file for result");
        }

        uint16_t curr_logical_cluster = entry->first_logical_cluster;
        uint32_t file_size = entry->file_size;
        uint32_t num_read = 0;
        uint8_t * file_buff = xmalloc(boot->bytes_per_sector);


        while(curr_logical_cluster <= 0xFF0 && curr_logical_cluster > 0 && num_read < file_size) { /* Copy all of file into new file */

            xfseek(disk, fatGetDataspaceLocation(boot,curr_logical_cluster),SEEK_SET); //go to logical cluster

            int to_read;
            if( file_size - num_read >=  boot->bytes_per_sector ) {
                to_read = boot->bytes_per_sector;
            } else {
                to_read = file_size - num_read;
            }

            num_read += xfread(file_buff, 1, to_read,disk);

            fwrite(file_buff, 1, to_read, out);

            curr_logical_cluster = fatGetFatEntry(disk,boot,curr_logical_cluster);
        }

        xfree(file_buff);

        if( num_read < file_size) {
            printf("Warning corrupted file, not all entries retrieved!\n");
        }
        
        printf("File extracted as %s\n",argv[2]);

    } else {
        printf("File not found\n");
        printf("The name of file was %s\n",argv[2]);
    }

    if(entry) xfree(entry);

    xfree(boot);
    fclose(disk);

    return 0;

}
