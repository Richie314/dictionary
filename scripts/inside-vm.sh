MODULE_POSITION=/root/modules/dictionary.ko

sudo /sbin/insmod $MODULE_POSITION debug=y tests=y timeout=10000
cd /dev
cat /dev/dictionary