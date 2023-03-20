#include "module.h"

static ssize_t next_command_start(const __user char* commands, size_t length, size_t curr_index)
{
    for ( ; curr_index < length && commands[curr_index] != COMMAND_SEPARATOR; curr_index++)
    {
        //Do nothing
    }
    if (curr_index < length)
    {
        //We hit the separator
        return (ssize_t) curr_index;
    }
    return (-1);
}

typedef int(*command_function)(pdictionary, const char* __user, size_t);
#define skip_spaces(command, i, length) \
    while (i < length && (command[i] == ' ' || command[i] == '\t')) \
    { \
        ++i; \
    } \
    if (i == length) \
        return
struct indices_t
{
    size_t key_start, key_length, value_start, value_length;
};
static bool parse_key_and_value(const char* __user str, size_t length, struct indices_t* indices)
{
    size_t index = 1;

    printd(KERN_DEBUG "Parsing key and value of \"%s\" (%d)\n", str, (int)length);
    memset(indices, 0, sizeof(struct indices_t));
    //search for key
    if (str[0] != '<' || length == 0)
    {
        printd(KERN_DEBUG "Bad first character or length == 0\n");
        return false;
    }
    indices->key_start = 1;
    while (index < length && str[index] != '\0' && str[index] != '>')
    {
        ++index;
    }
    if (index == length || str[index] == '\0')
    {
        printd(KERN_DEBUG "Closing '>' of key not found\n");
        return false;
    }
    indices->key_length = index - 1;
    ++index;

    //search for value (starts with first non space character after the key)
    skip_spaces(str, index, length) false;
    indices->value_start = index;
    indices->value_length = length - index;
    return true;
}
static bool parse_key(const char* __user str, size_t length, struct indices_t* indices)
{
    size_t index = 1;

    if (length == 0)
        return false;
    memset(indices, 0, sizeof(struct indices_t));
    if (str[0] == '<')
    {
        indices->key_start = index++;
        while (index < length && str[index] != '\0' && str[index] != '>')
        {
            ++index;
        }
        if (index == length)
            return false;
        indices->key_length = index - 1;
    } else {
        indices->key_start = 0;
        index = strlen(str);
        indices->key_length = min(length, index);
    }
    
    return true;
}

static int function_write(pdictionary dict, const char* __user keyAndValue, size_t length)
{
    struct indices_t indices;

    if (!parse_key_and_value(keyAndValue, length, &indices))
    {
        return (-1);//Bad format
    }
    printd(KERN_DEBUG "Executing write with params \"%s\"\n\tfirst %d of \"%s\"\n\tfirst %d of \"%s\"\n", 
        keyAndValue, 
        (int)(indices.key_length),  
        &keyAndValue[indices.key_start], 
        (int)(indices.value_length),
        &keyAndValue[indices.value_start]);
    return dictionary_append(dict, 
        &keyAndValue[indices.key_start], indices.key_length, 
        &keyAndValue[indices.value_start], indices.value_length);
}
static int function_append(pdictionary dict, const char __user* keyAndValue, size_t length)
{
    struct indices_t indices;

    if (!parse_key_and_value(keyAndValue, length, &indices))
    {
        return (-1);//Bad format
    }
    printd(KERN_DEBUG "Executing append with params \"%s\"\n first %d of \"%s\"\n first %d of \"%s\"\n", 
        keyAndValue, 
        (int)(indices.key_length),  
        &keyAndValue[indices.key_start], 
        (int)(indices.value_length),
        &keyAndValue[indices.value_start]);
    return dictionary_append(dict, 
        &keyAndValue[indices.key_start], indices.key_length, 
        &keyAndValue[indices.value_start], indices.value_length);
}
static int function_print(pdictionary dict, const char __user* keyAndValue, size_t length)
{
    struct indices_t indices;

    if (!parse_key(keyAndValue, length, &indices))
    {
        return (-1);//Bad format
    }
    return dictionary_print_key(dict, &keyAndValue[indices.key_start], indices.key_length);
}
static int function_delete(pdictionary dict, const char __user *keyAndValue, size_t length)
{
    struct indices_t indices;

    if (!parse_key(keyAndValue, length, &indices))
    {
        return (-1);//Bad format
    }
    return dictionary_delete_key(dict, &keyAndValue[indices.key_start], indices.key_length);
}
static int function_delete_all(pdictionary dict, const char*, size_t)
{
    int res;

    printd(KERN_DEBUG "delete all\n");
    res = dictionary_free(dict);
    return res;
}
static int function_count(pdictionary dict, const char __user*, size_t)
{
    int count;

    count = (int)dictionary_count(dict);
    printd(KERN_DEBUG "Keys in dictionary: %d\n", count);
    if (count == 0)
        return 1;
    return 0;
}
static int function_is_empty(pdictionary dict, const char __user*, size_t)
{
    if (dictionary_empty(dict))
    {
        printk(KERN_DEBUG "Dictionary is empty.\n");
    } else {
        printk(KERN_DEBUG "Dictionary is not empty.\n");
    }
    return 0;
}

static bool execute_single_command(pdictionary dict, const char* __user command, size_t length)
{
    size_t i = 0;
    bool need_for_parameters = false;
    command_function f = NULL;

    skip_spaces(command, i, length) false;
    //Skipped start spaces
    if (command[i] != '-')
        return false;//Bad format
    ++i;
    if (i == length)
        return false;//Bad format
    printd(KERN_DEBUG "character of command: '%c'\n", command[i]);
    switch (command[i])
    {
        case COMMAND_WRITE:
            f = function_write;
            need_for_parameters = true;
            break;
        case COMMAND_APPEND:
            f = function_append;
            need_for_parameters = true;
            break;
        case COMMAND_READ:
        case COMMAND_PRINT:
            f = function_print;
            need_for_parameters = true;
            break;
        case COMMAND_DELETE:
            f = function_delete;
            need_for_parameters = true;
            break;
        case COMMAND_DELETE_ALL:
            f = function_delete_all;
            break;
        case COMMAND_COUNT:
            f = function_count;
            break;
        case COMMAND_EMPTY:
            f = function_is_empty;
            break;
        default: 
            printk(KERN_ERR "Unrecognized command character '%c', from: \"%s\"\n", command[i], command);
            return false;
    }
    if (need_for_parameters && i + 1 < length)
    {
        ++i;
        skip_spaces(command, i, length) false;
    }
    if (f(dict, &command[i], length - i) != 0)
    {
        printk(KERN_ERR "Command reported error while executing.\n");
        return false;
    }
    return true;
}


int parse_command(pdictionary dict, const char* __user commands, size_t length)
{
    ssize_t prev_command = 0, next_command = 0;
    int count = 0;

    do
    {
        next_command = next_command_start(commands, length, next_command);
        if (next_command < 0)
        {
            //Last command
            if (execute_single_command(dict, &commands[prev_command], length - prev_command))
                ++count;
            return count;
        } else {
            //There are other commands next
            if (execute_single_command(dict, &commands[prev_command], next_command - prev_command))
                ++count;
            prev_command = next_command + 1;//+1 to skip the COMMAND_SEPARATOR character
        }

    }
    while(next_command < length && next_command >= 0);
    return count;
}