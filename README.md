# dictionary
**Linux Misc Device File** that handles a dictionary 

The dictionary (with both keys and values as cstrings) is protected through a mutex that is locked before read/write operations and unlocked after. 

A read on the device results in a read of all key-value pairs present at that time. 

A Through a write operation is possible to send commands to print, delete write to and append to keys.

The _`help command (-h)`_ prints all the commands syntax in a detailed way. 

Read (print) commands that want to read a non existing key are put in a waitqueue until the wanted key is created.

The module has three params
- **debug**: if set to true (y) prints extended informations about the functions that are being called
- **tests**: if set to true (y) executes a bunch of tests on the start of the module, the dictionary will have content after the tests
- **timeout**: if set to non zero (zero is the default value) puts a limit to the amount of time a read/print task can be sleeping waiting for one key. If set to zero tasks will wait until they receive an interrupt signal that kills them or the key is created and the value is printed

How to load the module:
Just write _`sudo /sbin/insmod modules/dictionary.ko debug=y tests=y timeout=20000`_ in your terminal. This example will load the module and tell it to print debug info, execute tests on start and put a time limit of 20 seconds to the waiting tasks.

An exmple of command to send to the dictionary is: _`echo -n > dev/dictionary "-w <Key> Value"`_. This command tells the module to write _`Value`_ to the key _`Key`_

A complete cheatsheet is printed by the module with the command _`echo -n > dev/dictionary "-h"`_ 

A read operation _`cat dev/dictionary`_ results in an output of the type:

`<Key 1>: "Value 1"\n

<Key 2>: "Value 2"\n

...

<Key N>: "Value N"\n` 
