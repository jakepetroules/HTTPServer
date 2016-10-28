//
//  http_request.c
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#include "http_request.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *kHeaderNameConnection = "Connection: ";
static const char *kHeaderNameContentLength = "Content-Length: ";

void _http_request_intro_parse(http_request_t request, int fd, const char *buffer);
void _http_request_parse(http_request_t request, int fd);

struct http_request {
    char *verb;
    char *path;
    int error;
    int keep_alive;
};

http_request_t http_request_create(int fd)
{
    http_request_t request = (http_request_t)malloc(sizeof(struct http_request));
    if (request) {
        request->verb = 0;
        request->path = 0;
        request->error = 0;
        request->keep_alive = 1;
        _http_request_parse(request, fd);
    }
    return request;
}

void http_request_destroy(http_request_t request)
{
    if (request) {
        free(request->verb);
        free(request->path);
    }
    free(request);
}

const char *http_request_get_verb(http_request_t request)
{
    return request->verb;
}

const char *http_request_get_path(http_request_t request)
{
    return request->path;
}

int http_request_get_error(http_request_t request)
{
    return request ? request->error : 1;
}

int http_request_get_keep_alive(http_request_t request)
{
    return request->keep_alive;
}

void _http_request_intro_parse(http_request_t request, int fd, const char *buffer)
{
    ssize_t off = strlen(buffer);
    size_t i = 0;
    size_t start = 0;
    for (start = i; i < off; ++i) {
        if (buffer[i] == ' ') {
            request->verb = malloc(i - start + 1);
            memcpy(request->verb, buffer + start, i - start);
            request->verb[i - start] = '\0';
            ++i;
            break;
        }
    }

    for (start = i; i < off; ++i) {
        if (buffer[i] == ' ') {
            request->path = malloc(i - start + 1);
            memcpy(request->path, buffer + start, i - start);
            request->path[i - start] = '\0';
            ++i;
            break;
        }
    }

    if (!request->verb || !request->path)
        request->error = 1;
}

void _http_request_parse(http_request_t request, int fd)
{
    size_t request_content_length = 0;

    ssize_t off = 0;
    char buffer[8192];
    char null;

    int lines_read = 0;

    do {
        // Read a line of the HTTP request
        const char *line = buffer + off;
        ssize_t nbytes_read = -1;
        while ((nbytes_read = read(fd, buffer + off, 1)) == 1) {
            if (buffer[off] == '\r') {
                buffer[off] = '\0';

                // assume \n follows
                if (read(fd, &null, 1) != 1) {
                    request->error = 1;
                    return;
                }

                ++off;

                break;
            }
            ++off;
        }

        // read() failed
        if (nbytes_read < 0) {
            request->error = 1;
            return;
        }

        lines_read++;

        if (lines_read > 1) {
            size_t hlen = strlen(kHeaderNameContentLength);
            if (strlen(line) >= hlen && strncmp(line, kHeaderNameContentLength, hlen) == 0)
                request_content_length = strtold(line + hlen, NULL);

            hlen = strlen(kHeaderNameConnection);
            if (strlen(line) >= hlen && strncmp(line, kHeaderNameConnection, hlen) == 0) {
                if (strncmp(line + hlen, "Close", 5) == 0 ||
                    strncmp(line + hlen, "close", 5) == 0)
                    request->keep_alive = 0;
            }

            // ignore other headers for now
        } else {
            _http_request_intro_parse(request, fd, line);
            if (request->error != 0)
                return;
        }

        if (strlen(line) == 0)
            break; // empty line indicating end of request headers
    } while (1);

    if (request_content_length > 0) {
        char content_body_buffer[request_content_length];
        if (read(fd, content_body_buffer, request_content_length) != request_content_length) {
            request->error = 1;
            return;
        }
    }
}
