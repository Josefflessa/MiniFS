// miniFS/utils.h

#ifndef UTILS_H
#define UTILS_H

// Divide uma string de entrada em um array de tokens (palavras).
// O chamador é responsável por liberar a memória com free_tokens.
char** split_string(const char* input, int* count);

// Libera a memória alocada por split_string.
void free_tokens(char** tokens);

#endif // UTILS_H