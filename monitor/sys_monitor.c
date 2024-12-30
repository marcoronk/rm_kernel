#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/namei.h> 
#include <linux/fs.h>      // Necessario per kstatfs e vfs_statfs
#include <linux/dcache.h>  // Necessario per struct path
#include <linux/path.h>    // Necessario per kern_path


#define PROC_NAME "sys_monitor"
/*
int get_filesystem_info(const char *path, struct kstatfs *stat) {
    struct path p;
    int ret;

    // Risolve il percorso al file system
    ret = kern_path(path, LOOKUP_FOLLOW, &p);
    if (ret) {
        printk(KERN_ERR "Impossibile risolvere il percorso: %d\n", ret);
        return ret;
    }

    // Ottiene le informazioni sul file system
    ret = vfs_statfs(&p, stat);
    path_put(&p); // Libera la struttura path

    if (ret) {
        printk(KERN_ERR "Errore nel recupero delle informazioni sul file system: %d\n", ret);
        return ret;
    }

    return 0;
}
*/

static int sys_monitor_show(struct seq_file *m, void *v) {
    struct sysinfo info;
    unsigned long total_mem, free_mem, used_mem,shared_ram;
    unsigned long total_swap, free_swap, used_swap;
    //struct kstatfs *fsstat;
    
     printk(KERN_INFO "reading info....");
    // Ottieni informazioni di sistema
    si_meminfo(&info);

    total_mem = info.totalram * 4;  // Converti in KB
    free_mem = info.freeram * 4;
    used_mem = total_mem - free_mem;

    total_swap = info.totalswap * 4;
    free_swap = info.freeswap * 4;
    used_swap = total_swap - free_swap;

    shared_ram = info.sharedram * 4;

    seq_printf(m, "Cpu Uptime: %lus\n", (unsigned long)jiffies_to_msecs(get_jiffies_64()) / 1000);
    seq_printf(m, "Total Processes: %d\n", info.procs);

    seq_printf(m, "Total RAM: %lu\n", info.totalram);
    seq_printf(m, "Free RAM: %lu\n", free_mem);
    seq_printf(m, "Used RAM: %lu\n", used_mem);
    seq_printf(m, "Shared RAM: %lu\n", shared_ram);
    seq_printf(m, "Total Swap: %lu\n", total_swap);
    seq_printf(m, "Free Swap: %lu\n", free_swap);
    seq_printf(m, "Used Swap: %lu\n", used_swap);

    seq_printf(m, "Load Average 1: %lu\n", info.loads[0]);
    seq_printf(m, "Load Average 5: %lu\n", info.loads[1]);
    seq_printf(m, "Load Average 15: %lu\n", info.loads[2]);
     
  /*  if (get_filesystem_info("/", fsstat) == 0) {
       
       seq_printf(m, "Disk Usage: %lu\n", (fsstat->f_blocks - fsstat->f_bfree) * fsstat->f_bsize);
    }*/
    return 0;
}

static int sys_monitor_open(struct inode *inode, struct file *file) {
    return single_open(file, sys_monitor_show, NULL);
}

static const struct proc_ops proc_ops = {
    .proc_open = sys_monitor_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init sys_monitor_init(void) {
    proc_create(PROC_NAME, 0, NULL, &proc_ops);
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
    return 0;
}

static void __exit sys_monitor_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(sys_monitor_init);
module_exit(sys_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("System Resource Monitor");