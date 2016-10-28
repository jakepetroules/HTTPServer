//
//  http_response.c
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#include "http_response.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct http_response {
    int fd;
    int status;
    size_t content_length;
    unsigned char *body;
    char *mime_type;
};

http_response_t http_response_create(int fd)
{
    http_response_t response = (http_response_t)malloc(sizeof(struct http_response));
    if (response) {
        response->fd = fd;
        response->status = 0;
        response->content_length = 0;
        response->body = NULL;
        response->mime_type = NULL;
    }
    return response;
}

void http_response_destroy(http_response_t response)
{
    if (response) {
        free(response->body);
        free(response->mime_type);
    }
    free(response);
}

int http_response_get_status(http_response_t response)
{
    return response->status;
}

void http_response_set_status(http_response_t response, int status)
{
    response->status = status;
}

void http_response_set_standard_status(http_response_t response, int status)
{
    http_response_set_status(response, status);
    const char *msg = NULL;
    switch (status) {
        case 404: {
            msg = "404 Not Found";
            break;
        }
        case 500: {
            msg = "500 Internal Server Error";
            break;
        }
    }

    if (msg) {
        http_response_set_content(response, (const unsigned char *)msg, strlen(msg));
        http_response_set_content_type(response, "text/plain");
    }
}

const unsigned char *http_response_get_content(http_response_t response)
{
    return response->body;
}

size_t http_response_get_content_length(http_response_t response)
{
    return response->content_length;
}

void http_response_set_content(http_response_t response, const unsigned char *body, size_t len)
{
    free(response->body);
    response->body = malloc(len);
    memcpy(response->body, body, len);
    response->content_length = len;
}

const char *http_response_get_content_type(http_response_t response)
{
    return response->mime_type;
}

void http_response_set_content_type(http_response_t response, const char *mime_type)
{
    if (mime_type) {
        free(response->mime_type);
        response->mime_type = malloc(strlen(mime_type));
        strcpy(response->mime_type, mime_type);
    } else {
        free(response->mime_type);
    }
}

void http_response_send(http_response_t response)
{
    const char *status_string = "";
    switch (response->status) {
        case 200:
            status_string = "OK";
            break;
        case 404:
            status_string = "Not Found";
            break;
        case 500:
            status_string = "Internal Server Error";
            break;
    }

    size_t size = 1024;
    char *header = malloc(size);
    do {
        header = realloc(header, size *= 2);
    } while (snprintf(header, size, "HTTP/1.1 %d %s\r\nContent-Length: %zu\r\n", response->status, status_string, response->content_length) > size);
    write(response->fd, header, strlen(header) + 1);
    free(header);

    if (response->mime_type) {
        const char prefix[] = "Content-Type: ";
        write(response->fd, prefix, sizeof(prefix));
        write(response->fd, response->mime_type, strlen(response->mime_type));
        write(response->fd, "\r\n", 2);
    }

    write(response->fd, "\r\n", 2);

    if (response->body && response->content_length)
        write(response->fd, response->body, response->content_length);
}
