/* Fat definitions and helper functions
 * Note these functions modify the disk seek position
 */

#include <stdint.h>
#include <stdio.h>

#include "FATheaders.h"
#include "utils.h"

/* Reads in 32bit little endian from buff */
uint32_t get_uint32(uint8_t * buff) {
    return buff[0] + (buff[1] << 8) + (buff[2] << 16) + (buff[3] << 24);
}

/* Reads in 16bit little endian from buff*/
uint16_t get_uint16(uint8_t * buff) {
    return buff[0] + (buff[1] << 8);
}

/* Packs int to 32bit little endian to val*/
void pack_uint32(uint8_t * buff, uint32_t val) {
    buff[0] = val & 0xFF;
    buff[1] = ((val & 0xFF00) >> 8);
    buff[2] = ((val & 0xFF0000) >>16);
    buff[3] = ((val & 0xFF000000) >>24);
}

/* Packs int to 16bit little endian to val */
void pack_uint16(uint8_t * buff, uint16_t val) {
    buff[0] = val & 0x00FF;
    buff[1] = ((val & 0xFF00) >>8);
}

/* Unpacks the strcut from file input */
void fatUnpackBoot(FATboot * boot, uint8_t * buff) {
    int i;
    for(i=0; i<11; i++) boot->ignore0[i] = buff[i];
    boot->bytes_per_sector = get_uint16(buff+11);
    boot->sectors_per_cluster = buff[13];
    boot->reserved_sectors = get_uint16(buff+14);
    boot->num_fats = buff[16];
    boot->max_root_entries = get_uint16(buff+17);
    boot->total_sectors = get_uint16(buff+19);
    boot->ignore1 = buff[21];
    boot->sectors_per_fat = get_uint16(buff+22);
    boot->sectors_per_track = get_uint16(buff+24);
    boot->num_heads = get_uint16(buff+26);
    for(i=0; i<10; i++) boot->ignore2[i] = buff[i+28];
    boot->boot_signature = buff[38];
    for(i=0; i<4; i++) boot->volume_id[i] = buff[39+i];
    for(i=0; i<11; i++) boot->volume_label[i] = buff[43+i];
    for(i=0; i<8; i++) boot->volume_label[i] = buff[54+i];
    for(i=0; i<2; i++) boot->ignore3[i] = buff[54+i];
}

/* Packs the struct for writting to file */
void fatPackBoot(FATboot * boot, uint8_t * buff) {
    int i;
    for(i=0; i<11; i++) buff[i] = boot->ignore0[i];
    pack_uint16(buff+11,boot->bytes_per_sector);
    buff[13] = boot->sectors_per_cluster;
    pack_uint16(buff+14, boot->reserved_sectors);
    buff[16] = boot->num_fats;
    pack_uint16(buff+17,boot->max_root_entries);
    pack_uint16(buff+19, boot->total_sectors);
    buff[21] = boot->ignore1;
    pack_uint16(buff+22, boot->sectors_per_fat);
    pack_uint16(buff+24, boot->sectors_per_track);
    pack_uint16(buff+26, boot->num_heads);
    for(i=0; i<10; i++) buff[i+28] = boot->ignore2[i];
    buff[38] = boot->boot_signature;
    for(i=0; i<4; i++) buff[39+i] = boot->volume_id[i];
    for(i=0; i<11; i++) buff[43+i] = boot->volume_label[i];
    for(i=0; i<8; i++) buff[54+i] = boot->volume_label[i];
    for(i=0; i<2; i++) buff[54+i] = boot->ignore3[i];
}

