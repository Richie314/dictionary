#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include "module.h"

MODULE_AUTHOR("Riccardo Ciucci <riccardo@richie314.it>");
MODULE_DESCRIPTION("Implementation of static dictionary controlled by a device file");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

//Module params

//Prints a lot of unnecessary data if true
bool debug = false;

//Executes a bunch of tests when the module is loaded
bool tests = false;

//Max timeout that read/print will wait for keys, if 0 they will wait until killed
static uint timeout = 0;

//Device filename, when loaded
#define DEVICE_FILE_NAME "dictionary"

//The dictionary the module operates upon
static dictionary_wrapper dictionary;

static int misc_device_open(struct inode *inode, struct file *file)
{
    printd("misc device (" DEVICE_FILE_NAME ") file opened.\n");
    return 0;
}

static int misc_device_close(struct inode *inode, struct file *file)
{
    printd("misc device (" DEVICE_FILE_NAME ") file closed.\n");
    return 0;
}

static ssize_t misc_device_read(struct file *file, char __user *buffer, size_t len, loff_t *ppos)
{
    ssize_t res;
    
    if (buffer == NULL || len == 0 || ppos == NULL)
    {
        printk(KERN_ERR "misc_device_read failed because of bad output buffer.\n");
        return -EINVAL;
    }
    if (*ppos > 0)
    {
        return 0;
    }
    res = dictionary_read_all(&dictionary, buffer, len, ppos);
    if (res < 0)
    {
        printk(KERN_ERR "dictionary_read_all failed.\n");
        return res;
    }
    printd("Bytes read: %d\n", (int)res);
    return res;
}

static ssize_t misc_device_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int res;
    
    if (buffer == NULL || count == 0)
    {
        printk(KERN_ERR "misc_device_write failed because of NULL input.\n");
        return -EINVAL;
    } 
    res = parse_command(&dictionary, buffer, count, timeout);

    if (res == 0)
    {
        return -EFAULT;
    }
    if (res < 0)
    {
        printk(KERN_ERR "Internal error of code %d.\n", res);
        return -EFAULT;
    }
    printk(KERN_DEBUG "Executed %d commands.\n", res);
    return count;
}

static struct file_operations dictionary_fops = {
    .owner =        THIS_MODULE,
    .read =         misc_device_read,
    .open =         misc_device_open,
    .release =      misc_device_close,
    .write =        misc_device_write,
    .llseek         = no_llseek
};

static struct miscdevice dictionary_device = {
    MISC_DYNAMIC_MINOR, DEVICE_FILE_NAME, &dictionary_fops
};

static int dictionary_module_init(void)
{
    int res;

    res = misc_register(&dictionary_device);
    printd("Misc Register returned %d\n", res);

    res = dictionary_init(&dictionary);
    if (res != 0)
    {
        printk(KERN_ALERT "dictionary_init failed! (code: %d)\n", res);
        return 0;
    }

    printd("Debug activated.\n");
    if (timeout != 0)
    {
        printk(KERN_INFO "Default timeout set to %d msecs.\n", (int)timeout);
    } else {
        printk(KERN_INFO "No timeout set. Reads will wait for missing keys indefinitely (or until they are killed).\n");
    }
    if (tests)
    {
        res = test_dictionary(&dictionary, timeout);
        if (res == 0)
        {
            printk(KERN_INFO "test_dictionary: all tests succeded!\n");
        } else {
            printk(KERN_ALERT "test_dictionary: %d tests failed\n", res);
        }
    }
    printk(KERN_INFO "dictionary: write \"-h\" to the device file to see the list of commands.\n");
    return 0;
}

static void dictionary_module_exit(void)
{
    int res;

    res = dictionary_free(&dictionary);
    if (res != 0)
    {
        printk(KERN_ALERT 
            "dictionary_free failed with exit code of %d.\n"
            "This could mean that the mutex was locked and it was impossible to unlock!\n", res);
    }
    misc_deregister(&dictionary_device);
    printd("Module " DEVICE_FILE_NAME " removed.\n");
}

module_init(dictionary_module_init);
module_exit(dictionary_module_exit);
module_param(debug, bool, 0);
module_param(tests, bool, 0);
module_param(timeout, uint, 0);