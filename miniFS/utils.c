// miniFS/utils.c

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"

char* trim_whitespace(char* str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

char** split_string(const char* input, int* count) {
    char* str = strdup(input);
    char* trimmed_str = trim_whitespace(str);
    int initial_alloc = 10;
    char** tokens = malloc(initial_alloc * sizeof(char*));
    char* token;
    *count = 0;

    token = strtok(trimmed_str, " \t\n");
    while (token != NULL) {
        tokens[*count] = strdup(token);
        (*count)++;
        if (*count >= initial_alloc) {
            initial_alloc *= 2;
            tokens = realloc(tokens, initial_alloc * sizeof(char*));
        }
        token = strtok(NULL, " \t\n");
    }
    tokens[*count] = NULL;
    free(str);
    return tokens;
}

void free_tokens(char** tokens) {
    if (!tokens) return;
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}