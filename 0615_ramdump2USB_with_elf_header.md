add elf header to ramdump 2 USB
===
Target and Design:

for ramdump2USB, need full dump and mini dump can be saved at the same time. 

1. Kernel panic/watchdog timeout
2. lk use mrdump cblock to construct elf header. related function is kdump_core_header_init(), which is called when ramdump2EMMC.
3. lk detect auto ramdump2USB, enter ramdump2USB flow
  * htc_fastboot command dump all ddr
  * lk backup last ram_console header. (off_console will not be lost)
4. user reboot device, enter lk
  * lk restore last ram_console for mini dump. 
5. aee do mini dump.
6. extract elf header from full memory, and join it with full memory. this is SYS_CORE_DUMP
  * use crash to analyze SYS_CORE_DUMP

for ramdump2EMMC

* refer chunbo's A50DWG project, 1G/8G project have 800M emmc left, and can do one full ramdump. 
* Maybe we can set No_Delete.rdmp size to 800M, or less.

enhance minidump<br>
status
* full ramdump and mini ramdump's work scenario
  * full ramdump just be used to device frozen issue
  * mini ramdump is used to 
* how many memory can we use in mini ramdump
  * expdb size is 0xa00000, 
  * panic: dbg file size is 2440k + 1M RTB + 1M LK log
  * wdt:   dbg file size is 3704k + 1M RTB + 1M LK log

action<br>
* force-hard reboot, save all task backtrace
  * force-hard reboot, do mini dump
* when writeconfig 8 8, no matther writeconfig 7 8000, force-hard will stop lk slect menu

##elf header
program header and section header's offset are all given oblivously, that means, vendor can extend elfhdr. this is an example, mtk add psinfo, prstatus, misc items. These items are all prepared for phdrs.
```
149 struct mrdump_mini_elf_header {
150   struct elfhdr ehdr;
151   struct elf_phdr phdrs[MRDUMP_MINI_NR_SECTION];  //all PT_NOTE, PT_LOAD
152   struct mrdump_mini_elf_psinfo psinfo;  //prepare for PT_NOTE
153   struct mrdump_mini_elf_prstatus prstatus[NR_CPUS + 1]; //prepare for PT_NOTE
154   struct mrdump_mini_elf_note misc[MRDUMP_MINI_NR_MISC];
155 };
```

```
typedef struct elf64_hdr {
  unsigned char e_ident[EI_NIDENT];  //magic, class, abi
  Elf64_Half e_type;    //core, shared objects, objects, excutable file
  Elf64_Half e_machine; //EM_ARM, EM_ARM64
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;   //program header offset in this file
  Elf64_Off e_shoff;   //section header offset in this file
  Elf64_Word e_flags;
  Elf64_Half e_ehsize; //elf header size
  Elf64_Half e_phentsize; //program header entry size
  Elf64_Half e_phnum;  //program header number
  Elf64_Half e_shentsize;  //section header entry size
  Elf64_Half e_shnum;  //section header number
  Elf64_Half e_shstrndx; 
 } Elf64_Ehdr;
```

##mini ramdump flow
###in lk
kernel has prepared kedump elf header, lk will copy memory accoring to PT_LOAD header.
```
 kedump_to_expdb() 
    |-->ehdr = kedump_elf_hdr();  contruct auxiliary header for lk
        |--> kehdr.start = (void*)(KE_RESERVED_MEM_ADDR);  
        |--> kehdr.e_phnum = ((struct elf64_hdr*)(kehdr.start))->e_phnum;
        |--> kehdr.e_phoff = ((struct elf64_hdr*)(kehdr.start))->e_phoff;
    |-->datasize = kedump_mini_rdump(ehdr, offset);
        |--> in for loop: kedump_dev_write(offset + elfoff, (void*)addr, size);  addr is PT_LOAD memory physical address
	    |--> after PT_LOAD done: kedump_dev_write(offset, ehdr->start, sz_header); start is mini elf header. sz_header is PAGE_SIZE align
    |-->add SYS_XXXX_RAW data to dbg
```

###in kernel
kernel set phnum and every program header's vaddr and phy_addr. kernel set program header's physical address, then lk can know memory offset.
```
void mrdump_mini_ke_cpu_regs(struct pt_regs * regs)
{     
  int cpu;
  struct pt_regs context;
  if (!regs) {
    regs = &context;
    ipanic_save_regs(regs);
  }
  cpu = get_HW_cpuid();
  mrdump_mini_cpu_regs(cpu, regs, 1);
  mrdump_mini_add_loads();
  mrdump_mini_build_task_info(regs);
}     

mrdump_mini_add_loads()
    |-->for all CPU+1, 
      + regs are saved in prstatus->pr_reg
      + to main thread which panic, dump all regs' 32K memory nearby
      + to all core, mrdump_mini_add_tsk_ti
    |--> save __per_cpu_offset, mem_map
    |--> if dump_all_cpu is true
      + mrdump_mini_add_entry, add every cpu's runqueue, tsk, stack's 32K memory nearby
```

