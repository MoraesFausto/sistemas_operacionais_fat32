#include "fat32.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t current_cluster = 0;
char current_path[256] = "/";

void list_directory(uint32_t cluster) {
    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];

if (cluster < 2) {
    fprintf(stderr, "Cluster inválido: %u\n", cluster);
    return;
}

uint32_t sector = bpb.BPB_RsvdSecCnt + 
                  (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + 
                  ((cluster - 2) * bpb.BPB_SecPerClus);    read_sector(sector, buffer);

    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *entry = (DirEntry*)(buffer + i);
        if ((unsigned char)entry->DIR_Name[0] == 0x00) break;
        if ((unsigned char)entry->DIR_Name[0] == 0xE5) continue;

        char name[13];
        snprintf(name, sizeof(name), "%.8s.%.3s", entry->DIR_Name, entry->DIR_Name + 8);
        printf("%s %s\n", (entry->DIR_Attr & 0x10) ? "[DIR]" : "[FILE]", name);
    }
}

void concat(char *s1, char *s2) {
    int i = 0;
  
    // Move to the end of str1
    while (s1[i] != '\0')
        i++;

    // Copy characters from str2 to str1
    int j = 0;
    while (s2[j] != '\0') {
        s1[i] = s2[j];  
        i++;
        j++;
    }

    // Null-terminate the concatenated string
    s1[i] = '\0';
}


uint32_t navigate_to_directory(const char *path, uint32_t arg_cluster) {
    if (strcmp(path, "/") == 0) {
        return bpb.BPB_RootClus; // Retorna o cluster do diretório raiz
    }

    char temp_path[256];
    strncpy(temp_path, path, sizeof(temp_path));
    temp_path[sizeof(temp_path) - 1] = '\0';

    char *token = strtok(temp_path, "/");
    uint32_t cluster = arg_cluster == 0 ? bpb.BPB_RootClus : arg_cluster;

    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    printf("token: %s\n", token);
    while (token) {
        bool found = false;
        uint32_t sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (cluster - 2) * bpb.BPB_SecPerClus;
        read_sector(sector, buffer);

        for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
            DirEntry *e = (DirEntry *)(buffer + i);
            char formatted_name[11];
            char formatted_dirname[11];
            snprintf(formatted_dirname, 11, "%-8.8s%-3.3s", e->DIR_Name, "");
            snprintf(formatted_name, 11, "%-8.8s%-3.3s", token, "");


            if(e->DIR_Attr & 0x10){

                if (strncmp(formatted_dirname, formatted_name, 11) == 0) { // Verifica se é um diretório
                    cluster = (e->DIR_FstClusHI << 16) | e->DIR_FstClusLO;
                    found = true;
                    break;
                }
            } else if (strncmp(formatted_dirname, formatted_name, 11) == 0) {             
                printf("Entrou no else\n"); 
                    found = true;
                    break;
            }

        }

        if (!found) {
            return 0; // Diretório não encontrado
        }

        token = strtok(NULL, "/");
    }

    return cluster;
}



uint32_t navigate_split_path(const char *path, char *name, bool path_has_filename) {
    char **result;
    uint32_t search_cluster = current_cluster;
        printf("path: %s\n", path);


    if(path == NULL || *path == '\0'){
        return NULL;
    }
    if(path[0] == '/'){
        search_cluster = bpb.BPB_RootClus;
    }

    int count = split_string(path, &result);
    if(result == NULL){
        return 0;
    }



    for(int i = 0; i < count; i++){
        printf("que foi %d", path_has_filename&& (i == count - 1 || count == 0));
        if(path_has_filename && (i == count - 1 || count == 0)){
           strncpy(name, result[i], strlen(result[i]));
            name[strlen(result[i])] = '\0';
            printf("name: %s\n", name);
            break;
        }
        search_cluster = navigate_to_directory(result[i], search_cluster);
    }
    return search_cluster;
}


