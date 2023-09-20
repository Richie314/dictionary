# dictionary
**Linux Misc Device File** that handles a dictionary 

The dictionary (with both keys and values as cstrings) is protected through a mutex that is locked before read/write operations and unlocked after. 

A read on the device results in a read of all key-value pairs present at that time. 

A Through a write operation is possible to send commands to print, delete write to and append to keys.

The _**help command** `-h`_ prints all the commands syntax in a detailed way. 

Read (print) commands that want to read a non existing key are put in a waitqueue until the wanted key is created.

The module has three params
- **debug**: if set to true (y) prints extended informations about the functions that are being called
- **tests**: if set to true (y) executes a bunch of tests on the start of the module, the dictionary will have content after the tests
- **timeout**: if set to non zero (zero is the default value) puts a limit to the amount of time a read/print task can be sleeping waiting for one key. If set to zero tasks will wait until they receive an interrupt signal that kills them or the key is created and the value is printed

How to load the module:
Just write `sudo /sbin/insmod /root/modules/dictionary.ko debug=y tests=y timeout=20000` in your terminal. This example will load the module and tell it to print debug info, execute tests on start and put a time limit of 20 seconds to the waiting tasks.

An exmple of command to send to the dictionary is: `echo -n > /dev/dictionary "-w <Key> Value"`. This command tells the module to write _`Value`_ to the key _`Key`_

A complete cheatsheet is printed by the module with the command `echo -n > /dev/dictionary "-h"`

A read operation `cat /dev/dictionary` results in an output of the type:

`<Key 1>: "Value 1"\n`

`<Key 2>: "Value 2"\n`

`...`

`<Key N>: "Value N"\n` 

# Build and Install
Note: do not install this module inside your OS's kernel, use a VM instead.

0.  You will need a compiled Linux Kernel, if you don't have one you can download the source code from this <a href="https://www.kernel.org/">url</a> and build it
1.  First start by cloning the repository to get the source code `git clone https://github.com/Richie314/dictionary.git`
2.  Make sure you know the path of the created folder (the one that contains the source code of this project) and build with the command `make -C <absolute_path_to_kernel_source> M=<absolute_path_to_dictionary_source_folder> -B`, this should generate a `dictionary_module.ko` file inside your module source folder. In the scripts/compile.sh folder there is a bash script that handles this: you can replace the paths with the ones on your pc.
3.  Copy the generated file to a directory visible from your VM
4.  Inside your VM load the module through the command `sudo /sbin/insmod /root/modules/dictionary_module.ko tests=y timeout=10000`, to see the variuos parameters you can pass to the module see before in this file. Alternatively you can use the bash script included with the module this way: `. /root/modules/dictionary.sh`, this will load he module and execute some tests; after that it will automatically `cd /dev` and try to print all the dictionary.
5.  Now your VM should have a _/dev/dictionary_ file you can write to/read from
6.  To read the whole content of the dictionary `cat /dev/dictionary`
7.  To write to the dictionary `echo -n > /dev/dictionary "-w <KEY_HERE> VALUE_HERE"`
8.  To perform other operations you can type the help command `echo -n > /dev/dictionary "-h"`
