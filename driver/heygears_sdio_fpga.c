#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/seq_file.h>
#include <linux/circ_buf.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm-generic/ioctl.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/uaccess.h>
#include "heygears_spi_fpga.h"

#define HEYGEARS_SDIO_BUS_NUM  (1)

#define MAP_PAGE_COUNT 5064
#define MAPLEN  (PAGE_SIZE*MAP_PAGE_COUNT)
#define BYTE_ALIGN(x)  (((x +(4 *1024 -1))>>12) << 12)  //4K字节对齐



//#define FPGA_DMA_ALLOC 
#define FPGA_VMALLOC 

extern void sunxi_mmc_rescan_card(unsigned ids);

static bool debug_info = false;             /* 调试信息 */
extern struct completion spi_update_completion;
#define MY_REG_RX 0
#define MY_REG_TX 0
#define MY_REG_STATUS 1
//data ready
#define BIT_STATUS_DR 0x01
//transmit empty
#define BIT_STATUS_THRE 0x01

#define UART_NR		4	                    /* Number of UARTs this driver can handle */

#define FIFO_SIZE	PAGE_SIZE
#define WAKEUP_CHARS	256


void map_vopen(struct vm_area_struct *vma);
void map_vclose(struct vm_area_struct *vma);
int map_fault(struct vm_area_struct *vma, struct vm_fault *vmf);
static char *vmalloc_area = NULL;
static unsigned long phy_addr;

static char *buff;

/* SDIO UART Port struct */
struct sdio_uart_port {
    uint32_t            magic;
    unsigned int		index;
    struct sdio_func* func;
    struct mutex		func_lock;
    struct task_struct* in_sdio_uart_irq;
    struct kfifo		xmit_fifo;
    struct kfifo		read_fifo;
    spinlock_t		write_lock; //actually read/write lock
    struct device* file_dev;
    struct cdev c_dev;
};


static dev_t heygears_dev = 0;              /* 设备编号 */
static struct class* heygears_class = 0;    /* 新的设备类 */
static struct sdio_uart_port* sdio_uart_table[UART_NR];
static DEFINE_SPINLOCK(sdio_uart_table_lock);


void map_vopen(struct vm_area_struct *vma)
{
	printk("mmap_open \n");
}
void map_vclose(struct vm_area_struct *vma)
{
	printk("mmap_close \n");
}

int map_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
    struct page *page;
    void *page_ptr;
    unsigned long offset ,virt_start,pfn_start;
    offset = (unsigned long)vmf->virtual_address - vma->vm_start;
    virt_start = (unsigned long)vmalloc_area + (unsigned long)(vmf->pgoff << PAGE_SHIFT);
    pfn_start = (unsigned long )vmalloc_to_pfn((void*)virt_start);
    page_ptr=NULL;
    if((vma==NULL)|| (vmalloc_area==NULL)){
	printk("return VM_FAULT_SIGBUS !\n");
	return VM_FAULT_SIGBUS;
    }
    if(offset >=MAPLEN){
	printk("return VM_FAULT_SIGBUS !\n");
	return VM_FAULT_SIGBUS;
    }
    page_ptr = vmalloc_area + offset;
    page = vmalloc_to_page(page_ptr);
    get_page(page);
    vmf->page = page;
    printk("%s ,map 0x%lx (0x%016lx) to 0x%lx , size: 0x%lx ,page:%ld \n",__func__,virt_start,pfn_start << PAGE_SHIFT,(unsigned long)vmf->virtual_address,PAGE_SIZE,vmf->pgoff);
    return 0;
}


static struct vm_operations_struct map_vm_ops = {
    .open = map_vopen,
    .close = map_vclose,
    .fault = map_fault,
};


#ifdef FPGA_DMA_ALLOC 
#define BYTE_ALIGN(x)  (((x +(4 *1024 -1))>>12) << 12)

