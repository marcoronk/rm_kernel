#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sched/loadavg.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#define NETLINK_USER 31


#define PROC_NAME "sys_monitor"

#define LIMIT_CPU_TEMP 30
#define LIMIT_MEMORY 1024000


struct sock *nl_sk = NULL;
pid_t pid = 0;
static void send_to_user_space(int pid, char *msg) {
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int msg_size = strlen(msg);
    int res;

    skb_out = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0) {
        printk(KERN_INFO "Error while sending to user\n");
    }
}


static void send_netlink_message(char *msg) {
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    int size=0;
               printk(KERN_INFO "SysMonitor sent netlink %s",msg);

    if (pid == 0)
    return;
    // Alloca un buffer per il messaggio

    size = strlen(msg);
    skb = nlmsg_new(size, GFP_KERNEL);
    if (!skb) {
        pr_err("Isend_netlink_message buffer error\n");
        return;
    }

    nlh = nlmsg_put(skb, 0, 0, NLMSG_DONE, size, 0);
    NETLINK_CB(skb).dst_group = 0;  // Messaggio per il gruppo 0
    memcpy(nlmsg_data(nlh), msg, size);

    // Invia il messaggio al socket Netlink
    if (nlmsg_unicast(nl_sk, skb, pid) < 0) {
        pr_err("Error netcast\n");
    }
     printk(KERN_INFO "Inviato messaggio %s", msg);
}


static int sys_monitor_show(struct seq_file *m, void *v)
{
    struct sysinfo info;
    unsigned long total_mem, free_mem, used_mem, shared_ram;
    unsigned long total_swap, free_swap, used_swap;
    struct net_device *dev;
    struct thermal_zone_device *tz;
    int temp;
    int cpu_temp = 0;
    char *mess;
    mess = kmalloc(1024, GFP_KERNEL);
    memset(mess,0,1024);
    si_meminfo(&info);
  
    total_mem = info.totalram * 4; // Converti in KB
    free_mem = info.freeram * 4;
    used_mem = total_mem - free_mem;

    total_swap = info.totalswap * 4;
    free_swap = info.freeswap * 4;
    used_swap = total_swap - free_swap;

    shared_ram = info.sharedram * 4;
    unsigned long load1 = (avenrun[0] * 100) >> FSHIFT;
    unsigned long load5 = (avenrun[1] * 100) >> FSHIFT;
    unsigned long load15 = (avenrun[2] * 100) >> FSHIFT;



    seq_printf(m, "CPU_UPTIME: %lus\n", (unsigned long)jiffies_to_msecs(get_jiffies_64()) / 1000);
    seq_printf(m, "CPU_LOAD_AVERAGE_1:  %lu.%02lu\n", load1 / 100, load1 % 100);
    seq_printf(m, "CPU_LOAD_AVERAGE_5:  %lu.%02lu\n", load5 / 100, load5 % 100);
    seq_printf(m, "CPU_LOAD_AVERAGE_15:  %lu.%02lu\n", load15 / 100, load15 % 100);
/*
    tz = thermal_zone_get_zone_by_name("x86_pkg_temp");
    if (thermal_zone_get_temp(tz, &temp) == 0) {
         cpu_temp = temp / 1000;          
         seq_printf(m, "CPU_TEMPERATURE:  %d\n", cpu_temp);  
         if (cpu_temp > LIMIT_CPU_TEMP)  {
            seq_printf(m, "ALERT: Cpu temperature is %d\n", cpu_temp);    
            sprintf(mess,"ALERT: Cpu temperature is %d\n", cpu_temp);
            send_netlink_message(mess);
         }
           
    } 
    */
    if (!IS_ERR(tz)) {
        printk(KERN_WARNING "ACPI thermal zone not found.\n");                
    }

    seq_printf(m, "TOTAL_RAM: %lu\n", total_mem);
    seq_printf(m, "FREE_RAM: %lu\n", free_mem);
    if (free_mem < LIMIT_MEMORY)  {
            seq_printf(m, "ALERT: Free memory is %d\n", free_mem);      
            sprintf(mess,"ALERT: Free memory is %d\n", free_mem);   
            send_netlink_message(mess);      
    }
    seq_printf(m, "USED_RAM: %lu\n", used_mem);
    seq_printf(m, "SHARED_RAM: %lu\n", shared_ram);
    seq_printf(m, "TOTAL_SWAP: %lu\n", total_swap);
    seq_printf(m, "FREE_SWAP: %lu\n", free_swap);
    seq_printf(m, "USED_SWAP: %lu\n", used_swap);

    for_each_netdev(&init_net, dev)
    {
        seq_printf(m, "NETWORK_INTERFACE: %s, RX: %lu bytes, TX: %lu bytes\n",
                   dev->name,
                   dev->stats.rx_bytes,
                   dev->stats.tx_bytes);
    }
    kfree(mess);
    return 0;
}

static int sys_monitor_open(struct inode *inode, struct file *file)
{
    return single_open(file, sys_monitor_show, NULL);
}
static void netlink_recv_msg(struct sk_buff *skb) {
    struct nlmsghdr *nlh = nlmsg_hdr(skb);
    pid = nlh->nlmsg_pid; 
    printk(KERN_INFO "Received pid %d\n", pid);    
}
static const struct proc_ops proc_ops = {
    .proc_open = sys_monitor_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
    
};

static int __init sys_monitor_init(void)
{
    proc_create(PROC_NAME, 0, NULL, &proc_ops);
    printk(KERN_INFO "/proc/%s created\n", PROC_NAME);

     struct netlink_kernel_cfg cfg = {
        .input = netlink_recv_msg, // Funzione di callback per gestire i messaggi
    };

    // Crea una socket Netlink
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        pr_err("Impossibile creare una socket Netlink\n");
        return -ENOMEM;
    } else {
        printk(KERN_INFO "Netlink created %d for user %d\n", nl_sk,NETLINK_USER);
    }


    return 0;
}

static void __exit sys_monitor_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
     netlink_kernel_release(nl_sk);
    printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

module_init(sys_monitor_init);
module_exit(sys_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("System Resource Monitor");