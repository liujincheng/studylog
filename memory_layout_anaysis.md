摘要：  
本文主要从物理内存的角度去分析memory layout。

---

MTK平台总体上，memory layout情况和QCT平台有较大差异，它更多地使用动态reserve memory。在MTK平台中，按照原始设计，仅有Framebuffer，Trusezone，Modem，Wifi这几个function，以及一些debug的feature需要reserve memory，

1. LK需要告诉Kernel memblock的size和count，可以直接将需要reserve的memory挖掉，Kernel看不见被挖掉的memory。使用者：Framebuffer，Trustzone
1. kernel中通过device tree或hard code指定base address和size，然后通过memblock_reserve从memblock中挖掉这段内存。使用者：htc_reboot_info, mrdump_clbock, minidump, RTB, LK LOG等。
1. 在device tree中指定要预留memory的size，由kernel动态去reserve一段物理内存。使用者包括：Modem，Wifi。

#memblock
kernel通过memblock来管理内存块，如下是memblock的数据结构，它包含两个主要部分，memory和reserved，分别用于表示可用内存与预留内存。memblock_type则用于记录每种类型的memblock中的memory region的cout，total_size等，而regions指针则指向一个记录所有memory_gegion的数组。

```c
struct memblock_type {
  unsigned long cnt;  /* number of regions */
  unsigned long max;  /* size of the allocated array */
  phys_addr_t total_size; /* size of all regions */
  struct memblock_region *regions;
};


struct memblock {
  phys_addr_t current_limit;
  struct memblock_type memory;
  struct memblock_type reserved;
};
```

可通过debugfs来查询这两种类型的memblock的具体细节。首先，通过mount命令查看debugfs的挂载目录，如果默认没有挂载，需要手动挂载。比如debugfs被挂载在/sys/kernel/debug下，那么可通过如下的文件接口去读取memory layout。常用的文件有以下几个。

  * /sys/kernel/debug/memblock/reserved
  * /sys/kernel/debug/memblock/memory
  * /proc/mtk_memcfg/memory_layout
  * /proc/iomem 它的值和debugfs/memblock/memory中的一致。

## memblock/reserved
这个文件可以看到当前系统reserve了哪些内存块，起始地址以及结束地址。需要注意，在这个layout中，reserved memory的最后一个，和memory size不匹配。比如memory为2G，其实地址为0x40000000，但实际的最后的结束地址却只有0xBDDC0000。这就是前面提到的，LK告诉Kernel的memblock siz就只有这么大。
```
cat /sys/kernel/debug/memblock/reserved                                        <
   0: 0x000000004007b000..0x000000004120c09f
   1: 0x0000000041e00000..0x0000000041ffffff
...
  30: 0x00000000bddbe000..0x00000000bddbffff
```

## memblock/memory
这个文件记录系统可用的物理内存的范围，换个角度来说，系统会为这个文件中所有的memory region来创建页表。实际的情况比这要稍微复杂一些，因为创建页表要按照pmd来对齐，一个pmd的size为1M，但实际上，这个文件中部分entry却不是1M对齐的。这里不详细分析，目前我的理解是，先建立的页表，创建完成之后，有模块使用memblock_remove()函数将之从可用内存中拿掉。
```
cat /sys/kernel/debug/memblock/memory                                          <
   0: 0x0000000040000000..0x0000000042ffffff
   1: 0x0000000043030000..0x0000000083afffff
   2: 0x0000000083c00000..0x0000000083c0ffff
   3: 0x0000000083c30400..0x0000000083dfffff
   4: 0x0000000083f00000..0x00000000b5ffffff
   5: 0x00000000bd010000..0x00000000bd9fffff
   6: 0x00000000bdc00000..0x00000000bddbffff
```

## /proc/mtk_memcfg/memory_layout
这个文件是MTK项目中用于查看memory layout的，比如如下这样的。如果找不到这个文件，那么可能是相关的kernel宏CONFIG_MTK_MEMCFG没有开导致，打开这个宏后重新编译kernel即可。在这个文件中，不仅仅告诉reserve了哪些memory，还会告诉是谁在reserve这段memory。在这个例子中，可以看到ccci reserve了82M + 26M memory。

```
[PHY layout]ccci_md_mem_reserve : 0xb8000000 - 0xbd1fffff (0x05200000) 
[PHY layout]ccci_md_mem_reserve : 0xb6000000 - 0xb79fffff (0x01a00000) 
Virtual kernel memory layout: 
vmalloc : 0xffffff8000000000 - 0xffffffbbffff0000 (245759 MB) 
vmemmap : 0xffffffbc00e00000 - 0xffffffbc02997000 ( 27 MB) 
modules : 0xffffffbffc000000 - 0xffffffc000000000 ( 64 MB) 
memory : 0xffffffc000000000 - 0xffffffc07e200000 ( 2018 MB) 
.init : 0xffffffc000a73000 - 0xffffffc000ac7840 ( 339 kB) 
.text : 0xffffffc000080000 - 0xffffffc000a73000 ( 10188 kB) 
.data : 0xffffffc000ac8000 - 0xffffffc000ba8b40 ( 899 kB) 
[PHY layout]DRAM size (dt) : 0x40000000 - 0x7fffffff (0x40000000) 
```

它的原理，是通过在memblock_reserve()函数中加上一些debug宏MTK_MEMCFG_LOG_AND_PRINTK，使用这个宏，会将reserve的size/base address/caller这些信息记录到预留内存中。

## /proc/iomem
这个proc文件在ioresources_init()函数中创建，描述了设备上的物理内存和IO内存的起始地址和范围。在体系结构初始化文件setup.c中使用resource_init，将boot_mem_map数组中的io信息按照内核要求的格式注册，部分信息如kernel的code与data的大小，通过读取内核镜像文件的段信息获取。可通过该文件判断是否NUMA内存。

#代码修改
Kernel bootup阶段reserve的memory都是通过memblock_reserve()这个API来reserve的，当然，有一部分memory在用完之后通过memblock_free()又还给系统了。

通过查看上面的两个memblock文件，已经对memory layout有了大致的了解。但这还不够，因此memblock文件中不包含memblock的owner。一种方法是使用前面的mtk_memcfg中的信息，但这通常不是特别完整。另外一种比较彻底的方法是修改code，对照uart打印来分析。

可以对照我的这份示例修改code。[http://git.htc.com:8081/#/c/605739/](http://git.htc.com:8081/#/c/605739/)

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

# memory layout申请大致流程
总的来说，memory相关初始化都在setup_arch()中，下面简要摘抄一些代码流程。
```
setup_arch()
    +--> setup_machine_fdt(__fdt_pointer);  读取fdt表，方便后续找reserve-memory相关的节点
    +--> arm64_memblock_init();  初始化memblock
        +--> memblock_reserve(__pa(_text), _end - _text);  内核code段
        +--> memblock_reserve(phys_initrd_start, phys_initrd_size); 根设备，放rootfs
        +--> 。。。 
    +--> paging_init();  创建页表
```

#分析运行期间memory使用情况
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