void *fpga_dma_malloc(struct device dev,u32 actual_bytes,void *phys_addr)
{
    u32 actual_bytes;
    void *address= NULL ;
    if(actual_bytes != 0){
        address = dma_alloc_coherent(dev,actual_bytes,(dma_addr_t *)phys_addr,GFP_KERNEL);
        if(address){
            printk(" dma alloc coherent ok  ,return address = %p size= 0x%x \n",
                  (void *)(*(unsigned long *)phys_addr),num_bytes);
            return address;
        }else{
            printk("dma_allco_coherent faild  \n ");
            return NULL;
        }
    }else{
            printk(" size is zero \n");

    }

    return NULL;


}
#endif

static int sdio_uart_add_port(struct sdio_uart_port* port)
{
    int index, ret = -EBUSY;

    mutex_init(&port->func_lock);
    spin_lock_init(&port->write_lock);
    if (kfifo_alloc(&port->xmit_fifo, FIFO_SIZE, GFP_KERNEL))
        return -ENOMEM;

    if (kfifo_alloc(&port->read_fifo, FIFO_SIZE, GFP_KERNEL))
    {
        kfifo_free(&port->xmit_fifo);
        return -ENOMEM;
    }

    spin_lock(&sdio_uart_table_lock);
    for (index = 0; index < UART_NR; index++) {
        if (!sdio_uart_table[index]) {
            port->index = index;
            sdio_uart_table[index] = port;
            ret = 0;
            break;
        }
    }
    spin_unlock(&sdio_uart_table_lock);

    return ret;
}

static void sdio_uart_port_remove(struct sdio_uart_port* port)
{
    struct sdio_func* func;

    spin_lock(&sdio_uart_table_lock);
    sdio_uart_table[port->index] = NULL;
    spin_unlock(&sdio_uart_table_lock);

    /*
     * We're killing a port that potentially still is in use by
     * the tty layer. Be careful to prevent any further access
     * to the SDIO function and arrange for the tty layer to
     * give up on that port ASAP.
     * Beware: the lock ordering is critical.
     */
    mutex_lock(&port->func_lock);
    func = port->func;
    sdio_claim_host(func);
    port->func = NULL;
    mutex_unlock(&port->func_lock);
    sdio_release_irq(func);
    sdio_disable_func(func);
    sdio_release_host(func);
}

static int sdio_uart_claim_func(struct sdio_uart_port* port)
{
    mutex_lock(&port->func_lock);
    if (unlikely(!port->func)) {
        mutex_unlock(&port->func_lock);
        return -ENODEV;
    }
    if (likely(port->in_sdio_uart_irq != current))
        sdio_claim_host(port->func);
    mutex_unlock(&port->func_lock);
    return 0;
}

static inline void sdio_uart_release_func(struct sdio_uart_port* port)
{
    if (likely(port->in_sdio_uart_irq != current))
        sdio_release_host(port->func);
}

static inline u8 sdio_in(struct sdio_uart_port* port, int offset)
{
    u8 c;
    c = sdio_readb(port->func, offset, NULL);
    return c;
}

static inline void sdio_out(struct sdio_uart_port* port, int offset, u8 value)
{
    sdio_writeb(port->func, value, offset, NULL);
}

/*
 * This handles the interrupt from one port.
 */
static void sdio_uart_irq(struct sdio_func* func)
{
    struct sdio_uart_port* port = sdio_get_drvdata(func);
    unsigned int status;

    /*
     * In a few places sdio_uart_irq() is called directly instead of
     * waiting for the actual interrupt to be raised and the SDIO IRQ
     * thread scheduled in order to reduce latency.  However, some
     * interaction with the tty core may end up calling us back
     * (serial echo, flow control, etc.) through those same places
     * causing undesirable effects.  Let's stop the recursion here.
     */
    if (unlikely(port->in_sdio_uart_irq == current))
        return;

    port->in_sdio_uart_irq = current;
    status = sdio_in(port, MY_REG_STATUS);
    /*
    if (status & BIT_STATUS_DR)
    {
        sdio_uart_receive_chars(port, &status);
    }

    if (status & BIT_STATUS_THRE)
    {
        sdio_uart_transmit_chars(port);
    }
    */

    port->in_sdio_uart_irq = NULL;
}