void change_directory(const char *path, bool isCdBack) {
    if (strcmp(path, "/") == 0) {
        current_cluster = bpb.BPB_RootClus;
        current_path[0] = '/';
        current_path[1] = '\0';
        //concat(current_path, path);
        return;
    }

    if (strcmp(path, "..") == 0) {
        if (strcmp(current_path, "/") == 0) {
            printf("Diretório raiz.\n");
            return;
        }

        char *last_slash = strrchr(current_path, '/');
        if (last_slash == current_path) {
            current_cluster = bpb.BPB_RootClus;
            current_path[1] = '\0';
            return;
        }

        *last_slash = '\0';
        current_cluster = bpb.BPB_RootClus;
        char *token = strtok(current_path, "/");
        int i = 0;
        while (token != NULL) {
            change_directory(token, true);
            token = strtok(NULL, "/");
        }
        return;
    }

    current_cluster = navigate_split_path(path, NULL, false);
    if(current_cluster == 0){
        printf("Diretório não encontrado: %s\n", path);
        return;
    }

    if(!isCdBack){
        if(path[0] == '/'){
            strncpy(current_path, path, strlen(path));
            current_path[strlen(path)] = '\0';
        } else {
            if(current_path[strlen(current_path) - 1] != '/') {
                concat(current_path, "/");
            }
            concat(current_path, path);
        }
    }
    return;

    
}


bool create_file(const char *name, uint8_t attr) {
    uint32_t free_cluster = find_free_cluster();
    if (free_cluster == 0) return false;

    DirEntry entry;
    memset(&entry, 0, sizeof(DirEntry));
    snprintf(entry.DIR_Name, 11, "%-8.8s%-3.3s", name, ""); 
    entry.DIR_Attr = attr;
    entry.DIR_FstClusLO = free_cluster & 0xFFFF;
    entry.DIR_FstClusHI = (free_cluster >> 16) & 0xFFFF;

    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    uint32_t sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (current_cluster - 2) * bpb.BPB_SecPerClus;
    read_sector(sector, buffer);

    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry*)(buffer + i);
        if ((unsigned char)e->DIR_Name[0] == 0x00 || (unsigned char)e->DIR_Name[0] == 0xE5) {
            memcpy(e, &entry, sizeof(DirEntry));
            write_sector(sector, buffer);
            write_fat(free_cluster, 0x0FFFFFFF); 
            return true;
        }
    }
    return false;
}

bool delete_file(const char *name, bool isAfterRmdir) {
    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    uint32_t sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (current_cluster - 2) * bpb.BPB_SecPerClus;
    read_sector(sector, buffer);

    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry*)(buffer + i);
        char formatted_name[11];
        char formatted_entryname[11];
        snprintf(formatted_entryname, 11, "%-8.8s%-3.3s", e->DIR_Name, "");
        snprintf(formatted_name, 11, "%-8.8s%-3.3s", name, "");

        if (strncmp(formatted_entryname, formatted_name, 11) == 0) {
            if(e->DIR_Attr & 0x10){
                if(!isAfterRmdir){
                    printf("O arquivo é um diretório, use rmdir para deletar diretórios.\n");
                    return false;
                }
            }
            uint32_t cluster = (e->DIR_FstClusHI << 16) | e->DIR_FstClusLO;
            e->DIR_Name[0] = 0xE5; 
            write_sector(sector, buffer);
            
            while (cluster < 0x0FFFFFF8) {
                uint32_t next = next_cluster(cluster);
                write_fat(cluster, 0);
                cluster = next;
            }
            return true;
        }
    }
    printf("Arquivo não encontrado: %s\n", name);
    return false;
}

bool delete_dir(const char *name) {
    uint32_t cluster_to_delete = navigate_split_path(name, NULL, false);
    if (cluster_to_delete == 0) {
        printf("Diretório não encontrado: %s\n", name);
        return false;
    }
    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    uint32_t sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (cluster_to_delete - 2) * bpb.BPB_SecPerClus;
    read_sector(sector, buffer);

    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry*)(buffer + i);
        char formatted_name[11];  
        uint32_t cluster = (e->DIR_FstClusHI << 16) | e->DIR_FstClusLO;
        e->DIR_Name[0] = 0xE5; 
        write_sector(sector, buffer);
        
        while (cluster < 0x0FFFFFF8) {
            uint32_t next = next_cluster(cluster);
            write_fat(cluster, 0);
            cluster = next;
        }
        delete_file(name, true);
        return true;
        
    }
    return false;
}

bool rename_file(const char *old_name, const char *new_name) {
    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    uint32_t sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (current_cluster - 2) * bpb.BPB_SecPerClus;
    read_sector(sector, buffer);

    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry*)(buffer + i);
        char formatted_name[11];  
        snprintf(formatted_name, 11, "%-8.8s%-3.3s", old_name, "");
        if (strncmp(e->DIR_Name, formatted_name, 11) == 0) {
            snprintf(e->DIR_Name, 11, "%-8.8s%-3.3s", new_name, ""); 
            write_sector(sector, buffer);
            return true;
        }
    }
    return false;
}