/* Unpacks struct from file input */
void fatUnpackDirectory(FATdirectory * dir, uint8_t * buff) {
    int i;
    for(i=0; i<8; i++) dir->filename[i] = buff[i];
    for(i=0; i<3; i++) dir->extention[i] = buff[i+8];
    dir->attributes = buff[11];
    for(i=0; i<2; i++) dir->ignore0[i] = buff[i+12];
    dir->creation_time = get_uint16(buff + 14); //CHANGED
    dir->creation_date = get_uint16(buff+16);
    dir->last_access_date = get_uint16(buff+18);
    for( i=0; i<2; i++) dir->ignore1[i] = buff[i+20];
    dir->modified_time = get_uint16(buff+22);
    dir->modified_date = get_uint16(buff+24);
    dir->first_logical_cluster = get_uint16(buff+26);
    dir->file_size = get_uint32(buff+28);
}


/* Packs struct for writting to file */
void fatPackDirectory(FATdirectory * dir, uint8_t * buff) {
    int i;
    for(i=0; i<8; i++) buff[i] = dir->filename[i];
    for(i=0; i<3; i++) buff[i+8] = dir->extention[i];
    buff[11] = dir->attributes;
    for(i=0; i<2; i++) buff[i+12] = dir->ignore0[i];
    pack_uint16(buff+14,dir->creation_time);
    pack_uint16(buff + 16,dir->creation_date);
    pack_uint16(buff+18,dir->last_access_date);
    for( i=0; i<2; i++) buff[i+20]= dir->ignore1[i];
    pack_uint16(buff+22,dir->modified_time);
    pack_uint16(buff+24,dir->modified_date);
    pack_uint16(buff+26, dir->first_logical_cluster);
    pack_uint32(buff+28, dir->file_size);
}


/* Get information from disk for boot sector. Caller must free boot struct. */
FATboot * fatGetBootInfo(FILE * disk) {
    uint8_t buff[sizeof(FATboot)];

    xfseek(disk,0,SEEK_SET);
    xfread(buff,sizeof(FATboot), 1, disk);

    FATboot * boot = xmalloc(sizeof(FATboot));
    fatUnpackBoot(boot,buff);

    return boot;
}


/* Check directory against expected name, returns 1 on match, 0 otherwise */
int fatCompareEntries(FATdirectory * entry, FATdircompare * expected) {
    uint16_t i;
    for( i =0; i < sizeof(entry->filename); i++) {
        if( entry->filename[i] != expected->filename[i]) return -1;
    }

    for( i =0; i < sizeof(entry->extention); i++) {
        if( entry->extention[i] != expected->extention[i]) return -1;
    }

    return 0;
}

/* Gets the offset in bytes of the root directory(not in sectors!) */
uint32_t fatGetRootStart(FATboot * boot) {
    return boot->bytes_per_sector*FAT_ROOT_OFFSET;
}

/* Gets the offset in bytes of the dataspace (not in sectors!) */
uint32_t fatGetDataspaceLocation(FATboot * boot, uint16_t index) {
    return boot->bytes_per_sector * (index + FAT_DATASPACE_OFFSET - 2);
}

/* Get the value of a entyr in the fat table */
uint16_t fatGetFatEntry(FILE * disk, FATboot * boot, uint16_t index) { //get value of fat entry
    uint8_t entry[2];
    xfseek(disk, boot->bytes_per_sector + (index  * 12)/8, SEEK_SET);
    xfread(entry,2,1,disk);

    if( index /2 * 2 == index ) { //even need to read two bytes
        return entry[0] + ((entry[1] & 0x0F) <<8);
    } else { //odd part remains
        return ((entry[0] & 0xF0) >>4) + (entry[1] <<4);
    }
}

/* Set the value of a fat entry in the fat table */
void fatPutFatEntry(FILE * disk, FATboot * boot, uint16_t index, uint16_t value) {

    uint8_t fat[2];

    xfseek(disk, boot->bytes_per_sector + ((index*12)/8), SEEK_SET);
    xfread(fat,1,2,disk);

    if( index /2 * 2 == index ) {
        fat[0] = value & 0x00FF;
        fat[1] = (fat[1] & 0xF0) | ((value & 0x0F00 ) >>8);
    } else { //odd case
        fat[0] = (fat[0] & 0x0F) | (value & 0x000F) << 4;
        fat[1] = (value & 0x0FF0) >>4;
    }

    xfseek(disk, boot->bytes_per_sector + ((index*12)/8), SEEK_SET); //go back to write over
    xfwrite(fat,1,2,disk);
}


