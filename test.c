#include "module.h"
#include <linux/slab.h>
#define increment_if_failed(res, expected, count, expr, ...) \
    if (res != expected) \
    { \
        ++count; \
        printk(KERN_ALERT expr, ## __VA_ARGS__); \
    }

#define failed_write(res, expected, count, key, value) \
    increment_if_failed(res, expected, count, "dictionary_write(\"%s\", \"%s\", %d) failed with code %d\n", key, value, (int)strlen(value), res)
#define test_write(dict, key, str, res, count, expected) \
    do { \
        res = dictionary_write(dict, key, strlen(key), str, strlen(str)); \
        if (strlen(key) > 0) \
        { \
            failed_write(res, expected, count, key, str); \
        } \
    } while(0)

#define failed_append(res, expected, count, key, value) \
    increment_if_failed(res, expected, count, "dictionary_append(\"%s\", \"%s\", %d) failed with code %d\n", key, value, (int)strlen(value), res)
#define test_append(dict, key, str, res, count, expected) \
    do { \
        res = dictionary_append(dict, key, strlen(key), str, strlen(str)); \
        if (strlen(key) > 0) \
        { \
            failed_append(res, expected, count, key, str); \
        } \
    } while(0)

    
#define failed_read(count, key, expected, got, expected_length, got_length) \
    increment_if_failed(1, 0, count, "dictionary_read(\"%s\") failed! Read \"%s\" (%d) instead of \"%s\" (%d).\n", key, got, got_length, expected, expected_length)
#define test_read(dict, key, buf, pos, expected, res, count, timeout) \
    do { \
        res = dictionary_read(dict, key, strlen(key), buf, sizeof(buf) - 1, timeout, &pos); \
        if (res != (int)strlen(expected) || strncmp(buf, expected, res) != 0) \
        { \
            failed_read(count, key, expected, buf, (int)strlen(expected), res); \
        } \
        memset(buf, 0, sizeof(buf)); \
        pos = 0; \
    } while(0)
#define test_count(dict, expected, res, count) \
    do { \
        res = (int)dictionary_count(dict) - expected; \
        increment_if_failed(res, 0, count, "dictionary_count() failed. Code: %d\n", res); \
    } while(0)
        
int test_dictionary(pdictionary dict, uint timeout)
{
    int res, count = 0;
    char readBuffer[128] = { 0 };
    loff_t pos = 0;

    //Testing dictionry_write
    printk(KERN_INFO 
        "-------------------------------------------------\n"
        "Tests: executing test on dictionary_write method.\n");
    //Test creation
    test_write(dict, "Chiave 1", "Valore 1", res, count, 0);
    //Test creation of length 0 (should not create)
    test_write(dict, "Chiave non creata", "", res, count, 1);
    //Test creation and delete
    test_write(dict, "Chiave 2", "Valore 2", res, count, 0);//Create here
    test_write(dict, "Chiave 2", "", res, count, 0);//Delete here

    test_write(dict, "Chiave 5", "Valore 5", res, count, 0);
    test_write(dict, "Lorem", "Ipsum", res, count, 0);

    //Testing dictionry_append
    printk(KERN_INFO 
        "--------------------------------------------------\n"
        "Tests: executing test on dictionary_append method.\n");
    //Test append to existing key
    test_append(dict, "Lorem", " dixit", res, count, 0);
    //Test append to non-existing key
    test_append(dict, "Language C", "was created by ", res, count, 0);
    //Test append to neo created key
    test_append(dict, "Language C", "Denis Ritchie", res, count, 0);
    //Test append nothing to an existing key: should return 1
    test_append(dict, "Language C", "", res, count, 1);
    //Test append nothing to a non-existing key: should return 1
    test_append(dict, "Language C++", "", res, count, 1);

    //Testing dictionary_read
    printk(KERN_INFO 
        "------------------------------------------------\n"
        "Tests: executing test on dictionary_read method.\n");
    test_read(dict, "Chiave 1", readBuffer, pos, "Valore 1", res, count, timeout);
    test_read(dict, "Chiave 5", readBuffer, pos, "Valore 5", res, count, timeout);
    test_read(dict, "Lorem", readBuffer, pos, "Ipsum dixit", res, count, timeout);

    //Test dictionary_count
    printk(KERN_INFO 
        "-------------------------------------------------\n"
        "Tests: executing test on dictionary_count method.\n");
    test_count(dict, 4, res, count);

    //Test dictionary_count
    printk(KERN_INFO 
        "-------------------------------------------------\n"
        "Tests: executing test on parse_command function.\n");
    parse_command(dict, "-w <Hello1> World1|-w <Hello2> World2", 39, 0, true);
    parse_command(dict, "-w <Hello3> World3|-w <Hello4> World4", 39, 0, false);

    return count;
}