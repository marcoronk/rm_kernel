#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/memory.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/slab.h>
#include <linux/cpu.h>

#define NETLINK_USER 31

static struct sock *nl_sock = NULL;
static int cpu_threshold = 80;
static int disk_threshold = 90;

static struct proc_dir_entry *proc_file;

static void send_alert(const char *msg) {
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    int msg_size = strlen(msg) + 1;
    int pid = 0; // Replace with user-space process PID if needed.
/*
    skb = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb)
        return;

    nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, msg_size, 0);
    strncpy(nlmsg_data(nlh), msg, msg_size);

    netlink_unicast(nl_sock, skb, pid, MSG_DONTWAIT);
    */
}

static int read_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data) {
    struct sysinfo si;
    struct kstatfs fs;
    struct file_system_type *fstype;
    struct path root_path;
    char *output;
    int len;

    output = kmalloc(1024, GFP_KERNEL);
    if (!output)
        return -ENOMEM;

    si_meminfo(&si);
    len = sprintf(output, "kernel_memory=%lu\n", si.totalram - si.freeram);

    fstype = get_fs_type("ext4"); // Change filesystem type as needed
    if (!fstype) {
        kfree(output);
        return -EINVAL;
    }

    kernfs_path("/", LOOKUP_FOLLOW, &root_path);
    vfs_statfs(&root_path, &fs);

    len += sprintf(output + len, "disk_usage=%llu\n", (fs.f_blocks - fs.f_bfree) * fs.f_bsize);
    len += sprintf(output + len, "cpu_usage=%d\n", 50); // Replace with actual CPU usage calculation.

    if ((100 - (fs.f_bfree * 100 / fs.f_blocks)) > disk_threshold)
        send_alert("Disk usage threshold exceeded.");

    if (50 > cpu_threshold) // Replace 50 with actual CPU usage percentage
        send_alert("CPU usage threshold exceeded.");

    strncpy(buf, output, len);
    kfree(output);

    return len;
}

static ssize_t write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *pos) {
    char *input;

    input = kmalloc(count + 1, GFP_KERNEL);
    if (!input)
        return -ENOMEM;

    if (copy_from_user(input, buffer, count)) {
        kfree(input);
        return -EFAULT;
    }

    input[count] = '\0';

    if (sscanf(input, "cpu_threshold=%d", &cpu_threshold) == 1 ||
        sscanf(input, "disk_threshold=%d", &disk_threshold) == 1) {
        kfree(input);
        return count;
    }

    kfree(input);
    return -EINVAL;
}

static int __init sys_monitor_init(void) {
    proc_file = create_proc_entry("sys_monitor", 0666, NULL);
    if (!proc_file)
        return -ENOMEM;

    proc_file->read_proc = read_proc;
    proc_file->write_proc = write_proc;

    nl_sock = netlink_kernel_create(&init_net, NETLINK_USER, NULL);
    if (!nl_sock) {
        remove_proc_entry("sys_monitor", NULL);
        return -ENOMEM;
    }

    return 0;
}

static void __exit sys_monitor_exit(void) {
    if (proc_file)
        remove_proc_entry("sys_monitor", NULL);
    if (nl_sock)
        netlink_kernel_release(nl_sock);
}

module_init(sys_monitor_init);
module_exit(sys_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Ronchini");
MODULE_DESCRIPTION("System Monitor Kernel Module");
