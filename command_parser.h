#ifndef _COMMAND_PARSER_H
#define _COMMAND_PARSER_H

#include "dictionary.h"

#define COMMAND_SEPARATOR '|'
#define COMMAND_INFO 'h'

#define COMMAND_WRITE 'w'
#define COMMAND_APPEND 'a'

#define COMMAND_DELETE 'd'
#define COMMAND_DELETE_ALL 'f'

#define COMMAND_READ 'r'
#define COMMAND_PRINT 'p'

#define COMMAND_COUNT 'c'
#define COMMAND_EMPTY 'e'

#define COMMAND_LOCK 'l'
#define COMMAND_UNLOCK 'u'
#define COMMAND_IS_LOCKED 'i'

/// @brief Parses a list of commands and executes them
/// @param dict pointer to the dictionary object 
/// @param commands the string containing the commands
/// @param length length of the string containing the commands
/// @param timeout msecs reads will wait for keys that have not been created yet
/// @returns number of commands executed
int parse_command(pdictionary dict, const char __user *commands, size_t length, uint timeout);

#endif