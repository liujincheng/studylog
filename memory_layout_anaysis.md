摘要：  
本文主要介绍如何分析memory layout。

---

有几个层面在reserve memory：  
1. LK中直接从memblock中花掉
1. Kernel中，通过查看memblock/reserved有预留
1. Kernel中，遍历memory类型的memblock，检查Page flag为reserved。大概有56M。
1. 比较/proc/meminfo中的MemTotol和mem_init中打印的free_pages，仍然8M的Gap，也即在这个位置之后，还有code在reserve memory。（需要使用最新的code比较）

##memblock
kernel通过memblock来管理内存块，可通过debugfs来查询。Kernel bootup阶段reserve的memory都是通过memblock_reserve()这个API来reserve的，当然，有一部分memory在用完之后通过memblock_free()又还给系统了。

首先，通过mount命令查看debugfs的挂载目录，如果默认没有挂载，需要手动挂载。比如debugfs被挂载在/proc/debugfs下，那么可通过如下的文件接口去读取memory layout。
  * /proc/debugfs/memblock/reserved
  * /proc/debugfs/memblock/memory
  * /proc/mtk_memcfg/memory_layout
  * /proc/iomem 它的值和debugfs/memblock/memory中的一致。

**/proc/iomem**  
这个proc文件在ioresources_init()函数中创建，描述了设备上的物理内存和IO内存的起始地址和范围。在体系结构初始化文件setup.c中使用resource_init，将boot_mem_map数组中的io信息按照内核要求的格式注册，部分信息如kernel的code与data的大小，通过读取内核镜像文件的段信息获取。可通过该文件判断是否NUMA内存。

通过查看上面的两个memblock文件，已经对memory layout有了大致的了解。但这还不够，因此memblock文件中不包含memblock的owner。一种方法是使用前面的mtk_memcfg中的信息，但这通常不是特别完整。另外一种比较彻底的方法是修改code，对照uart打印来分析。

1. **修改`memblock_dbg`默认打印级别**。虽然可以通过修改printk的打印级别，但因为memblock的打印在开机非常早就印出来，所以只能修改code。
```c
#define memblock_dbg(fmt, ...) \
printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
```
改为：
```
#define memblock_dbg(fmt, ...) \
printk(KERN_WARNING pr_fmt(fmt), ##__VA_ARGS__)
```
2. 找到memblock的申请者，修改memblock_reserve() 和memblock_free()
    * 在函数开头处添加show_stack(0,0);
    * 修改该函数中的memblock_dbg()调用，将size打印出来

3. 修改kernel的启动commandline
默认情况下memblock_debug为0，可通过kernel的启动commandline调整。  
修改方法：修改项目的BoardConfig.mk中BOARD_KERNEL_CMDLINE

然后重新编译版本，抓取一份开机的uart log，从kernel开头处的log可以看到有打印出每一笔kernel reserve出来的memory，包括其backtrace以及size大小
当然，有些被free掉的memory也会打印出来


##分析运行期间memory使用情况
从kernel及userspace可用内存数量的角度来分析memory。

相关接口： /proc/meminfo  
相关文件： fs/proc/meminfo.c

Memtotal: 根据totalram_pages计算  
Memfree: 

```c
free_highmem_page(page),
free_bootmem_late(addr, size)  nobootmem.c中将page释放给伙伴系统。
free_reserved_page(page)
mem_init() -> free_all_bootmem();
```


