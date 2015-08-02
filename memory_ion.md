缘由：
* 底层驱动使用DMA访问读取数据，需要将相应的memory映射到用户态。
* 不同模块之间快速共享内存，避免数据拷贝。


**调试接口**  
ion的调试接口挂在debufs下   
* /sys/kernel/debug/ion/heaps/ion_mm_heap   包括ion_mm、ion_fb、ion_mm三种ion_device。
* /sys/kernel/debug/ion/clients/

**相关文件**  
ion_mm_heap.c

Android的内存管理器，在用户态分配，在不同的driver之间共享。Google本身实现的是ion内存管理框架，各个vendor可以在此基础上实现他们自己的heap需求。MTK在Android原生ion的基础上改进，增加MultiMedia和Framebuffer两种ion_type。目前供支持sys_contig、mm（multimedia）、fb三种ion。

Ion使用字符设备向用户态导出管理接口，使用platform总线在内核管理ion的heap。Platform是kernel的一种虚拟总线，方便管理内核虚拟设备，比如这里的ion_dev。使用该总线，可以利用现有的platform框架来初始化设备、导出debugfs调试接口。ION字符设备，对应的结构体类型为ion_device，内部使用红黑树管理申请者ion_client信息，以及内存池buffers。

通过字符设备ion_device的ioctl接口申请buffer，返回一个handle给申请者，该handle在kernel中指向一个buffer。申请者可以来自用户态，也可以来自内核态。Ion驱动使用ion_client结构体记录申请者的信息，比如name、pid、size。Heap的管理动作委托给ion_heap_ops完成。

```c
ion_ioctl
__ion_alloc(client, len, align, heap_id_mask, flags) 按page对齐，指定id
-> buffer = ion_buffer_create(heap,dev,len,align,flag)，根据id找heap。
-> buffer = kzalloc(sizeof(struct ion_buffer), GFP_KERNEL); 管理结构-> heap->ops->allocate
-> pVA=vmalloc_user(size) 为userspace申请连续内存，返回va
->heap->ops->map_dma 为dma映射内存
->heap->ops->map_kernel  为kernel映射不连续的内存
->heap->ops->map_user  映射到vma
->handle = ion_handle_create(client, buffer)

struct ion_buffer
. priv_virt  #指向ion_mm_buffer_info
.pVA  #vmalloc alloc, and virtural address
.MVA #M4U分配的虚拟地址
. priv_phys   # ion_phys_addr_t
.sg_table  #page管理结构
.eModuleID
.dev   #反指向ion_device
ion driver初始化时根据platform_data中的创建heap。
```




---
参考链接：  
1. [The Android ION memory allocator](https://lwn.net/Articles/480055/)
1. [Integrating the ION memory allocator](https://lwn.net/Articles/565469/)