int split_string(const char *str, char ***result) {
    if (str == NULL || *str == '\0') {  // Verifica se a string de entrada é vazia
        *result = NULL;
        return 0;
    }

    int count = 0;
    char *temp_str = strdup(str);  // Cria uma cópia da string original
    if (temp_str == NULL) {
        perror("strdup failed");
        exit(1);
    }

    // Contar quantos tokens existem
    char *token = strtok(temp_str, "/");
    while (token != NULL) {
        count++;
        token = strtok(NULL, "/");
    }
    free(temp_str); // Libera a cópia usada para contar

    // Alocar memória para o array de strings

    *result = (char **)malloc(count * sizeof(char *));
    if (*result == NULL || *result[0] == NULL) {
        perror("malloc failed");
        exit(1);
    }

    // Criar uma nova cópia da string para tokenização real
    temp_str = strdup(str);
    if (temp_str == NULL) {
        perror("strdup failed");
        free(*result);
        exit(1);
    }

    token = strtok(temp_str, "/");
    int i = 0;
    while (token != NULL) {
        (*result)[i] = strdup(token);  // Aloca e copia cada substring
        if ((*result)[i] == NULL) {
            perror("strdup failed");
            // Libera memória alocada antes de sair
            for (int j = 0; j < i; j++) {
                free((*result)[j]);
            }
            free(*result);
            free(temp_str);
            exit(1);
        }
        i++;
        token = strtok(NULL, "/");
    }

    free(temp_str);  // Libera a cópia usada para tokenização
    return count;
}

bool copy_host_to_image(const char *host_file, const char *dest_path) {
    FILE *src = fopen(host_file, "rb");
    if (!src) {
        perror("Failed to open source file");
        return false;
    }

    // Extract the file name from the destination path
    char dest_name[11] = {0}; // 8.3 file name format
    uint32_t dest_cluster = navigate_split_path(dest_path, dest_name, true);
    if (dest_cluster == 0) {
        fclose(src);
        fprintf(stderr, "Failed to navigate or create destination path\n");
        return false;
    }

    printf("Destination cluster: %u\n", dest_cluster);

    // Find a free cluster for the file data
    uint32_t free_cluster = find_free_cluster();
    if (free_cluster == 0) {
        fclose(src);
        fprintf(stderr, "No free clusters available\n");
        return false;
    }

    printf("Free cluster: %u\n", free_cluster);

    // Initialize the directory entry
    DirEntry new_entry;
    memset(&new_entry, 0, sizeof(DirEntry));
    snprintf((char *)new_entry.DIR_Name, 11, "%-8.8s%-3.3s", dest_name, ""); // 8.3 format
    new_entry.DIR_Attr = 0x20; // Archive attribute
    new_entry.DIR_FstClusLO = free_cluster & 0xFFFF;
    new_entry.DIR_FstClusHI = (free_cluster >> 16) & 0xFFFF;

    // Get the file size
    fseek(src, 0, SEEK_END);
    new_entry.DIR_FileSize = ftell(src);
    fseek(src, 0, SEEK_SET);

    printf("File size: %u bytes\n", new_entry.DIR_FileSize);

    // Write the directory entry
    uint32_t dest_sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) +
                           (dest_cluster - 2) * bpb.BPB_SecPerClus;
    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    read_sector(dest_sector, buffer);

    bool entry_written = false;
    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry *)(buffer + i);
        if ((unsigned char)e->DIR_Name[0] == 0x00 || (unsigned char)e->DIR_Name[0] == 0xE5) {
            memcpy(e, &new_entry, sizeof(DirEntry));
            write_sector(dest_sector, buffer);
            entry_written = true;
            printf("Directory entry written at sector %u, offset %zu\n", dest_sector, i);
            break;
        }
    }

    if (!entry_written) {
        fclose(src);
        fprintf(stderr, "No free directory entry found\n");
        return false;
    }

    // Write the file data to clusters
    uint32_t current_cluster = free_cluster;
    while (!feof(src)) {
        uint8_t cluster_buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
        size_t read_size = fread(cluster_buffer, 1, sizeof(cluster_buffer), src);

        printf("Read %zu bytes from source file\n", read_size);

        if (read_size == 0) break;

        // Debug: Print the first few bytes of the cluster buffer
        printf("First 16 bytes of cluster buffer: ");
        for (size_t i = 0; i < 16 && i < read_size; i++) {
            printf("%02X ", cluster_buffer[i]);
        }
        printf("\n");

        write_cluster(current_cluster, cluster_buffer);
        printf("Data written to cluster %u\n", current_cluster);

        // Find the next free cluster
        uint32_t next_cluster = find_free_cluster();
        if (next_cluster == 0) {
            fclose(src);
            fprintf(stderr, "No free clusters available during file write\n");
            return false;
        }

        // Update the FAT
        write_fat(current_cluster, next_cluster);
        printf("FAT updated: cluster %u -> %u\n", current_cluster, next_cluster);
        current_cluster = next_cluster;
    }

    // Mark the end of the file in the FAT
    write_fat(current_cluster, 0x0FFFFFFF); // End of cluster chain
    printf("End of file marked at cluster %u\n", current_cluster);

    fclose(src);
    return true;
}


