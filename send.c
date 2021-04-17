#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include "send.h"
#include "util.h"

#define HEADERFORMAT "%s %s HTTP/1.0\r\nHost: %s\r\n"
#define HEADERFORMATBUFLEN 21

#define MAXSTRINGBUFSIZ 40

struct data {
   char *header;
   size_t headerLen;
   unsigned char *body;
   size_t bodyLen;
};

static void get_method_string(int method, char *methodString) {
   switch (method) {
      case GET:
	      strcpy(methodString, "GET");
	      break;
      case POST:
	      strcpy(methodString, "POST");
	      break;
      case HEAD:
	      strcpy(methodString, "HEAD");
	      break;
      case PUT:
	      strcpy(methodString, "PUT");
	      break;
      case UPDATE:
	      strcpy(methodString, "UPDATE");
	      break;
      default:
	      fprintf(stderr, "Can't send request: invalid method\n");
	      exit(EXIT_FAILURE);
   }
}

static size_t calc_header_len(const char *host, const char *methodString, const char *endpoint, struct httpreq *httpreq) {
   size_t headerLen = HEADERFORMATBUFLEN, i;
   headerLen += strlen(host);
   headerLen += strlen(methodString);
   headerLen += strlen(endpoint);
   if (httpreq->headers != NULL) {
      for (i = 0; i < httpreq->numHeaders; ++i) {
         headerLen += (strlen(httpreq->headers[i]) + 2);
      }
   }
   return headerLen + 2;
}

static char *copy_header_fields(char *header, struct httpreq *httpreq) {
   size_t i;
   char *curHeaderPtr = header;
   for (i = 0; i < httpreq->numHeaders; ++i) {
      curHeaderPtr = strcpy_retend(curHeaderPtr, httpreq->headers[i]);
      curHeaderPtr = strcpy_retend(curHeaderPtr, "\r\n");
   }
   return curHeaderPtr;
}

static size_t copy_to_header(char *host, char *endpoint, char *methodString, char **headerPtr, struct httpreq *httpreq) {
   char *header, *headerEnd;
   int amountInit;
   size_t headerLen = calc_header_len(host, methodString, endpoint, httpreq);
   header = safe_malloc(sizeof(char) * headerLen + 1);
   amountInit = snprintf(header, headerLen + 1, HEADERFORMAT, methodString, endpoint, host);
   headerEnd = header + amountInit;
   headerEnd = copy_header_fields(header + amountInit, httpreq);
   strcpy(headerEnd, "\r\n");
   *headerPtr = header;
   return headerLen;
}

static size_t build_header(char *host, char *endpoint, int method, char **headerPtr, struct httpreq *httpreq) {
   char methodString[METHODBUFSIZ];
   get_method_string(method, methodString);
   return copy_to_header(host, endpoint, methodString, headerPtr, httpreq);
}

static size_t get_hostname_len(char *host) {
   int i;
   for (i = 0; host[i] != '\0' && host[i] != '/'; ++i);
   return i;
}

static void parse_url(char *url, char **addrstr, char **endpoint) {
   const char *http = "http://";
   int httpLen = strlen(http);
   size_t hostnameLen, endpointLen;
   if (memcmp(http, url, httpLen) != 0) {
      str_exit("Invalid URL");
   }
   if (url[httpLen] == '\0') {
      str_exit("Invalid URL");
   }
   hostnameLen = get_hostname_len(url + httpLen);
   *addrstr = safe_malloc(sizeof(char) * hostnameLen + 1);
   snprintf(*addrstr, hostnameLen + 1, "%s", url + httpLen);
   endpointLen = strlen(url + (httpLen + hostnameLen));
   *endpoint = safe_malloc(sizeof(char) * endpointLen + 2);
   strcpy(*endpoint, url + (httpLen + hostnameLen));
   if (endpointLen == 0) {
      strcpy(*endpoint, "/");
   }
}

