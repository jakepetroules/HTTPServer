//
//  client.c
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#include "client.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

struct client {
    int fd;
};

client_t client_create(int fd)
{
    client_t c = (client_t)malloc(sizeof(struct client));
    if (c) {
        c->fd = fd;
    }
    return c;
}

void client_close(client_t client)
{
    if (client->fd != -1)
        close(client->fd);
}

void client_destroy(client_t client)
{
    free(client);
}

int client_get_fd(client_t client)
{
    assert(client != NULL);
    return client->fd;
}
