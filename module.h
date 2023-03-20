#ifndef _MODULE_H
#define _MODULE_H
#include "command_parser.h"
extern bool debug;
#define printd(fmt, ...) if (debug) { printk(fmt, ## __VA_ARGS__);}
#endif