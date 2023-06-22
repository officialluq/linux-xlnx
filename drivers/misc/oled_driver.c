#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/capability.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/ptrace.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#define OLED_DRIVER_PROC_ENTRY "driver/oled_driver"

void __iomem *mem;

void writeCharOled(char* address,char myChar){
	int status=0;
	writeb(myChar, address + 8);
	printk("I am here %c \n", myChar);
	writeb(0x1, address); 
	while(!status){
		status = readl(address+4); //polling mode
	}
	writeb(0x0, address + 4);
}

ssize_t write_oled_driver(struct file *file, const char __user *usr, size_t size, loff_t *off)
{
	printk("Printing: %s \n", usr);
	for(int i=0;i<64;i++)
		writeCharOled(mem,' ');

	for(int i = 0; i < size; i++)
	{
		writeCharOled(mem, usr[i]);
	}
	return size;
}
ssize_t read_oled_driver(struct file *file, char __user *usr, size_t size, loff_t *off)
{
    return 0;
}

static ssize_t store_oled_driver(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	write_oled_driver(NULL, buf, count, NULL);
	return count;
}
static ssize_t show_oled_driver(struct device *device, struct device_attribute *attr, char *buf)
{
	return 0;
}
// static DEVICE_ATTR(oled_driver, S_IWUSR, store_oled_driver);
static DEVICE_ATTR(oled_driver, S_IWUSR | S_IRUGO, show_oled_driver, store_oled_driver);

static const struct proc_ops fops = {
	.proc_write	= write_oled_driver,
    .proc_read = read_oled_driver
};

static int __init oled_driver_probe(struct platform_device *ofdev)
{
	struct device_node *np = of_node_get(ofdev->dev.of_node);
	struct proc_dir_entry *oled_driver;

	mem = of_iomap(np, 0);
	if (mem == NULL)
		dev_err(&ofdev->dev, "%s couldnt map register.\n", __func__);

	oled_driver = proc_create(OLED_DRIVER_PROC_ENTRY, 0600, NULL, &fops);
	if (oled_driver == NULL) {
		dev_err(&ofdev->dev, "\n%s (%d): cannot create /proc/%s\n",
			__FILE__, __LINE__, OLED_DRIVER_PROC_ENTRY);
	} else {
		dev_info(&ofdev->dev, "created \"/proc/%s\"\n", OLED_DRIVER_PROC_ENTRY);
	}

	if (device_create_file(&ofdev->dev, &dev_attr_oled_driver))
		dev_warn(&ofdev->dev, "%s couldnt register sysFS entry.\n", __func__);

	return 0;
}

static int __exit oled_driver_remove(struct platform_device *ofdev)
{
	BUG();
	return 0;
}

static const struct of_device_id __initconst oled_driver_match[] = {
	{
		.compatible = "agh,oled_driver",
	},
	{},
};
MODULE_DEVICE_TABLE(of, oled_driver_match);

static struct platform_driver oled_drv = {
	.driver = {
		.name = "oled_driver",
		.of_match_table = oled_driver_match,
		.owner = THIS_MODULE,
	},
	.probe = oled_driver_probe,
	.remove = oled_driver_remove,
};


static int __init oled_driver_init(void)
{
	return platform_driver_register(&oled_drv);
}

static void __exit oled_driver_cleanup(void)
{
	if (mem != NULL)
		iounmap(mem);
	remove_proc_entry(OLED_DRIVER_PROC_ENTRY, NULL);
}


module_init(oled_driver_init);
module_exit(oled_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukasz Przybylik");
MODULE_DESCRIPTION("Simple driver for OLED display");