// 直接从被调用函数中获取数据
static int sdio_receive_inline(struct sdio_func* func, char* buf, int size)
{
    struct sdio_uart_port* port = sdio_get_drvdata(func);
    int ret;

    if (unlikely(port->in_sdio_uart_irq == current))
        return 0;

    port->in_sdio_uart_irq = current;

    {
        unsigned int addr = 0;
        ret = sdio_readsb(port->func, buf, addr, size);
    }

    port->in_sdio_uart_irq = NULL;

    return (ret == 0) ? size : 0;
}

static int sdio_write_inline(struct sdio_func* func, const char* buf, int size)
{
    struct sdio_uart_port* port = sdio_get_drvdata(func);
    int ret;

    if (unlikely(port->in_sdio_uart_irq == current))
        return 0;

    port->in_sdio_uart_irq = current;

    {
        unsigned int addr = 0;
        ret = sdio_writesb(port->func, addr, (char*)buf, size);
    }

    port->in_sdio_uart_irq = NULL;

    return (ret == 0) ? size : 0;
}

static int sdio_uart_activate(struct sdio_uart_port* port)
{
    int ret;

    if (debug_info) pr_info("SDIO Driver: sdio_uart_activate start\n");
    kfifo_reset(&port->xmit_fifo);
    kfifo_reset(&port->read_fifo);

    ret = sdio_uart_claim_func(port);
    if (ret)
        return ret;

    if (debug_info) pr_info("SDIO Driver: sdio_uart_activate sdio_uart_claim_func ok\n");

    ret = sdio_enable_func(port->func);
    if (ret)
        goto err1;

    if (debug_info) pr_info("SDIO Driver: sdio_uart_activate sdio_enable_func ok\n");
    ret = sdio_claim_irq(port->func, sdio_uart_irq);
    if (ret)
        goto err2;

    if (debug_info) pr_info("SDIO Driver: sdio_uart_activate sdio_claim_irq ok\n");
    // Kick the IRQ handler once while we're still holding the host lock
    sdio_uart_irq(port->func);

    if (debug_info) pr_info("SDIO Driver: sdio_uart_activate sdio_uart_irq complete\n");
    sdio_uart_release_func(port);

    if (debug_info) pr_info("SDIO Driver: sdio_uart_activate complete\n");
    return 0;

err2:
    sdio_disable_func(port->func);
err1:
    sdio_uart_release_func(port);
    return ret;
}

static void sdio_uart_shutdown(struct sdio_uart_port* port)
{
    int ret;

    ret = sdio_uart_claim_func(port);
    if (ret)
        return;

    sdio_release_irq(port->func);
    sdio_disable_func(port->func);

    sdio_uart_release_func(port);
}

static int sdio_uart_write(struct sdio_uart_port* port, const char* buf,
    int count)
{
    int ret;

    if (!port->func)
        return -ENODEV;

    {
        int err = sdio_uart_claim_func(port);
        if (!err) {
            ret = sdio_write_inline(port->func, buf, count);
            sdio_uart_release_func(port);
        }
        else
            ret = err;
    }

    return ret;
}

static int sdio_uart_read(struct sdio_uart_port* port, char* buf, int count)
{
    int ret;

    if (!port->func)
        return -ENODEV;

    int err = sdio_uart_claim_func(port);
    if (!err)
    {
        ret = sdio_receive_inline(port->func, buf, count);
        sdio_uart_release_func(port);
    }
    else
        ret = err;

    return ret;
}

static int sdio_file_open(struct inode* inode, struct file* file)
{

    if (debug_info)pr_info("SDIO Driver: open()\n");
    struct sdio_uart_port* port;
    port = container_of(inode->i_cdev, struct sdio_uart_port, c_dev);
    file->private_data = port;
    if (debug_info)pr_info("SDIO Driver: open() portidx=%u magic=%x\n", port->index, port->magic);
    return sdio_uart_activate(port);
}

