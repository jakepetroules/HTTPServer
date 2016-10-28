//
//  server.c
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#include "server.h"
#include "client.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>

struct server {
    uint16_t port;
    int socket_fd;
};

server_t server_create(uint16_t port)
{
    server_t s = (server_t)malloc(sizeof(struct server));
    if (s) {
        s->port = port;
        s->socket_fd = -1;
    }
    return s;
}

void server_destroy(server_t server)
{
    if (server)
        server_close(server);
    free(server);
}

int server_open(server_t server)
{
    assert(server != NULL);
    int result = 0;

    // socket already open
    if (server->socket_fd != -1) {
        return -1;
    }

    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server->port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if ((result = bind(server->socket_fd, (struct sockaddr *)&server_address, sizeof(server_address))) != 0) {
        perror("bind");
        return result;
    }

    if ((result = listen(server->socket_fd, 10)) != 0) {
        perror("listen");
        return result;
    }

    return result;
}

void server_close(server_t server)
{
    assert(server != NULL);
    if (server->socket_fd != -1)
        close(server->socket_fd);
}

client_t server_accept(server_t server)
{
    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    int client_socket_fd = accept(server->socket_fd, (struct sockaddr *)&client_address, &client_address_size);
    if (client_socket_fd < 0) {
        perror("accept");
        return NULL;
    }

    return client_create(client_socket_fd);
}
