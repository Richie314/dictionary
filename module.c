#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include "dictionary.h"

MODULE_AUTHOR("Riccardo Ciucci");
MODULE_DESCRIPTION("Implementation of static dictionary controlled by a device file");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static dictionary_wrapper dictionary;

static int my_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int my_close(struct inode *inode, struct file *file)
{
    return 0;
}

ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    int res;
    const char* key;

    key = (const char*)file->f_path.dentry->d_name.name;
    if (key == NULL)
    {
        printk(KERN_ERR "Invalid call! could not retrieve the key from the file name.\n");
        return -EFAULT;
    }
    res = dictionary_read(&dictionary, key, buf, len);
    if (res)
    {
        printk(KERN_ERR "dictionary_read failed with exit code of %d.\n", res);
        if (res == 1)
        {
            printk(
                KERN_DEBUG 
                "\tThis value could mean that the key \"%s\" was not found and was impossible to wait for its creation.\n"
                "\tAnother possibility is that an invalid buffer was given.\n", key);
        } else {
            printk(
                KERN_DEBUG 
                "\tThis value could mean that %u bytes couldn't be copied from the node to the buffer.\n", (unsigned int)res);
        }
        return -EFAULT;
    }

    return res;
}

static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int err;
    const char* key;

    key = (const char*)file->f_path.dentry->d_name.name;
    if (key == NULL)
    {
        printk(KERN_ERR "Invalid call! could not retrieve the key from the file name.\n");
        return -EFAULT;
    }
    
    //printk(KERN_DEBUG "my_write(%p, \"%s\", %zu, %p)", (void*)file, buf, count, (void*)ppos);
    if (buf != NULL && count > 0)
    {
        err = dictionary_write(&dictionary, key, buf, count);
    } else {
        err = dictionary_delete_key(&dictionary, key);
    }

    if (err != 0)
    {
        printk(KERN_ERR "dictionary_write failed with exit code of %d.\n", err);
        if (err == 1)
        {
            printk(KERN_DEBUG "This value could mean that krealloc failed.\n");
        } else {
            printk(KERN_DEBUG "This value could mean that copy_from_user couldn't copy %u bytes.\n", (unsigned int)err);
        }
        return -EFAULT;
    }

    return count;
}

static struct file_operations dictionary_fops = {
    .owner =        THIS_MODULE,
    .read =         my_read,
    .open =         my_open,
    .release =      my_close,
    .write =        my_write,
};

static struct miscdevice test_device = {
    MISC_DYNAMIC_MINOR, "dictionary", &dictionary_fops
};


static int dictionary_module_init(void)
{
    int res;

    res = misc_register(&test_device);

    printk(KERN_INFO "Misc Register returned %d\n", res);
    res = dictionary_init(&dictionary);
    if (res == 0)
        printk(KERN_INFO "dictionary_init succeded!\n");
    else
        printk(KERN_ALERT "dictionary_init failed!\n");
  return 0;
}

static void dictionary_module_exit(void)
{
    int res;

    res = dictionary_free(&dictionary);
    if (res != 0)
    {
        printk(KERN_ALERT 
            "dictionry_free failed with exit code of %d.\n"
            "This could mean that the mutex was locked and it was impossible to unlock!\n", res);
    }
    misc_deregister(&test_device);
}

module_init(dictionary_module_init);
module_exit(dictionary_module_exit);
