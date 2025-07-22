#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>  // Necessário para wchar_t, wcscpy, wcslen, etc.
#include <wctype.h> // Necessário para wcsrchr
#include <locale.h> // Necessário para setlocale (apesar de já no main.c, garante)

#include "fs.h"
#include "utils.h" // Inclui utils.h para acessar wcs_dup, char_to_wcs, wcs_to_char

// Definição das variáveis globais declaradas em fs.h
Node *root = NULL; // É uma boa prática inicializar com NULL
Node *current_dir = NULL;

// --- Protótipos de Funções Estáticas (Auxiliares Internas) ---
// *******************************************************************
// ATENÇÃO: Assinaturas foram atualizadas para wchar_t* onde necessário
// *******************************************************************
static Node* find_node_in_dir(Node* dir, const wchar_t* name); // Alterado de char* para wchar_t*
static Node* find_node_by_path(const char *path); // Permanece char* na entrada, conversão interna
static Node* get_parent_dir_and_basename(const char* path, wchar_t* out_basename); // Alterado de char* para wchar_t*
static void detach_node(Node* node);
static void attach_node(Node* parent, Node* child);
static Node* copy_node_recursive(Node* source, Node* new_parent);
static void save_node_recursive(FILE *file, Node *node); // Funções recursivas geralmente são static
static Node* load_node_recursive(FILE *file, Node *parent); // Funções recursivas geralmente são static
static void export_recursive(FILE *file, Node *node, int is_last);


// --- Funções Auxiliares de Manipulação da Árvore ---

static char* escape_json_string_content(const char* raw_string) {
    if (!raw_string) return strdup(""); // Retorna string vazia para NULL

    size_t len = strlen(raw_string);
    // Aloca um buffer grande o suficiente para a pior das hipóteses (cada caractere escapado)
    // O maximo de um char é 4 bytes + '\0'. Se cada char for escapado (ex: " -> \"),
    // o tamanho pode dobrar.
    char* escaped_string = (char*)malloc(len * 2 + 1); // +1 para null terminator
    if (!escaped_string) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (raw_string[i] == '"') {
            escaped_string[j++] = '\\';
            escaped_string[j++] = '"';
        } else if (raw_string[i] == '\\') {
            escaped_string[j++] = '\\';
            escaped_string[j++] = '\\';
        }
        // Adicione aqui outros caracteres que você queira escapar, como \n, \r, \t, etc.
        // Para o seu caso, aspas e barras invertidas são as mais importantes.
        else {
            escaped_string[j++] = raw_string[i];
        }
    }
    escaped_string[j] = '\0'; // Termina a string escapada
    return escaped_string;
}

