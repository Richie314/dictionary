#include <linux/slab.h>
#include <linux/poll.h>
#include "dictionary.h"
//Useful functions 
static int wait_for_mutex_unlock(struct mutex *dict_mutex)
{
    return mutex_lock_interruptible(dict_mutex);
}
static struct list_head* dictionary_find_node(pdictionary dict, 
    const char* key, size_t key_length, struct node ** node_obj)
{
    struct list_head *pos, *q;
    
    if (key_length == 0)
    {
        key_length = strlen(key);
    }
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
static pnode create_node_and_insert(pdictionary dict, const char* key, size_t key_length)
{
    pnode new_node;
    size_t ret;

    //Create the new element
    new_node = (pnode)kmalloc(sizeof(struct node), GFP_USER);
    if (new_node == NULL)
    {
        return NULL;
    }
    memset(new_node, 0, sizeof(struct node));
    INIT_LIST_HEAD(&new_node->list);
    if (key_length == 0)
    {
    key_length = strlen(key);
    }
    new_node->key = (char*)kmalloc(key_length + 1, GFP_USER);
    if (new_node->key == NULL)
    {
        //An error occured
        kfree(new_node);
        return NULL;
    }
    memset(new_node->key, 0, key_length + 1);
    ret = copy_from_user(new_node->key, key, key_length);
    if (ret != 0)
    {
        //An error occured: copy_from_user failed
        if (ret <= key_length)
        {
            //A possible cause is that key was not a user space string
            memcpy(new_node->key, key, key_length);
        } else {
            kfree(new_node->key);
            kfree(new_node);
            return NULL;
        }
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
    int res;

    /*
    if (node->value == NULL)
    {
        nove->value = kmalloc(length, GFP_USER);
    } else {
        node->value = krealloc(node->value, length, GFP_USER)
    }
    */
    //krealloc behaves as the code above if node->value is NULL
    node->value = (char*)krealloc(node->value, length + 1, GFP_USER);
    if (node->value == NULL)
        return 1;
    memset(node->value, 0, length + 1);
    res = copy_from_user(node->value, str, length);
    if (res != 0)
    {
        if (res < length)
        {
            //copy_from user failed
            kfree(node->value);
            node->value = NULL;
            return res;
        }
        //This might be cause because str was not a user space string (could be a literal for example)
        if (memcpy(node->value, str, length) == node->value)
        {
            //The error was fixed
            res = 0;
        }
    }
    return res;
}
static int append_node(pnode node, const char* str, size_t length)
{
    int res;
    size_t old_size;
    size_t old_strlen;
    void* new_offSet;

    old_size = ksize(node->value);//ksize returns the actual size of the block (is >= actual length of the string)
    old_strlen = strlen(node->value);
    old_size = min(old_strlen, old_size);
    printk(KERN_DEBUG "appennd_node on \"%s\": appending \"%s\" (%d) to \"%s\" (%d)\n", node->key, str, (int)length, node->value, (int)old_size + 1);
    node->value = (char*)krealloc(node->value, old_size + length + 1, GFP_USER);
    if (node->value == NULL)
        return 1;
    new_offSet = &node->value[old_size];
    memset(new_offSet, 0, length);
    res = copy_from_user(new_offSet, str, length);
    if (res != 0)
    {
        if (res < length)
        {
            //copy_from user failed
            kfree(node->value);
            node->value = NULL;
            return res;
        }
        //This might be cause because str was not a user space string (could be a literal for example)
        if (memcpy(new_offSet, str, length) == new_offSet)
        {
            //The error was fixed
            res = 0;
        }
    }
    return res;
}

//
//
//      Header functions body
//
//

//Init functon: the first one to be called
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

//Write function
int dictionary_write(pdictionary dict, 
    const char* key, size_t key_length,
    const char* str, size_t str_len)
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
    node_ref = dictionary_find_node(dict, key, key_length, &node_ptr);
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
            node_ptr = create_node_and_insert(dict, key, key_length);
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

//Append function
int dictionary_append(pdictionary dict, 
    const char* key, size_t key_length,
    const char* str, size_t str_len)
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
    node_ref = dictionary_find_node(dict, key, key_length, &node_ptr);
    if (str_len == 0 || str == NULL)
    {
        //Bad call
        res = 1;
    } else {
        if (node_ptr == NULL)
        {
            //Node needs to be created
            node_ptr = create_node_and_insert(dict, key, key_length);
            res = update_node(node_ptr, str, str_len);
        } else {
            //Node exists and we append data to it
            res = append_node(node_ptr, str, str_len);
        }
    }
    //End of the write operations
    ////////////////////////////////////////
    //Unlock the mutex here
    mutex_unlock(&dict->mutex);
    return res;
}

//Read to buffer function
int dictionary_read(pdictionary dict, const char* key, size_t key_length, char* buffer, size_t maxsize)
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
    node_ref = dictionary_find_node(dict, key, key_length, &node_ptr);
    if (node_ptr == NULL)
    {
        //Key not created, wait here??
        //
        //
        // Code to add in the future
        //
        //
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

//Read all keys to buffer function
int dictionary_read_all(pdictionary dict, char* buffer, size_t maxsize)
{
    struct list_head *pos, *q;
    pnode temp;
    size_t index;
    size_t node_size;

    if (dict == NULL)
    {
        printk(KERN_ERR "dictionary_read_all: dictionary was NULL!\n");
        return 1;
    }
    if (buffer == NULL)
    {
        printk(KERN_ERR "dictionary_read_all: output buffer was NULL!\n");
        return 1;
    }
    sprintf(buffer, "Starting to print key-value paiers, max output length set to %d\n", (int)maxsize);
    index = 65;
    if (wait_for_mutex_unlock(&dict->mutex) != 0)
    {
        printk(KERN_ERR "dictionary_read_all: Couldn't unlock the mutex!\n");
        return 1;
    }
    
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        temp = list_entry(pos, struct node, list);
        if (temp != NULL)
        {
            node_size = ksize(temp->key) + ksize(temp->value) + 7;
            if (index + node_size < maxsize)
            {
                sprintf(&buffer[index], "\"%s\": \"%s\"\n", temp->key, temp->value);
                index += node_size;
            }
        }
    }
    
    mutex_unlock(&dict->mutex);
    return 0;
}

//Print key function
int dictionary_print_key(pdictionary dict, const char* key, size_t key_length)
{
    struct list_head *node_ref;
    pnode node_ptr;
    int res;

    if (dict == NULL)
        return 1;
    if (wait_for_mutex_unlock(&dict->mutex) != 0)
    {
        return 1;
    }
    ////////////////////////////////////////
    //Mutex is locked from now on
    node_ref = dictionary_find_node(dict, key, key_length, &node_ptr);
    if (node_ptr == NULL)
    {
        printk(KERN_ALERT "\"%s\": Not found!\n", key);
        res = 1;
    } else {
        printk(KERN_DEBUG "\"%s\": \"%s\"\n", node_ptr->key, node_ptr->value);
    }
    //End of the read operations
    ////////////////////////////////////////
    //Unlock the mutex here
    mutex_unlock(&dict->mutex);
    return res;
}

//Print all keys function
int dictionary_print_all(pdictionary dict)
{
    struct list_head *pos, *q;
    pnode temp;

    if (dict == NULL)
    {
        printk(KERN_ERR "dictionary_print_all: dictionary was NULL!\n");
        return 1;
    }
    if (wait_for_mutex_unlock(&dict->mutex) != 0)
    {
        printk(KERN_ERR "dictionary_print_all: Couldn't unlock the mutex!\n");
        return 1;
    }
    
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        temp = list_entry(pos, struct node, list);
        if (temp != NULL)
        {
            printk(KERN_DEBUG "\"%s\": \"%s\"\n", temp->key, temp->value);
        }
    }
    
    mutex_unlock(&dict->mutex);
    return 0;
}

//Free function
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

//Count function
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