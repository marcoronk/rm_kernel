/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */ 

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_


#include "aesd-circular-buffer.h"


int aesd_open(struct inode *inode, struct file *filp);
int aesd_release(struct inode *inode, struct file *filp);
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos);
int aesd_init_module(void);
void aesd_cleanup_module(void);

struct aesd_dev
{    
    struct aesd_circular_buffer aesd_buffer;
    struct aesd_buffer_entry aesd_entry;
    struct mutex lock;
    struct cdev cdev;     /* Char device structure      */
};


#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