static Node* find_node_in_dir(Node* dir, const wchar_t* name) {
    if (!dir || dir->type != DIR_NODE) return NULL;
    Node* current = dir->child;
    while (current != NULL) {
        if (wcscmp(current->name, name) == 0) { // Usa wcscmp para comparar wchar_t*
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static Node* find_node_by_path(const char *path) {
    if (path == NULL || strlen(path) == 0) return current_dir;
    if (strcmp(path, "/") == 0) return root;

    // Converte o path de char* para wchar_t* para manipulação interna
    // Esta é a string original, que NÃO será modificada por wcstok.
    wchar_t *original_path_wcs = char_to_wcs(path);
    if (!original_path_wcs) return NULL;

    // Cria uma CÓPIA explícita para ser modificada por wcstok.
    // É esta cópia que será tokenizada.
    wchar_t *path_wcs_for_tok = wcs_dup(original_path_wcs);
    if (!path_wcs_for_tok) {
        free(original_path_wcs); // Libera a primeira alocação se a segunda falhar
        return NULL;
    }

    Node *start_node = (path[0] == '/') ? root : current_dir;
    Node *current_node = start_node;

    wchar_t *token_wcs;
    wchar_t *saveptr; // Para wcstok_s ou wcstok (no Linux)

    // O primeiro argumento de wcstok é o ponteiro para a string a ser tokenizada.
    // As chamadas subsequentes usam NULL como primeiro argumento.
    token_wcs = wcstok(path_wcs_for_tok, L"/", &saveptr);
    while (token_wcs != NULL && current_node != NULL) {
        if (wcscmp(token_wcs, L"..") == 0) {
            current_node = current_node->parent ? current_node->parent : root;
        } else if (wcscmp(token_wcs, L".") != 0) {
            current_node = find_node_in_dir(current_node, token_wcs);
        }
        token_wcs = wcstok(NULL, L"/", &saveptr); // Chamadas subsequentes
    }

    // Liberar AMBAS as cópias alocadas.
    free(path_wcs_for_tok); // Libera a cópia que wcstok modificou
    free(original_path_wcs); // Libera a cópia original

    return current_node;
}

static Node* get_parent_dir_and_basename(const char* path, wchar_t* out_basename) {
    // Converte o path de char* para wchar_t*
    wchar_t* path_wcs_copy = char_to_wcs(path);
    if (!path_wcs_copy) return NULL;

    wchar_t* last_slash = wcsrchr(path_wcs_copy, L'/');
    wchar_t* basename_wcs;
    Node* parent_dir = NULL;

    if (last_slash == NULL) { // Não há slash, é um nome no diretório atual
        basename_wcs = path_wcs_copy; // O nome é o próprio path_wcs_copy
        parent_dir = current_dir; // O pai é o diretório atual
    } else if (last_slash == path_wcs_copy) { // É a raiz (ex: /file)
        basename_wcs = path_wcs_copy + 1; // O nome começa depois da barra
        parent_dir = root; // O pai é a raiz
    } else { // Tem um caminho e um nome (ex: /a/b/file)
        *last_slash = L'\0'; // Termina o diretório pai (temporariamente)
        
        // Converte o diretório pai (dirname) de volta para char* para usar find_node_by_path
        char* dirname_char = wcs_to_char(path_wcs_copy);
        if(dirname_char) {
            parent_dir = find_node_by_path(dirname_char);
            free(dirname_char); // Liberar o buffer char*
        }
        basename_wcs = last_slash + 1; // O nome é depois da última barra
    }
    
    if (parent_dir) {
        // Garante que o buffer de saída 'out_basename' é grande o suficiente
        wcscpy(out_basename, basename_wcs); // Copia o nome base para o buffer de saída
    }
    
    free(path_wcs_copy); // Libera a cópia wide
    return parent_dir;
}


// --- Inicialização e Destruição ---

void fs_init() {
    root = (Node*)malloc(sizeof(Node));
    if (!root) { perror("Failed to allocate root"); exit(1); }
    wcscpy(root->name, L"/"); // Usa wcscpy e literal L"/"
    root->type = DIR_NODE;
    root->parent = NULL;
    root->child = NULL;
    root->next = NULL;
    root->content = NULL; // Conteúdo de diretórios é sempre NULL
    current_dir = root;
}

void fs_destroy(Node *node) {
    if (node == NULL) return;
    fs_destroy(node->child);
    fs_destroy(node->next);
    if (node->type == FILE_NODE && node->content != NULL) {
        free(node->content); // 'free' funciona para qualquer ponteiro alocado com malloc/strdup, mesmo wchar_t
    }
    free(node);
}

// --- Comandos do Sistema de Arquivos (API Pública) ---

void fs_mkdir(const char *path) {
    wchar_t name_wcs[100]; // Buffer para o nome em wchar_t
    Node *parent = get_parent_dir_and_basename(path, name_wcs); // get_parent_dir_and_basename preenche name_wcs

    if (!parent) {
        fprintf(stderr, "mkdir: cannot create directory '%s': No such file or directory\n", path);
        return;
    }
    if (parent->type != DIR_NODE) { // Adiciona verificação se o pai é um diretório
        fprintf(stderr, "mkdir: parent path '%s' is not a directory\n", path);
        return;
    }

    // Converte name_wcs para char* temporariamente para mensagem de erro
    char* name_char_for_err = wcs_to_char(name_wcs); 

    if (find_node_in_dir(parent, name_wcs) != NULL) { // find_node_in_dir espera wchar_t*
        if(name_char_for_err) {
            fprintf(stderr, "mkdir: cannot create directory '%s': File or directory exists\n", name_char_for_err);
            free(name_char_for_err); // Libera o buffer
        }
        return;
    }
    if(name_char_for_err) free(name_char_for_err); // Libera o buffer

    Node* new_dir = (Node*)malloc(sizeof(Node));
    if (!new_dir) { perror("mkdir: Failed to allocate new directory"); return; }
    wcscpy(new_dir->name, name_wcs); // Usa wcscpy
    new_dir->type = DIR_NODE;
    new_dir->content = NULL;
    new_dir->child = NULL;
    attach_node(parent, new_dir);
}

void fs_touch(const char *path) {
    wchar_t name_wcs[100]; // Buffer para o nome em wchar_t
    Node *parent = get_parent_dir_and_basename(path, name_wcs); // get_parent_dir_and_basename preenche name_wcs
    if (!parent) {
        fprintf(stderr, "touch: cannot create file '%s': No such file or directory\n", path);
        return;
    }
    if (parent->type != DIR_NODE) { // Adiciona verificação se o pai é um diretório
        fprintf(stderr, "touch: parent path '%s' is not a directory\n", path);
        return;
    }

    // Converte para char* temporariamente para mensagem de erro
    char* name_char_for_err = wcs_to_char(name_wcs);
    if (find_node_in_dir(parent, name_wcs)) { // find_node_in_dir espera wchar_t*
        if(name_char_for_err) free(name_char_for_err);
        return; // Já existe, não faz nada (touch padrão)
    }
    if(name_char_for_err) free(name_char_for_err);

    Node* new_file = (Node*)malloc(sizeof(Node));
    if (!new_file) { perror("touch: Failed to allocate new file"); return; }
    wcscpy(new_file->name, name_wcs); // Usa wcscpy
    new_file->type = FILE_NODE;
    new_file->content = NULL; // Arquivo vazio
    new_file->child = NULL;
    attach_node(parent, new_file);
}

void fs_ls(const char *path) {
    Node* dir_to_list = find_node_by_path(path); // path continua char*

    if (dir_to_list == NULL) {
        fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", path);
        return;
    }
    if (dir_to_list->type != DIR_NODE) {
        char* name_char = wcs_to_char(dir_to_list->name); // Converte para char* para imprimir
        if(name_char) {
            printf("%s\n", name_char);
            free(name_char);
        }
        return;
    }

    Node *current = dir_to_list->child;
    while (current != NULL) {
        char* name_char = wcs_to_char(current->name); // Converte para char* para imprimir
        if(!name_char) {
            fprintf(stderr, "ls: failed to convert node name for display.\n"); // Erro de conversão
            current = current->next;
            continue; 
        }

        if (current->type == DIR_NODE) {
            printf("d %s/\n", name_char);
        } else {
            size_t size = current->content ? wcslen(current->content) : 0; // Tamanho em caracteres wide
            printf("- %s (%zu chars)\n", name_char, size); 
        }
        free(name_char); // Libera o buffer char*
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

    // Usar wchar_t para o buffer do caminho
    wchar_t path_buffer_wcs[1024]; // Tamanho em wchar_t
    wchar_t *p_wcs = &path_buffer_wcs[sizeof(path_buffer_wcs)/sizeof(wchar_t) - 1]; // Ponteiro para o final do buffer
    *p_wcs = L'\0'; // Termina com wide null char

    for (Node *temp = current_dir; temp != root; temp = temp->parent) {
        size_t name_len_wcs = wcslen(temp->name); // Usa wcslen
        size_t needed_wcs = name_len_wcs + 1; // Para o nome + L'/'

        if ((p_wcs - path_buffer_wcs) < needed_wcs) { // Verifica se há espaço suficiente
            // Caminho muito longo para o buffer, mostra o que couber
            // Ou poderia retornar um erro e não imprimir nada
            break; 
        }

        p_wcs -= name_len_wcs; // Move o ponteiro para trás para copiar o nome
        wmemcpy(p_wcs, temp->name, name_len_wcs); // Copia o nome (wmemcpy para wchar_t)
        p_wcs--; // Move para trás para adicionar a barra
        *p_wcs = L'/'; // Adiciona a barra (L'/' para wide char)
    }

    char* final_path_char = wcs_to_char(p_wcs); // Converte para char* para imprimir
    if(final_path_char) {
        printf("%s\n", final_path_char);
        free(final_path_char);
    } else {
        fprintf(stderr, "pwd: failed to convert path for display.\n");
    }
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
    target->next = NULL; // Assegura que o nó não aponte para o próximo irmão (para fs_destroy)
    fs_destroy(target); // Libera o nó e seus conteúdos/filhos recursivamente (se houver)
}

void fs_cat(const char *path) {
    Node *target = find_node_by_path(path);
    if (target == NULL) {
        fprintf(stderr, "cat: %s: No such file or directory\n", path);
    } else if (target->type != FILE_NODE) {
        fprintf(stderr, "cat: %s: Is a directory\n", path);
    } else if (target->content) { // Se o arquivo tem conteúdo
        char* content_char = wcs_to_char(target->content); // Converte para char* para imprimir
        if(content_char) {
            printf("%s\n", content_char);
            free(content_char);
        } else {
            fprintf(stderr, "cat: failed to convert content for display.\n");
        }
    }
}

void fs_echo(const char *path, const char *content_char) {
    wchar_t name_wcs[100]; // Buffer para o nome em wchar_t
    Node *parent = get_parent_dir_and_basename(path, name_wcs);

    if (!parent) {
        fprintf(stderr, "echo: cannot write to '%s': No such file or directory\n", path);
        return;
    }
    if (parent->type != DIR_NODE) { // Adiciona verificação se o pai é um diretório
        fprintf(stderr, "echo: parent path '%s' is not a directory\n", path);
        return;
    }
    
    Node *target = find_node_in_dir(parent, name_wcs); // find_node_in_dir espera wchar_t*

    if (target == NULL) {
        // Se o arquivo não existe, cria um novo arquivo.
        Node* new_file = (Node*)malloc(sizeof(Node));
        if (!new_file) {
            perror("echo: Failed to allocate new file");
            return;
        }
        wcscpy(new_file->name, name_wcs); // Usa wcscpy para o nome
        new_file->type = FILE_NODE;
        new_file->content = NULL; // Conteúdo inicial NULL
        new_file->child = NULL; // Arquivos não têm filhos
        attach_node(parent, new_file); // Adiciona à árvore
        target = new_file; // O target agora é o novo arquivo
    }

    if (target->type != FILE_NODE) { // Verifica se o target é um arquivo (não diretório)
        char* name_char_for_err = wcs_to_char(name_wcs);
        if(name_char_for_err) {
            fprintf(stderr, "echo: %s: Is a directory\n", name_char_for_err);
            free(name_char_for_err);
        }
        return;
    }

    if (target->content) free(target->content); // Libera conteúdo antigo, se houver
    target->content = char_to_wcs(content_char); // Converte e duplica o conteúdo wide
    if (!target->content && content_char && strlen(content_char) > 0) {
        fprintf(stderr, "echo: Failed to convert content to wide characters.\n");
    }
}


// --- Funções de Mover e Copiar ---

static void detach_node(Node* node) {
    if (!node || !node->parent) return; // Não pode desanexar nó nulo ou sem pai
    Node* parent = node->parent;
    if (parent->child == node) { // Se o nó é o primeiro filho
        parent->child = node->next;
    } else { // Procura o irmão anterior para mudar seu ponteiro 'next'
        Node* sibling = parent->child;
        while (sibling && sibling->next != node) {
            sibling = sibling->next;
        }
        if (sibling) sibling->next = node->next; // Pula o nó a ser desanexado
    }
    node->parent = NULL; // Limpa o ponteiro pai do nó
    node->next = NULL;   // Limpa o ponteiro próximo do nó
}

static void attach_node(Node* parent, Node* child) {
    if (!parent || parent->type != DIR_NODE || !child) return; // Pai deve ser um diretório
    child->parent = parent; // Define o pai do filho
    child->next = NULL;     // Garante que o filho não tenha um próximo irmão por enquanto
    if (parent->child == NULL) { // Se o pai não tem filhos, este é o primeiro
        parent->child = child;
    } else { // Se o pai já tem filhos, adiciona no final da lista de irmãos
        Node* sibling = parent->child;
        while (sibling->next != NULL) sibling = sibling->next; // Encontra o último irmão
        sibling->next = child; // Adiciona o novo filho como o próximo do último irmão
    }
}

void fs_mv(const char *source_path, const char *dest_path) {
    Node *source_node = find_node_by_path(source_path);
    if (source_node == NULL) {
        fprintf(stderr, "mv: cannot stat '%s': No such file or directory\n", source_path);
        return;
    }
    if (source_node == root) { // Não permite mover a raiz
        fprintf(stderr, "mv: cannot move root directory '%s'\n", source_path);
        return;
    }

    Node *dest_target = find_node_by_path(dest_path);
    
    Node *dest_parent;
    wchar_t new_name_wcs[100]; // Buffer para o novo nome em wchar_t

    if (dest_target && dest_target->type == DIR_NODE) { // Destino é um diretório existente
        dest_parent = dest_target;
        wcscpy(new_name_wcs, source_node->name); // Mantém o nome original do source
    } else { // Destino é um novo nome ou caminho para um novo arquivo/diretório
        dest_parent = get_parent_dir_and_basename(dest_path, new_name_wcs); // Preenche new_name_wcs
    }
    
    if (!dest_parent) {
        fprintf(stderr, "mv: cannot move to '%s': No such file or directory\n", dest_path);
        return;
    }
    if (dest_parent->type != DIR_NODE) {
        fprintf(stderr, "mv: cannot move to '%s': Not a directory\n", dest_path);
        return;
    }

    // Verifica se o destino já existe no diretório pai com o novo nome
    if (find_node_in_dir(dest_parent, new_name_wcs)) { 
        fprintf(stderr, "mv: cannot move to '%s': Destination already exists\n", dest_path);
        return;
    }
    
    // Verifica se está tentando mover um diretório para dentro de si mesmo ou um de seus subdiretórios
    if (source_node->type == DIR_NODE) {
        Node* temp = dest_parent;
        while(temp != NULL) {
            if (temp == source_node) {
                fprintf(stderr, "mv: cannot move '%s' to a subdirectory of itself ('%s')\n", source_path, dest_path);
                return;
            }
            temp = temp->parent;
        }
    }

    detach_node(source_node); // Remove o nó de sua localização atual
    wcscpy(source_node->name, new_name_wcs); // Atualiza o nome do nó (para renomear)
    attach_node(dest_parent, source_node); // Adiciona o nó à nova localização
}

static Node* copy_node_recursive(Node* source, Node* new_parent) {
    if (!source) return NULL;
    
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        perror("copy_node_recursive: Failed to allocate new node");
        return NULL;
    }
    wcscpy(new_node->name, source->name); // Copia o nome (wchar_t)
    new_node->type = source->type;
    new_node->parent = new_parent;
    // Copia o conteúdo se for arquivo (usa wcs_dup)
    new_node->content = (source->type == FILE_NODE && source->content) ? wcs_dup(source->content) : NULL;
    new_node->next = NULL; // Inicializa ponteiros
    new_node->child = NULL; 

    if (source->type == DIR_NODE && source->child) { // Se for um diretório e tiver filhos, copia recursivamente
        Node* current_source_child = source->child;
        Node* prev_new_child = NULL;
        while (current_source_child) {
            Node* copied_child = copy_node_recursive(current_source_child, new_node); // Chamada recursiva
            if (copied_child) {
                if (!new_node->child) { // Se for o primeiro filho copiado
                    new_node->child = copied_child;
                } else { // Adiciona como irmão
                    prev_new_child->next = copied_child;
                }
                prev_new_child = copied_child; // Atualiza o último filho copiado
            }
            current_source_child = current_source_child->next;
        }
    }
    
    return new_node;
}

void fs_cp(const char *source_path, const char *dest_path) {
    Node *source_node = find_node_by_path(source_path);
    if (source_node == NULL) {
        fprintf(stderr, "cp: cannot stat '%s': No such file or directory\n", source_path);
        return;
    }

    Node *dest_target = find_node_by_path(dest_path);
    
    Node* dest_parent;
    wchar_t new_name_wcs[100]; // Buffer para o novo nome em wchar_t

    if(dest_target && dest_target->type == DIR_NODE){ // Se o destino é um diretório existente
        dest_parent = dest_target;
        wcscpy(new_name_wcs, source_node->name); // Usa o nome original do source
    } else { // Se o destino é um novo nome ou caminho para um novo arquivo/diretório
        dest_parent = get_parent_dir_and_basename(dest_path, new_name_wcs); // Preenche new_name_wcs
    }

    if (!dest_parent) {
        fprintf(stderr, "cp: cannot create '%s': No such file or directory\n", dest_path);
        return;
    }
    if (dest_parent->type != DIR_NODE) { // Destino pai deve ser um diretório
        fprintf(stderr, "cp: cannot create '%s': Not a directory\n", dest_path);
        return;
    }
    
    if (find_node_in_dir(dest_parent, new_name_wcs)) { // Verifica se o destino já existe
        fprintf(stderr, "cp: cannot copy to '%s': Destination already exists\n", dest_path);
        return;
    }
    
    Node* new_node = copy_node_recursive(source_node, dest_parent); // Copia o nó recursivamente
    if (!new_node) {
        fprintf(stderr, "cp: Failed to copy node '%s'\n", source_path);
        return;
    }
    wcscpy(new_node->name, new_name_wcs); // Define o nome do nó copiado
    attach_node(dest_parent, new_node); // Anexa o nó copiado à árvore
}

// --- Funções de Serialização (Save/Load) e Exportação ---

void save_node_recursive(FILE *file, Node *node) {
    if (node == NULL) return;

    // 1. Salva o tipo do nó (FILE_NODE ou DIR_NODE)
    fwrite(&node->type, sizeof(NodeType), 1, file);
    
    // 2. Converte o nome (wchar_t*) para uma string de bytes (char*) para salvar
    char* name_char = wcs_to_char(node->name);
    // Calcula o tamanho em bytes, incluindo o terminador nulo, para salvar
    size_t name_len = name_char ? strlen(name_char) + 1 : 0;
    
    // Salva o tamanho do nome
    fwrite(&name_len, sizeof(size_t), 1, file);
    // Salva os bytes do nome
    if (name_len > 0) {
        fwrite(name_char, sizeof(char), name_len, file);
    }
    // Libera a memória temporária alocada para a string de bytes
    if (name_char) {
        free(name_char);
    }

    // 3. Salva o conteúdo se for um arquivo, ou um tamanho zero se for um diretório
    if (node->type == FILE_NODE) {
        // Converte o conteúdo (wchar_t*) para uma string de bytes (char*) para salvar
        char* content_char = wcs_to_char(node->content);
        // Calcula o tamanho em bytes, incluindo o terminador nulo
        size_t content_len = content_char ? strlen(content_char) + 1 : 0;
        
        // Salva o tamanho do conteúdo
        fwrite(&content_len, sizeof(size_t), 1, file);
        // Salva os bytes do conteúdo
        if (content_len > 0) {
            fwrite(content_char, sizeof(char), content_len, file);
        }
        // Libera a memória temporária
        if (content_char) {
            free(content_char);
        }
    } else { // Se for um diretório, o conteúdo é sempre vazio (tamanho 0)
        size_t content_len = 0;
        fwrite(&content_len, sizeof(size_t), 1, file);
    }

    // 4. Conta e salva o número de filhos (para saber quantos nós recursivamente carregar depois)
    int child_count = 0;
    for (Node *child = node->child; child; child = child->next) {
        child_count++;
    }
    fwrite(&child_count, sizeof(int), 1, file);
    
    // 5. Chama recursivamente para cada filho
    for (Node *child = node->child; child; child = child->next) {
        save_node_recursive(file, child);
    }
}

void fs_save(const char* filepath) {
    FILE *file = fopen(filepath, "wb"); // Abre o arquivo em modo binário de escrita
    if (!file) {
        perror("Error opening file for saving");
        return;
    }
    save_node_recursive(file, root); // Chama a função recursiva para salvar a árvore a partir da raiz
    fclose(file); // Fecha o arquivo
    printf("File system saved to %s\n", filepath);
}

Node* load_node_recursive(FILE *file, Node *parent) {
    NodeType type;
    // 1. Lê o tipo do nó
    if (fread(&type, sizeof(NodeType), 1, file) != 1) return NULL;

    size_t name_len;
    // 2. Lê o tamanho em bytes do nome
    if (fread(&name_len, sizeof(size_t), 1, file) != 1) return NULL;
    
    char* name_char = NULL;
    // 3. Aloca um buffer temporário para ler o nome em bytes
    if (name_len > 0) {
        name_char = (char*)malloc(name_len); 
        if (!name_char) return NULL; // Falha na alocação
        // Lê os bytes do nome
        if (fread(name_char, sizeof(char), name_len, file) != name_len) {
            free(name_char); // Falha na leitura
            return NULL;
        }
    }

    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) {
        if(name_char) free(name_char); // Libera o nome lido se a alocação do nó falhar
        return NULL;
    }
    
    // 4. Converte o nome de bytes (char*) para wchar_t* e atribui ao nó
    if (name_len > 0 && name_char) {
        // Usa mbstowcs para converter para o buffer wchar_t name[] do Node
        // O tamanho da conversão é o tamanho total em bytes lidos (name_len)
        mbstowcs(new_node->name, name_char, name_len); 
        // Garante que a string wchar_t seja terminada em nulo
        // A conversão mbstowcs já deve fazer isso se o buffer for grande o suficiente
        new_node->name[name_len] = L'\0'; 
    } else {
        new_node->name[0] = L'\0'; // Garante que o nome esteja vazio se name_len for 0
    }
    if(name_char) free(name_char); // Libera o buffer temporário de bytes do nome
    
    new_node->type = type;
    new_node->parent = parent;
    new_node->child = NULL; // Inicializa ponteiros para evitar lixo de memória
    new_node->next = NULL;
    new_node->content = NULL;

    size_t content_len;
    // 5. Lê o tamanho em bytes do conteúdo
    if (fread(&content_len, sizeof(size_t), 1, file) != 1) {
        free(new_node); // Falha na leitura do tamanho do conteúdo
        return NULL;
    }
    // 6. Se for um arquivo e tiver conteúdo, lê e converte
    if (type == FILE_NODE && content_len > 0) {
        char* content_char = (char*)malloc(content_len); // Buffer temporário char*
        if (!content_char) {
            free(new_node); // Falha na alocação do conteúdo
            return NULL;
        }
        if (fread(content_char, sizeof(char), content_len, file) != content_len) {
            free(content_char); // Falha na leitura do conteúdo
            free(new_node);
            return NULL;
        }
        new_node->content = char_to_wcs(content_char); // Converte para wchar_t* e atribui
        if (!new_node->content) { // Falha na conversão de conteúdo
            fprintf(stderr, "load_node_recursive: Failed to convert content to wide chars.\n");
        }
        free(content_char); // Libera o buffer temporário de bytes do conteúdo
    }

    int child_count;
    // 7. Lê o número de filhos
    if (fread(&child_count, sizeof(int), 1, file) != 1) {
        free(new_node); // Falha na leitura da contagem de filhos
        return NULL;
    }
    
    // 8. Chama recursivamente para carregar os filhos
    Node *prev_child = NULL;
    for (int i = 0; i < child_count; i++) {
        Node *child_node = load_node_recursive(file, new_node);
        if (!child_node) { 
            // Se um filho falhar ao carregar, o nó atual e seus filhos já carregados precisam ser limpos.
            // fs_destroy já é recursivo, então chame para o filho inicial se ele existir.
            fs_destroy(new_node->child); 
            free(new_node); // Libera o nó atual
            return NULL; 
        }
        if (i == 0) new_node->child = child_node; // Primeiro filho
        else prev_child->next = child_node;       // Outros filhos (como irmãos)
        prev_child = child_node;
    }
    return new_node;
}

void fs_load(const char* filepath) {
    FILE *file = fopen(filepath, "rb"); // Abre o arquivo em modo binário de leitura
    if (!file) {
        printf("No save file found. Starting a new file system.\n");
        fs_init(); // Se o arquivo não existe, inicializa um novo sistema
        return;
    }
    
    // Antes de carregar, destrói a árvore existente na memória para evitar vazamentos
    // (Útil se fs_init() já foi chamado ou se houver estado anterior)
    if (root != NULL) {
        fs_destroy(root); 
        root = NULL; // Garante que root seja NULL antes de carregar um novo
    }

    // Chama a função recursiva para carregar a árvore a partir do arquivo
    root = load_node_recursive(file, NULL); 
    
    // Se a carga falhar por algum motivo (arquivo corrompido, etc.)
    if (root == NULL) {
        fprintf(stderr, "Error loading file system. Starting a new file system.\n");
        fs_init(); // Inicializa um novo sistema em caso de falha na carga
    }
    
    current_dir = root; // Define o diretório atual como a raiz
    fclose(file); // Fecha o arquivo
    printf("File system loaded from %s\n", filepath);
}

void export_recursive(FILE *file, Node *node, int is_last) {
    if (node == NULL) return;
    fprintf(file, "{");
    
    // Converte nome wide para char* para JSON
    char* name_char = wcs_to_char(node->name);
    if(name_char) {
        // Escapar aspas e barras para JSON (ex: \" -> \", \\ -> \)
        // Para uma implementação robusta, você precisaria de uma função de escape JSON.
        // Por simplicidade, assumimos que o nome não terá aspas ou barras.
        fprintf(file, "\"name\":\"%s\",", name_char);
        free(name_char);
    } else {
         fprintf(file, "\"name\":\"ERROR\","); // Em caso de falha na conversão
    }
    
    fprintf(file, "\"type\":\"%s\"", (node->type == DIR_NODE) ? "directory" : "file");

    if (node->type == DIR_NODE && node->child) {
        fprintf(file, ",\"children\":[");
        Node *child = node->child;
        while (child) {
            export_recursive(file, child, child->next == NULL);
            child = child->next;
        }
        fprintf(file, "]");
    } else if (node->type == FILE_NODE && node->content) {
    char* content_char = wcs_to_char(node->content);
    if(content_char) {
        // *** AQUI É A MUDANÇA PRINCIPAL ***
        char* escaped_content = escape_json_string_content(content_char);
        if(escaped_content) {
            fprintf(file, ",\"content\":\"%s\"", escaped_content); // Imprime a string ESCAPADA
            free(escaped_content);
        } else {
            fprintf(stderr, "export_recursive: failed to escape content.\n"); // Em caso de falha de alocação
        }
        free(content_char); // Libera o content_char temporário
    } else {
        fprintf(file, ",\"content\":\"ERROR_CONTENT\""); // Em caso de falha de conversão wcs_to_char
    }
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