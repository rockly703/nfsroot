#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

#include <plat/gpio-cfg.h>
#include <mach/gpio-bank-n.h>
#include <mach/gpio-bank-l.h>

#define DEVICE_NAME     "buttons"

struct button_irq_desc {
    int irq;
    char *name;	
};

static struct button_irq_desc button_irqs [] = {
    {IRQ_EINT( 0),"KEY0"},
    {IRQ_EINT( 1),"KEY1"},
    {IRQ_EINT( 2),"KEY2"},
    {IRQ_EINT( 3),"KEY3"},
    {IRQ_EINT( 4),"KEY4"},
    {IRQ_EINT( 5),"KEY5"},
    {IRQ_EINT(19),"KEY6"},
    {IRQ_EINT(20),"KEY7"},
};

static irqreturn_t buttons_interrupt(int irq, void *dev_id)
{
    printk("%s press!\n", ((struct button_irq_desc *)dev_id)->name);
    return IRQ_RETVAL(IRQ_HANDLED);
}


static int s3c64xx_buttons_open(struct inode *inode, struct file *file)
{
    return 0;
}


static int s3c64xx_buttons_close(struct inode *inode, struct file *file)
{
    return 0;
}


static int s3c64xx_buttons_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    return 0;
}


static struct file_operations dev_fops = {
    .owner   =   THIS_MODULE,
    .open    =   s3c64xx_buttons_open,
    .release =   s3c64xx_buttons_close, 
    .read    =   s3c64xx_buttons_read,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &dev_fops,
};

static int __init dev_init(void)
{
	int ret, i;

	ret = misc_register(&misc);

    for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
        if (button_irqs[i].irq < 0) {
            continue;
        }

        ret = request_irq(button_irqs[i].irq, buttons_interrupt, IRQ_TYPE_EDGE_FALLING, 
                          button_irqs[i].name, (void *)&button_irqs[i]);
        if (ret)
            break;
    }

	printk (DEVICE_NAME"\tinitialized\n");

	return ret;
}

static void __exit dev_exit(void)
{
    int i;
#if 1
    for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
        if (button_irqs[i].irq < 0) {
            continue;
        }
        
        free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
    }
#endif
    
	misc_deregister(&misc);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
