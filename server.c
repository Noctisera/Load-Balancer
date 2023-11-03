/* Copyright 2023 <> */
#include <stdlib.h>
#include <string.h>

#include "load_balancer.h"
#include "server.h"
#include "utils.h"

struct server_memory {
	int server_id;
    int replica_id;

    hashtable_t *hash_table;
};

struct item_memory {
    void *value;
    void *key;
};

server_memory *init_server_memory(int server_id, int replica_id)
{
    server_memory *server = malloc(sizeof(struct server_memory));
    DIE(server == NULL, "server malloc failed");

    server->server_id = server_id;
    server->replica_id = replica_id;
    server->hash_table = ht_create(HMAX, hash_function_key,
                                   compare_function_keys, key_val_free_function);

	return server;
}

item_memory *init_item_memory()
{
    item_memory *item = malloc(sizeof(item_memory));

    item->value = NULL;
    item->key = NULL;

    return item;
}

void server_store(server_memory *server, char *key, char *value) {
    ht_put(server->hash_table, key, strlen(key) + 1, value, strlen(value) + 1);
}

char *server_retrieve(server_memory *server, char *key) {
    return ht_get(server->hash_table, key);
}

void server_remove(server_memory *delete_server, server_memory *destination_server) {
    ht_merge(destination_server->hash_table, delete_server->hash_table);
    ht_free(delete_server->hash_table);
    free(delete_server);
}

void free_server_memory(server_memory *server) {
    ht_free(server->hash_table);
    free(server);
}

int *server_get_id(server_memory *server) {
    return &server->server_id;
}

void server_get_lower_items(server_memory *next_server, server_memory *new_server)
{
    unsigned int key = 100000 * next_server->replica_id + next_server->server_id;
    unsigned int key_hash1 = hash_function_servers(&key);
    key = 100000 * new_server->replica_id + new_server->server_id;
    unsigned int key_hash2 = hash_function_servers(&key);
    ht_get_lower_items(next_server->hash_table, new_server->hash_table, key_hash1, key_hash2);
}



// COPY-PASTED SHIT HERE

typedef struct ll_node_t
{
    void* data;
    struct ll_node_t* next;
} ll_node_t;

typedef struct linked_list_t
{
    ll_node_t* head;
    unsigned int data_size;
    unsigned int size;
} linked_list_t;

linked_list_t *ll_create(unsigned int data_size)
{
    linked_list_t* ll;

    ll = malloc(sizeof(*ll));

    ll->head = NULL;
    ll->data_size = data_size;
    ll->size = 0;

    return ll;
}

void ll_add_nth_node(linked_list_t* list, unsigned int n, const void* new_data)
{
    ll_node_t *prev, *curr;
    ll_node_t* new_node;

    if (!list) {
        return;
    }

    if (n > list->size) {
        n = list->size;
    }

    curr = list->head;
    prev = NULL;
    while (n > 0) {
        prev = curr;
        curr = curr->next;
        --n;
    }

    new_node = malloc(sizeof(*new_node));

    new_node->data = malloc(list->data_size);
    memcpy(new_node->data, new_data, list->data_size);

    new_node->next = curr;
    if (prev == NULL) {
        /* Adica n == 0. */
        list->head = new_node;
    } else {
        prev->next = new_node;
    }

    list->size++;
}

ll_node_t *ll_remove_nth_node(linked_list_t* list, unsigned int n)
{
    ll_node_t *prev, *curr;

    if (!list || !list->head) {
        return NULL;
    }

    if (n > list->size - 1) {
        n = list->size - 1;
    }

    curr = list->head;
    prev = NULL;
    while (n > 0) {
        prev = curr;
        curr = curr->next;
        --n;
    }

    if (prev == NULL) {
        /* Adica n == 0. */
        list->head = curr->next;
    } else {
        prev->next = curr->next;
    }

    list->size--;

    return curr;
}

unsigned int ll_get_size(linked_list_t* list)
{
    if (!list) {
        return -1;
    }

    return list->size;
}

void ll_free(linked_list_t** pp_list)
{
    ll_node_t* currNode;

    if (!pp_list || !*pp_list) {
        return;
    }

    while (ll_get_size(*pp_list) > 0) {
        currNode = ll_remove_nth_node(*pp_list, 0);
        free(currNode->data);
        currNode->data = NULL;
        free(currNode);
        currNode = NULL;
    }

    free(*pp_list);
    *pp_list = NULL;
}

