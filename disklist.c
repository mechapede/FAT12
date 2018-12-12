/* 
 * Implementation of disklist for listing files
*/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "FATheaders.h"
#include "ADTlinkedlist.h"
#include "utils.h"



/* Information needed for printing path and its entries */
typedef struct subdir_info {
    FATdirectory * directory;
    char * path; //path, including own name
} subdir_info;


/* Reads next directory entry from disk using current disk position
 * Adds subdirs to subdir list
 * Returns 0 if end of disk, 1 otherwise */
int parse_next_entry(FILE * disk, ADTlinkedlist * subdirs, char * curr_path) {

    uint8_t dir_buff[FAT_DIRECTORY_SIZE];
    FATdirectory dir_entry;

    xfread(dir_buff, FAT_DIRECTORY_SIZE,1, disk);
    fatUnpackDirectory(&dir_entry,dir_buff);

    if( dir_entry.filename[0] == 0x00 ) return 0; //end of directory case

    if( dir_entry.filename[0] !=  0xEF
            && dir_entry.filename[0] != '.'
            && dir_entry.first_logical_cluster > 1
            && dir_entry.attributes != 0x0F //not all bit set
            && !(dir_entry.attributes & 0x08) ) { //not system

        char name[13]; //format filename for printing with null termination
        memcpy(name, dir_entry.filename,8);
        name[8] = '.';
        memcpy(name + 9, dir_entry.extention,3);
        name[12] = 0;

        printf("%c %10u %-s %u-%u-%0u %02u:%02u  \n",
               (dir_entry.attributes & 0x10) ? 'D':'F',
               dir_entry.file_size,
               name,
               1980 + ((dir_entry.creation_date & 0xFE00) >>9), //year //AS defined by examples
               ((dir_entry.creation_date & 0x01E0) >>5), //month
               (dir_entry.creation_date & 0x001F), //day
               (dir_entry.creation_time & 0xF800) >>11, //hour
               (dir_entry.creation_time & 0x07E0) >>5 //minute
              );

        if( dir_entry.attributes & 0x10) { //save directory for recurse
            ADTlinkednode * node = xmalloc(sizeof(ADTlinkednode));
            subdir_info * sub_info = xmalloc(sizeof(subdir_info));

            int curr_path_length = strlen(curr_path);
            char * path = xmalloc(curr_path_length + 1 + strlen(name) + 1); //room to add name + /
            strcpy(path,curr_path);
            path[curr_path_length] = '/';
            strcpy(path + curr_path_length + 1,name);

            FATdirectory * subdir = xmalloc(sizeof(FATdirectory));
            memcpy(subdir,&dir_entry,sizeof(FATdirectory));

            sub_info->directory = subdir;
            sub_info->path = path;

            adtInitiateLinkedNode(node, sub_info);
            adtAddEndLinkedNode(subdirs, node);
        }
    }

    return 1;
}


int main(int argc, char * argv[]) {

    if( argc != 2 ) {
        printf("Usage: ./disklist <diskname> \n");
        return 2;
    }

    FILE * disk = fopen(argv[1],"r");
    if( !disk) {
        perror("Opening disk failed:");
        printf("Name given %s\n",argv[1]);
        return 3;
    }

    FATboot * boot = fatGetBootInfo(disk);

    ADTlinkedlist subdirs;
    adtInitiateLinkedList(&subdirs); //for directories to recurse

    /* Perform an inorder traversal of all directories */

    xfseek(disk, fatGetRootStart(boot),SEEK_SET);

    printf("/ \n==================\n");

    /* Root directory entries */
    uint32_t entries_read;
    for( entries_read=0; entries_read <  boot->max_root_entries; entries_read++) {
        if( !parse_next_entry(disk,&subdirs,"") ) break;
    }

    /* Now all Subdirectory entries */
    while( subdirs.num > 0 ) {

        ADTlinkednode * node = adtPopLinkedNode(&subdirs,0);
        subdir_info * curr_dir = node->val;
        uint16_t curr_logical_cluster = curr_dir->directory->first_logical_cluster;

        printf("%s \n==================\n", curr_dir->path);


        while( curr_logical_cluster <= 0xFF0 && curr_logical_cluster > 0) { //iterate through all FAT entries

            xfseek(disk, fatGetDataspaceLocation(boot,curr_logical_cluster),SEEK_SET); //go to logical cluster

            for( entries_read=0; entries_read < boot->bytes_per_sector/FAT_DIRECTORY_SIZE ; entries_read++) { //read all entries in cluster
                if( !parse_next_entry(disk,&subdirs,curr_dir->path) ) goto break_dir_search;

            }

            curr_logical_cluster = fatGetFatEntry(disk,boot,curr_logical_cluster); //update entry to next value
        }

break_dir_search:
        xfree(node);
        xfree(curr_dir->directory);
        xfree(curr_dir->path);
        xfree(curr_dir);

    }


    xfree(boot);
    fclose(disk);

    return 0;
}
