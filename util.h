#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

#define SIZE_T_MAX ~((size_t)0)

void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
void *safe_calloc(size_t nmemb, size_t size);
int memsnprintf(char *str, size_t size, const char *format, ...);
char *strcpy_retend(char *dest, const char *src);
size_t batch_strlen(char **batch);
void str_exit(const char *str);
void perror_exit(const char *str);
int safe_send(int sockfd, const void *buf, size_t len, int flags);

#endif
