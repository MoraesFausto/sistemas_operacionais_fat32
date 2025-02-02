#include "fat32.h"
#include <stdlib.h>

void cmd_info() {
    printf("=== Disk Info ===\n");
    printf("Bytes per Sector: %u\n", bpb.BPB_BytsPerSec);
    printf("Sectors per Cluster: %u\n", bpb.BPB_SecPerClus);
    printf("FAT Size: %u sectors\n", bpb.BPB_FATSz32);
    printf("Root Cluster: %u\n", bpb.BPB_RootClus);
}

void cmd_cluster(uint32_t num) {
    uint8_t buffer[bpb.BPB_BytsPerSec];
    uint32_t sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (num - 2) * bpb.BPB_SecPerClus;
    read_sector(sector, buffer);
    for (size_t i = 0; i < sizeof(buffer); i++) {
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
}

void cmd_help() {
    printf("Available commands:\n");
    printf("info - Display disk information\n");
    printf("cluster <num> - Display content of cluster <num>\n");
    printf("pwd - Display current directory\n");
    printf("attr <name> - Display attributes of file or directory <name>\n");
    printf("cd <path> - Change current directory to <path>\n");
    printf("touch <filename> - Create an empty file <filename>\n");
    printf("mkdir <dirname> - Create an empty directory <dirname>\n");
    printf("rm <filename> - Remove file <filename>\n");
    printf("rmdir <dirname> - Remove empty directory <dirname>\n");
    printf("cp <source> <target> - Copy file <source> to <target>\n");
    printf("mv <source> <target> - Move file <source> to <target>\n");
    printf("rename <filename> <newfilename> - Rename file <filename> to <newfilename>\n");
    printf("ls - List files and directories in current directory\n");
}

void cmd_pwd() {
    printf("Current Directory: %s\n", current_path);
}

void cmd_attr(const char *name) {

}

void cmd_cd(const char *path) {

}

void cmd_touch(const char *filename) {

}

void cmd_mkdir(const char *dirname) {

}

void cmd_rm(const char *filename) {

}

void cmd_rmdir(const char *dirname) {

}

void cmd_cp(const char *source_path, const char *target_path) {

}

void cmd_mv(const char *source_path, const char *target_path) {

}

void cmd_rename(const char *filename, const char *newfilename) {

}