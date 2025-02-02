#include <stddef.h>
#include <stdio.h>
#include <string.h>

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

char** split_path(const char *path) {
    char **result;
    int count = split_string(path, &result);
    return result;
}
