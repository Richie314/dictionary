KERNEL_DIR ?= /lib/modules/`uname -r`/build

obj-m = dictionary_module.o
dictionary_module-objs = module.o dictionary.o command_parser.o test.o

all:
	make -C $(KERNEL_DIR) M=`pwd` modules

clean:
	make -C $(KERNEL_DIR) M=`pwd` clean
