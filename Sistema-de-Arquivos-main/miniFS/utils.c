#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>  // Novo
#include <locale.h> // Novo
#include "utils.h"

// Funções auxiliares char <-> wchar_t (MOVIDAS PARA CÁ)
// Essas funções não são "static" para poderem ser usadas em outros arquivos.

wchar_t* wcs_dup(const wchar_t* s) {
    if (!s) return NULL;
    wchar_t* ws = (wchar_t*)malloc((wcslen(s) + 1) * sizeof(wchar_t));
    if (ws) wcscpy(ws, s);
    return ws;
}

wchar_t* char_to_wcs(const char* s_char) {
    if (!s_char) return NULL;
    // mbstowcs com NULL calcula o tamanho necessário em wchar_t (caracteres).
    // Incluímos +1 para o terminador nulo.
    size_t len_wcs = mbstowcs(NULL, s_char, 0) + 1;
    if (len_wcs == (size_t)-1) return NULL; // Erro de conversão

    wchar_t* s_wcs = (wchar_t*)malloc(len_wcs * sizeof(wchar_t));
    if (!s_wcs) return NULL;

    // Converte e garante que o número de caracteres wide lidos seja o esperado.
    // O último argumento é o tamanho MÁXIMO do buffer de destino em wchar_t.
    size_t converted_len = mbstowcs(s_wcs, s_char, len_wcs);
    if (converted_len == (size_t)-1) {
        free(s_wcs);
        return NULL; // Erro de conversão
    }
    s_wcs[converted_len] = L'\0'; // Garante terminação nula explícita
    return s_wcs;
}

// Converte wchar_t* para char* (saída para console ou arquivo)
char* wcs_to_char(const wchar_t* s_wcs) {
    if (!s_wcs) return NULL;
    // wcstombs com NULL calcula o tamanho necessário em bytes.
    // Incluímos +1 para o terminador nulo.
    size_t len_char = wcstombs(NULL, s_wcs, 0) + 1;
    if (len_char == (size_t)-1) return NULL; // Erro de conversão

    char* s_char = (char*)malloc(len_char * sizeof(char));
    if (!s_char) return NULL;

    // Converte e garante que o número de bytes lidos seja o esperado.
    // O último argumento é o tamanho MÁXIMO do buffer de destino em char (bytes).
    size_t converted_len = wcstombs(s_char, s_wcs, len_char);
    if (converted_len == (size_t)-1) {
        free(s_char);
        return NULL; // Erro de conversão
    }
    s_char[converted_len] = '\0'; // Garante terminação nula explícita
    return s_char;
}


// Funções de string padrão (já existentes)
// Remove espaços em branco do início e fim de uma string
char* trim_whitespace(char* str) {
    char* end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

// Divide uma string em tokens usando um delimitador
char** split_string(const char* input, int* count) {
    char* str_copy = strdup(input); // Duplica a string para poder modificá-la
    if (!str_copy) {
        *count = 0;
        return NULL;
    }

    // Remove a nova linha do final, se houver
    // strcspn busca o primeiro caractere \n na string e retorna o índice
    // Se \n for encontrado, substitui por \0.
    str_copy[strcspn(str_copy, "\n")] = '\0';
    char* trimmed_str = trim_whitespace(str_copy); // Trima espaços em branco

    int capacity = 5; // Capacidade inicial do array de tokens
    char** tokens = (char**)malloc(capacity * sizeof(char*));
    if (!tokens) {
        free(str_copy);
        *count = 0;
        return NULL;
    }

    *count = 0;
    char* token;
    // Usa " \t" como delimitadores (espaço e tab) para separar palavras.
    // str_copy deve ser passado na primeira chamada.
    token = strtok(trimmed_str, " \t");
    while (token != NULL) {
        // Se a capacidade for atingida, realoca o array para o dobro do tamanho.
        if (*count >= capacity) {
            capacity *= 2;
            tokens = (char**)realloc(tokens, capacity * sizeof(char*));
            if (!tokens) {
                // Se realloc falhar, libera o que já foi alocado e retorna NULL.
                for (int i = 0; i < *count; i++) free(tokens[i]);
                free(tokens);
                free(str_copy);
                *count = 0;
                return NULL;
            }
        }
        // Duplica o token encontrado e armazena no array.
        tokens[*count] = strdup(token);
        if (!tokens[*count]) {
            // Se strdup falhar, libera o que já foi alocado e retorna NULL.
            for (int i = 0; i < *count; i++) free(tokens[i]);
            free(tokens);
            free(str_copy);
            *count = 0;
            return NULL;
        }
        (*count)++;
        token = strtok(NULL, " \t"); // Próxima chamada usa NULL.
    }

    // *** A LINHA CRUCIAL PARA CORRIGIR O VALGRIND E A FALHA DE SEGMENTAÇÃO ***
    // Garante que o array de tokens é SEMPRE terminado com NULL.
    // Isso é fundamental para free_tokens saber quando parar.
    if (*count < capacity) { // Se há espaço no array alocado
        tokens[*count] = NULL; // Adiciona o terminador NULL
    } else { // Se o array está cheio até a capacidade, precisa de mais um slot
        tokens = (char**)realloc(tokens, (capacity + 1) * sizeof(char*));
        if (!tokens) { // Se realloc falhar, significa falha crítica de memória.
            *count = 0; // Indica falha
            return NULL;
        }
        tokens[*count] = NULL; // Adiciona o terminador NULL
    }

    free(str_copy); // Libera a string duplicada original.
    return tokens;
}


// Libera a memória alocada para os tokens
void free_tokens(char** tokens) {
    if (tokens) {
        for (int i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
    }
}