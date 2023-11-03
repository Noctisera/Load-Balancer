/* Copyright 2023 <> */
#ifndef SERVER_H_
#define SERVER_H_

#define HMAX 10000

struct server_memory;
typedef struct server_memory server_memory;
struct item_memory;
typedef struct item_memory item_memory;

struct linked_list_t;
typedef struct linked_list_t linked_list_t;
struct hashtable_t;
typedef struct hashtable_t hashtable_t;
struct ll_node_t;
typedef struct ll_node_t ll_node_t;

/** init_server_memory() -  Initializes the memory for a new server struct.
 * 							Make sure to check what is returned by malloc using DIE.
 * 							Use the linked list implementation from the lab.
 *
 * Return: pointer to the allocated server_memory struct.
 */
server_memory *init_server_memory(int server_id, int replica_id);

/** free_server_memory() - Free the memory used by the server.
 * 						   Make sure to also free the pointer to the server struct.
 * 						   You can use the server_remove() function for this.
 *
 * @arg1: Server to free
 */
void free_server_memory(server_memory *server);

/**
 * server_store() - Stores a key-value pair to the server.
 *
 * @arg1: Server which performs the task.
 * @arg2: Key represented as a string.
 * @arg3: Value represented as a string.
 */
void server_store(server_memory *server, char *key, char *value);

/**
 * server_remove() - Removes a server and transfers its items to the next server.
 *                   Makes sure to also free the giving server.
 *
 * @arg1: Server to be removed.
 * @arg2: Server to which the items will be transferred.
 */
void server_remove(server_memory *delete_server, server_memory *destination_server);

/**
 * server_retrieve() - Gets the value associated with the key.
 * @arg1: Server which performs the task.
 * @arg2: Key represented as a string.
 *
 * Return: String value associated with the key
 *         or NULL (in case the key does not exist).
 */
char *server_retrieve(server_memory *server, char *key);

int *server_get_id(server_memory *server);

void server_get_lower_items(server_memory *next_server, server_memory *new_server);

/* Basic hashtable and linked list functions copy-pasted from old sources*/
linked_list_t *ll_create(unsigned int data_size);
void ll_add_nth_node(linked_list_t* list, unsigned int n, const void* new_data);
ll_node_t *ll_remove_nth_node(linked_list_t* list, unsigned int n);
unsigned int ll_get_size(linked_list_t* list);
void ll_free(linked_list_t** pp_list);
hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
                       int (*compare_function)(void*, void*),
                       void (*key_val_free_function)(void*));
void ht_put(hashtable_t *ht, void *key, unsigned int key_size,
            void *value, unsigned int value_size);
void *ht_get(hashtable_t *ht, void *key);
void ht_free(hashtable_t *ht);
void key_val_free_function(void *data);
int compare_function_keys(void *a, void *b);

/**
 * ht_merge() -  Puts all the items from a hash table into another
 *
 * @arg1: Hashtable to receive items.
 * @arg2: Hashtable to give items.
 */
void ht_merge(hashtable_t *ht1, hashtable_t *ht2);

/**
 * ht_get_lower_items() -  Put all the items from a hash table into another
 *                         if the key hash is lower than the ones of the server
 *
 * @arg1: Hashtable to give items.
 * @arg2: Hashtable to receive items.
 */
void ht_get_lower_items(hashtable_t *ht1, hashtable_t *ht2, unsigned int key_hash1,unsigned int key_hash2);

#endif /* SERVER_H_ */
