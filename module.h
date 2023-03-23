#ifndef _MODULE_H
#define _MODULE_H
#include "command_parser.h"
extern bool debug;
//#define printd(fmt, ...) if (debug) { printk(KERN_INFO fmt, ## __VA_ARGS__); printk(KERN_DEBUG fmt, ## __VA_ARGS__);}
#define printd(fmt, ...) if (debug) { printk(KERN_DEBUG fmt, ## __VA_ARGS__); }
#endif