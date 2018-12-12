/* Implementation of disput. Takes a file from linux and puts it into the file system (if room)
*/


#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <regex.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "FATheaders.h"
#include "ADTlinkedlist.h"
#include "utils.h"


/* Check next file entry to see if it matches returns logical cluster
 * Returns 0 no match, -1 end of directory, > 1 for match, 0 otherwise */
int search_next_entry(FILE * disk, FATdircompare * expected_dir) {

    uint8_t dir_buff[FAT_DIRECTORY_SIZE];
    FATdirectory dir_entry;

    xfread(dir_buff, FAT_DIRECTORY_SIZE,1, disk);
    fatUnpackDirectory(&dir_entry,dir_buff);

    if( dir_entry.filename[0] == 0x00 ) {
        return -1;
    }

    if( dir_entry.filename[0] !=  0xEF
            && dir_entry.filename[0] != '.'
            && dir_entry.first_logical_cluster > 1
            && dir_entry.attributes & 0x10
            && dir_entry.attributes != 0x0F //not all bit set
            && !(dir_entry.attributes & 0x08) //not system
            && fatCompareEntries(&dir_entry, expected_dir) == 0 ) {

        return dir_entry.first_logical_cluster;
    }

    return 0;
}

/* Check next file entry to see if it is free
 * Returns 1 if free, 0 otherwise and exit on same name */
int check_free_space(FILE * disk, FATdircompare * name) {
    uint8_t dir_buff[FAT_DIRECTORY_SIZE];
    FATdirectory dir_entry; //dir_buffer

    xfread(dir_buff, FAT_DIRECTORY_SIZE, 1, disk);
    fatUnpackDirectory(&dir_entry,dir_buff);

    if( dir_entry.filename[0] == 0xEF || dir_entry.filename[0] == 0x00) {
        return 1;
    } else if( fatCompareEntries(&dir_entry,name) == 0 ) {
        printf("File with same name already exists in directory\n");
        exit(2);
    }

    return 0;
}

