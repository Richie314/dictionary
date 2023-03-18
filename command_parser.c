#include "command_parser.h"

static ssize_t next_command_start(const char* commands, size_t length, size_t curr_index)
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

typedef int(*command_function)(pdictionary, const char*, size_t);
#define skip_spaces(command, i, length) \
    while (i < length && (command[i] == ' ' || command[i] == '\t')) \
    { \
        ++i; \
    } \
    if (i == length) \
        return
static bool parse_key_and_value(const char* str, size_t length, size_t indices[4])
{
    size_t index = 1;

    if (str[0] != '\"' || length == 0)
        return false;
    indices[0] = 1;
    while (index < length && str[index] != '\"')
    {
        ++index;
    }
    if (index == length)
        return false;
    indices[1] = index - 1;
    ++index;
    skip_spaces(str, index, length) false;
    if (str[index] == '\"')
    {
        //value is wrapped between ""
        indices[2] = ++index;
        while (index < length && str[index] != '\"')
        {
            ++index;
        }
        if (index == length)
            return false;
        indices[3] = index - indices[2];
    } else {
        indices[2] = index;
        indices[3] = length - index;
    }
    return true;
}
static bool parse_key(const char* str, size_t length, size_t indices[2])
{
    size_t index = 1;

    if (length == 0)
        return false;
    if (str[0] == '\"')
    {
        indices[0] = 1;
        while (index < length && str[index] != '\"')
        {
            ++index;
        }
        if (index == length)
            return false;
        indices[1] = index - 1;
    } else {
        indices[0] = 0;
        indices[1] = length;
    }
    
    return true;
}

static int function_write(pdictionary dict, const char* keyAndValue, size_t length)
{
    size_t indices[4] = { 0 };

    if (!parse_key_and_value(keyAndValue, length, indices))
    {
        return (-1);//Bad format
    }
    printk(KERN_DEBUG "Executing write with values --%s--\n%s\n%s\n%d, %d, %d, %d\n", 
        keyAndValue, 
        &keyAndValue[indices[0]], 
        &keyAndValue[indices[2]], 
        (int)indices[0], 
        (int)indices[1], 
        (int)indices[2], 
        (int)indices[3]);
    return dictionary_write(dict, 
        &keyAndValue[indices[0]], indices[1], 
        &keyAndValue[indices[2]], indices[3]);
}
static int function_append(pdictionary dict, const char* keyAndValue, size_t length)
{
    size_t indices[4] = { 0 };

    if (!parse_key_and_value(keyAndValue, length, indices))
    {
        return (-1);//Bad format
    }
    printk(KERN_DEBUG "Executing append with values --%s--\n%s\n%s\n%d, %d, %d, %d\n", 
        keyAndValue, 
        &keyAndValue[indices[0]], 
        &keyAndValue[indices[2]], 
        (int)indices[0], 
        (int)indices[1], 
        (int)indices[2], 
        (int)indices[3]);
    return dictionary_append(dict, 
        &keyAndValue[indices[0]], indices[1], 
        &keyAndValue[indices[2]], indices[3]);
}
static int function_print(pdictionary dict, const char* keyAndValue, size_t length)
{
    size_t indices[2] = { 0 };

    if (!parse_key(keyAndValue, length, indices))
    {
        return (-1);//Bad format
    }
    return dictionary_print_key(dict, &keyAndValue[indices[0]], indices[1]);
}
static int function_delete(pdictionary dict, const char* keyAndValue, size_t length)
{
    size_t indices[2] = { 0 };

    if (!parse_key(keyAndValue, length, indices))
    {
        return (-1);//Bad format
    }
    return dictionary_delete_key(dict, &keyAndValue[indices[0]], indices[1]);
}
static int function_delete_all(pdictionary dict, const char* keyAndValue, size_t length)
{
    return dictionary_free(dict);
}
static int function_count(pdictionary dict, const char* keyAndValue, size_t length)
{
    int count;

    count = (int)dictionary_count(dict);
    printk(KERN_DEBUG "Keys in dictionary: %d\n", count);
    return count;
}
static int function_is_empty(pdictionary dict, const char* keyAndValue, size_t length)
{
    if (dictionary_empty(dict))
    {
        printk(KERN_DEBUG "Dictionary is empty.\n");
    } else {
        printk(KERN_DEBUG "Dictionary is not empty.\n");
    }
    return 0;
}

static void execute_single_command(pdictionary dict, const char* command, size_t length)
{
    size_t i = 0;
    command_function f = NULL;

    skip_spaces(command, i, length);
    //Skipped start spaces
    if (command[i] != '-')
        return;//Bad format
    ++i;
    switch (command[i])
    {
        case COMMAND_WRITE:
            f = function_write;
            break;
        case COMMAND_APPEND:
            f = function_append;
            break;
        case COMMAND_READ:
        case COMMAND_PRINT:
            f = function_print;
            break;
        case COMMAND_DELETE:
            f = function_delete;
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
            printk(KERN_ERR "Unrecognized command character '%c', from: --%s--\n", command[i], command);
            return;
    }
    ++i;
    skip_spaces(command, i, length);
    f(dict, &command[i], length - i);
}


int parse_command(pdictionary dict, const char* commands, size_t length)
{
    ssize_t prev_command = 0, next_command = 0;
    int count = 0;

    do
    {
        next_command = next_command_start(commands, length, next_command);
        ++count;
        if (next_command < 0)
        {
            //Last command
            execute_single_command(dict, &commands[prev_command], length - prev_command);
            return count;
        } else {
            //There are other commands next
            execute_single_command(dict, &commands[prev_command], next_command - prev_command);
            prev_command = next_command + 1;//+1 to skip the COMMAND_SEPARATOR character
        }

    }
    while(next_command < length && next_command >= 0);
    return count;
}