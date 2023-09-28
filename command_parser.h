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
/// @param dict Pointer to the dictionary object 
/// @param commands The string containing the commands
/// @param length Length of the string containing the commands
/// @param timeout msecs reads will wait for keys that have not been created yet
/// @param allow_multiple_commands Allow or not multiple commands to be executed
/// @returns number of commands executed if allow_multple_commands is true, 
/// below zero for errors zero for success of the first and only command where allow_multiple_commands is false
int parse_command(pdictionary dict, const char __user *commands, size_t length, uint timeout, bool allow_multiple_commands);

#endif