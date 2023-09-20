KERNEL_SOURCE_PATH=/home/richie/kernel_source # Path to the linux distro (must be compiled)
MODULE_OUTPUT_DIR=/home/richie/dictionary     # Path to the repo
VM_SHARED_DIR=/home/richie/qemu/autoload      # Path to a folder accessible from your VM (QEMU in this case) to safely test the module

make -C $KERNEL_SOURCE_PATH M=$MODULE_OUTPUT_DIR -B
cp $MODULE_OUTPUT_DIR/dictionary_module.ko $VM_SHARED_DIR/dictionary.ko
cp $MODULE_OUTPUT_DIR/scripts/inside-vm.sh $VM_SHARED_DIR/dictionary.sh