static int sdio_file_close(struct inode* inode, struct file* file)
{
    if (debug_info)pr_info("SDIO Driver: close()\n");
    struct sdio_uart_port* port;
    port = container_of(inode->i_cdev, struct sdio_uart_port, c_dev);
    if (debug_info)pr_info("SDIO Driver: close() portidx=%u\n", port->index);
    sdio_uart_shutdown(port);
    return 0;
}

static ssize_t sdio_file_read(struct file* file, char __user* buf, size_t len, loff_t* off)
{
    size_t len_cur;
    size_t sum_len_out = 0;
    int len_out;
    char data[4096];
    struct sdio_uart_port* port = file->private_data;
    //if (debug_info) pr_info("SDIO Driver: read() magic=%x buf=%x len=%u\n", port->magic, (uint32_t)buf, (uint32_t)len);

    while (1)
    {
        len_cur = len;
        if (len_cur > sizeof(data))
            len_cur = sizeof(data);

        len_out = sdio_uart_read(port, data, len_cur);
        if (len_out <= 0)
            break;
        if (copy_to_user(buf, data, len_out))
            return -EFAULT;
        sum_len_out += len_out;
        if (len >= len_out)
            break;
        len -= len_out;
        buf += len_out;
    }

    if (debug_info) pr_info("SDIO Driver: read() sum_len_out=%u\n", (uint32_t)sum_len_out);
    return sum_len_out;
}

static ssize_t sdio_file_write(
    struct file* file, const char __user* buf, size_t len, loff_t* off)
{
    struct sdio_uart_port* port = file->private_data;
    int ret = 0;
    ssize_t write_size = 0;
    size_t len_write;
    char data[6144];//6144/512=12，连续块传输12

    if (debug_info) pr_info("SDIO Driver: write()\n");

    /* 发送帧同步信号，0x0f地址写1 */
    if (!port->func)
        return -ENODEV;
    {
        int err = sdio_uart_claim_func(port);
        if (!err) {
            sdio_out(port, 0x0f, 1);
            sdio_uart_release_func(port);
        }
        else
        {
            pr_warn("SDIO Driver: write() frame sync failed!\n");
            ret = err;
        }
    }

    while (len > 0)
    {
        len_write = len;
        if (len_write > sizeof(data))
            len_write = sizeof(data);

        ret = copy_from_user(data, buf, len_write);
        if (ret == 0)
            write_size += sdio_uart_write(port, data, len_write);
        else
            pr_warn("SDIO Driver: write() copy_from_user failed\n");

        buf += len_write;
        len -= len_write;
    }

    if (debug_info) pr_info("SDIO Driver: write() len=%u, ret=%i\n", (uint32_t)len, ret);
    return write_size;
}







