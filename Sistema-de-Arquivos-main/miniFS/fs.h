// miniFS/fs.h

#ifndef FS_H
#define FS_H

#include <stddef.h> // Para size_t

// 1. Estruturas de Dados
typedef enum { FILE_NODE, DIR_NODE } NodeType;

typedef struct Node {
    char name[100];
    NodeType type;
    struct Node *parent;
    struct Node *child;    // Ponteiro para o primeiro filho
    struct Node *next;     // Ponteiro para o próximo irmão
    char *content;         // Conteúdo, se for um arquivo
} Node;

// 2. Variáveis Globais (Estado do Sistema)
extern Node *root;
extern Node *current_dir;

// 3. Funções da API do Sistema de Arquivos
void fs_init();
void fs_destroy(Node *node);

// Funções agora recebem caminhos (paths)
void fs_mkdir(const char *path);
void fs_touch(const char *path);
void fs_ls(const char *path);
void fs_cd(const char *path);
void fs_rm(const char *path);
void fs_cat(const char *path);
void fs_echo(const char *path, const char *content);

// Novas funções
void fs_mv(const char *source_path, const char *dest_path);
void fs_cp(const char *source_path, const char *dest_path);

// Funções existentes
void fs_pwd();

// Funções de Serialização e Visualização
void fs_save(const char* filepath);
void fs_load(const char* filepath);
void fs_export_tree_json(const char* filepath); // Exporta para o Python ler

#endif // FS_H