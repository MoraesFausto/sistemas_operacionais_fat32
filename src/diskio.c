#include "fat32.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int disk_fd = -1;
BPB bpb;

int disk_init(const char *path) {
    disk_fd = open(path, O_RDWR);
    if (disk_fd == -1) return -1;

    uint8_t sector[512];
    read(disk_fd, sector, 512);
    memcpy(&bpb, sector, sizeof(BPB));
    return 0;
}

bool read_sector(uint32_t sector, uint8_t *buffer) {
    // Open the FAT32 image file for reading
    FILE *image = fopen("myimagefat32.img", "rb");
    if (!image) {
        perror("Failed to open FAT32 image");
        return false;
    }

    // Seek to the correct position in the file
    if (fseek(image, sector * bpb.BPB_BytsPerSec, SEEK_SET) != 0) {
        perror("Failed to seek in FAT32 image");
        fclose(image);
        return false;
    }

    // Read the sector data
    size_t bytes_read = fread(buffer, 1, bpb.BPB_BytsPerSec, image);
    if (bytes_read != bpb.BPB_BytsPerSec) {
        perror("Failed to read sector data");
        fclose(image);
        return false;
    }

    // Close the file
    fclose(image);
    return true;
}

void write_sector(uint32_t sector, uint8_t *buffer) {
    // Open the FAT32 image file for reading and writing
    FILE *image = fopen("myimagefat32.img", "r+b");
    if (!image) {
        perror("Failed to open FAT32 image");
        return ;
    }

    // Seek to the correct position in the file
    if (fseek(image, sector * bpb.BPB_BytsPerSec, SEEK_SET) != 0) {
        perror("Failed to seek in FAT32 image");
        fclose(image);
        return ;
    }

    // Write the sector data
    size_t bytes_written = fwrite(buffer, 1, bpb.BPB_BytsPerSec, image);
    if (bytes_written != bpb.BPB_BytsPerSec) {
        perror("Failed to write sector data");
        fclose(image);
        return;
    }

    // Close the file
    fclose(image);
}

bool read_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t first_data_sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);
    uint32_t sector = first_data_sector + (cluster - 2) * bpb.BPB_SecPerClus;

    printf("Reading from cluster %u (sector %u)\n", cluster, sector);

    for (uint8_t i = 0; i < bpb.BPB_SecPerClus; i++) {
        printf("Reading sector %u\n", sector + i);
        if (!read_sector(sector + i, buffer + (i * bpb.BPB_BytsPerSec))) {
            return false;
        }
    }

    return true;
}

void write_cluster(uint32_t cluster, uint8_t *data) {
    uint32_t first_data_sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32);
    uint32_t sector = first_data_sector + (cluster - 2) * bpb.BPB_SecPerClus;

    printf("Writing to cluster %u (sector %u)\n", cluster, sector);

    for (uint8_t i = 0; i < bpb.BPB_SecPerClus; i++) {
        printf("Writing sector %u\n", sector + i);
        write_sector(sector + i, data + (i * bpb.BPB_BytsPerSec));
    }

}

uint32_t read_fat(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = bpb.BPB_RsvdSecCnt + (fat_offset / bpb.BPB_BytsPerSec);
    uint32_t ent_offset = fat_offset % bpb.BPB_BytsPerSec;

    uint8_t sector[512];
    read_sector(fat_sector, sector);
    return *((uint32_t*)(sector + ent_offset)) & 0x0FFFFFFF;
}

void write_fat(uint32_t cluster, uint32_t value) {
    // Calculate the sector and offset of the FAT entry
    uint32_t fat_offset = cluster * 4; // Each FAT entry is 4 bytes
    uint32_t fat_sector = bpb.BPB_RsvdSecCnt + (fat_offset / bpb.BPB_BytsPerSec);
    uint32_t entry_offset = fat_offset % bpb.BPB_BytsPerSec;

    // Read the FAT sector
    uint8_t sector_buffer[bpb.BPB_BytsPerSec];
    read_sector(fat_sector, sector_buffer);

    // Update the FAT entry
    *(uint32_t *)(sector_buffer + entry_offset) = value;

    // Write the updated sector back to the FAT
    write_sector(fat_sector, sector_buffer);

    // If there are multiple FATs, update all of them
    for (uint8_t i = 1; i < bpb.BPB_NumFATs; i++) {
        fat_sector += bpb.BPB_FATSz32; // Move to the next FAT
        write_sector(fat_sector, sector_buffer);
    }

}
uint32_t next_cluster(uint32_t cluster) {
    return read_fat(cluster);
}

uint32_t find_free_cluster() {
    // Busca sequencial (simplificada)
    for (uint32_t i = 2; i < bpb.BPB_TotSec32 / bpb.BPB_SecPerClus; i++) {
        if (read_fat(i) == 0) return i;
    }
    return 0; // Sem clusters livres
}