int main(int argc, char * argv[]) {

    if( argc != 3 ) {
        printf("Usage: ./diskget <disk> <filename> \n");
        return 1;
    }

    FILE * disk = fopen(argv[1],"r+");
    if( !disk) {
        perror("Aborting: Opening disk failed:");
        return 3;
    }

    FATboot * boot = fatGetBootInfo(disk);

    regex_t preg;
    regmatch_t matches[3];
    char * pattern = "^/?([[:alpha:][:digit:]]{1,8}).?([[:alpha:][:digit:]]{0,3})(/|$)";
    if( regcomp(&preg, pattern,REG_EXTENDED) ) {
        printf("FATAL: Compiling regex failed!");
        return 3;
    }

    ADTlinkedlist path; //path to entry
    adtInitiateLinkedList(&path);

    char * path_point = argv[2];
    while(*path_point) {

        if( regexec(&preg,path_point,3, matches,0) ) {
            printf("Aborting invalid path format provided!\n");
            return 3;
        }

        FATdircompare * entry = xmalloc(sizeof(FATdircompare));

        int j;
        for(j = 0; j < matches[1].rm_eo - matches[1].rm_so; j++ ) entry->filename[j] = toupper(path_point[matches[1].rm_so+j]);
        for( ; j  < (int) sizeof(entry->filename); j++) entry->filename[j] = 0x20; //pad with 0x20

        for( j = 0; j < matches[2].rm_eo - matches[2].rm_so; j++ ) entry->extention[j] = toupper(path_point[matches[2].rm_so + j]);
        for( ; j < (int) sizeof(entry->extention); j++) entry->extention[j] = 0x20; //pad with 0x20

        ADTlinkednode * node = xmalloc(sizeof(ADTlinkednode));
        adtInitiateLinkedNode(node,entry);
        adtAddEndLinkedNode(&path,node);

        path_point += matches[0].rm_eo;
    }


    ADTlinkednode * node = adtPopLinkedNode(&path,path.num-1);//final entry properly formated name, needed for checking final directory
    FATdircompare * name = node->val;


    char * in_filename = path_point - matches[0].rm_eo; //extract filename from last match
    if( *in_filename == '/') in_filename++; //deal with trailing space

    FILE * in_file = fopen(in_filename,"r");
    if( !in_file) {
        perror("Opening input disk failed:");
        printf("Aborting: Input file could not be found\n");
        return 3;
    }

    struct stat in_file_stats;
    if( stat(in_filename,&in_file_stats) ) {
        perror("Aborting: Failed to get stats on input file: ");
        return 3;
    }


    xfseek(disk,fatGetRootStart(boot),SEEK_SET);  //start of root

    int directory_address = 0; //place to put file entry

    if( path.num == 0) { //root directory case
        int entries_read;
        for( entries_read=0; entries_read <  boot->max_root_entries; entries_read++) {
            if( check_free_space(disk,name) == 1 ) {
                directory_address = fatGetRootStart(boot) + entries_read * FAT_DIRECTORY_SIZE;
                break;
            }
        }

        if(!directory_address) {
            printf("Aborting: No room left in root directory\n");
            exit(7);
        }

    } else if( path.num > 0 ) { // subdirectory case

        ADTlinkednode * node = adtPopLinkedNode(&path,0);
        FATdircompare * expected_entry = node->val;
        int curr_logical_cluster = 0; //for searching
        int prev_logical_cluster = 0; //for directory expansion case
        int ret = 0;

        int entries_read;
        for( entries_read=0; entries_read <  boot->max_root_entries; entries_read++) {
            if( (curr_logical_cluster = search_next_entry(disk,expected_entry)) ) break;
        }

        if(curr_logical_cluster <= 0) { //based on return statement
            printf("Aborting: Path cannot be found\n");
            return 7;
        }

        xfree(node);
        xfree(expected_entry);

        while( path.num > 0) {

            node = adtPopLinkedNode(&path,0);
            expected_entry = node->val;

            while( curr_logical_cluster <= 0xFF0 && curr_logical_cluster > 0) { //iterate through all FAT entries of each directory

                xfseek(disk, fatGetDataspaceLocation(boot,curr_logical_cluster),SEEK_SET); //go to logical cluster

                for( entries_read=0; entries_read < boot->bytes_per_sector/FAT_DIRECTORY_SIZE; entries_read++) { //read all entries in cluster
                    if( (ret = search_next_entry(disk,expected_entry)) ) {
                        curr_logical_cluster = ret;
                        goto break_directory_search;
                    }
                }
                curr_logical_cluster = fatGetFatEntry(disk,boot,curr_logical_cluster); //next cluster for a directory
            }
            //if it makes it here it couldn't find the entry name
            printf("Aborting: Path cannot be found!\n");
            return 7;

break_directory_search:
            xfree(node);
            xfree(expected_entry);

        }

        if(curr_logical_cluster <= 1) {
            printf("Aborting: Path cannot be found\n");
            return 7;
        }

        while( curr_logical_cluster <= 0xFF0 && curr_logical_cluster > 0) { //iterate through all FAT entries of each directory

            xfseek(disk, fatGetDataspaceLocation(boot,curr_logical_cluster),SEEK_SET); //go to logical cluster

            for( entries_read=0; entries_read < boot->bytes_per_sector/FAT_DIRECTORY_SIZE ; entries_read++) { //read all entries in cluster
                if( check_free_space(disk,name) == 1 ) {
                    if( !directory_address) {
                        directory_address = fatGetDataspaceLocation(boot,curr_logical_cluster) + entries_read * FAT_DIRECTORY_SIZE;
                    }
                }
            }
            prev_logical_cluster = curr_logical_cluster; //needed for directory expansion
            curr_logical_cluster = fatGetFatEntry(disk,boot,curr_logical_cluster); //update entry to next cluster for a directory
        }

        if(!directory_address) { //if 0, not space was found so directory must be expanded TODO TODO.... will be hardeer to test TOO

            if( fatGetFreeSpace(disk,boot)*boot->bytes_per_sector < in_file_stats.st_size + boot->bytes_per_sector ) {
                printf("Aborting: Not enough space for file!\n");
                return 8;
            }
            
            uint16_t new_entry = fatGetFreeFatEntry(disk,boot); 
            fatPutFatEntry(disk,boot,new_entry,0xFF8); //set as last sector
            fatPutFatEntry(disk,boot,prev_logical_cluster,new_entry);  //expand prev entry, since curr is now set to end value

            directory_address = fatGetDataspaceLocation(boot,new_entry);

        }
    }

    if( fatGetFreeSpace(disk,boot)*boot->bytes_per_sector < in_file_stats.st_size) {
        printf("Aborting: Not enough space for file!\n");
        return 7;
    }

    uint8_t dir_buff[FAT_DIRECTORY_SIZE];
    FATdirectory * dir_entry = xmalloc(sizeof(FATdirectory));
    memset(dir_entry,0,sizeof(FATdirectory));

    int i;
    //now that entry has been found modify directory and add to BOTH FAT TABLES
    for( i =0; i < 8; i++) {
        dir_entry->filename[i] = name->filename[i];
    }

    for( i =0; i < 3; i++) {
        dir_entry->extention[i] = name->extention[i];
    }

    //add date from linux time
    struct tm * in_file_localtime = localtime( &(in_file_stats.st_mtim.tv_sec));

    dir_entry->creation_date |= (in_file_localtime->tm_year - 80) << 9; //DOS years start at 1980 not 1900
    dir_entry->creation_date |= ((in_file_localtime->tm_mon + 1) << 5); //DOS months from 1-12, not 0 -11
    dir_entry->creation_date |= in_file_localtime->tm_mday;
    dir_entry->creation_time |= in_file_localtime->tm_hour << 11;
    dir_entry->creation_time |= in_file_localtime->tm_min << 5;
    dir_entry->modified_date = dir_entry->creation_date;
    dir_entry->modified_time = dir_entry->creation_time;


    dir_entry->file_size = in_file_stats.st_size;
    dir_entry->first_logical_cluster = fatPutFile(disk, boot, in_file, in_file_stats.st_size); //copy file to system
    fatPackDirectory(dir_entry,dir_buff);

    xfseek(disk, directory_address,SEEK_SET); //wrtie directory entry
    xfwrite(dir_buff,1,FAT_DIRECTORY_SIZE,disk);

    xfree(node);
    xfree(name);

    regfree(&preg);
    xfree(dir_entry);
    xfree(boot);
    fclose(disk);
    fclose(in_file);

    return 0;
}
