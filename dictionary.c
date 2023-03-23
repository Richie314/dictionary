#include <linux/slab.h>
#include <linux/poll.h>
#include "module.h"
//Useful functions 
static bool key_check(pnode node, const char *key, size_t key_length)
{
    if (strlen(node->key) != key_length)
        return false;
    return strncmp(node->key, key, key_length) == 0;
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
        if (key_check(*node_obj, key, key_length))
        {
            return pos;
        }
    }
    *node_obj = NULL;
    printd("\t\tKey <%s> (%d) missing at the moment.\n", key, (int)key_length);
    return (struct list_head*)NULL;
}
static pnode create_node_and_insert(pdictionary dict, const char* key, size_t key_length)
{
    pnode new_node;
    size_t ret;

    //Create the new element
    new_node = (pnode)kzalloc(sizeof(struct node), GFP_USER);
    if (new_node == NULL)
    {
        //Failed to allocate node
        return NULL;
    }
    //No need to call memset(new_node, 0, sizeof(struct node)) since we allocated with kzalloc
    INIT_LIST_HEAD(&new_node->list);
    if (key_length == 0)
    {
        key_length = strlen(key);
    }
    new_node->key = (char*)kzalloc(key_length + 1, GFP_USER);
    if (new_node->key == NULL)
    {
        //An error occured
        kfree(new_node);
        return NULL;
    }
    ret = copy_from_user(new_node->key, key, key_length);
    if (ret != 0)
    {
        //An error occured: copy_from_user failed
        if (ret != key_length || memcpy(new_node->key, key, key_length) != new_node->key)
        {
            kfree(new_node->key);
            kfree(new_node);
            return NULL;
        } 
        //A possible cause is that key was not a user space string
        //Error was fixed with memcpy
    }
    list_add(&new_node->list, &dict->key_value_list);
    return new_node;
}
static void delete_dict_entry(struct list_head* entry, pnode node_ptr)
{
    printd("Deleting item of key \"%s\" and value \"%s\"\n", node_ptr->key, node_ptr->value);
    kfree(node_ptr->key);
    kfree(node_ptr->value);
    list_del(entry);
}

static int update_node(pnode node, const char __user *str, size_t length)
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
    if (node == NULL)
        return 1;
    //if node->value is NULL krealloc behaves the same as kmalloc(length + 1, GFP_USER)
    node->value = (char*)krealloc(node->value, length + 1, GFP_USER);
    if (node->value == NULL)
        return 1;
    memset(node->value, 0, ksize(node->value));//The allocated block could be greater than what was requested
    res = copy_from_user(node->value, str, length);
    if (res != 0)
    {
        if (res < length)
        {
            //copy_from_user failed
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
static int append_node(pnode node, const char __user *str, size_t length)
{
    int res;
    size_t old_strlen;
    void* new_offSet;

    old_strlen = strlen(node->value);
    printd("Appennd_node on \"%s\": appending \"%s\" (%d) to \"%s\" (%d)\n", node->key, str, (int)length, node->value, (int)old_strlen);
    node->value = (char*)krealloc(node->value, old_strlen + length + 1, GFP_USER);
    if (node->value == NULL || ksize(node->value) < old_strlen + length + 1)
        return 1;
    new_offSet = &node->value[old_strlen];
    //Initialize new part to '\0'
    memset(new_offSet, 0, length + 1);
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
            //The error was fixed: memcpy succeded in copying str to the node
            res = 0;
        }
    }
    return res;
}
static bool dictionary_wait_for_key_callback(pdictionary dict, const char* key, size_t key_length, pnode *node_ptr)
{
    bool res;

    res = dictionary_lock(dict);
    if (res)
    {
        res = dictionary_find_node(dict, key, key_length, node_ptr) != NULL;
        dictionary_unlock(dict);
    }
    return res;
}
#define dictionary_wait_for_key(dict, key, key_lenght, node_ptr) \
    (wait_event_interruptible(dict->queue, dictionary_wait_for_key_callback(dict, key, key_length, &node_ptr)) == 0)
