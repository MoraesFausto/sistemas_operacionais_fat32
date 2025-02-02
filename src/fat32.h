#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdbool.h>

// BPB (BIOS Parameter Block)
typedef struct {
    uint8_t BS_jmpBoot[3];
    char BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
} __attribute__((packed)) BPB;

// Entrada de Diretório
typedef struct {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} __attribute__((packed)) DirEntry;


// Variáveis globais
extern BPB bpb;
extern uint32_t current_cluster;
extern char current_path[256];

// Protótipos de funções
int disk_init(const char *path);
void read_sector(uint32_t sector, uint8_t *buffer);
void write_sector(uint32_t sector, uint8_t *buffer);
uint32_t read_fat(uint32_t cluster);
void write_fat(uint32_t cluster, uint32_t value);
uint32_t next_cluster(uint32_t cluster);
uint32_t find_free_cluster();
void list_directory(uint32_t cluster);
bool create_file(const char *name, uint8_t attr);
bool rename_file(const char *old_name, const char *new_name);
bool delete_file(const char *name, bool isAfterRmdir);
bool delete_dir(const char *name);
bool copy_file(const char *src_name, const char *dest_name);
bool move_file(const char *src_name, const char *dest_name);
void change_directory(const char *path, bool isCdBack);
void cmd_info();
void cmd_cluster(uint32_t num);
void start_shell();
void cmd_pwd();
void cmd_attr(const char *name);
void cmd_cd(const char *path);
void cmd_touch(const char *name);
void cmd_mkdir(const char *name);
void cmd_rm(const char *name);
void cmd_rmdir(const char *name);
void cmd_cp(const char *source_path, const char *target_path);
void cmd_mv(const char *source_path, const char *target_path);
void cmd_rename(const char *name, const char *newname);
void cmd_help();

#endif