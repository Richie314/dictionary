#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include "module.h"

MODULE_AUTHOR("Riccardo Ciucci");
MODULE_DESCRIPTION("Implementation of static dictionary controlled by a device file");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
bool debug = false;
#define DEVICE_FILE_NAME "dictionary"

static dictionary_wrapper dictionary;

static int misc_device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int misc_device_close(struct inode *inode, struct file *file)
{
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
    res = dictionary_read_all(&dictionary, buffer, len);
    if (res < 0)
    {
        printk(KERN_ERR "dictionary_read_all failed.\n");
        return res;
    }
    *ppos += res;
    res = min(len, res);
    return res;
}

static ssize_t misc_device_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    int res;
    
    if (buffer == NULL || count == 0)
    {
        printk(KERN_ERR "misc_device_write failed because of NULL input.\n");
        return -EFAULT;
    } 
    res = parse_command(&dictionary, buffer, count);

    if (res == 0)
    {
        printk(KERN_ALERT "No command executed.\n");
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

#define d_write(key, str, res) \
    res = dictionary_write(&dictionary, key, strlen(key), str, strlen(str)); \
    if (res != 0) \
        printk(KERN_ERR "dictionary_write(\"%s\", \"%s\", %d) failed with code %d\n", key, str, (int)strlen(str), res)
static int dictionary_module_init(void)
{
    int res;

    res = misc_register(&dictionary_device);

    printd(KERN_INFO "Misc Register returned %d\n", res);
    res = dictionary_init(&dictionary);
    if (res != 0)
    {
        printk(KERN_ALERT "dictionary_init failed!\n");
        return 0;
    }
    
    d_write("Chiave 4", "Did you ever hear the tragedy of Darth Plagueis The Wise?", res);
    test_command(&dictionary, "-w <Chiave 2> Ciao ciao");
    test_command(&dictionary, "-a <Chiave 1>\t mondo");
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
}

module_init(dictionary_module_init);
module_exit(dictionary_module_exit);
module_param(debug, bool, 0);