static int sdio_file_mmap(struct file *file,struct vm_area_struct *vma)
{
#if 0
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;
    if(size > MAPLEN){
        printk("size too big size =%ld MAPLEN= %ld \n",size,MAPLEN);
        return -ENXIO;
    }
    if((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)){
        printk("writeable mappings must be shared ,rejecting \n");
        return -EINVAL;
    }
     /* do not want to have this area swapped out, lock it */
    vma->vm_flags |= VM_LOCKONFAULT;
    if(offset ==0){
        vma->vm_ops = &map_vm_ops;
    }else{
        printk("offset out of range \n");
        return -ENXIO;
    }
    return 0;

#endif
    int ret ;
    ret =   remap_pfn_range(vma,vma->vm_start,virt_to_phys(buff) >> PAGE_SHIFT,PAGE_SIZE,vma->vm_page_prot);
    return ret ;

}
//构造ioctl命令参数
#define HEYGEARS_FPGA_SDIO 'h' //魔数
#define FPGA_SDIO_WRITE   _IOWR(HEYGEARS_FPGA_SDIO,0x1010,unsigned long)
#define FPGA_SDIO_READ    _IOWR(HEYGEARS_FPGA_SDIO,0x1011,unsigned long)
#define FPGA_MEM_ALLOC    _IOWR(HEYGEARS_FPGA_SDIO,0x1012,unsigned long)
#define WRITE_SIZE 6144
static long sdio_fpga_ioctl(struct file *fp, unsigned int cmd , unsigned long arg)
{
    unsigned long write_size = 0;
    int err;
    struct sdio_uart_port* port = fp->private_data;
    unsigned long offset = 0;
    unsigned long len = arg;
    unsigned long len_write;
    unsigned long  virt_addr;
    unsigned long actual_bytes = 0;
    if(!port->func)
        return ENXIO;
 
    
    switch(cmd){
        case FPGA_SDIO_WRITE:
         /* 发送帧同步信号，0x0f地址写1 */
        err = sdio_uart_claim_func(port);
        if (!err) {
            sdio_out(port, 0x0f, 1);
            sdio_uart_release_func(port);
        }
        else{
            pr_warn("SDIO Driver: write() frame sync failed!\n");
        }
        while(len){
        len_write = len;
        if(len_write >  WRITE_SIZE){
            len_write = WRITE_SIZE;
        }
            write_size = sdio_uart_write(port,vmalloc_area + offset ,len_write);
            len -= len_write;
            offset +=len_write;
        }
        break;
        case FPGA_SDIO_READ:
        break;
        case FPGA_MEM_ALLOC:

#ifdef FPGA_VMALLOC 
            vmalloc_area = vmalloc(len);
            actual_bytes = BYTE_ALIGN(len);
            printk("actual_byte =l%d  len = %ld \n",actual_bytes,len);
            virt_addr = (unsigned long)vmalloc_area;
            for(virt_addr =(unsigned long )vmalloc_area ; virt_addr < (unsigned long)vmalloc_area + (actual_bytes >> 12) ; virt_addr += PAGE_SIZE){
                SetPageReserved(vmalloc_to_page((void *)virt_addr));
            }
#endif
#ifdef FPGA_DMA_ALLOC
            of_dma_confgure(port->dev,port->dev->of_node);
            virt_addr = fpga_dma_malloc( port->dev,actual_bytes ,phy_addr);
                  

#endif

      
        break;
        default:
            printk("no cmd\n");
        break;
    }
    return 0;
}

/* 操作接口 */
static const struct file_operations sdio_uart_proc_fops = {
    .owner = THIS_MODULE,
    .open = sdio_file_open,
    .release = sdio_file_close,
    .read = sdio_file_read,
    .write = sdio_file_write,
    .mmap = sdio_file_mmap,
    //.compat_ioctl = sdio_fpga_ioctl,
    .unlocked_ioctl = sdio_fpga_ioctl,
};


/* 侦测 */
static int sdio_uart_probe(struct sdio_func* func,
    const struct sdio_device_id* id)
{
    int ret;
    struct sdio_uart_port* port;
    dev_t dev_idx;
    buff = (char *)__get_free_page(GFP_KERNEL);
    if(buff ==NULL){
        printk("get free page failed \n");
        return -1;
    }
    

    /* 内存申请 */
    port = kzalloc(sizeof(struct sdio_uart_port), GFP_KERNEL);
    if (!port)
        return -ENOMEM;
    port->magic = 0x12345678;

    /* 功能类型 */
    if (func->class == SDIO_CLASS_UART) {
        pr_warn("%s: own driver init UART over SDIO\n",
            sdio_func_id(func));
    }
    else {
        pr_warn("sdio_uart_probe Unknow func class:%d\n", func->class);
        ret = -EINVAL;
        goto err1;
    }
    
    port->func = func;
    sdio_set_drvdata(func, port);

    /* add to sdio_uart table */
    ret = sdio_uart_add_port(port);
    if (ret) {
        pr_warn("sdio_uart_add_port error\n");
        kfree(port);
        return ret;
    }

    dev_idx = heygears_dev + port->index;

    /* 设备文件创建 */
    port->file_dev = device_create(heygears_class, NULL, dev_idx, NULL, "HeygearsSDIO%d", port->index);
    if (IS_ERR(port->file_dev))
    {
        sdio_uart_port_remove(port);
        ret = PTR_ERR(port->file_dev);
    }

    port->file_dev->driver_data = port;

    cdev_init(&port->c_dev, &sdio_uart_proc_fops);
    ret = cdev_add(&port->c_dev, dev_idx, 1);
    if (ret < 0) {
        goto err2;
    }

    if (debug_info)pr_warn("sdio_uart_probe all ok");
    return ret;
err2:
    device_destroy(heygears_class, dev_idx);
err1:
    kfree(port);
    return ret;
}