typedef struct hashtable_t hashtable_t;
struct hashtable_t {
    linked_list_t **buckets;
    unsigned int size;
    unsigned int hmax;
    unsigned int (*hash_function)(void*);
    int (*compare_function)(void*, void*);
    void (*key_val_free_function)(void*);
};

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
                       int (*compare_function)(void*, void*),
                       void (*key_val_free_function)(void*))
{
    hashtable_t *ht;
    ht = malloc(sizeof(hashtable_t));
    DIE(ht == NULL, "ht malloc");
    ht->buckets = malloc(hmax * sizeof(linked_list_t*));
    DIE(ht->buckets == NULL, "ht->buckets malloc");
    for (unsigned int i = 0; i < hmax; i++) {
        ht->buckets[i] = ll_create(sizeof(item_memory));
        DIE(ht->buckets[i] == NULL, "ll_create");
    }
    ht->size = 0;
    ht->hmax = hmax;
    ht->hash_function = hash_function;
    ht->compare_function = compare_function;
    ht->key_val_free_function = key_val_free_function;
    return ht;
}

void *ht_get(hashtable_t *ht, void *key)
{
    if (!ht) {
        return NULL;
    }

    unsigned int index = ht->hash_function(key) % ht->hmax;
    ll_node_t *current = ht->buckets[index]->head;
    while(current != NULL && index < ht->hmax) {
        if (ht->compare_function(((item_memory *)current->data)->key, key) == 0) {
            return ((item_memory *)current->data)->value;
        }
        current = ht->buckets[++index]->head;
    }
    return NULL;
}

void ht_put(hashtable_t *ht, void *key, unsigned int key_size,
            void *value, unsigned int value_size)
{
    if (!ht || !ht->buckets) {
        return;
    }
    // Introduces new key-value pair in hashtable
    ht->size += 1;
    item_memory *item = malloc(sizeof(item_memory));
    DIE(item == NULL, "item malloc");
    item->key = malloc(key_size * sizeof(char));
    DIE(item->key == NULL, "item->key malloc");
    item->value = malloc(value_size * sizeof(char));
    DIE(item->value == NULL, "item->value malloc");
    memcpy(item->key, key, key_size);
    memcpy(item->value, value, value_size);

    /* If the key already exists, introduce into the next bucket */
    unsigned int index = ht->hash_function(key) % ht->hmax;
    while (ht->buckets[index]->head != NULL) {
        index += 1;
        index %= ht->hmax;
    }
    ll_add_nth_node(ht->buckets[index], 0, item);
    free(item);
}

void ht_free(hashtable_t *ht)
{
    if (!ht) {
        return;
    }
    for (unsigned int i = 0; i < ht->hmax; i++) {
        ll_node_t *current = ht->buckets[i]->head;
        while(current != NULL) {
            free(((item_memory *)current->data)->key);
            free(((item_memory *)current->data)->value);
            current = current->next;
        }
        ll_free(&ht->buckets[i]);
    }
    free(ht->buckets);
    free(ht);
}

void key_val_free_function(void *data)
{
    item_memory *item = (item_memory *)data;
    free(item->key);
    free(item->value);
    free(item);
}

int compare_function_keys(void *a, void *b)
{
    char *str_a = (char *)a;
    char *str_b = (char *)b;

    return strcmp(str_a, str_b);
}

void ht_merge(hashtable_t *ht1, hashtable_t *ht2)
{
    if (!ht1 || !ht2) {
        return;
    }

    /* Put all the items from ht2 into ht1 */
    for (unsigned int i = 0; i < ht2->hmax; i++) {
        ll_node_t *current = ht2->buckets[i]->head;
        if(current != NULL) {
            ht_put(ht1, ((item_memory *)current->data)->key,
                   strlen(((item_memory *)current->data)->key) + 1,
                   ((item_memory *)current->data)->value,
                   strlen(((item_memory *)current->data)->value) + 1);
        }
    }
}

void ht_get_lower_items(hashtable_t *ht1, hashtable_t *ht2, unsigned int key_hash1,unsigned int key_hash2)
{
    /* Put all the items from ht1 with hash smaller than key_hash into ht2 */
    for (unsigned int i = 0; i < ht1->hmax; i++) {
        ll_node_t *current = ht1->buckets[i]->head;
        if(current == NULL) {
            continue;
        }
        if(key_hash2 < key_hash1) {
            if (ht1->hash_function(((item_memory *) current->data)->key) <= key_hash2 ||
                ht1->hash_function(((item_memory *) current->data)->key) > key_hash1) {
                ht_put(ht2, ((item_memory *) current->data)->key,
                       strlen(((item_memory *) current->data)->key) + 1,
                       ((item_memory *) current->data)->value,
                       strlen(((item_memory *) current->data)->value) + 1);
            }
        }
        else {
            /* Special case for when the new server is the last on the hash ring */
            if (ht1->hash_function(((item_memory *) current->data)->key) <= key_hash2 &&
                ht1->hash_function(((item_memory *) current->data)->key) > key_hash1) {
                ht_put(ht2, ((item_memory *) current->data)->key,
                       strlen(((item_memory *) current->data)->key) + 1,
                       ((item_memory *) current->data)->value,
                       strlen(((item_memory *) current->data)->value) + 1);
            }
        }
    }
}