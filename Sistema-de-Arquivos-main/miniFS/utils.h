#ifndef UTILS_H
#define UTILS_H

#include <wchar.h> // Necessário para wchar_t

// Funções de string padrão (existentes)
char** split_string(const char* input, int* count);
void free_tokens(char** tokens);

// Novas funções auxiliares para manipulação de wide chars
wchar_t* wcs_dup(const wchar_t* s);
wchar_t* char_to_wcs(const char* s_char);
char* wcs_to_char(const wchar_t* s_wcs);

#endif // UTILS_H