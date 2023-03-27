# dictionary
Linux Misc Device File that handles a dictionary
The dictionary (with both keys and values as cstrings) is protected through a mutex that is locked before read/write operations and unlocked after.
A read on the device results in a read of all key-value pairs present at that time.
A Through a write operation is possible to send commands to print, delete write to and append to keys.
The help command (-h) prints all the commands syntax in a detailed way.
Read (print) commands that want to read a non existing key are put in a waitqueue until the wanted key is created.

The module has three params
-debug: if set to true (y) prints extended informations about the functions that are being called
-tests: if set to true (y) executes a bunch of tests on the start of the module, the dictionary will have content after the tests
-timeout
