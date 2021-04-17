#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
void *safe_malloc(size_t size) {
   void *buffer;
   if ((buffer = malloc(size)) == NULL) {
      perror(NULL);
      exit(EXIT_FAILURE);
   }
   return buffer;
}

void *safe_realloc(void *ptr, size_t size) {
   void *newptr;
   if ((newptr = realloc(ptr, size)) == NULL) {
      perror(NULL);
      exit(EXIT_FAILURE);
   }
   return newptr;
}

void *safe_calloc(size_t nmemb, size_t size) {
   void *ptr;
   if ((ptr = calloc(nmemb, size)) == NULL) {
      perror(NULL);
      exit(EXIT_FAILURE);
   }
   return ptr;
}
/*snprintf without nul terminator*/
int memsnprintf(char *str, size_t size, const char *format, ...) {
   int ret = 0;
   char *buf;
   va_list ap;

   va_start(ap, format);
   size++;
   buf = safe_malloc(sizeof(char) * size);
   ret = vsnprintf(buf, size, format, ap);
   va_end(ap);
   if (ret >= size) {
      ret = size - 1;
   }
   memcpy(str, buf, ret);
   free(buf);
   return ret;
}

/*Mabye change implementation to just use a loop*/
char *strcpy_retend(char *dest, const char *src) {
   size_t srclen = strlen(src);
   memcpy(dest, src, srclen + 1);
   return dest + srclen;
}

void str_exit(const char *str) {
   fprintf(stderr, "%s\n", str);
   exit(EXIT_FAILURE);
}

void perror_exit(const char *str) {
   perror(str);
   exit(EXIT_FAILURE);
}

size_t batch_strlen(char **batch) {
   size_t len;
   for (len = 0; *batch != NULL; batch++) {
      len += strlen(*batch);
   }
   return len;
}

int safe_send(int sockfd, const void *buf, size_t len, int flags) {
   size_t amtSent = 0;
   ssize_t ret;
   while (amtSent < len) {
      ret = send(sockfd, buf + amtSent, len - amtSent, flags);
      if (ret == -1) {
         return -1;
      }
      amtSent += ret;
   }
   return 1;
}







