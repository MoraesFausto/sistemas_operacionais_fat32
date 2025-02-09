#include "fat32.h"
#include <stdio.h>
#include <string.h>

void start_shell() {
    char input[256];
    char cmd[32], arg1[64], arg2[64];

    printf("=== FATShell ===\n");
    current_cluster = bpb.BPB_RootClus;

    while (1) {
        printf("fatshell:[%s] $ ", current_path);
        fgets(input, sizeof(input), stdin);
        sscanf(input, "%s %s %s", cmd, arg1, arg2);

        if (strcmp(cmd, "info") == 0) {
            cmd_info();
        } else if (strcmp(cmd, "cluster") == 0) {
            cmd_cluster(atoi(arg1));
        }else if(strcmp(cmd, "help") == 0) {
            cmd_help();
        } else if (strcmp(cmd, "pwd") == 0) {
            cmd_pwd();
        } else if (strcmp(cmd, "attr") == 0) {
            cmd_attr(arg1);
        } else if (strcmp(cmd, "cd") == 0) {
            change_directory(arg1, false);
        } else if (strcmp(cmd, "touch") == 0) {
            create_file(arg1, 0);
        } else if (strcmp(cmd, "mkdir") == 0) {
            create_file(arg1, 0x10);
        } else if (strcmp(cmd, "rm") == 0) {
            delete_file(arg1, false);
        } else if (strcmp(cmd, "rmdir") == 0) {
            delete_dir(arg1);
        } else if (strcmp(cmd, "cp") == 0) {
            copy_file(arg1, arg2);
        } else if (strcmp(cmd, "mv") == 0) {
            move_file(arg1, arg2);
        } else if (strcmp(cmd, "rename") == 0) {
            rename_file(arg1, arg2);
        } else if (strcmp(cmd, "ls") == 0) {
            list_directory(current_cluster);
        } else {
            printf("Comando desconhecido.\n");
        }
    }
}