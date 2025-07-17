// miniFS/shell.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "fs.h"
#include "utils.h"

#define MAX_INPUT 1024
#define JSON_TREE_FILE "fs_tree.json"

void print_prompt() {
    char path_buffer[1024];
    char *final_path;

    if (current_dir == root) {
        final_path = "/";
    } else {
        char *p = &path_buffer[sizeof(path_buffer) - 1];
        *p = '\0';

        for (Node *temp = current_dir; temp != root; temp = temp->parent) {
            size_t name_len = strlen(temp->name);
            size_t needed = name_len + 1; // for name + '/'

            if ((p - path_buffer) < needed) {
                break; // Path too long, show partial path
            }

            p -= name_len;
            memcpy(p, temp->name, name_len);
            p--;
            *p = '/';
        }
        final_path = p;
    }
    printf("MiniFS:%s$ ", final_path);
}

void shell_loop() {
    char input[MAX_INPUT];
    int running = 1;

    while (running) {
        print_prompt();
        if (!fgets(input, MAX_INPUT, stdin)) {
            printf("\n"); // Handle Ctrl+D
            break; 
        }

        int argc = 0;
        char** argv = split_string(input, &argc);

        if (argc == 0) {
            free_tokens(argv);
            continue;
        }

        char* cmd = argv[0];

        if (strcmp(cmd, "exit") == 0) {
            running = 0;
        } else if (strcmp(cmd, "mkdir") == 0) {
            if (argc > 1) fs_mkdir(argv[1]);
            else fprintf(stderr, "mkdir: missing operand\n");
        } else if (strcmp(cmd, "touch") == 0) {
            if (argc > 1) fs_touch(argv[1]);
            else fprintf(stderr, "touch: missing operand\n");
        } else if (strcmp(cmd, "ls") == 0) {
            fs_ls(argc > 1 ? argv[1] : "");
        } else if (strcmp(cmd, "cd") == 0) {
            if (argc > 1) fs_cd(argv[1]);
            else fs_cd("/"); // cd para a raiz por padrão
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
            if (argc > 3 && strcmp(argv[argc - 2], ">") == 0) {
                char content[MAX_INPUT] = "";
                for (int i = 1; i < argc - 2; i++) {
                    strcat(content, argv[i]);
                    if (i < argc - 3) strcat(content, " ");
                }
                fs_echo(argv[argc - 1], content);
            } else {
                fprintf(stderr, "Usage: echo <content> > <filepath>\n");
            }
        } else {
            fprintf(stderr, "%s: command not found\n", cmd);
        }

        free_tokens(argv);
    }
}