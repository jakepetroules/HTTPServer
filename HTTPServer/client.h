//
//  client.h
//  HTTPServer
//
//  Created by Jake Petroules on 7/27/16.
//  Copyright © 2016 Jake Petroules. All rights reserved.
//

#ifndef client_h
#define client_h

#include <stdio.h>

typedef struct client * client_t;

client_t client_create(int fd);
void client_close(client_t);
void client_destroy(client_t);

int client_get_fd(client_t);

#endif /* client_h */