bool copy_image_to_host(const char *src_name, const char *host_path) {
    // Extract the file name from the source path
    char file_name[11] = {0}; // 8.3 file name format
    printf("dir_cluster: %s\n", src_name);

    uint32_t dir_cluster = navigate_split_path(src_name, file_name, true);

    if (dir_cluster == 0) {
        fprintf(stderr, "Failed to navigate source path\n");
        return false;
    }

    printf("Searching for file '%s' in directory cluster %u\n", file_name, dir_cluster);

    // Read the directory cluster to find the file
    uint8_t cluster_buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    if (!read_cluster(dir_cluster, cluster_buffer)) {
        fprintf(stderr, "Failed to read directory cluster\n");
        return false;
    }

    // Search for the file in the directory entries
    DirEntry *entry = NULL;
    for (size_t i = 0; i < sizeof(cluster_buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry *)(cluster_buffer + i);
        char formatted_name[11];
        char formatted_entryname[11];

        snprintf(formatted_name, 11, "%-8.8s%-3.3s", file_name, "");
        snprintf(formatted_entryname, 11, "%-8.8s%-3.3s", e->DIR_Name, "");

        if (strncmp(formatted_entryname, formatted_name, 11) == 0) {
            entry = e;
            break;
        }
    }

    if (!entry) {
        fprintf(stderr, "File '%s' not found in directory\n", file_name);
        return false;
    }

    printf("File found: size=%u bytes, start cluster=%u\n", entry->DIR_FileSize, entry->DIR_FstClusLO | (entry->DIR_FstClusHI << 16));

    // Open the host file for writing
    FILE *host_file = fopen(host_path, "wb");
    if (!host_file) {
        perror("Failed to open host file");
        return false;
    }

    // Read the file data from the FAT32 image and write it to the host file
    uint32_t current_cluster = entry->DIR_FstClusLO | (entry->DIR_FstClusHI << 16);
    uint32_t bytes_remaining = entry->DIR_FileSize;

    while (bytes_remaining > 0) {
        // Read the current cluster
        if (!read_cluster(current_cluster, cluster_buffer)) {
            fprintf(stderr, "Failed to read cluster %u\n", current_cluster);
            fclose(host_file);
            return false;
        }

        // Write the data to the host file
        size_t bytes_to_write = (bytes_remaining < sizeof(cluster_buffer)) ? bytes_remaining : sizeof(cluster_buffer);
        if (fwrite(cluster_buffer, 1, bytes_to_write, host_file) != bytes_to_write) {
            perror("Failed to write to host file");
            fclose(host_file);
            return false;
        }

        // Update the remaining bytes
        bytes_remaining -= bytes_to_write;

        // Get the next cluster from the FAT
        uint32_t next_cluster = 0; // Implement a function to read the FAT entry for `current_cluster`
        if (next_cluster == 0x0FFFFFFF) {
            break; // End of file
        }
        current_cluster = next_cluster;
    }

    fclose(host_file);
    printf("File copied successfully to '%s'\n", host_path);
    return true;
}