##full ramdump
main flow:
kernel panic
prepare mrdump_control_block, sig is XRDUMP3, 

compress memory in lk, log
```
 [2440] D:Boot record found at 0x42300000[5852], cb 0x42300000
 [2440] D:sram record with mode 1
 [2440] D:reset_string of aee: Kernel Oops
 [2460] I:Kdump triggerd by 'KERNEL-OOPS'
 [2460] I:Output to EXT4 Partition
 [2460] [PART_LK][get_part] userdata
 [2480] I:userdata offset: 1209139ALPS.L1.MP3.TC7SP.6753.p20_0529_V72, size: 776 Mb
 [2480] I: emmc dumping(address 0x40000000, size:0M)
 [2480] mem_size:0x7edc0000, phys_offset:0x40000000, DRAM_P_A:0x40000000, total:0x7edc0000
 [2500] I: NO_Delete.rdmp starts at LBA: 4096
 [2500] I: SYS_COREDUMP   starts at LBA: 4572
 [2500] I:kernel page offset 18446743798831644672
 [2520] I:64b kernel detected
 [2520] ... Written 0M
 [2520] D:kzip_add_file: zf 0x41eae648(0x41e1b789) SYS_COREDUMP
 [2520] D:-- Compress memory 41eac63c, size 8192
 [2520] D:-- Compress memory 40000000, size 2128347136
```

SYS_CORE_DUMP format
```
-rwxr--r-- 1 liu liu 2128355328  5月  5 16:43 SYS_COREDUMP*    --0x7EDC2000
Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  NOTE           0x00000000000000b0 0x0000000000000000 0x0000000000000000
                 0x0000000000000db8 0x0000000000000000         0
  LOAD           0x0000000000002000 0xffffffc000000000 0x0000000040000000
                 0x000000007edc0000 0x000000007e130000  RWE    0
PT_NOTE: auxiliary information, such as core registers, KERNEL_LOG, PROC_CUR_TSK
PT_LOAD: please notice that it assign physical and virtual address map relation, and memory size.

Displaying notes found at file offset 0x000000b0 with length 0x00000db8:
  Owner                 Data size Description
  CORE                 0x00000088 NT_PRPSINFO (prpsinfo structure)
  MACHDESC             0x00000024 Unknown note type: (0x0000aee0)
  CPU0                 0x00000188 NT_PRSTATUS (prstatus structure)
  CPU1                 0x00000188 NT_PRSTATUS (prstatus structure)
  CPU2                 0x00000188 NT_PRSTATUS (prstatus structure)
  CPU3                 0x00000188 NT_PRSTATUS (prstatus structure)
  CPU4                 0x00000188 NT_PRSTATUS (prstatus structure)
  CPU5                 0x00000188 NT_PRSTATUS (prstatus structure)
  CPU6                 0x00000188 NT_PRSTATUS (prstatus structure)
  CPU7                 0x00000188 NT_PRSTATUS (prstatus structure)
```

hexedit SYS_COREDUMP worklog/2015/05/ramdump/fulldump/db.fatal.00.KE.dbg.DEC<br>
00000000   7F 45 4C 46  02 01 01 00  00 00 00 00  00 00 00 00  04 00 B7 00  01 00 00 00  00 00 00 00  00 00 00 00  .ELF............................<br>
02302000   58 52 44 55  4D 50 30 32  00 00 00 00  02 00 00 00  08 00 00 00  00 00 00 00  00 00 00 00  C0 FF FF FF  XRDUMP02........................<br>

another case, hexdump mrdump_cblock.bin   live capture<br>
00000000   58 52 44 55  4D 50 30 33  00 00 00 00  02 00 00 00  08 00 00 00  00 00 00 00  00 00 00 00  C0 FF FF FF  XRDUMP03........................

02302000   58 52 44 55  4D 50 30 32  00 00 00 00  02 00 00 00  08 00 00 00  00 00 00 00  00 00 00 00  C0 FF FF FF  XRDUMP02........................<br>

```
 126 struct mrdump_control_block {
 127     char sig[8];
 128
 129     struct mrdump_machdesc machdesc;
 130     struct mrdump_crash_record crash_record;
 131
 132     struct mrdump_cblock_result result;
 133 };

  95 struct mrdump_machdesc {
  96     uint32_t crc;
  97 
  98     uint32_t output_device;
  99 
 100     uint32_t nr_cpus;
 101 
 102     uint64_t page_offset;
 103     uint64_t high_memory;
 104 
 105     uint64_t vmalloc_start;
 106     uint64_t vmalloc_end;
 107 
 108     uint64_t modules_start;
 109     uint64_t modules_end;           
 110    
 111     uint64_t phys_offset;           
 112     uint64_t master_page_table;     
 113 
 114     // don't affect the original data structure...
 115     uint32_t output_fstype;         
 116     uint32_t output_lbaooo;         
 117 };                                  
 118 
 119 struct mrdump_cblock_result {       
 120     char status[128];               
 121    
 122     uint32_t log_size;              
 123     char log_buf[2048];             
 124 }; 
```