/* Get free space of dataspace of disk in bytes */
uint32_t fatGetFreeSpace(FILE * disk, FATboot * boot) {

    uint8_t fat[2]; //read in fat entries

    xfseek(disk, boot->bytes_per_sector*1 + 3, SEEK_SET); //ignore reserved 2(3bytes)
    int count = 0;


    int i; //need to read odd ammounts per time
    for(i=2; i < boot->total_sectors - FAT_DATASPACE_OFFSET + 2; i++) {
        uint16_t fat_value;
        if( i /2 * 2 == i ) { //even need to read two bytes
            xfread(fat,1,2,disk);
            fat_value = fat[0] + ((fat[1] & 0x0F) <<8);
        } else { //odd part remains
            fat[0] = fat[1]; //odd swap and  read in one byte
            xfread(fat+1,1,1,disk);
            fat_value = ((fat[0] & 0xF0) >>4) + (fat[1] <<4);
        }
        if(!fat_value) {
            count++;
        }
    }

    return count;
}

/* Returns the index of a free fat entry or 0 if none */
uint16_t fatGetFreeFatEntry(FILE * disk, FATboot * boot) {

    uint8_t fat[2]; //read in fat entries

    xfseek(disk, boot->bytes_per_sector*1 + 3, SEEK_SET); //ignore reserved 2(3bytes)

    int i; //need to read odd ammounts per time
    for(i=2; i < boot->total_sectors - FAT_DATASPACE_OFFSET + 2; i++) {
        uint16_t fat_value;
        if( i /2 * 2 == i ) { //even need to read two bytes
            xfread(fat,1,2,disk);
            fat_value = fat[0] + ((fat[1] & 0x0F) <<8);
        } else { //odd part remains
            fat[0] = fat[1]; //odd swap and  read in one byte
            xfread(fat+1,1,1,disk);
            fat_value = ((fat[0] & 0xF0) >>4) + (fat[1] <<4);
        }
        if(!fat_value) {
            return i;
        }
    }
    return 0;
}


/* Copy a file into the fat table, does not create directory reference */
uint16_t fatPutFile(FILE * disk, FATboot * boot, FILE * in_file, uint32_t size) {

    uint8_t fat[2]; //read in fat entries

    uint8_t * buff = xmalloc(boot->bytes_per_sector); //for copying file

    xfseek(disk, boot->bytes_per_sector*1 + 3, SEEK_SET); //ignore reserved 2

    uint32_t file_copied = 0;
    //need to copy whole size

    int i;

    uint16_t prev_chunk = 0;
    uint16_t first_chunk = 0;

    for(i=2; i <= boot->total_sectors - FAT_DATASPACE_OFFSET + 2 && file_copied < size; i++) {
        uint16_t fat_value;
        if( i /2 * 2 == i ) { //even need to read two bytes
            xfread(fat,1,2,disk);
            fat_value = fat[0] + ((fat[1] & 0x0F) <<8);


        } else { //odd part remains
            fat[0] = fat[1]; //odd swap and  read in one byte
            xfread(fat+1,1,1,disk);
            fat_value = ((fat[0] & 0xF0) >>4) + (fat[1] <<4);
        }

        if(!fat_value) {
            uint32_t to_read = file_copied + 512 <= size ? 512 : size - file_copied;

            xfseek(disk, fatGetDataspaceLocation(boot,i),SEEK_SET); //go to place
            file_copied += xfread(buff,1,to_read,in_file);
            xfwrite(buff,1,to_read,disk);
            if( prev_chunk != 0) fatPutFatEntry(disk,boot,prev_chunk,i);
            if( first_chunk == 0) first_chunk = i;
            prev_chunk = i;
        }

    }
    fatPutFatEntry(disk,boot,prev_chunk,0xFF8);

    xfree(buff);
    return first_chunk;

}