#define dictionary_wake_waiting(dict) wake_up_all(&dict->queue)
/*********************************************/
/*                                           */
/*          Header functions body            */
/*                                           */
/*********************************************/

//Init functon: the first one to be called
int dictionary_init(pdictionary dict)
{
    if (dict == NULL)
    {
        return 1;
    }
    mutex_init(&dict->mutex);
    init_waitqueue_head(&dict->queue);
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
    int res = 0;
    bool created_new = false;

    if (dict == NULL)
        return 1;
    if (!dictionary_lock(dict))
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
            //Trying to delete a non-existing key
            res = 1;
        }
    } else {
        if (node_ptr == NULL)
        {
            //Node needs to be created
            node_ptr = create_node_and_insert(dict, key, key_length);
            created_new = node_ptr != NULL;
        }
        //Values are assigned here
        res = update_node(node_ptr, str, str_len);
    }
    //End of the write operations
    ////////////////////////////////////////
    //Unlock the mutex here
    dictionary_unlock(dict);
    if (created_new)
    {
        dictionary_wake_waiting(dict);
    }
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
    if (!dictionary_lock(dict))
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
    dictionary_unlock(dict);
    return res;
}

//Read to buffer function
ssize_t dictionary_read(
    pdictionary dict, 
    const char* key, size_t key_length, 
    char __user *buffer, size_t maxsize, 
    loff_t *ppos)
{
    pnode node_ptr;
    ssize_t res;
    size_t value_length;

    if (dict == NULL || buffer == NULL || maxsize == 0)
        return -EINVAL;
    if (!dictionary_lock(dict))
    {
        return -EFAULT;
    }
    ////////////////////////////////////////
    //Mutex is locked from now on
    if (dictionary_find_node(dict, key, key_length, &node_ptr) == NULL)
    {
        //Key not created, wait here
        printd("Key not found in dictionary at the moment.\nTask will be set to UNINTERRUPIBLE and put in a waitqueue.\n");
        dictionary_unlock(dict);
        if (dictionary_wait_for_key(dict, key, key_length, node_ptr))
        {
            dictionary_lock(dict);
        } else {
            printk(KERN_ALERT "An error happened.\nTask was probably killed\n");
            return 1;
        }
    }
    value_length = strlen(node_ptr->value);
    res = simple_read_from_buffer(buffer, maxsize, ppos, node_ptr->value, value_length);
    //End of the read operations
    ////////////////////////////////////////
    //Unlock the mutex here
    dictionary_unlock(dict);
    return res;
}

//Read all keys to buffer function
ssize_t dictionary_read_all(pdictionary dict, char __user *buffer, size_t maxsize, loff_t *ppos)
{
    struct list_head *pos, *q;
    pnode temp;
    size_t node_key_size;
    size_t node_value_size;
    const char* print_helpers = "<>: \"\"\n";
    ssize_t index = 0;

    if (dict == NULL)
    {
        printk(KERN_ERR "dictionary_read_all: dictionary was NULL!\n");
        return -EINVAL;
    }
    if (buffer == NULL)
    {
        printk(KERN_ERR "dictionary_read_all: output buffer was NULL!\n");
        return -EINVAL;
    }
    
    if (!dictionary_lock(dict))
    {
        printk(KERN_ERR "dictionary_read_all: Couldn't unlock the mutex!\n");
        return -EFAULT;
    }
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        temp = list_entry(pos, struct node, list);
        if (temp == NULL)
        {
            printk(KERN_ALERT "NULL value inside list detected\n");  
            continue;
        }
        node_key_size = strlen(temp->key);
        node_value_size = strlen(temp->value);
        if (node_key_size + node_value_size+index >= maxsize)
        {
            printk(KERN_ALERT "Reaached buffer limit but more could be printed.\n");
            break;
        }
        
        if (copy_to_user(&buffer[index], print_helpers, 1) != 0)
        {
            printk(KERN_ERR "Couldn't copy char '<' to output buffer\n");
            break;
        }
        index++;

        if (copy_to_user(&buffer[index], temp->key, node_key_size) != 0)
        {
            printk(KERN_ERR "Couldn't copy key  \"%s\" (%d) to output buffer\n", temp->key, (int)node_key_size);
            break;
        }
        index += node_key_size;
            
        if (copy_to_user(&buffer[index], print_helpers + 1, 4) != 0)
        {
            break;
        }
        index += 4;

        if (copy_to_user(&buffer[index], temp->value, node_value_size) != 0)
        {
            printk(KERN_ERR "Couldn't copy value \"%s\" (%d) to output buffer\n", temp->value, (int)node_value_size);
            break;
        }
        index += node_value_size;

        if (copy_to_user(&buffer[index], print_helpers + 5, 2) != 0)
        {
            break;
        }
        index += 2;
        
        //printk(KERN_DEBUG "Copied key value <%s> to buffer\n", temp->key);
    }    
    dictionary_unlock(dict);
    (*ppos) += index;
    return index;
}

