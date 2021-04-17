#ifndef RECEIVE_H
#define RECEIVE_H

#include <stdlib.h>

#define ERRORCODE -1

struct receive {
   char *header;
   size_t headerLength;
   unsigned char *body;
   char *error;
   size_t bodyLength;
};
int receive(int sockfd, struct receive *data);

#endif
