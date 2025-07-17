// miniFS/main.c

#include <stdio.h>
#include "fs.h"
#include "shell.h"

#define SAVE_FILE "minifs.dat"

int main() {
    // Tenta carregar o estado anterior do sistema de arquivos
    fs_load(SAVE_FILE);

    // Inicia o loop do shell
    shell_loop();

    // Salva o estado atual do sistema de arquivos ao sair
    fs_save(SAVE_FILE);

    // Libera toda a memória alocada para a árvore
    fs_destroy(root);

    printf("Exiting MiniFS. Goodbye!\n");
    return 0;
}