static int create_connection(char *addrstr) {
   int sockfd, err;
   struct addrinfo *aip, *ailist, hint;
   memset(&hint, 0, sizeof(struct addrinfo));
   hint.ai_socktype = SOCK_STREAM;
   hint.ai_family = AF_INET;
   err = getaddrinfo(addrstr, "http", &hint, &ailist);
   if (err) {
      str_exit(gai_strerror(err));
   }
   for (aip = ailist; aip != NULL; aip = aip->ai_next) {
      if ((sockfd = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol)) == -1) {
         perror_exit(NULL); 
      }
      if (connect(sockfd, aip->ai_addr, aip->ai_addrlen) == -1) {
	 perror_exit(NULL);
      }
   }
   freeaddrinfo(ailist);
   return sockfd;
}

static void send_data(int sockfd, struct data *data) {
   if (safe_send(sockfd, data->header, data->headerLen, 0) == -1) {
      perror_exit(NULL);
   }
   if (data->body == NULL) {
      return;
   }
   if (safe_send(sockfd, data->body, data->bodyLen, 0) == -1) {
      perror_exit(NULL);
   }
}

static void copy_over_headers(struct httpreq *httpreq, char **extraHeaders, size_t numExtraHeaders) {
   size_t i, newHeadersIndex;
   char **newHeaders = safe_malloc(sizeof(char *) * (httpreq->numHeaders + numExtraHeaders));
   for (i = 0, newHeadersIndex = 0; i < httpreq->numHeaders; ++i) {
      newHeaders[newHeadersIndex++] = httpreq->headers[i];
   }
   for (i = 0; i < numExtraHeaders; ++i) {
      newHeaders[newHeadersIndex++] = extraHeaders[i];
   }
   httpreq->headers = newHeaders;
   httpreq->numHeaders += numExtraHeaders;
}

static int build_and_send(struct httpreq *httpreq, char **extraHeaders, size_t numExtraHeaders) {
   char *header, *addrstring, *endpoint;
   int sockfd;
   size_t headerLen;
   struct data data;
   parse_url(httpreq->url, &addrstring, &endpoint);
   if (extraHeaders != NULL) {
      copy_over_headers(httpreq, extraHeaders, numExtraHeaders);
   }
   headerLen = build_header(addrstring, endpoint, httpreq->method, &header, httpreq);
   sockfd = create_connection(addrstring);
   data.header = header;
   data.headerLen = headerLen;
   data.body = httpreq->body;
   data.bodyLen = httpreq->bodylength;
   send_data(sockfd, &data);
   free(header);
   free(addrstring);
   free(endpoint);
   if (extraHeaders != NULL) {
      free(httpreq->headers);
   }
   return sockfd;
}

static int check_headers(struct httpreq *httpreq, char *header) {
   size_t i;
   for (i = 0; i < httpreq->numHeaders; ++i) {
      if (strstr(httpreq->headers[i], header) != NULL) {
         return 1;
      }
   }
   return 0;
}

static int setup_post(struct httpreq *httpreq) {
   char *extraHeaders[2];
   char contentLengthHeader[MAXSTRINGBUFSIZ];
   int result, i = 0;
   if (!check_headers(httpreq, "Content-Type")) {
      extraHeaders[i++] = "Content-Type: text/plain";
   }
   if (!check_headers(httpreq, "Content-Length")) {
      result = snprintf(contentLengthHeader, MAXSTRINGBUFSIZ, "Content-Length: %zu", httpreq->bodylength);
      if (result == MAXSTRINGBUFSIZ) {
         str_exit("Content-Length too long");
      }
   }
   extraHeaders[i++] = contentLengthHeader;
   if (i == 0) {
      return build_and_send(httpreq, NULL, 0);
   }
   return build_and_send(httpreq, extraHeaders, i);
}

int send_request(struct httpreq httpreq) {
   switch (httpreq.method) {
      case POST:
	      return setup_post(&httpreq);
      case GET:
	      return build_and_send(&httpreq, NULL, 0);
   }
   return -1;
}

