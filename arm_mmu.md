在MT6795的LK中，有设置platform_init_mmu_mappings，会配置MMU属性。

MTK平台支持MEMORY_RESERVE_MODE，和Jerry_White sync后了解到，如果不使能该功能，则有较少概率memory中部分bit会丢掉，使能该功能后有助于debug。但实时上即使没有使能该功能，TW team也没有观察到bit丢失。

配置mmu
```c
void platform_init_mmu_mappings(void)
{
    /* configure available RAM banks */
    dram_init();

    /* Enable D-cache  */
#if 1
    unsigned int addr;
    unsigned int dram_size = 0;

    dram_size = physical_memory_size();

    for (addr = 0; addr < dram_size; addr += (1024*1024))
    {    
        /*virtual to physical 1-1 mapping*/
#if _MAKE_HTC_LK
        if (
                (bi_dram[0].start+addr) >= HTC_MEM_RESERVED_PHYS_ADDR &&
                bi_dram[0].start+addr < (HTC_MEM_RESERVED_PHYS_ADDR + HTC_MEM_RESERVED_SIZE) )
        {    
            arm_mmu_map_section(
                                    bi_dram[0].start+addr,bi_dram[0].start+addr,
                                    MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_NO_ALLOCATE | MMU_MEMORY_AP_READ_WRITE
            );  
        } else {
            arm_mmu_map_section(
                                    bi_dram[0].start+addr,bi_dram[0].start+addr,
                                    MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
        }    
#else
        arm_mmu_map_section(bi_dram[0].start+addr,bi_dram[0].start+addr, MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | MMU_MEMORY_AP_READ_WRITE);
#endif
    }    
#endif
}
```
这段代码，如果是HTC reserve的内存，则配置相应内存为MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_NO_ALLOCATE ，否则为MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE 。从两个角度分析这段代码：
1. MMU的属性的含义。
    write through：直接写memory，任一CPU发出写信号送到Cache的同时，也写入主存，保证主存的数据同步更新。
    write back：只有当Cache中的数据被换出或强制进行”清空“操作时，才将原更新的数据写入主存相应的单元中。
    allocate：如果写没有命中，则更新memory，之后将地址块回写到cache。
2. 如何配置mmu_mapping，即translation table。
    static uint32_t tt[4096] __ALIGNED(16384);
    tt[index] = (paddr & ~(MB-1)) | (0<<5) | (2<<0) | flags;
    将物理地址的低20位清零，然后用这低20位设置mmu的flag。

##初始化mmu
```c
void arm_mmu_init(void)
{
    int i;
    /* set some mmu specific control bits:
     * access flag disabled, TEX remap disabled, mmu disabled
     */
    arm_write_cr1(arm_read_cr1() & ~((1<<29)|(1<<28)|(1<<0)));
    /* set up an identity-mapped translation table with
     * strongly ordered memory type and read/write access.
     */
    for (i=0; i < 4096; i++) {
        arm_mmu_map_section(i * MB,
                    i * MB,
                    MMU_MEMORY_TYPE_STRONGLY_ORDERED |
                    MMU_MEMORY_AP_READ_WRITE);
    }  
    /* set up the translation table base */
    arm_write_ttbr((uint32_t)tt);
    /* set up the domain access register */
    arm_write_dacr(0x00000001);
    /* turn on the mmu */
    arm_write_cr1(arm_read_cr1() | 0x1);
}
```

ARM将Memory划分为16个protection area，可针对每个protection设置权限。相关函数参考init_mem_pgprot。

platform_init_mmu_mappings


