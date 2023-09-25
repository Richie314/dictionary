#ifndef _MODULE_H
#define _MODULE_H

#include "command_parser.h"

extern bool debug;
extern bool tests;

#define printd(fmt, ...) if (debug) { printk(KERN_INFO "\t" fmt, ## __VA_ARGS__); }

/// @brief Executes a series of tests
/// @param dict The dictionary to test
/// @param timeout max amount of msecs to wait for eventual tasks waiting. If 0, tasks will wait until they are killed
/// @return the number of tests failed
int test_dictionary(pdictionary dict, uint timeout);

#endif