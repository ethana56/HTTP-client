#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include "receive.h"
#include "util.h"

#define BASEBUFSIZ 20
#define BASHEADSIZ 20
#define INCREASEMOD 2 
#define ERRINDEX 9

static unsigned char *get_data_known(int sockfd, size_t amt, size_t *size) {
   unsigned char *buffer;
   size_t amtRead = 0;
   ssize_t result;
   buffer = safe_malloc(sizeof(unsigned char) * amt);
   while ((result = read(sockfd, buffer + amtRead, amt - amtRead)) > 0) {
      amtRead += result;
   }
   if (result < 0) {
      perror_exit(NULL);
   }
   *size = amtRead;
   return buffer;
}

static unsigned char *get_data_unkown(int sockfd, size_t *size) {
   unsigned char *buffer;
   size_t buffersize = BASEBUFSIZ;
   size_t amtread = 0;
   ssize_t result;
   buffer = safe_malloc(sizeof(unsigned char) * buffersize);
   while ((result = read(sockfd, buffer + amtread, buffersize - amtread)) > 0) {
      amtread += result;
      if (amtread != buffersize) {
         continue;
      }
      buffersize *= INCREASEMOD;
      buffer = safe_realloc(buffer, buffersize);
   }
   if (result < 0) {
      perror_exit(NULL);
   }
   *size = amtread;
   buffer = safe_realloc(buffer, amtread);
   return buffer;
}

static int set_toggle(char letter, int *newlinetimes, int *toggle) {
   if (letter == '\r') {
      if ((*newlinetimes & 1) == 0) {
         (*newlinetimes)++;
         *toggle = 1;
         return 1;
      }
      *newlinetimes = 0;
      *toggle = 0;
      return -1;
   }
   if (letter == '\n') {
      if (*toggle == 0 || (*newlinetimes & 1) == 0) {
         *newlinetimes = 0;
         *toggle = 0;
         return -1;
      }
      (*newlinetimes)++;
      return 1;
   }
   *newlinetimes = 0;
   *toggle = 0;
   return 1;
}

static char *get_header(int sockfd, size_t *size) {
   char *buffer;
   size_t bufferSize = BASHEADSIZ;
   size_t amtRead = 0;
   ssize_t result;
   int newlinetimes = 0, toggle = 0;
   buffer = safe_malloc(sizeof(char) * bufferSize);
   while ((result = read(sockfd, buffer + amtRead, 1)) > 0) {
      set_toggle(buffer[amtRead], &newlinetimes, &toggle);
      if (newlinetimes == 4) {
         amtRead++;
	 break;
      }   
      amtRead++;
      if (amtRead != bufferSize) {
         continue;
      }
      bufferSize *= INCREASEMOD;
      buffer = safe_realloc(buffer, bufferSize);
   }
   if (result < 0) {
      perror_exit(NULL);
   }
   buffer = safe_realloc(buffer, amtRead + 1);
   buffer[amtRead] = '\0';
   *size = amtRead;
   return buffer;
}

static int check_content_length(char *header, size_t *len) {
   const char *contentLength = "Content-Length";
   int errnoSave;
   char *contentLenStrPtr = strstr(header, contentLength);
   char *endptr;
   uintmax_t contentLen;
   if (contentLenStrPtr == NULL) {
      return 0;
   }
   errnoSave = errno;
   errno = 0;
   contentLen = strtoumax(contentLenStrPtr, &endptr, 10);
   if (contentLenStrPtr == endptr || *endptr != '\0' || contentLen > SIZE_T_MAX) {
      return 0;
   }
   errno = errnoSave;
   return contentLen;
}

static size_t count_header_line(char *line) {
   int i;
   for (i = 0; line[i] != '\r' && line[i] != '\n'; ++i);
   return i + 1;
}

static int check_error_code(struct receive *data) {
   char *err = data->header + ERRINDEX;
   size_t errLen;
   if (err[0] != '4' && err[0] != '5') {
      return 0;
   }
   errLen = count_header_line(err);
   data->error = safe_malloc(sizeof(char) * errLen);
   snprintf(data->error, errLen, "%s", err);
   return ERRORCODE;
}

int receive(int sockfd, struct receive *data) {
   size_t bodyLength, contentLength, headerLength;
   data->header = get_header(sockfd, &headerLength);
   data->headerLength = headerLength;
   data->error = NULL;
   data->body = NULL;
   if (check_error_code(data) == ERRORCODE) {
      return ERRORCODE;
   }
   if (check_content_length(data->header, &contentLength)) {
      if (contentLength == 0) {
         data->body = NULL;
         data->bodyLength = 0;
      } else {
         data->body = get_data_known(sockfd, contentLength, &bodyLength);
      }
   } else {
      data->body = get_data_unkown(sockfd, &bodyLength);
   }
   data->bodyLength = bodyLength;
   close(sockfd);
   return 0;
}









