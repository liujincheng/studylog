摘要：  
介绍页表建立过程，以及在开发中页表的使用实例。

---
#页表原理

**相关文件**
include/linux/mm_types.h

**术语**
页框        具体的memory，大小通常为4K  
页表项     pte，用于虚拟地址映射时的页转换  
页描述符  struct page  

##页表属性
在linux内核中，很多情况下都需要设置page为只读。比如设置_text段为只读，防止代码段被踩。再比如fork出来的子进程，会设置为只读，当有写需求时，会触发中断拷贝新的page。除了只读属性，还有smp shared，cache type等属性。

在管理页框过程中，比如分配新的page，或查找不活跃的page，或回收page，或写page到emmc等情况，会检查struct page即页描述符中的属性。 
page的属性设置在页表项pte，与设置在页描述符之间有何区别和关联？以do_brk()中申请memory设置属性为例，去掉与page属性设置无关的代码。
```c
unsigned long do_brk(addr, len)
{
    flags = VM_DATA_DEFAULT_FLAGS | VM_ACCOUNT | mm->def_flags;
    执行权限检查，各种检查是否能够分配
    使用vma_merge从已有的vma中延生，或kmem_cache_zalloc从vm_area_cachep中分配新的vma
    vma->vm_flags = flags;   保存原有flag
    vma->vm_page_prot = vm_get_page_prot(flags);  根据flag产生page的保护属性
}
  74 pgprot_t protection_map[16] = {
  75     __P000, __P001, __P010, __P011, __P100, __P101, __P110, __P111,
  76     __S000, __S001, __S010, __S011, __S100, __S101, __S110, __S111
  77 };
  79 pgprot_t vm_get_page_prot(unsigned long vm_flags)
  80 {
  81     return __pgprot(pgprot_val(protection_map[vm_flags &
  82                 (VM_READ|VM_WRITE|VM_EXEC|VM_SHARED)]) |
  83             pgprot_val(arch_vm_get_page_prot(vm_flags)));
  84 }
```
protection_map，P代表private映射，S代表Shared映射。之后的三个字符分别代表R/W/Exec。该字符在common code中定义，具体的实现取决与不同的arch。  
这里的关键在于protection_map。检查vm_flags中的read/write/exec/shared属性，查询protection_map，得到prot_val。或者根据arch_vm_get_page_prot查询prot_val。

以__P001为例，可以看到，最终会被转化为pte的属性
```c
#define __P001  __PAGE_READONLY
#define __PAGE_READONLY     __pgprot(_PAGE_DEFAULT | PTE_USER | PTE_NG | PTE_PXN | PTE_UXN | PTE_RDONLY)
#define PTE_USER        (_AT(pteval_t, 1) << 6)     /* AP[1] */
```
我们知道，pte本质上是一个指针，指向页框。这个指针是按照12bit对齐的，因此最低12个bit可以用于表示protection属性。而在64位系统中，高30位也未使用，可以用来表示page的属性。
page和pte都有类似readonly的保护属性，两者的区别，在于page中的保护属性，是在物理内存的角度，表示这个page的固有属性。而pte，则是站在线性地址的角度，表示这个page的属性，并非一成不变的。


#页表的初始化流程
```c
Setup_arch
|--> paging_init()
|-->map_mem()
|-->create_mapping(phys, virt, size)
   |-->do alloc_init_pud() while
      |-->do alloc_init_pmd() while
         |-->do_alloc_init_pte() while
```

pgd建立过程
```c
static inline pgd_t *pgd_alloc(struct mm_struct *mm)     //在mips中，mm参数其实没有用到
{
 pgd_t *ret, *init;

 ret = (pgd_t *) __get_free_pages(GFP_KERNEL, PGD_ORDER);           //获取用于pgd管理的page。
 if (ret) {
  init = pgd_offset(&init_mm, 0UL);            //使用init_mm中预先初始化的pgd_t来初始化pgd。
  pgd_init((unsigned long)ret);
  memcpy(ret + USER_PTRS_PER_PGD, init + USER_PTRS_PER_PGD,
         (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
 }
return ret; 
}
```

