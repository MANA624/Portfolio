#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/pid_namespace.h>

#include "tester.h"

MODULE_AUTHOR("Matt Niemiec");
MODULE_LICENSE("GPL");

#define BUFFER_SIZE 1024
#define DEVICE_NAME "procDriver"
#define MAJOR_NUMBER 261


static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    int cmd_sign = (int)cmd;
    int pid;
    struct procinfo info;
    struct list_head *curr_node;
    int num_siblings = 0;

    struct pid *pid_struct;
    struct task_struct *task;

    printk(KERN_ALERT "PID: %d\n", cmd_sign);

    if(cmd_sign < 0){
        // PID of current process
        pid = current->pid;

        // Get PID of process's parent
        pid_struct = find_get_pid(pid);
        if(pid_struct == NULL){
            return -EACCES;
        }
        task = pid_task(pid_struct,PIDTYPE_PID);
        pid = task->real_parent->pid;
    }
    else if(cmd_sign == 0)
    {
        pid = current->pid;
    }
    else{
        pid = cmd_sign;
    }

    printk(KERN_ALERT "The pid is %d\n", pid);

    pid_struct = find_get_pid(pid);
    if(pid_struct == NULL){
        return -EACCES;
    }

    task = pid_task(pid_struct,PIDTYPE_PID);
    list_for_each(curr_node, &(task->sibling)) {
        num_siblings++;
    }
    num_siblings--;
    info.pid = task->pid;
    info.ppid = task->real_parent->pid;
    info.start_time.tv_sec = task->start_time / 1000000000;
    info.start_time.tv_nsec = task->start_time % 1000000000;
    info.num_sib = num_siblings;


    if(copy_to_user((struct procinfo *)arg, &info, sizeof(struct procinfo))){
        return -EACCES;
    }

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
    .unlocked_ioctl = my_ioctl,
    .open = simple_char_driver_open,
    .release = simple_char_driver_close
};

static int simple_char_driver_init(void)
{
    // Print to the log file that the init function is called
    printk(KERN_ALERT "Initializing device\n");

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
