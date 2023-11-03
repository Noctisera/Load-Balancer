/* Copyright 2023 <> */
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "server.h"
#include "utils.h"

struct load_balancer {
    void **hash_table_servers;
    void **hash_table_keys;

    int servers_count;
};

unsigned int hash_function_servers(void *a) {
    unsigned int uint_a = *((unsigned int *)a);
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
    uint_a = (uint_a >> 16u) ^ uint_a;
    return uint_a;
}

unsigned int hash_function_key(void *a) {
    unsigned char *puchar_a = (unsigned char *)a;
    unsigned int hash = 5381;
    int c;

    while ((c = *puchar_a++))
        hash = ((hash << 5u) + hash) + c;

    return hash;
}

load_balancer *init_load_balancer() {
    load_balancer *main = malloc(sizeof(struct load_balancer));
    DIE(main == NULL, "load_balancer malloc failed");

    main->hash_table_servers = NULL;
    main->hash_table_keys = NULL;
    main->servers_count = 0;

    return main;
}

void loader_add_server(load_balancer *main, int server_id) {
    /* Expand the hashtable with 3 more slots everytime we add a server */
    if (main->hash_table_servers == NULL) {
        main->hash_table_servers = malloc(3 * sizeof(server_memory*));
        DIE(main->hash_table_servers == NULL, "hash_table_servers malloc failed");

        main->hash_table_keys = malloc(3 * sizeof(unsigned int*));
        DIE(main->hash_table_keys == NULL, "hash_table_keys failed");
        main->servers_count = 3;
    }
    else {
        main->hash_table_servers = realloc(main->hash_table_servers,
                                           (main->servers_count + 3) * sizeof(server_memory*));
        DIE(main->hash_table_servers == NULL, "hash_table_servers realloc failed");
        main->hash_table_keys = realloc(main->hash_table_keys,
                                        (main->servers_count + 3) * sizeof(unsigned int*));
        DIE(main->hash_table_keys == NULL, "hash_table_keys realloc failed");
        main->servers_count += 3;
    }

    /* Add the 3 servers are replicas of the same server at the end of the hash */
    for (int i = 0; i < 3; i++) {
        server_memory *server = init_server_memory(server_id, i);
        main->hash_table_servers[main->servers_count - 3 + i] = server;

        unsigned int *server_id_hash = malloc(sizeof(unsigned int));
        DIE(server_id_hash == NULL, "server_id_hash malloc failed");
        unsigned int key = 100000 * i + server_id;
        *server_id_hash = hash_function_servers(&key);
        main->hash_table_keys[main->servers_count - 3 + i] = server_id_hash;
    }

    /* Sort the hash_table_servers by the hash values in hash_table_keys */
    /* Maybe not the most efficient sorting algorithm, but it will do its job */
    for (int i = 0; i < main->servers_count - 1; i++) {
        for (int j = i + 1; j < main->servers_count; j++) {
            unsigned int i_key = *((unsigned int *)main->hash_table_keys[i]);
            unsigned int j_key = *((unsigned int *)main->hash_table_keys[j]);
            if (i_key > j_key) {
                void *aux = main->hash_table_keys[i];
                main->hash_table_keys[i] = main->hash_table_keys[j];
                main->hash_table_keys[j] = aux;

                aux = main->hash_table_servers[i];
                main->hash_table_servers[i] = main->hash_table_servers[j];
                main->hash_table_servers[j] = aux;
            }
        }
    }

    /* Give the new servers the items from the next server */
    for (int i = 0; i < main->servers_count; i++) {
        if (server_id == *server_get_id((server_memory*)main->hash_table_servers[i])) {
            server_memory *next_server = (server_memory*)main->hash_table_servers[(i + 1) % main->servers_count];
            server_get_lower_items(next_server, (server_memory*)main->hash_table_servers[i]);
        }
    }
}

void loader_remove_server(load_balancer *main, int server_id) {
    /* Find all the servers with the given id */
    for (int i = 0; i < main->servers_count; i++) {
        server_memory *server = (server_memory*)main->hash_table_servers[i];
        if (*server_get_id(server) == server_id) {
            server_memory *new_server = (server_memory*)main->hash_table_servers[(i + 1) % main->servers_count];
            server_remove(server, new_server);
            free(main->hash_table_keys[i]);

            /* Rearrange the server vector */
            for (int j = i; j < main->servers_count - 1; j++) {
                main->hash_table_servers[j] = main->hash_table_servers[j + 1];
                main->hash_table_keys[j] = main->hash_table_keys[j + 1];
            }
            main->hash_table_servers = realloc(main->hash_table_servers,
                                               (main->servers_count - 1) * sizeof(server_memory*));
            DIE(main->hash_table_servers == NULL, "hash_table_servers realloc failed");
            main->hash_table_keys = realloc(main->hash_table_keys,
                                            (main->servers_count - 1) * sizeof(unsigned int*));
            DIE(main->hash_table_keys == NULL, "hash_table_keys realloc failed");
            main->servers_count--;

            i--;
        }
    }
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id) {
    /* Calculate the hash of the item key */
    unsigned int key_hash = hash_function_key(key);

    /* Find the server with the smallest hash that is bigger than the hash of the item */
    int i = 0;
    while (i < main->servers_count && *(unsigned int*)main->hash_table_keys[i] < key_hash) {
        i++;
    }
    if (i == main->servers_count) {
        i = 0;
    }

    server_memory *server = (server_memory*)main->hash_table_servers[i];
    *server_id = *server_get_id(server);
    server_store(server, key, value);
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
    /* Find the server with the smallest hash that is bigger than the hash of the item */
    unsigned int key_hash = hash_function_key(key);
    int i = 0;
    while (i < main->servers_count && *(unsigned int*)main->hash_table_keys[i] < key_hash) {
        i++;
    }
    if (i == main->servers_count) {
        i = 0;
    }

    server_memory *server = (server_memory*)main->hash_table_servers[i];
    *server_id = *server_get_id(server);
    char *item_key = server_retrieve(server, key);
    return item_key;
}

void free_load_balancer(load_balancer *main) {
    /* Free hast_table_servers and hash_table_keys */
    for (int i = 0; i < main->servers_count; i++) {
        free(main->hash_table_keys[i]);
        free_server_memory((server_memory*)main->hash_table_servers[i]);
    }
    free(main->hash_table_servers);
    free(main->hash_table_keys);

    free(main);
}
