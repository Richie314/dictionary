#ifndef MODULE_DICTIONARY_H
#define MODULE_DICTIONARY_H
#include <linux/mutex.h>
#include <linux/list.h>

typedef struct node {
    struct list_head list;
    char* key;
    char* value;
} *pnode;


typedef struct dictionary_base
{
    struct mutex mutex;
    struct list_head key_value_list;
} dictionary_wrapper, *pdictionary;

/// @brief Initiates the dictionary instance, inits its mutex
/// @param dict pointer to the dictionary_base object to initiate
/// @return zero for success, non zero otherwise
int dictionary_init(pdictionary dict);

/// @brief Writes str to the specified key
/// @param dict pointer to the dictionary_base object
/// @param key assumed not NULL, the key we want to write to
/// @param str the value we want to assign to the key
/// @param str_len the length of the value (could contain \0, so we cannot call strlen() on it)
/// @return zero for success, non zero otherwise
int dictionary_write(pdictionary dict, const char* key, const char* value, size_t str_len);

#define dictionary_delete_key(dict, key) dictionary_write(dict, key, NULL, 0)

/// @brief Reads the content of key and puts it into buffer
/// @param dict pointer to the dictionary_base object
/// @param key assumed not NULL, the key we want to write to
/// @param buffer the buffer where the stored data will be copied
/// @param maxsize the max length of the buffer that we can receive
/// @return zero for success, non zero otherwise
int dictionary_read(pdictionary dict, const char* key, char* buffer, size_t maxsize);

/// @brief De allocates all the keys, frees the mutex
/// @param dict pointer to the dictionary_base object
/// @return zero for success, non zero otherwise
int dictionary_free(pdictionary dict);

/// @brief Returns the number of keys present
/// @param dict pointer to the dictionary_base object
/// @return the number of keys, 0 for errors or for empty dictionary
size_t dictionary_count(pdictionary dict);

#define dictionary_empty(dict) (dictionary_count(dict) == 0)

#endif