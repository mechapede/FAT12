/*
 * Implementation of diskinfo. Prints stats from spec.
*/

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "FATheaders.h"
#include "ADTlinkedlist.h"
#include "utils.h"

/* Count next directory entry from disk if its valid and not a subdirectory
 * Adds subdirs to the subdirs list
 * Returns 0 if end of disk, 1 otherwise */
int count_next_entry(FILE * disk, ADTlinkedlist * subdirs, int * num_files) {

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

        if( dir_entry.attributes & 0x10) { //save directory for recurse
            ADTlinkednode * node = xmalloc(sizeof(ADTlinkednode));
            FATdirectory * subdir = xmalloc(sizeof(FATdirectory));
            memcpy(subdir,&dir_entry,sizeof(FATdirectory));
            adtInitiateLinkedNode(node, subdir);
            adtAddEndLinkedNode(subdirs, node);
        } else {
            (*num_files)++;
        }
    }

    return 1;
}

int main(int argc, char * argv[]) {

    if( argc != 2 ) {
        printf("Usage: ./diskinfo <diskname> \n");
        return 2;
    }

    FILE * disk = fopen(argv[1],"r");
    if( !disk) {
        perror("Opening disk failed:");
        printf("Name given %s\n",argv[1]);
        return 3;
    }

    FATboot * boot = fatGetBootInfo(disk);

    uint32_t count = fatGetFreeSpace(disk,boot);

    char os_name[9];
    memcpy(os_name,boot->ignore0 + 3,8);
    os_name[8] = 0;

    char disk_label[12];

    uint8_t dir_buff[FAT_DIRECTORY_SIZE];
    FATdirectory dir_entry;

    int entries_read; //get volume label from root directory if it exisits
    xfseek(disk, fatGetRootStart(boot),SEEK_SET);
    for( entries_read=0; entries_read <  boot->max_root_entries; entries_read++) { //root directory entries
        xfread(dir_buff, FAT_DIRECTORY_SIZE,1, disk);
        fatUnpackDirectory(&dir_entry,dir_buff);
        if( dir_entry.attributes != 0x0F && dir_entry.attributes & 0x08 ) break; //based on examples, must be explicitiry 0x08
    }

    if(  dir_entry.attributes != 0x0F && dir_entry.attributes & 0x08 ) { //use label from boot
        memcpy(disk_label, dir_entry.filename,11);
    } else { //if found use label from root directory
        memcpy(disk_label, boot->volume_label,11);
    }
    disk_label[11] = 0;

    printf("OS Name: %.8s\nLabel of disk: %.11s\nTotal size of the disk: %u bytes\nFree size of the disk: %u bytes\n", 
           os_name,
           disk_label,
           boot->total_sectors*512,
           count*512);

    printf("==================\n");


    ADTlinkedlist subdirs;
    adtInitiateLinkedList(&subdirs); //for directories in order traversal


    xfseek(disk, fatGetRootStart(boot),SEEK_SET);
    /* Traversal of whole file system*/
    int num_files = 0;

    for( entries_read=0; entries_read <  boot->max_root_entries; entries_read++) { //root directory entries
        if(!count_next_entry(disk,&subdirs,&num_files)) break;
    }


    while( subdirs.num > 0 ) { //subdirs to list
        ADTlinkednode * node = adtPopLinkedNode(&subdirs,0);
        FATdirectory * curr_dir = node->val;
        uint16_t curr_logical_cluster = curr_dir->first_logical_cluster;

        while( curr_logical_cluster <= 0xFF0 && curr_logical_cluster > 0) { //iterate through all FAT entries

            xfseek(disk, fatGetDataspaceLocation(boot,curr_logical_cluster),SEEK_SET);

            for( entries_read=0; entries_read < boot->bytes_per_sector/FAT_DIRECTORY_SIZE ; entries_read++) { //read all entries in cluster
                if(!count_next_entry(disk,&subdirs,&num_files)) goto break_dir_count;
            }
            curr_logical_cluster = fatGetFatEntry(disk,boot,curr_logical_cluster); //update entry to next value

        }

break_dir_count:

        xfree(node);
        xfree(curr_dir);

    }

    printf("The number of files in the disk: %u\n\n",num_files);
    printf("==================\n");
    printf("Number of FAT copies: %u\n",boot->num_fats);
    printf("Sectors per FAT: %u\n",boot->sectors_per_fat);


    xfree(boot);
    fclose(disk);

    return 0;

}
