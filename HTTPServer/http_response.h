//
//  http_response.h
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#ifndef http_response_h
#define http_response_h

#include <stdio.h>

typedef struct http_response * http_response_t;

http_response_t http_response_create(int fd);
void http_response_destroy(http_response_t response);

int http_response_get_status(http_response_t response);
void http_response_set_status(http_response_t response, int status);
void http_response_set_standard_status(http_response_t response, int status);

const unsigned char *http_response_get_content(http_response_t response);
size_t http_response_get_content_length(http_response_t response);
void http_response_set_content(http_response_t response, const unsigned char *body, size_t len);

const char *http_response_get_content_type(http_response_t response);
void http_response_set_content_type(http_response_t response, const char *mime_type);

void http_response_send(http_response_t response);

#endif /* http_response_h */