//Print key function
int dictionary_print_key(pdictionary dict, const char* key, size_t key_length)
{
    pnode node_ptr;
    int res;

    if (dict == NULL)
        return 1;
    if (!dictionary_lock(dict))
    {
        return 1;
    }
    ////////////////////////////////////////
    //Mutex is locked from now on
    if (dictionary_find_node(dict, key, key_length, &node_ptr) == NULL)
    {
        //Key not created, wait here
        printd("Key not found in dictionary at the moment.\nTask will be set to UNINTERRUPIBLE and put in a waitqueue.\n");
        dictionary_unlock(dict);
        if (dictionary_wait_for_key(dict, key, key_length, node_ptr))
        {
            dictionary_lock(dict);
        } else {
            printk(KERN_ALERT "An error happened.\nTask was probably killed\n");
            return 1;
        }
    }
    printk(KERN_INFO "<%s>: \"%s\"\n", node_ptr->key, node_ptr->value);
    //End of the read operations
    ////////////////////////////////////////
    //Unlock the mutex here
    dictionary_unlock(dict);
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
    if (!dictionary_lock(dict))
    {
        printk(KERN_ERR "dictionary_print_all: Couldn't unlock the mutex!\n");
        return 1;
    }
    
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        temp = list_entry(pos, struct node, list);
        if (temp != NULL)
        {
            printk(KERN_INFO "\t<%s>: \"%s\"\n", temp->key, temp->value);
            printk(KERN_DEBUG "\t<%s>: \"%s\"\n", temp->key, temp->value);
        }
    }
    
    dictionary_unlock(dict);
    return 0;
}

//Free function
int dictionary_free(pdictionary dict)
{
    struct list_head *pos, *q;
    pnode temp;

    if (dict == NULL)
        return 1;
    if (!dictionary_lock(dict))
    {
        return 1;
    }
    
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        temp = list_entry(pos, struct node, list);
        //Delete every entry
        delete_dict_entry(pos, temp);
    }

    dictionary_unlock(dict);
    return 0;
}

//Count function
size_t dictionary_count(pdictionary dict)
{
    struct list_head *pos, *q;
    pnode temp;
    size_t count;

    if (dict == NULL)
        return 0;
    if (!dictionary_lock(dict))
    {
        return 0;
    }
    
    list_for_each_safe(pos, q, &dict->key_value_list)
    {
        temp = list_entry(pos, struct node, list);
        if (temp != NULL)
            ++count;
    }

    dictionary_unlock(dict);
    return count;
}

bool dictionary_lock(pdictionary dict)
{
    if (mutex_lock_interruptible(&dict->mutex) == 0)
    {
        //We aquired the mutex or were interruped by a signal
        printd("mutext_lock_interruptible returned 0.\n");
        if (dictionary_is_locked(dict))
        {
            //We really aquired the mutex
            printd("Dictionary locked.\n");
            return true;
        }
        printd("mutext_lock_interruptible exeited with success but mutex was not locked.\n");
        return false;
        //We were interrupted by a signal, continue the iteration
    }
    printd("mutext_lock_interruptible failed.\n");
    return false;
}