bool copy_file_within_image(const char *src_name, const char *dest_path){

    char* src_name_without_slash = src_name + 1;
        printf("src_name: %s\n", src_name);
    printf("dest_path: %s\n", dest_path);
    printf("src_name_without_slash: %s\n", src_name_without_slash);
    uint32_t src_cluster = navigate_split_path(src_name, src_name_without_slash, true);

    printf("src_cluster: %d\n", src_cluster);

    uint8_t buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
    uint32_t sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (src_cluster - 2) * bpb.BPB_SecPerClus;
    read_sector(sector, buffer);
    DirEntry *src_entry = NULL;
    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry*)(buffer + i);
        char formatted_name[11];  
        char formatted_dirname[11];
        snprintf(formatted_name, 11, "%-8.8s%-3.3s", src_name_without_slash, "");
        snprintf(formatted_dirname, 11, "%-8.8s%-3.3s", e->DIR_Name, "");
        if (strncmp(formatted_dirname, formatted_name, 11) == 0) {
            src_entry = e;
            break;
        }
    }

    if (!src_entry) return false; // Arquivo origem não encontrado

    char dest_name[11];

    uint32_t dest_cluster = navigate_split_path(dest_path, dest_name, true);
    printf("Destino: %s\n", dest_name);
    if (dest_cluster == 0) return false; // Diretório não encontrado

    uint32_t free_cluster = find_free_cluster();
    if (free_cluster == 0) return false; // Sem espaço livre

    DirEntry new_entry;
    memcpy(&new_entry, src_entry, sizeof(DirEntry));
    snprintf(new_entry.DIR_Name, 11, "%-8.8s%-3.3s", dest_name, "");
    new_entry.DIR_FstClusLO = free_cluster & 0xFFFF;
    new_entry.DIR_FstClusHI = (free_cluster >> 16) & 0xFFFF;

    // Insere no diretório de destino
    uint32_t dest_sector = bpb.BPB_RsvdSecCnt + (bpb.BPB_NumFATs * bpb.BPB_FATSz32) + (dest_cluster - 2) * bpb.BPB_SecPerClus;
    read_sector(dest_sector, buffer);
    for (size_t i = 0; i < sizeof(buffer); i += sizeof(DirEntry)) {
        DirEntry *e = (DirEntry*)(buffer + i);
        if ((unsigned char)e->DIR_Name[0] == 0x00 || (unsigned char)e->DIR_Name[0] == 0xE5) {
            memcpy(e, &new_entry, sizeof(DirEntry));
            write_sector(dest_sector, buffer);
            break;
        }
    }

    
    uint32_t dest_data_cluster = free_cluster;
    while (src_cluster < 0x0FFFFFF8) {
        uint8_t cluster_buffer[bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus];
        read_sector(bpb.BPB_RsvdSecCnt + (src_cluster - 2) * bpb.BPB_SecPerClus, cluster_buffer);
        write_sector(bpb.BPB_RsvdSecCnt + (dest_data_cluster - 2) * bpb.BPB_SecPerClus, cluster_buffer);
        
        uint32_t next_src_cluster = next_cluster(src_cluster);
        uint32_t next_dest_cluster = find_free_cluster();
        if (next_dest_cluster == 0) return false; // Falha ao encontrar espaço livre

        write_fat(dest_data_cluster, next_dest_cluster);
        dest_data_cluster = next_dest_cluster;
        src_cluster = next_src_cluster;
    }
    write_fat(dest_data_cluster, 0x0FFFFFFF); // Finaliza cópia

    return true;
}

void remove_img(char *path) {
    char *pos = strstr(path, "img");  // encontra a primeira ocorrência de "img"
    
    if (pos != NULL) {
        // desloca o restante da string para a esquerda, removendo "img"
        memmove(pos, pos + 3, strlen(pos + 3) + 1);  // 3 é o tamanho de "img"
    }
}


bool copy_file(const char *src_name, const char *dest_path) {
    bool src_is_img = (strncmp(src_name, "img/", 4) == 0);
    bool dest_is_img = (strncmp(dest_path, "img/", 4) == 0);

    remove_img(src_name);
    remove_img(dest_path);

    if (src_is_img && dest_is_img) {
        // Copiar dentro da imagem FAT32
        return copy_file_within_image(src_name, dest_path);
    } else if (!src_is_img && dest_is_img) {
        // Copiar do host para a imagem FAT32
        return copy_host_to_image(src_name, dest_path);
    } else if (src_is_img && !dest_is_img) {
        // Copiar da imagem FAT32 para o host
        return copy_image_to_host(src_name, dest_path);
    }
    
    
}


bool move_file(const char *src_name, const char *dest_name) {
    if (!copy_file(src_name, dest_name)) return false;
    if (!delete_file(src_name, false)) return false;
    return true;
}

