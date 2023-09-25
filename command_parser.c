#include "module.h"

//Prototypes used and explicitated later

struct indices_t
{
    size_t key_start, key_length, value_start, value_length;
};
static bool parse_key_and_value(const char __user*, size_t, struct indices_t*);
static bool parse_key(const char __user*, size_t, struct indices_t*);

/**************************************************************************************
 * 
 *                   Functions that will be called by the command parser
 *                        each of them represents a command
 * 
***************************************************************************************/

static int function_write(pdictionary dict, const char* __user keyAndValue, size_t length, uint)
{
    struct indices_t indices;

    if (!parse_key_and_value(keyAndValue, length, &indices))
    {
        return (-1);//Bad format
    }
    printd("Executing write with params \"%s\"\n\tfirst %d of \"%s\"\n\tfirst %d of \"%s\"\n", 
        keyAndValue, 
        (int)(indices.key_length),  
        &keyAndValue[indices.key_start], 
        (int)(indices.value_length),
        &keyAndValue[indices.value_start]);
    return dictionary_write(dict, 
        &keyAndValue[indices.key_start], indices.key_length, 
        &keyAndValue[indices.value_start], indices.value_length);
}
static int function_append(pdictionary dict, const char __user* keyAndValue, size_t length, uint)
{
    struct indices_t indices;

    if (!parse_key_and_value(keyAndValue, length, &indices))
    {
        return (-1);//Bad format
    }
    printd("Executing append with params \"%s\"\n first %d of \"%s\"\n first %d of \"%s\"\n", 
        keyAndValue, 
        (int)(indices.key_length),  
        &keyAndValue[indices.key_start], 
        (int)(indices.value_length),
        &keyAndValue[indices.value_start]);
    return dictionary_append(dict, 
        &keyAndValue[indices.key_start], indices.key_length, 
        &keyAndValue[indices.value_start], indices.value_length);
}
static int function_print(pdictionary dict, const char __user* keyAndValue, size_t length, uint timeout)
{
    struct indices_t indices;

    if (!parse_key(keyAndValue, length, &indices))
    {
        return -EINVAL;//Bad format
    }
    return dictionary_print_key(dict, &keyAndValue[indices.key_start], indices.key_length, timeout);
}
static int function_delete(pdictionary dict, const char __user *keyAndValue, size_t length, uint)
{
    struct indices_t indices;

    if (!parse_key(keyAndValue, length, &indices))
    {
        return (-1);//Bad format
    }
    return dictionary_delete_key(dict, &keyAndValue[indices.key_start], indices.key_length);
}
static int function_delete_all(pdictionary dict, const char*, size_t, uint)
{
    int res;

    printd("Delete all\n");
    res = dictionary_free(dict);
    return res;
}
static int function_count(pdictionary dict, const char __user*, size_t, uint)
{
    int count;

    count = (int)dictionary_count(dict);
    printk(KERN_INFO "Keys in dictionary: %d\n", count);
    if (count == 0)
        return 1;
    return 0;
}
static int function_is_empty(pdictionary dict, const char __user*, size_t, uint)
{
    if (dictionary_empty(dict))
    {
        printk(KERN_INFO "Dictionary is empty.\n");
    } else {
        printk(KERN_INFO "Dictionary is not empty.\n");
    }
    return 0;
}
static int function_lock(pdictionary dict, const char __user*, size_t, uint)
{
    if (dictionary_is_locked(dict))
    {
        printk(KERN_ALERT "Dictionary is already locked, we will wait until it unlocks...\n");
    }
    if (dictionary_lock(dict))
    {
        printd("Dictionary is now locked.\n");
        return 0;
    }
    printk(KERN_ERR "Dictionary couldn't be locked!\n");
    return 1;
}
static int function_unlock(pdictionary dict, const char __user*, size_t, uint)
{
    if (dictionary_is_locked(dict))
    {
        dictionary_unlock(dict);
        printd("Dictionary is now unlocked.\n");
    } else {
        printd("Dictionary was not locked, no need to unlock it\n");
    }
    return 0;
}
static int function_is_locked(pdictionary dict, const char __user*, size_t, uint)
{
    if (dictionary_is_locked(dict))
    {
        printd("Dictionary is locked.\n");
    } else {
        printd("Dictionary is not locked.\n");
    }
    return 0;
}
 
/**************************************************************************************
 * 
 *                                Command parser body
 * 
***************************************************************************************/

//Basic object declarations

typedef int(*command_function)(pdictionary, const char __user *, size_t, uint);
#define skip_spaces(command, i, length) \
    while (i < length && (command[i] == ' ' || command[i] == '\t') && command[i] != '\0') \
    { \
        ++i; \
    } \
    if (i >= length) \
        return

//Actual functions

static ssize_t next_command_start(const char __user *commands, size_t length, ssize_t curr_index)
{
    for ( ; curr_index < (ssize_t)length && commands[curr_index] != COMMAND_SEPARATOR && commands[curr_index] != '\0'; ++curr_index)
    {
        //Do nothing
    }
    if ((size_t)curr_index < length)
    {
        //We hit the separator
        return curr_index;
    }
    return (-1);
}

