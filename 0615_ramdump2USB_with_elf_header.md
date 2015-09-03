add elf header to ramdump 2 USB

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

##mini ramdump flow
###in lk
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
###SYS_CORE_DUMP format

### code flow in lk