#页表应用举例
##动态设置page只读###
设置page只读的方法，调用者需保证起始虚拟地址按page对齐，len按page对齐。核心在于找到pte，然后使用pte_wrprotect修改pte的属性。
```c
int setKVaddrRDONLY(unsigned long virt_addr_start,unsigned long len)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	swp_entry_t entry;
	unsigned long virt_addr_end;

	if (virt_addr_start & ~PAGE_MASK)
	{
		printk("Start address is not PAGE alignment\n");
		return -EINVAL;
	}
	if(len != 4096){
		printk("Just support one page for now\n");

	return -EINVAL;
}

virt_addr_end = virt_addr_start + len;

if (virt_addr_end <= virt_addr_start)
return -ENOMEM;

pgd = pgd_offset_k(virt_addr_start);
pud = pud_offset(pgd, virt_addr_start);
pmd = pmd_offset(pud, virt_addr_start);
pte = pte_offset_kernel(pmd, virt_addr_start);

set_pte_kernel(&init_mm, virt_addr_start, pte, *pte);
//         entry = pte_to_swp_entry(*pte);
//         if (is_write_migration_entry(entry)) {
//                   make_migration_entry_read(&entry);
//        }
}
EXPORT_SYMBOL(setKVaddrRDONLY);

static inline void set_pte_kernel(struct mm_struct *mm, unsigned long addr,
pte_t *ptep, pte_t pte)
{
      pte = pte_wrprotect(pte);
set_pte(ptep, pte);
}
```

## 静态设置page只读
1. 在device tree的reserved-memory中，设置no-map

```
  reserved-memory {
    #address-cells = <2>;
    #size-cells = <2>;
    status = "okay";
    ranges;

    htc_reboot_info@83C30000 {
      reg = <0x0 0x83C30000 0x0 0x400>;
      save-emmc-feature = <1>;
      save-emmc-partition = "/dev/block/platform/mtk-msdc.0/by-name/para";
      save-emmc-offset = <0x24600>;
      no-map;
      compatible = "htc,reboot-info";
      reg-names = "htc_reboot_info_res";
    };  
  }
```

2. setup_arch过程中，会扫描reserved-memory，并建立页表

大概的函数流程为： 
```
setup_arch() -> arm64_memblock_init() -> early_init_fdt_scan_reserved_mem();
    +--> of_scan_flat_dt(__fdt_scan_reserved_mem, NULL);
        +--> 遍历所有的reserved memory node，并执行__fdt_scan_reserved_mem
            +--> 检查node是否合法，比如depth，status等。
            +--> ret = __reserved_mem_reserve_reg
                +--> 查找node中是否有通过reg指定base address，如果没有，则返回-NOENT，将会动态reserve memory
                +--> 查找node中是否有no-map属性
                +--> while循环，检查该node中所有的cell，并执行early_init_dt_reserve_memory_arch(base, size, nomap)
                    +--> early_init_dt_reserve_memory_arch(base, size, nomap)  执行reserve操作
                        +--> if no-map, memblock_remove(base, size)
                        +--> else，memblock_reserve(base, size)
                    +--> 保存first cell fdt_reserved_mem_save_node(node, uname, base, size);
            +--> 如果返回值为为-NOEINT， 则fdt_reserved_mem_save_node将node添加到reserved_mem数组
    +--> fdt_init_reserved_mem();  从reserved_mem中，为没有指定reg的node动态reserve memory
```

简单将，通过no-map标记的memory，不会为它创建页表，除非使用ioremap，为这些page重新映射虚拟地址，否则无法正常访问。如果有模块直接通过写地址的方式访问，将会触发data-abort。

##Zero page
ZERO_PAGE，比如进程fork的时候，待分配page。





