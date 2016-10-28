//
//  http_request.h
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#ifndef http_request_h
#define http_request_h

#include <stdio.h>

typedef struct http_request * http_request_t;

http_request_t http_request_create(int fd);
void http_request_destroy(http_request_t request);

const char *http_request_get_verb(http_request_t request);
const char *http_request_get_path(http_request_t request);
int http_request_get_error(http_request_t request);
int http_request_get_keep_alive(http_request_t request);

#endif /* http_request_h */
