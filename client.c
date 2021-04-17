#include <stdio.h>
#include <string.h>
#include "send.h"
#include "receive.h"
#include "util.h"

#define INPUTBUFSIZ 10
#define MOD         2

#define HEADERMODE  1
#define METHODMODE  2
#define PRINTHEADER 3

#define HEADEROP 0x0001

static void print_usage() {
   str_exit("Usage: URL HEADER");
}

static int set_mode(char *str) {
   if (strcmp(str, "-h") == 0) {
      return HEADERMODE;
   }
   if (strcmp(str, "-m") == 0) {
      return METHODMODE;
   }
   if (strcmp(str, "-v") == 0) {
      return PRINTHEADER;
   }
   str_exit("Invalid option");
   return -1;
}

static int get_method_code(char *methodStr) {
   if (strcmp(methodStr, "GET") == 0) {
      return GET;
   }
   if (strcmp(methodStr, "POST") == 0) {
      return POST;
   }
   if (strcmp(methodStr, "HEAD") == 0) {
      return HEAD;
   }
   if (strcmp(methodStr, "DELETE") == 0) {
      return DELETE;
   }
   if (strcmp(methodStr, "PUT") == 0) {
      return PUT;
   }
   str_exit("Invalid method");
   return -1;
}

static int has_args(int mode) {
   return mode != PRINTHEADER;
}

static short get_op(int mode) {
   if (mode == PRINTHEADER) {
      return HEADEROP;
   }
   return -1;
}

static int check_args(int argc, char *argv[], struct httpreq *httpreq) {
   size_t i;
   int mode;
   short ops = 0;
   if (argc < 2) {
      print_usage();
   }
   httpreq->url = argv[1];
   httpreq->numHeaders = 0;
   httpreq->headers = NULL;
   httpreq->method = GET;
   if (argc == 2) {
      return ops;
   }
   for (i = 2; argv[i] != NULL; ++i) {
      if (argv[i][0] == '-') {
         mode = set_mode(argv[i]);
	 if (!has_args(mode)) {
	   ops |= get_op(mode);
	 }
	 continue;
      }
      if (i == 2) {
         print_usage();
      }
      if (mode == HEADERMODE) {
         httpreq->headers = safe_realloc(httpreq->headers, sizeof(char *) * (httpreq->numHeaders + 1));
	 httpreq->headers[httpreq->numHeaders++] = argv[i];
      } else if (mode == METHODMODE) {
         httpreq->method = get_method_code(argv[i]);
      }
   }
   return ops;
}

static void read_input(struct httpreq *httpreq) {
   size_t amtRead = 0, result, bufSize = INPUTBUFSIZ;
   unsigned char *buffer = safe_malloc(sizeof(unsigned char) * INPUTBUFSIZ);
   while (1) {
      result = fread(buffer + amtRead, sizeof(unsigned char), bufSize - amtRead, stdin);
      if (result < (bufSize - amtRead)) {
	 if (ferror(stdin)) {
	    perror_exit(NULL);
	 }
         if (feof(stdin)) {
	    break;
	 }
      }
      amtRead += result;
      if (amtRead != bufSize) {
         continue;
      }
      bufSize *= MOD;
      buffer = safe_realloc(buffer, bufSize);
   }
   amtRead += result;
   if (amtRead != 0) {
      buffer = safe_realloc(buffer, amtRead);
      httpreq->body = buffer;
   } else {
      free(buffer);
      httpreq->body = NULL;
   }
   httpreq->bodylength = amtRead;
}

static void write_data(struct receive *data, short ops) {
   size_t ret;
   if ((ops & HEADEROP) == HEADEROP) {
      ret = fwrite(data->header, sizeof(char), data->headerLength, stdout);
      if (ret < data->headerLength) {
         perror_exit(NULL);
      }
   }
   if (data->error != NULL) {
      fprintf(stderr, "%s\n", data->error);
      return;
   }
   ret = fwrite(data->body, sizeof(unsigned char), data->bodyLength, stdout);
   if (ret < data->bodyLength) {
      perror_exit(NULL);
   }
}

int main(int argc, char *argv[]) {
   int sockfd;
   short opts;
   struct receive data = {0};
   struct httpreq httpreq = {0};
   opts = check_args(argc, argv, &httpreq);
   if (httpreq.method != GET && httpreq.method != HEAD) {
      read_input(&httpreq);
   } else {
      httpreq.body = NULL;
      httpreq.bodylength = 0;
   }
   sockfd = send_request(httpreq);
   receive(sockfd, &data); 
   write_data(&data, opts);   
   free(data.body);
   free(httpreq.headers);
   free(httpreq.body);
   free(data.header);
   free(data.error);
}
