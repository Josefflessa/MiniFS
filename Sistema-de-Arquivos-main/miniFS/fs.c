// miniFS/fs.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h> // Essencial para basename() e dirname()
#include "fs.h"

// Definição das variáveis globais declaradas em fs.h
Node *root;
Node *current_dir;

// --- Protótipos de Funções Estáticas (Auxiliares Internas) ---
static Node* find_node_in_dir(Node* dir, const char* name);
static Node* find_node_by_path(const char *path);
static Node* get_parent_dir_and_basename(const char* path, char* out_basename);
static void detach_node(Node* node);
static void attach_node(Node* parent, Node* child);
static Node* copy_node_recursive(Node* source, Node* new_parent);
void save_node_recursive(FILE *file, Node *node);
Node* load_node_recursive(FILE *file, Node *parent);
void export_recursive(FILE *file, Node *node, int is_last);


// --- Funções Auxiliares de Manipulação da Árvore ---

static Node* find_node_in_dir(Node* dir, const char* name) {
    if (!dir || dir->type != DIR_NODE) return NULL;
    Node* current = dir->child;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static Node* find_node_by_path(const char *path) {
    if (path == NULL || strlen(path) == 0) return current_dir;
    if (strcmp(path, "/") == 0) return root;

    char *path_copy = strdup(path);
    if (!path_copy) return NULL;
    
    Node *start_node = (path[0] == '/') ? root : current_dir;
    Node *current_node = start_node;

    char *token = strtok(path_copy, "/");
    while (token != NULL && current_node != NULL) {
        if (strcmp(token, "..") == 0) {
            current_node = current_node->parent ? current_node->parent : root;
        } else if (strcmp(token, ".") != 0) {
            current_node = find_node_in_dir(current_node, token);
        }
        token = strtok(NULL, "/");
    }

    free(path_copy);
    return current_node;
}

static Node* get_parent_dir_and_basename(const char* path, char* out_basename) {
    char* path_copy1 = strdup(path);
    char* path_copy2 = strdup(path);
    if (!path_copy1 || !path_copy2) {
        if(path_copy1) free(path_copy1);
        if(path_copy2) free(path_copy2);
        return NULL;
    }

    char* bname = basename(path_copy1);
    char* dname = dirname(path_copy2);
    
    strcpy(out_basename, bname);
    Node* parent_dir = find_node_by_path(dname);

    free(path_copy1);
    free(path_copy2);
    return parent_dir;
}


// --- Inicialização e Destruição ---

void fs_init() {
    root = (Node*)malloc(sizeof(Node));
    if (!root) { perror("Failed to allocate root"); exit(1); }
    strcpy(root->name, "/");
    root->type = DIR_NODE;
    root->parent = NULL;
    root->child = NULL;
    root->next = NULL;
    root->content = NULL;
    current_dir = root;
}

void fs_destroy(Node *node) {
    if (node == NULL) return;
    fs_destroy(node->child);
    fs_destroy(node->next);
    if (node->type == FILE_NODE && node->content != NULL) {
        free(node->content);
    }
    free(node);
}

// --- Comandos do Sistema de Arquivos (API Pública) ---

void fs_mkdir(const char *path) {
    char name[100];
    Node *parent = get_parent_dir_and_basename(path, name);

    if (!parent) {
        fprintf(stderr, "mkdir: cannot create directory '%s': No such file or directory\n", path);
        return;
    }
    if (find_node_in_dir(parent, name) != NULL) {
        fprintf(stderr, "mkdir: cannot create directory '%s': File or directory exists\n", name);
        return;
    }

    Node* new_dir = (Node*)malloc(sizeof(Node));
    strcpy(new_dir->name, name);
    new_dir->type = DIR_NODE;
    new_dir->content = NULL;
    new_dir->child = NULL;
    attach_node(parent, new_dir);
}

void fs_touch(const char *path) {
    char name[100];
    Node *parent = get_parent_dir_and_basename(path, name);
    if (!parent) {
        fprintf(stderr, "touch: cannot create file '%s': No such file or directory\n", path);
        return;
    }
    if (find_node_in_dir(parent, name)) {
        return;
    }
    
    Node* new_file = (Node*)malloc(sizeof(Node));
    strcpy(new_file->name, name);
    new_file->type = FILE_NODE;
    new_file->content = NULL;
    new_file->child = NULL;
    attach_node(parent, new_file);
}

void fs_ls(const char *path) {
    Node* dir_to_list = find_node_by_path(path);

    if (dir_to_list == NULL) {
        fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", path);
        return;
    }
    if (dir_to_list->type != DIR_NODE) {
        printf("%s\n", dir_to_list->name);
        return;
    }

    Node *current = dir_to_list->child;
    while (current != NULL) {
        if (current->type == DIR_NODE) {
            printf("d %s/\n", current->name);
        } else {
            size_t size = current->content ? strlen(current->content) : 0;
            printf("- %s (%zu bytes)\n", current->name, size);
        }
        current = current->next;
    }
}

void fs_cd(const char *path) {
    Node *target = find_node_by_path(path);
    if (target == NULL) {
        fprintf(stderr, "cd: %s: No such file or directory\n", path);
    } else if (target->type != DIR_NODE) {
        fprintf(stderr, "cd: %s: Not a directory\n", path);
    } else {
        current_dir = target;
    }
}

void fs_pwd() {
    if (current_dir == root) {
        printf("/\n");
        return;
    }

    char path_buffer[1024];
    char *p = &path_buffer[sizeof(path_buffer) - 1];
    *p = '\0'; // Null-terminate at the very end

    for (Node *temp = current_dir; temp != root; temp = temp->parent) {
        size_t name_len = strlen(temp->name);
        size_t needed = name_len + 1; // for name + '/'

        // Check if there is enough space to prepend this part
        if ((p - path_buffer) < needed) {
            // Not enough space, path will be truncated. Break and show what we have.
            break; 
        }

        p -= name_len;
        memcpy(p, temp->name, name_len);
        p--;
        *p = '/';
    }
    printf("%s\n", p);
}

void fs_rm(const char *path) {
    Node *target = find_node_by_path(path);
    if (target == NULL) {
        fprintf(stderr, "rm: cannot remove '%s': No such file or directory\n", path);
        return;
    }
    if (target == root) {
        fprintf(stderr, "rm: cannot remove root directory '/'\n");
        return;
    }
    if (target->type == DIR_NODE && target->child != NULL) {
        fprintf(stderr, "rm: cannot remove '%s': Directory not empty\n", path);
        return;
    }
    
    detach_node(target);
    target->next = NULL;
    fs_destroy(target);
}

void fs_cat(const char *path) {
    Node *target = find_node_by_path(path);
    if (target == NULL) {
        fprintf(stderr, "cat: %s: No such file or directory\n", path);
    } else if (target->type != FILE_NODE) {
        fprintf(stderr, "cat: %s: Is a directory\n", path);
    } else if (target->content) {
        printf("%s\n", target->content);
    }
}

void fs_echo(const char *path, const char *content) {
    char name[100];
    Node *parent = get_parent_dir_and_basename(path, name);
    if (!parent) {
        fprintf(stderr, "echo: cannot write to '%s': No such file or directory\n", path);
        return;
    }
    
    Node *target = find_node_in_dir(parent, name);
    if (target == NULL) {
        fs_touch(path);
        target = find_node_in_dir(parent, name);
        if (!target) return;
    }

    if (target->type != FILE_NODE) {
        fprintf(stderr, "echo: %s: Is a directory\n", name);
        return;
    }

    if (target->content) free(target->content);
    target->content = strdup(content);
}


// --- Funções de Mover e Copiar (Lógica Principal) ---

static void detach_node(Node* node) {
    if (!node || !node->parent) return;
    Node* parent = node->parent;
    if (parent->child == node) {
        parent->child = node->next;
    } else {
        Node* sibling = parent->child;
        while (sibling && sibling->next != node) {
            sibling = sibling->next;
        }
        if (sibling) sibling->next = node->next;
    }
    node->parent = NULL;
    node->next = NULL;
}

static void attach_node(Node* parent, Node* child) {
    if (!parent || parent->type != DIR_NODE || !child) return;
    child->parent = parent;
    child->next = NULL;
    if (parent->child == NULL) {
        parent->child = child;
    } else {
        Node* sibling = parent->child;
        while (sibling->next != NULL) sibling = sibling->next;
        sibling->next = child;
    }
}

void fs_mv(const char *source_path, const char *dest_path) {
    Node *source_node = find_node_by_path(source_path);
    if (!source_node || source_node == root) {
        fprintf(stderr, "mv: cannot move '%s': Invalid source or root\n", source_path);
        return;
    }
    
    Node *dest_target = find_node_by_path(dest_path);
    Node *dest_parent;
    char new_name[100];

    if (dest_target && dest_target->type == DIR_NODE) {
        dest_parent = dest_target;
        strcpy(new_name, source_node->name);
    } else {
        dest_parent = get_parent_dir_and_basename(dest_path, new_name);
    }
    
    if (!dest_parent) {
        fprintf(stderr, "mv: cannot move to '%s': Destination path not found\n", dest_path);
        return;
    }
    if (find_node_in_dir(dest_parent, new_name)) {
        fprintf(stderr, "mv: cannot move to '%s': Destination already exists\n", dest_path);
        return;
    }
    
    detach_node(source_node);
    strcpy(source_node->name, new_name);
    attach_node(dest_parent, source_node);
}

static Node* copy_node_recursive(Node* source, Node* new_parent) {
    if (!source) return NULL;
    
    Node* new_node = (Node*)malloc(sizeof(Node));
    strcpy(new_node->name, source->name);
    new_node->type = source->type;
    new_node->parent = new_parent;
    new_node->content = source->content ? strdup(source->content) : NULL;
    new_node->next = NULL;
    
    Node *child_head = NULL;
    Node *last_copied_child = NULL;
    for (Node *source_child = source->child; source_child != NULL; source_child = source_child->next) {
        Node* copied_child = copy_node_recursive(source_child, new_node);
        if (child_head == NULL) {
            child_head = copied_child;
        } else {
            last_copied_child->next = copied_child;
        }
        last_copied_child = copied_child;
    }
    new_node->child = child_head;
    return new_node;
}

void fs_cp(const char *source_path, const char *dest_path) {
    Node *source_node = find_node_by_path(source_path);
    if (!source_node) {
        fprintf(stderr, "cp: cannot stat '%s': No such file or directory\n", source_path);
        return;
    }

    Node* dest_target = find_node_by_path(dest_path);
    Node* dest_parent;
    char new_name[100];

    if(dest_target && dest_target->type == DIR_NODE){
        dest_parent = dest_target;
        strcpy(new_name, source_node->name);
    } else {
        dest_parent = get_parent_dir_and_basename(dest_path, new_name);
    }

    if (!dest_parent) {
        fprintf(stderr, "cp: cannot copy to '%s': Destination path not found\n", dest_path);
        return;
    }
    if (find_node_in_dir(dest_parent, new_name)) {
        fprintf(stderr, "cp: cannot copy to '%s': Destination already exists\n", dest_path);
        return;
    }
    
    Node* new_node = copy_node_recursive(source_node, dest_parent);
    strcpy(new_node->name, new_name);
    attach_node(dest_parent, new_node);
}

// --- Funções de Serialização (Save/Load) e Exportação ---

void save_node_recursive(FILE *file, Node *node) {
    if (node == NULL) return;

    fwrite(&node->type, sizeof(NodeType), 1, file);
    size_t name_len = strlen(node->name) + 1;
    fwrite(&name_len, sizeof(size_t), 1, file);
    fwrite(node->name, sizeof(char), name_len, file);

    if (node->type == FILE_NODE) {
        size_t content_len = node->content ? strlen(node->content) + 1 : 0;
        fwrite(&content_len, sizeof(size_t), 1, file);
        if (content_len > 0) {
            fwrite(node->content, sizeof(char), content_len, file);
        }
    }

    int child_count = 0;
    for (Node *child = node->child; child; child = child->next) child_count++;
    fwrite(&child_count, sizeof(int), 1, file);
    
    for (Node *child = node->child; child; child = child->next) {
        save_node_recursive(file, child);
    }
}

void fs_save(const char* filepath) {
    FILE *file = fopen(filepath, "wb");
    if (!file) { perror("Error opening file for saving"); return; }
    save_node_recursive(file, root);
    fclose(file);
    printf("File system saved to %s\n", filepath);
}

Node* load_node_recursive(FILE *file, Node *parent) {
    NodeType type;
    if (fread(&type, sizeof(NodeType), 1, file) != 1) return NULL;

    size_t name_len;
    fread(&name_len, sizeof(size_t), 1, file);
    char name[100];
    fread(name, sizeof(char), name_len, file);

    Node* new_node = (Node*)malloc(sizeof(Node));
    strcpy(new_node->name, name);
    new_node->type = type;
    new_node->parent = parent;
    
    // CORREÇÃO: Atribuições separadas para evitar erro de tipo de ponteiro incompatível.
    new_node->child = NULL;
    new_node->next = NULL;
    new_node->content = NULL;

    if (type == FILE_NODE) {
        size_t content_len;
        fread(&content_len, sizeof(size_t), 1, file);
        if (content_len > 0) {
            new_node->content = (char*)malloc(content_len);
            fread(new_node->content, sizeof(char), content_len, file);
        }
    }

    int child_count;
    fread(&child_count, sizeof(int), 1, file);
    
    Node *prev_child = NULL;
    for (int i = 0; i < child_count; i++) {
        Node *child_node = load_node_recursive(file, new_node);
        if (i == 0) new_node->child = child_node;
        else prev_child->next = child_node;
        prev_child = child_node;
    }
    return new_node;
}

void fs_load(const char* filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        printf("No save file found. Starting a new file system.\n");
        fs_init();
        return;
    }
    fs_destroy(root);
    root = load_node_recursive(file, NULL);
    current_dir = root;
    fclose(file);
    printf("File system loaded from %s\n", filepath);
}

void export_recursive(FILE *file, Node *node, int is_last) {
    if (node == NULL) return;
    fprintf(file, "{");
    fprintf(file, "\"name\":\"%s\",", node->name);
    fprintf(file, "\"type\":\"%s\"", (node->type == DIR_NODE) ? "directory" : "file");

    if (node->type == DIR_NODE && node->child) {
        fprintf(file, ",\"children\":[");
        Node *child = node->child;
        while (child) {
            export_recursive(file, child, child->next == NULL);
            child = child->next;
        }
        fprintf(file, "]");
    }
    fprintf(file, "}");
    if (!is_last) fprintf(file, ",");
}

void fs_export_tree_json(const char *filepath) {
    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Error opening file for JSON export");
        return;
    }
    export_recursive(file, root, 1);
    fclose(file);
    printf("File system tree exported to %s\n", filepath);
}