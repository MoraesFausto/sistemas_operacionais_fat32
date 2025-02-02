#include "fat32.h"
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
void read_sector(uint32_t sector_num, uint8_t *buffer) {
    lseek(disk_fd, sector_num * bpb.BPB_BytsPerSec, SEEK_SET);
    read(disk_fd, buffer, bpb.BPB_BytsPerSec);
}

void write_sector(uint32_t sector_num, uint8_t *buffer) {
    lseek(disk_fd, sector_num * bpb.BPB_BytsPerSec, SEEK_SET);
    write(disk_fd, buffer, bpb.BPB_BytsPerSec);
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
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = bpb.BPB_RsvdSecCnt + (fat_offset / bpb.BPB_BytsPerSec);
    uint32_t ent_offset = fat_offset % bpb.BPB_BytsPerSec;

    uint8_t sector[512];
    read_sector(fat_sector, sector);
    *((uint32_t*)(sector + ent_offset)) = (*((uint32_t*)(sector + ent_offset)) & 0xF0000000) | (value & 0x0FFFFFFF);
    write_sector(fat_sector, sector);
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