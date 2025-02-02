#include "fat32.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Uso: %s <imagem.img>\n", argv[0]);
        return 1;
    }

    if (disk_init(argv[1]) != 0) {
        printf(disk_init(argv[1]));
        printf("Erro ao abrir a imagem.\n");
        return 1;
    }

    start_shell();
    return 0;
}