//
//  server.h
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#ifndef server_h
#define server_h

#include <stdio.h>

typedef struct client * client_t;
typedef struct server * server_t;

server_t server_create(uint16_t port);
void server_destroy(server_t);

int server_open(server_t);
void server_close(server_t);

client_t server_accept(server_t);

#endif /* server_h */
