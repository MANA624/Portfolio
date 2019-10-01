#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <asm/paravirt.h>
#include <linux/string.h>


MODULE_AUTHOR("Matt Niemiec");
MODULE_LICENSE("GPL");

#define BUFFER_SIZE 1024
#define DEVICE_NAME "detect"
#define MAJOR_NUMBER 262
#define TABLE_SIZE 326

unsigned long **sys_call_table;
static long *recorded_addresses[TABLE_SIZE];


static long my_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
    int ret;
    int diffs[TABLE_SIZE];
    int num_diffs = 0;
    int i;
    int was_mismatch = 0;
    char say[4*TABLE_SIZE] = "";
    char string_number[4];

    for(i=0; i<TABLE_SIZE; i++){
        if(recorded_addresses[i] != (void *)sys_call_table[i]){
            sprintf(string_number, "%d", i);
            printk(KERN_ALERT "Printfing i: %s\n", string_number);
            strcat(say, string_number);
            strcat(say, ",");

            was_mismatch = 1;
        }
    }

    if(!was_mismatch){
        strcpy(say, "No intercepted calls");
    }

    ret = copy_to_user(buffer, say, strlen(say));

    return 0;
}

int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
    // Print to the log file that the device is opened

    printk(KERN_ALERT "Device opened");

    return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
    /* print to the log file that the device is closed and also print the number of times this device has been closed until now*/
    printk(KERN_ALERT "Device closed");

    return 0;
}

struct file_operations simple_char_driver_file_operations = {
        .owner = THIS_MODULE,
        .read = my_read,
        .open = simple_char_driver_open,
        .release = simple_char_driver_close
};

// Find the address of the system call table to offset from
static unsigned long **aquire_sys_call_table(void)
{
    // PAGE_OFFSET is where kernel memory is designed to start
    unsigned long int offset = PAGE_OFFSET;
    unsigned long **sct;

    while (offset < ULLONG_MAX) {
        sct = (unsigned long **)offset;

        if (sct[__NR_close] == (unsigned long *) sys_close)
            return sct;

        offset += sizeof(void *);
    }

    return NULL;
}

static int simple_char_driver_init(void)
{
    int i;

    // We must first find where in the sys_call table the original system call is located
    if(!(sys_call_table = aquire_sys_call_table())) {
        return -1;
    }

    for(i=0; i<TABLE_SIZE; i++){
        recorded_addresses[i] = (void *)sys_call_table[i];
    }

    printk(KERN_ALERT "The size of the table is %d\n", __NR_close);

    // Register the device
    register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &simple_char_driver_file_operations);

    return 0;
}

static void simple_char_driver_exit(void)
{
    printk(KERN_ALERT "Removing device\n");

    // Unregister the device
    unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);

}

module_init(simple_char_driver_init);
module_exit(simple_char_driver_exit);
