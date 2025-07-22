// miniFS/shell.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>  // Necessário para wchar_t, wcslen, wmemcpy
#include <locale.h> // Necessário para setlocale (já no main, mas bom aqui)
#include "shell.h"
#include "fs.h"
#include "utils.h" // Necessário para as funções de conversão wcs_to_char

#define MAX_INPUT 1024
#define JSON_TREE_FILE "fs_tree.json"

void print_prompt() {
    // Se o diretório atual é a raiz, simplesmente imprime "/".
    if (current_dir == root) {
        printf("MiniFS:/$ ");
        return;
    }

    // Usar wchar_t para construir o caminho, pois Node->name é wchar_t.
    wchar_t path_buffer_wcs[1024]; // Buffer para o caminho completo em wchar_t
    // Inicializa o ponteiro 'p_wcs' para o final do buffer e adiciona o terminador nulo wide.
    wchar_t *p_wcs = &path_buffer_wcs[sizeof(path_buffer_wcs)/sizeof(wchar_t) - 1]; 
    *p_wcs = L'\0'; 

    // Percorre do diretório atual até a raiz para construir o caminho.
    for (Node *temp = current_dir; temp != root; temp = temp->parent) {
        size_t name_len_wcs = wcslen(temp->name); // Usa wcslen para o comprimento do nome wchar_t.
        size_t needed_wcs = name_len_wcs + 1; // Espaço necessário para o nome + a barra '/'.

        // Verifica se há espaço suficiente no buffer.
        if ((p_wcs - path_buffer_wcs) < needed_wcs) {
            break; // Caminho muito longo, mostra um caminho parcial.
        }

        p_wcs -= name_len_wcs; // Move o ponteiro para trás para copiar o nome.
        wmemcpy(p_wcs, temp->name, name_len_wcs); // Copia o nome (wmemcpy para wchar_t).
        p_wcs--; // Move o ponteiro para trás para adicionar a barra.
        *p_wcs = L'/'; // Adiciona a barra (L'/' para wide char).
    }

    // Converte o caminho final de wchar_t* para char* para ser impresso com printf.
    char* final_path_char = wcs_to_char(p_wcs); 
    if(final_path_char) {
        printf("MiniFS:%s$ ", final_path_char);
        free(final_path_char); // Libera a memória alocada por wcs_to_char.
    } else {
        printf("MiniFS:/?$ "); // Fallback se a conversão falhar.
    }
}

void shell_loop() {
    char input[MAX_INPUT];
    int running = 1;

    while (running) {
        print_prompt();
        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("\n"); // Lida com Ctrl+D (EOF).
            break; 
        }

        // Remove o caractere de nova linha lido por fgets.
        input[strcspn(input, "\n")] = '\0'; 

        int argc = 0;
        char** argv = split_string(input, &argc); // split_string retorna char**.

        if (argc == 0) { // Se não há comandos digitados, continua.
            free_tokens(argv);
            continue;
        }

        char* cmd = argv[0]; // O primeiro token é o comando.

        // Dispatch de comandos. As funções fs_ já lidam com a conversão char* para wchar_t* internamente.
        if (strcmp(cmd, "exit") == 0) {
            running = 0;
        } else if (strcmp(cmd, "mkdir") == 0) {
            if (argc > 1) fs_mkdir(argv[1]);
            else fprintf(stderr, "mkdir: missing operand\n");
        } else if (strcmp(cmd, "touch") == 0) {
            if (argc > 1) fs_touch(argv[1]);
            else fprintf(stderr, "touch: missing operand\n");
        } else if (strcmp(cmd, "ls") == 0) {
            fs_ls(argc > 1 ? argv[1] : ""); // Lista o diretório fornecido ou o atual se nenhum.
        } else if (strcmp(cmd, "cd") == 0) {
            if (argc > 1) fs_cd(argv[1]);
            else fs_cd("/"); // cd para a raiz por padrão.
        } else if (strcmp(cmd, "pwd") == 0) {
            fs_pwd();
        } else if (strcmp(cmd, "rm") == 0) {
            if (argc > 1) fs_rm(argv[1]);
            else fprintf(stderr, "rm: missing operand\n");
        } else if (strcmp(cmd, "cat") == 0) {
            if (argc > 1) fs_cat(argv[1]);
            else fprintf(stderr, "cat: missing operand\n");
        } else if (strcmp(cmd, "mv") == 0) {
            if (argc > 2) fs_mv(argv[1], argv[2]);
            else fprintf(stderr, "Usage: mv <source> <destination>\n");
        } else if (strcmp(cmd, "cp") == 0) {
            if (argc > 2) fs_cp(argv[1], argv[2]);
            else fprintf(stderr, "Usage: cp <source> <destination>\n");
        } else if (strcmp(cmd, "tree") == 0) {
            fs_export_tree_json(JSON_TREE_FILE);
        } else if (strcmp(cmd, "echo") == 0) {
            // Lógica para 'echo <content> > <filepath>'.
            if (argc > 3 && strcmp(argv[argc - 2], ">") == 0) {
                char content[MAX_INPUT] = ""; // Buffer para reconstruir o conteúdo.
                // Concatena todos os argumentos de conteúdo.
                for (int i = 1; i < argc - 2; i++) {
                    strcat(content, argv[i]);
                    if (i < argc - 3) strcat(content, " "); // Adiciona espaço entre palavras.
                }
                fs_echo(argv[argc - 1], content); // Chama fs_echo com o caminho e o conteúdo.
            } else {
                fprintf(stderr, "Usage: echo <content> > <filepath>\n");
            }
        } else {
            fprintf(stderr, "%s: command not found\n", cmd);
        }

        free_tokens(argv); // Libera a memória alocada por split_string.
    }
}