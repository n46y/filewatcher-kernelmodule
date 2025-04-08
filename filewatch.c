#include <linux/module.h> // always include
#include <linux/init.h> // include for the init and exit macros
#include <linux/fs.h> // include for chrdev
#include <linux/uaccess.h> // include for copy_to_user & copy_from_user
#include <linux/cdev.h> // include for chrdev
#include <linux/device.h> // include for chrdev
#include <linux/stat.h>
#include <linux/cred.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/err.h>
#include <linux/path.h>
#include <linux/namei.h>

#define CHRDEV_NAME "filewatch_chrdev"
#define MAX_PATHS 8
#define MAX_PATH_LEN 128

// watched file path + last modification
struct watch_path {
    char str_path[MAX_PATH_LEN];
    struct timespec64 last_mod;
    int used;
};

static struct watch_path watch_list[MAX_PATHS] = {0};

static int major; // major number for the character device
static struct class* cls; // IDK what class is for


static char buff[MAX_PATHS * 128]; // chrdev buffer


static int get_last_mod(const struct path* path, struct timespec64 *ts) {
    struct kstat stat;
    int ret;
    
    ret = vfs_getattr(path, &stat, STATX_MTIME, AT_STATX_FORCE_SYNC);
    if (ret) {
        pr_err("filewatch -> Failed to get file attributes: %d\n", ret);
        return -EIO;
    }

    *ts = stat.mtime;

    return 0;
}

static int add_path(char* path_str) {
    struct path path;
    struct timespec64 ts;
    int ret;
    int i;

    ret = kern_path(path_str, LOOKUP_FOLLOW, &path);
    if (ret) {
        pr_err("filewatch -> Failed to get path for %s: %d\n", path_str, ret);
        return -EIO;  // Input/output error
    }

    ret = get_last_mod(&path, &ts);
    if(ret) {
        return ret;
    }
    
    for(i = 0; i < (sizeof(watch_list) / sizeof(struct watch_path)); ++i) {
        if(!watch_list[i].used) {
            strncpy(watch_list[i].str_path, path_str, MAX_PATH_LEN);
            watch_list[i].last_mod = ts;
            watch_list[i].used = true;
            pr_info("filewatch -> ADDED PATH: %s\n", watch_list[i].str_path);
            break;
        }
    }

    if (i >= (sizeof(watch_list) / sizeof(struct watch_path))) {
        pr_err("filewatch -> Watch list full\n");
        return -ENOSPC;
    }

    return i;
}


// read function for the fops struct (kernel -> user)
static ssize_t hello_read (struct file *file, char __user *user, size_t size, loff_t *offset) {
    struct timespec64 ts;
    struct path path;
    int not_copied, delta, to_copy = (size + *offset) < sizeof(buff) ? size : (sizeof(buff) - *offset);
    int i;
    int o;
    int ret;

    pr_info("filewatch -> Write was called on the chardev\n");
    pr_info("filewatch -> Want to write: %ld, copying: %d, offset: %lld\n", size, to_copy, *offset);



    o = 0;
    for(i = 0; i < (sizeof(watch_list) / sizeof(struct watch_path)); ++i) {
        if (watch_list[i].used) {
            ret = kern_path(watch_list[i].str_path, LOOKUP_FOLLOW, &path);
            if (ret) {
                pr_err("filewatch -> Failed to get path for %s: %d\n", watch_list[i].str_path, ret);
                return 0;  // Input/output error
            }

            ret = get_last_mod(&path, &ts);
            if(ts.tv_sec == watch_list[i].last_mod.tv_sec) {
                o += snprintf(buff + o, sizeof(buff) - o, "%s: no change since last time\n", watch_list[i].str_path);
            } else {
                o += snprintf(buff + o, sizeof(buff) - o, "%s: MODIFIED!!!\n", watch_list[i].str_path);
                watch_list[i].last_mod = ts;
            }
        }    }

    if (*offset > sizeof(buff)) 
        return 0;

    not_copied = copy_to_user(user, buff, to_copy);

    delta = to_copy - not_copied;
    if(not_copied)
        pr_warn("filewatch -> could only write %d bytes\n", delta);

    *offset += delta;

    return delta;
}

// write function for the fops struct (user -> kernel)
static ssize_t hello_write (struct file *file, const char __user *user, size_t size, loff_t *offset){
    int not_copied, delta, to_copy = (size + *offset) < sizeof(buff) ? size : (sizeof(buff) - *offset);
    pr_info("filewatch -> Write was called on the chardev\n");
    pr_info("filewatch -> Want to write: %ld, copying: %d, offset: %lld\n", size, to_copy, *offset);

    if (*offset > sizeof(buff)) 
        return 0;

    not_copied = copy_from_user(&buff[*offset], user, to_copy);
    delta = to_copy - not_copied;
    if(not_copied)
        pr_warn("filewatch -> could only write %d bytes\n", delta);

    if(!strncmp(&buff[*offset], "ADD:", 4)) {
        pr_info("filewatch -> ADD method called\n");
        add_path(&buff[*offset + 4]);
    } else if (!strncmp(&buff[*offset], "RMV:", 4)) {
        pr_info("filewatch -> RMV method called\n");
    } else {
        pr_err("filewatch -> unknown method!\n");
    }

    *offset += delta;

    return delta;
}

// open function for the fops struct
static int hello_open (struct inode *inode, struct file *file) {
    pr_info("filewatch -> open chrdev: major: %d, minor: %d\n", imajor(inode), iminor(inode));
    try_module_get(THIS_MODULE);
    return 0;
}

// release function for the fops struct
static int hello_release (struct inode *, struct file *) {
    pr_info("filewatch -> release chrdev\n");
    module_put(THIS_MODULE);
    return 0;
}

// here you can specify your chrdev's supported file operations
// https://elixir.bootlin.com/linux/v6.13.7/source/include/linux/fs.h#L2071
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = hello_write,
    .read = hello_read,
    .open = hello_open,
    .release = hello_release
}; 

// init callback
static int __init hello_init(void) {
    major = register_chrdev(0, CHRDEV_NAME, &fops);
    if(major < 0) {
        pr_err("filewatch -> unable to register major number QwQ\n");
        return major;

    }

    cls = class_create(CHRDEV_NAME); // IDK what class is for
    if (IS_ERR(cls)) {
        pr_err("Failed to create class\n");
        unregister_chrdev(major, CHRDEV_NAME);
        return PTR_ERR(cls);
    }

    // here we create a "file" for the chrdev in /dev
    if(!device_create(cls, NULL, MKDEV(major, 0), NULL, CHRDEV_NAME)) {
        pr_err("Failed to create device node\n");
        class_destroy(cls);
        unregister_chrdev(major, CHRDEV_NAME);
        return -ENODEV;
    };

    pr_err("filewatch -> module init :3 major dev number: %d\n", major);
    return 0;
}

// exit callback
static void __exit hello_exit(void) {
    device_destroy(cls, MKDEV(major, 0));
    class_destroy(cls);
    unregister_chrdev(major, CHRDEV_NAME);
    pr_info("filewatch -> module exit QwQ\n");
}

// specifying the init and exit functions
module_init(hello_init);
module_exit(hello_exit);

// providing a module license is mandatory
MODULE_LICENSE("GPL");



