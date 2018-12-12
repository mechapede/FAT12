/* Fat definitions and helper functions
 * Note these functions modify the disk seek position
 */

#ifndef _FATHEADERS_H
#define _FATHEADERS_H

#include <stdint.h>
#include <stdio.h>

/* Offset definitions for FAT 12 file system. Cannot easily determine 
 * these from boot but can be different on other fats */
#define FAT_DIRECTORY_SIZE 32
#define FAT_BOOT_SIZE 64
#define FAT_FIRST_TABLE 1
#define FAT_TABLE_SIZE 9
#define FAT_DATASPACE_OFFSET 33
#define FAT_ROOT_OFFSET 19

/* Exhaustive but not complete struct of FAT12 boot fields
 * Ignored field are read in, so struct can be written to disk easily */
typedef struct FATboot{
    uint8_t ignore0[11];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t max_root_entries;
    uint16_t total_sectors;
    uint8_t ignore1;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint8_t ignore2[10];
    uint8_t boot_signature;
    uint8_t volume_id[4];
    uint8_t volume_label[11];
    uint8_t file_system_type[8];
    uint8_t ignore3[2];
    //.... other unspecified fields 
} FATboot;


/* Struct for directory entry */
typedef struct FATdirectory{
    uint8_t filename[8];
    uint8_t extention[3];
    uint8_t attributes;
    uint8_t ignore0[2];
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint8_t ignore1[2];
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t first_logical_cluster;
    uint32_t file_size;
}FATdirectory;


/* Struct with values needed for finding entry with same name  */
typedef struct FATdircompare{
    uint8_t filename[8];
    uint8_t extention[3]; //tell if directory or not
}FATdircompare;


/* NOTE: Due to struct packing, structs in memory may not be formated the same as disk, 
 * therefore it is recommended to use the helper functions for packing unpacking structs. */

/* Unpacks the strcut from file input */
void fatUnpackBoot(FATboot * boot, uint8_t * buff);

/* Packs the struct for writting to file */
void fatPackBoot(FATboot * boot, uint8_t * buff);

/* Unpacks struct from file input */
void fatUnpackDirectory(FATdirectory * dir, uint8_t * buff);

/* Packs struct for writting to file */
void fatPackDirectory(FATdirectory * dir, uint8_t * buff);


/* Get information from disk for boot sector. Caller must free boot struct. */
FATboot * fatGetBootInfo(FILE * disk);

/* Check directory against expected name, returns 1 on match, 0 otherwise */
int fatCompareEntries(FATdirectory * entry, FATdircompare * expected);

/* Gets the offset in bytes of the root directory(not in sectors!) */
uint32_t fatGetRootStart(FATboot * boot);

/* Gets the offset in bytes of the dataspace (not in sectors!) */
uint32_t fatGetDataspaceLocation(FATboot * boot, uint16_t index);

/* Get the value of a entry in the fat table */
uint16_t fatGetFatEntry(FILE * disk, FATboot * boot, uint16_t index);

/* Get free space of dataspace of disk in bytes */
uint32_t fatGetFreeSpace(FILE * disk, FATboot * boot);

/* Set the value of a fat entry in the fat table */
void fatPutFatEntry(FILE * disk, FATboot * boot, uint16_t index, uint16_t value);

/* Returns the index of a free fat entry or 0 if none */
uint16_t fatGetFreeFatEntry(FILE * disk, FATboot * boot);

/* Copy a file into the fat table, does not create directory reference */
uint16_t fatPutFile(FILE * disk, FATboot * boot, FILE * in_file, uint32_t size);

#endif
