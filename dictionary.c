#include <linux/slab.h>
#include <linux/poll.h>
#include "dictionary.h"

int dictionary_init(pdictionary dict)
{
    if (dict == NULL)
    {
        return 1;
    }
    mutex_init(&dict->mutex);
    INIT_LIST_HEAD(&dict->key_value_list);
    return 0;
}

static int wait_for_mutex_unlock(struct mutex *dict_mutex)
{
    return mutex_lock_interruptible(dict_mutex);
}
static struct list_head* dictionary_find_node(pdictionary dict, const char* key, struct node ** node_obj)
{
    struct list_head *pos, *q;
    size_t key_length;
    
    key_length = strlen(key);
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        *node_obj = list_entry(pos, struct node, list);
        if (strncmp((*node_obj)->key, key, key_length) == 0)
        {
            return pos;
        }
    }
    *node_obj = NULL;
    return (struct list_head*)NULL;
}
static pnode create_node_and_insert(pdictionary dict, const char* key)
{
    pnode new_node;

    //Create the new element
    new_node = (pnode)kmalloc(sizeof(struct node), GFP_USER);
    if (new_node == NULL)
    {
        return NULL;
    }
    memset(new_node, 0, sizeof(struct node));
    INIT_LIST_HEAD(&new_node->list);
    new_node->key = (char*)kmalloc(sizeof(char) * (strlen(key) + 1), GFP_USER);
    if (new_node->key == NULL)
    {
        //An error occured
        kfree(new_node);
        return NULL;
    }
    list_add(&new_node->list, &dict->key_value_list);
    return new_node;
}
static void delete_dict_entry(struct list_head* entry, pnode node_ptr)
{
    printk("Deleting item of key \"%s\" and value \"%s\"\n", node_ptr->key, node_ptr->value);
    kfree(node_ptr->key);
    kfree(node_ptr->value);
    list_del(entry);
}

static int update_node(pnode node, const char* str, size_t length)
{
    /*
    if (node->value == NULL)
    {
        nove->value = kmalloc(length, GFP_USER);
    } else {
        node->value = krealloc(node->value, length, GFP_USER)
    }
    */
    //krealloc behaves as the code above if node->value is NULL
    node->value = krealloc(node->value, length, GFP_USER);
    if (node->value == NULL)
        return 1;
    return copy_from_user(node->value, str, length);
}
int dictionary_write(pdictionary dict, const char* key, const char* str, size_t str_len)
{
    struct list_head *node_ref;
    struct node* node_ptr;
    int res;

    if (dict == NULL)
        return 1;
    if (wait_for_mutex_unlock(&dict->mutex) != 0)
    {
        return 1;
    }
    ////////////////////////////////////////
    //Mutex is locked from now on
    node_ref = dictionary_find_node(dict, key, &node_ptr);
    if (str_len == 0 || str == NULL)
    {
        //length of 0 means delete the node if present
        if (node_ptr != NULL)
        {
            //Delete the node here
            delete_dict_entry(node_ref, node_ptr);
        } else {
            res = 1;
        }
    } else {
        if (node_ptr == NULL)
        {
            //Node needs to be created
            node_ptr = create_node_and_insert(dict, key);
        }
        //Values are assigned here
        res = update_node(node_ptr, str, str_len);
    }
    //End of the write operations
    ////////////////////////////////////////
    //Unlock the mutex here
    mutex_unlock(&dict->mutex);
    return res;
}

int dictionary_read(pdictionary dict, const char* key, char* buffer, size_t maxsize)
{
    struct list_head *node_ref;
    pnode node_ptr;
    int res;
    size_t block_size;

    if (dict == NULL || buffer == NULL || maxsize == 0)
        return 1;
    if (wait_for_mutex_unlock(&dict->mutex) != 0)
    {
        return 1;
    }
    ////////////////////////////////////////
    //Mutex is locked from now on
    node_ref = dictionary_find_node(dict, key, &node_ptr);
    if (node_ptr == NULL)
    {
        //Key not created, wait here??
        res = 1;
    } else {
        block_size = ksize(node_ptr->value);
        //copy_to_user returns the bytes that could not be copied (0 for success)
        res = copy_to_user(buffer, node_ptr->value, min(maxsize, block_size));
    }
    //End of the read operations
    ////////////////////////////////////////
    //Unlock the mutex here
    mutex_unlock(&dict->mutex);
    return res;
}
int dictionary_free(pdictionary dict)
{
    struct list_head *pos, *q;
    pnode temp;

    if (dict == NULL)
        return 1;
    if (wait_for_mutex_unlock(&dict->mutex) != 0)
    {
        return 1;
    }
    
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        temp = list_entry(pos, struct node, list);
        //Delete every entry
        delete_dict_entry(pos, temp);
    }

    mutex_unlock(&dict->mutex);
    return 0;
}
size_t dictionary_count(pdictionary dict)
{
    struct list_head *pos, *q;
    size_t count;

    if (dict == NULL)
        return 0;
    if (wait_for_mutex_unlock(&dict->mutex) != 0)
    {
        return 0;
    }
    
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        ++count;
    }

    mutex_unlock(&dict->mutex);
    return count;
}