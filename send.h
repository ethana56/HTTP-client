#ifndef SEND_H
#define SEND_H

/* methods */
#define GET 1
#define POST 2
#define PUT 3
#define HEAD 4
#define UPDATE 5
#define DELETE 6
#define METHODBUFSIZ 7
#define HOSTNAMEBUFLEN 254

#define CONTENTLENGTH "Content-Length"

struct httpreq {
   char **headers;
   size_t numHeaders;
   unsigned char *body;
   char *url;
   size_t bodylength;
   int method;
};

int send_request(struct httpreq httpreq);

#endif
