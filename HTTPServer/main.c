//
//  main.c
//  HTTPServer
//
//  Created by Jake Petroules on 7/26/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "client.h"
#include "server.h"
#include "http_request.h"
#include "http_response.h"

#ifdef __APPLE__
#import <CoreFoundation/CoreFoundation.h>
#import <CoreServices/CoreServices.h>
#endif

char *get_mime_type(const char *path)
{
    char *mime = 0;
#ifdef __APPLE__
    CFStringRef filePath = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingUTF8);
    if (filePath) {
        CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, filePath, kCFURLPOSIXPathStyle, false);
        if (url) {
            CFStringRef ext = CFURLCopyPathExtension(url);
            if (ext) {
                CFStringRef uti = UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension, ext, NULL);
                if (uti) {
                    CFStringRef mimeStr = UTTypeCopyPreferredTagWithClass(uti, kUTTagClassMIMEType);
                    if (mimeStr) {
                        size_t sz = CFStringGetMaximumSizeForEncoding(CFStringGetLength(mimeStr), kCFStringEncodingUTF8);
                        mime = malloc(sz);
                        CFStringGetCString(mimeStr, mime, sz, kCFStringEncodingUTF8);
                    }
                    CFRelease(uti);
                }
                CFRelease(ext);
            }
            CFRelease(url);
        }
        CFRelease(filePath);
    }
#endif
    return mime;
}

static volatile int canceled = 0;

static void handle_sigint(int sig) {
    canceled = 1;
}

static void read_file_into_response(http_response_t response, const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        size_t sz = ftell(file);
        fseek(file, 0, SEEK_SET);
        unsigned char *buffer = malloc(sz);
        fread(buffer, 1, sz, file);
        http_response_set_status(response, 200);
        http_response_set_content(response, buffer, sz);
        free(buffer);
        fclose(file);

        char *mime = get_mime_type(path);
        http_response_set_content_type(response, mime);
        free(mime);
    } else {
        http_response_set_standard_status(response, 404);
        const char *err = strerror(errno);
        http_response_set_content(response, (const unsigned char *)err, strlen(err));
    }
}

static bool read_ok(ssize_t c)
{
    return c > 0 || errno == EINTR || errno == EAGAIN;
}

static void execute_file_into_response(http_request_t request, http_response_t response, const char *path)
{
    int pipefd[2];
    pipe(pipefd);
    pid_t child = fork();
    if (child == 0) {
        // child
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        dup2(pipefd[1], 2);
        close(pipefd[1]);
        execve(http_request_get_path(request), NULL, NULL);
    } else if (child > 0) {
        // parent
        unsigned char *buffer = malloc(1024);
        size_t off = 0;
        ssize_t ret = -1;
        close(pipefd[1]);
        while (read_ok(ret = read(pipefd[0], buffer + off, 1024))) {
            off += ret;
            buffer = realloc(buffer, off + 1024);
        }

        if (ret != -1) {
            http_response_set_status(response, 200);
            http_response_set_content(response, buffer, off);
            http_response_set_content_type(response, "text/plain");
            return; // else fall back to 500
        }
    }

    http_response_set_standard_status(response, 500);
    const char *err = strerror(errno);
    http_response_set_content(response, (const unsigned char *)err, strlen(err));
}

static char *get_absolute_path(const char *base_path, const char *path)
{
    const size_t base_path_sz = strlen(base_path);
    const size_t path_sz = strlen(path);
    char *abs_path = malloc(base_path_sz + path_sz + 1);
    memcpy(abs_path, base_path, base_path_sz);
    memcpy(abs_path + base_path_sz, path, path_sz);
    abs_path[base_path_sz + path_sz] = 0;
    return abs_path;
}

int main(int argc, const char * argv[])
{
    signal(SIGINT, handle_sigint);

    uint16_t port = 8080;
    if (argc > 1) {
        port = (uint16_t)strtold(argv[1], NULL);
    }

    const char *base_path = "/";
    if (argc > 2) {
        base_path = argv[2];
    }

    server_t server = server_create(port);
    if (server_open(server) != 0)
        return 1;

    fprintf(stderr, "Listening on port %hu at base path %s...\n", port, base_path);

    do {
        client_t client = server_accept(server);
        if (client) {
            pid_t child = fork();
            if (child < 0)
                perror("fork");

            fprintf(stderr, "Accepted new client\n");
            while (child == 0) {
                http_request_t request = http_request_create(client_get_fd(client));
                if (http_request_get_error(request) != 0) {
                    fprintf(stderr, "Failed to parse HTTP request\n");
                    break;
                } else {
                    const char *verb = http_request_get_verb(request);
                    const char *path = http_request_get_path(request);

                    char *abs_path = get_absolute_path(base_path, path);

                    fprintf(stderr, "Parsed new HTTP request: %s %s for file %s\n", verb, path, abs_path);

                    http_response_t response = http_response_create(client_get_fd(client));
                    if (strcmp(verb, "GET") == 0) {
                        struct stat sb;
                        if (stat(abs_path, &sb) == 0 && sb.st_mode & S_IXUSR)
                            execute_file_into_response(request, response, abs_path); // CGI
                        else
                            read_file_into_response(response, abs_path);
                    } else {
                        http_response_set_standard_status(response, 500);
                    }
                    free(abs_path);
                    http_response_send(response);
                    http_response_destroy(response);
                    int keep_alive = http_request_get_keep_alive(request);
                    http_request_destroy(request);

                    if (!keep_alive)
                        break;
                }
            }
            if (child == 0)
                client_close(client);
            client_destroy(client);
        }
    } while (!canceled);

    server_destroy(server);
    return 0;
}