/* 移除 */
static void sdio_uart_remove(struct sdio_func* func)
{
    if (debug_info)pr_warn("sdio_uart_remove start");
    struct sdio_uart_port* port = sdio_get_drvdata(func);

    device_destroy(heygears_class, heygears_dev + port->index);
    kfifo_free(&port->read_fifo);
    kfifo_free(&port->xmit_fifo);
    sdio_uart_port_remove(port);
    kfree(port);

    pr_warn("sdio_uart_remove complete");
}

/**
 * @brief SDIO设备驱动结构
 */
 /* ids */
static const struct sdio_device_id sdio_uart_ids[] = {
    { SDIO_DEVICE_CLASS(SDIO_CLASS_UART) },
    { /* end: all zeroes */				 },
};
MODULE_DEVICE_TABLE(sdio, sdio_uart_ids);
/* sdio driver */
static struct sdio_driver sdio_uart_driver = {
    .probe = sdio_uart_probe,
    .remove = sdio_uart_remove,
    .name = "heygears_sdio_uart",
    .id_table = sdio_uart_ids,
};

/* Driver Init */
static int __init sdio_uart_init(void)
{
    int ret = 0;
    printk(KERN_WARNING "sdio_uart_init!\n");
    if (debug_info)pr_warn("sdio_uart_init start\n");
   wait_for_completion(&spi_update_completion);
    /* 触发一次sdio检卡，bus_num=1 */
    sunxi_mmc_rescan_card(HEYGEARS_SDIO_BUS_NUM);
    
    /* 动态申请设备编号 */
    ret = alloc_chrdev_region(&heygears_dev, 0, 1, "heygears_dev");
    if (ret < 0)
    {
        return ret;
    }
    if (debug_info)pr_warn("sdio_uart_init alloc_chrdev_region ok");

    /* 创建设备类 */
    heygears_class = class_create(THIS_MODULE, "heygears_sdio");
    if (IS_ERR(heygears_class))
    {
        ret = PTR_ERR(heygears_class);
        goto err1;
    }

    /* 注册SDIO驱动，将驱动加载到内核 */
    ret = sdio_register_driver(&sdio_uart_driver);
    if (ret)
        goto err2;
    if (debug_info)pr_warn("sdio_uart_init complete\n");
    return 0;

err2:
    /* 销毁设备类 */
    class_destroy(heygears_class);
    heygears_class = 0;

err1:
    /* 释放设备编号 */
    unregister_chrdev_region(heygears_dev, UART_NR);
    heygears_dev = 0;
    return ret;
}

/* Driver exit */
static void __exit sdio_uart_exit(void)
{
    printk(KERN_WARNING "sdio_uart_exit!\n");

    sdio_unregister_driver(&sdio_uart_driver);

    if (heygears_class)
    {
        class_destroy(heygears_class);
        heygears_class = 0;
    }

    if (heygears_dev)
    {
        /* 释放设备编号 */
        unregister_chrdev_region(heygears_dev, UART_NR);
        heygears_dev = 0;
    }
}

module_init(sdio_uart_init);
module_exit(sdio_uart_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heygears");
MODULE_DESCRIPTION("Heygears SDIO FPGA comm");
