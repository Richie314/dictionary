#ifndef _MODULE_DICTIONARY_H
#define _MODULE_DICTIONARY_H
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/wait.h>

/// @brief Node of the list: has key, value and a struct list_head object
typedef struct node {
    struct list_head list;
    char* key;
    char* value;
} *pnode;

/// @brief Dictionary class: has list of nodes and a mutex to protect them
typedef struct dictionary_base
{
    wait_queue_head_t queue;
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
/// @param key_length the length of the key
/// @param str the value we want to assign to the key
/// @param str_len the length of the value (could contain \0, so we cannot call strlen() on it)
/// @return zero for success, non zero otherwise
int dictionary_write(pdictionary dict, 
    const char* key, size_t key_length,
    const char* value, size_t str_len);

#define dictionary_delete_key(dict, key, key_length) dictionary_write(dict, key, key_length, NULL, 0)

/// @brief Appends str to the specified key, if the key is not present is created
/// @param dict pointer to the dictionary_base object
/// @param key assumed not NULL, the key we want to write to
/// @param key_length the length of the key
/// @param str the value we want to append to the key
/// @param str_len the length of the value (could contain \0, so we cannot call strlen() on it)
/// @return zero for success, non zero otherwise
int dictionary_append(pdictionary dict, 
    const char* key, size_t key_length,
    const char* value, size_t str_len);

/// @brief Reads the content of key and puts it into buffer
/// @param dict pointer to the dictionary_base object
/// @param key assumed not NULL, the key we want to write to
/// @param key_length the length of the key
/// @param buffer the buffer where the stored data will be copied
/// @param maxsize the max length of the buffer that we can receive
/// @param ppos passed from the Misc device file read method
/// @return number of bytes read, below zero for errors
ssize_t dictionary_read(pdictionary dict, 
    const char *key, size_t key_length, 
    char __user* buffer, size_t maxsize, loff_t *ppos);

/// @brief Reads all the key-value pairs
/// @param dict The dictionary we want to read
/// @param buffer the buffer where the stored data will be copied
/// @param maxsize the max length of the buffer that we can receive
/// @param ppos passed from the Misc device file read method
/// @return total number of bytes read, below zero for errors
ssize_t dictionary_read_all(pdictionary dict, char __user *buffer, size_t maxsize, loff_t *ppos);

/// @brief Reads the content of key and puts it into buffer
/// @param dict pointer to the dictionary_base object
/// @param key assumed not NULL, the key we want to write to
/// @param key_length the length of the key
/// @return zero for success, non zero otherwise
int dictionary_print_key(pdictionary dict, const char* key, size_t key_length);

/// @brief Reads all the key-value pairs
/// @param dict The dictionary we want to read
/// @return zero for success
int dictionary_print_all(pdictionary dict);

/// @brief De allocates all the keys, frees the mutex
/// @param dict pointer to the dictionary_base object
/// @return zero for success, non zero otherwise
int dictionary_free(pdictionary dict);

/// @brief Returns the number of keys present
/// @param dict pointer to the dictionary_base object
/// @return the number of keys, 0 for errors or for empty dictionary
size_t dictionary_count(pdictionary dict);

/// @brief Counts all the elements and checks if the result is zero
/// @param dict pointer to dictionary object
/// @return true if empty or for error, false otherwise
#define dictionary_empty(dict) (dictionary_count(dict) == 0)

/// @brief Locks the dictionary mutex, if it's already locked waits until it becomes avaible
/// @param dict pointer to dictionary object
/// @return true, awaits for completition
bool dictionary_lock(pdictionary dict);

/// @brief Checks if the dictionary is currently locked
/// @param dict pointer to dictionary object
/// @return true if locked, false otherwise
#define dictionary_is_locked(dict) mutex_is_locked(&dict->mutex)

/// @brief Unlocks dictionary's mutex
/// @param dict pointer to dictionary object
#define dictionary_unlock(dict) mutex_unlock(&dict->mutex); printd("Dictionary unlocked.\n")

#endif