static bool parse_key_and_value(const char __user *str, size_t length, struct indices_t* indices)
{
    size_t index = 1;

    printd("Parsing key and value of \"%s\" (%d)\n", str, (int)length);
    memset(indices, 0, sizeof(struct indices_t));
    //search for key
    if (str[0] != '<' || length == 0)
    {
        printk(KERN_ALERT "Bad first character or length == 0\n");
        return false;
    }
    indices->key_start = 1;
    while (index < length && str[index] != '\0' && str[index] != '>')
    {
        ++index;
    }
    if (index >= length || str[index] == '\0')
    {
        printk(KERN_ALERT "Closing '>' of key not found\n");
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
static bool parse_key(const char __user *str, size_t length, struct indices_t* indices)
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
  
static void print_commands_format(void)
{
    printk(
        "*****************************************************************\n"
        "                  Format of the commands:\n"                           
        "# Command separator character: '%c'\n"                              
        "# Write to key \"-%c <KEY_HERE> VALUE_HERE\"\n"                     
        "   # If the key is not present it is created\n"                     
        "   # The key is searched inside <> and the value is\n"              
        "     interpreted since the first non space character after >\n"     
        "   # Wrtinig an empty string to a key deletes it\n"                 
        "   # Creating a new key wakes all the tasks that are waiting\n"     
        "     for a non-existing key\n", 
        COMMAND_SEPARATOR, 
        COMMAND_WRITE);
    printk(                                   
        "# Append to key \"-%c <KEY_HERE> VALUE_HERE\"\n"                    
        "   # If the key is not present it is created but does not wake\n"   
        "     tasks in the waitqueue\n"                                      
        "   # Same format as Write\n"                                        
        "   # Wrtinig an empty string to a key doeas nothing\n"              
        "# Delete key \"-%c KEY_HERE\"\n"                                    
        "   # The key is interpreted since the first non space character\n" 
        "# Delete all keys \"-%c\"\n"                                        
        "   # No parameters to this command\n", 
        COMMAND_APPEND, 
        COMMAND_DELETE, 
        COMMAND_DELETE_ALL);
    printk(                                                               
        "# Print key \"-%c KEY_HERE\" or \"-%c KEY_HERE\"\n"                 
        "   # Prints the key, if present. If not waits until its created\n"  
        "# Count keys \"-%c\"\n"                                             
        "# Is empty? \"-%c\"\n", 
        COMMAND_PRINT, 
        COMMAND_READ, 
        COMMAND_COUNT, 
        COMMAND_EMPTY);
    printk(                                                                  
        "# Lock \"-%c\"\n"                                                   
        "   # Locks the dictionary, no call will be able to\n"               
        "     read/write on it until Unlock is called\n"                     
        "# Unlock \"-%c\"\n"                                                 
        "# Is locked? \"-%c\"\n"                                             
        "# Print commands format \"-%c\"\n"                                  
        "*****************************************************************\n", 
        COMMAND_LOCK, 
        COMMAND_UNLOCK, 
        COMMAND_IS_LOCKED, 
        COMMAND_INFO);
}

static bool execute_single_command(pdictionary dict, const char __user *command, size_t length, uint timeout)
{
    size_t i = 0;
    bool need_for_parameters = false;
    command_function f = NULL;
    int function_output = 0;

    skip_spaces(command, i, length) false;
    //Skipped start spaces
    if (command[i] != '-')
        return false;//Bad format
    ++i;
    if (i == length)
        return false;//Bad format: "  -"
    printd("Character of command: '%c'\n", command[i]);
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
        case COMMAND_LOCK:
            f = function_lock;
            break;
        case COMMAND_UNLOCK:
            f = function_unlock;
            break;
        case COMMAND_IS_LOCKED:
            f = function_is_locked;
            break;
        case COMMAND_INFO:
            print_commands_format();
            return true;
        default: 
            printk(KERN_ERR 
                "Unrecognized command character '%c', from: \"%s\"\n"
                "If you need info about the command format send command -%c.\n", 
                command[i], command,
                COMMAND_INFO);
            return false;
    }
    if (need_for_parameters && i + 1 < length)
    {
        ++i;
        skip_spaces(command, i, length) false;
    }
    function_output = f(dict, &command[i], length - i, timeout);
    if (function_output != 0)
    {
        printk(KERN_ERR "Command reported error %d while executing\n", function_output);
        switch (function_output)
        {
            case -EFAULT:
                printk(KERN_ERR "Detected error: EFAULT.\n");
                break;
            case -EINVAL:
                printk(KERN_ERR "Detected error: EINVAL.\n");
                break;
            case -EAGAIN:
                printk(KERN_ERR "Detected error: EAGAIN.\n");
                break;
        }
        
        return false;
    }
    return true;
}


int parse_command(pdictionary dict, const char __user *commands, size_t length, uint timeout)
{
    ssize_t command_start = 0, command_end = 0;
    int count = 0;

    if (commands == NULL || length == 0)
        return -EINVAL;
    printd("parse command of \"%s\" (%d)", commands, (int)length);
    do
    {
        command_end = next_command_start(commands, length, command_start);
        if (command_end < 0)
        {
            //Last command
            printd("Last command: \"%s\" (%d)\n", &commands[command_start], (int)(length - command_start));
            if (execute_single_command(dict, &commands[command_start], length - command_start, timeout))
                ++count;
            return count;
        } else {
            printd("Command: \"%s\" (%d)\n of many", &commands[command_start], (int)(command_end - command_start));
            //There are other commands next
            if (execute_single_command(dict, &commands[command_start], command_end - command_start, timeout))
                ++count;
            command_start = command_end + 1;//+1 to skip the COMMAND_SEPARATOR character
        }

    }
    while(command_end < length);
    return count;
}