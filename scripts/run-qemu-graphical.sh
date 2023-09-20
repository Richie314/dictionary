KERNEL_SOURCE_PATH=/home/richie/kernel_source # Path to the linux distro (must be compiled)
INIT_DIR_PATH=/home/richie/qemu/initrd.gz     # Path to the initial ram disk
AUTOLOAD_DIR_PATH=/home/richie/qemu/autoload  # Path to the directory qemu will load automatically

qemu-system-x86_64 -kernel $KERNEL_SOURCE_PATH/arch/x86_64/boot/bzImage \
    -initrd $INIT_DIR_PATH \
    -virtfs local,path=$AUTOLOAD_DIR_PATH,mount_tag=host0,security_model=passthrough,id=host0 