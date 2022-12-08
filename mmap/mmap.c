#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
 #include <linux/uaccess.h>
 #include <linux/io.h>

#define MISC_DEV_MINOR 5

static char *kbuff;


static ssize_t misc_dev_read(struct file *filep, char __user *buf, size_t count, loff_t *offset)
{

 int ret;

 size_t len = (count > PAGE_SIZE ? PAGE_SIZE : count);

 pr_info("###### %s:%d kbuff:%s ######\n", __func__, __LINE__, kbuff);
 
 ret = copy_to_user(buf, kbuff, len);  //这里使用copy_to_user  来进程内核空间到用户空间拷贝

 return len - ret;
}

static ssize_t misc_dev_write(struct file *filep, const char __user *buf, size_t count, loff_t *offset)
{
 pr_info("###### %s:%d ######\n", __func__, __LINE__);
 return 0;
}

static int misc_dev_mmap(struct file *filep, struct vm_area_struct *vma)
{
 int ret;
 unsigned long start;

 start = vma->vm_start;
 
 ret =  remap_pfn_range(vma, start, vma->vm_pgoff,
   PAGE_SIZE, vma->vm_page_prot); //使用remap_pfn_range来映射物理页面到进程的虚拟内存中  virt_to_phys(kbuff) >> PAGE_SHIFT作用是将内核的虚拟地址转化为实际的物理地址页帧号  创建页表的权限为通过mmap传递的 vma->vm_page_prot   映射大小为1页

 return ret;
}

static long misc_dev_ioctl(struct file *filep, unsigned int cmd, unsigned long args)
{
 pr_info("###### %s:%d ######\n", __func__, __LINE__);
 return 0;
}



static int misc_dev_open(struct inode *inodep, struct file *filep)
{
 pr_info("###### %s:%d ######\n", __func__, __LINE__);
 return 0;
}

static int misc_dev_release(struct inode *inodep, struct file *filep)
{
 pr_info("###### %s:%d ######\n", __func__, __LINE__);
 return 0;
}


static struct file_operations misc_dev_fops = {
 .open = misc_dev_open,
 .release = misc_dev_release,
 .read = misc_dev_read,
 .write = misc_dev_write,
 .unlocked_ioctl = misc_dev_ioctl,
 .mmap = misc_dev_mmap,
};

static struct miscdevice misc_dev = {
 MISC_DEV_MINOR,
 "misc_dev",
 &misc_dev_fops,
};

static int __init misc_demo_init(void)
{
 misc_register(&misc_dev);  //注册misc设备 （让misc来帮我们处理创建字符设备的通用代码，这样我们就不需要在去做这些和我们的实际逻辑无关的代码处理了）
 unsigned char * data;
 
 kbuff = (char *)__get_free_page(GFP_KERNEL);  //申请一个物理页面（返回对应的内核虚拟地址，内核初始化的时候会做线性映射，将整个ddr内存映射到线性映射区，所以我们不需要做页表映射）
 if (NULL == kbuff)
  return -ENOMEM;
// data = (char * )ioremap(0x49000000,4096);
// *data = 1;

 pr_info("###### %s:%d ######\n", __func__, __LINE__);
 return 0;
}

static void __exit misc_demo_exit(void)
{
 free_page((unsigned long)kbuff);

 misc_deregister(&misc_dev);
 pr_info("###### %s:%d ######\n", __func__, __LINE__);
}

module_init(misc_demo_init);
module_exit(misc_demo_exit);
MODULE_LICENSE("GPL");


