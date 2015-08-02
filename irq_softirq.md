软中断，用一个per-CPU的unsigned long型变量的bit位图表示中断类型，也即支持32种软中断。实际的软中断没有那么多。相关的enum类型为HI_SOFTIRQ, TIMER_SOFTIRQ, NET_TX_SOFTIRQ, NET_RX_SOFTIRQ, BLOCK_SOFTIRQ, TASKLET_SOFTIRQ, SCHED_SOFTIRQ。

使用__cpuinitdata标记，保证在内核启动不久时，创建一个内核线程，名称为ksoftirqd/0，其中最后的编号，是cpu的编号。也就是每个cpu上都会有一个这样的线程在执行do_softirq()。

##数据结构
```c
typedef struct {
 unsigned int __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;
irq_cpustat_t irq_stat[NR_CPUS] ____cacheline_aligned;
static struct softirq_action softirq_vec[32] __cacheline_aligned_in_smp;      
```

##softirq的执行时机
执行irq_exit()应该在恢复中断前的现场之前，因此恢复现场后，即恢复PC等寄存器之后，cpu开始接着处理之前的工作，可能是一个用户态的进程，如果放在恢复现场后，这段代码是没有机会执行到的。应该说，任何放在恢复现场后的代码都不会有机会执行到，因此在恢复现场时，就不会写LR寄存器，也不会回到这个中断处理函数。

什么时候关中断？应该有具体某一个中断的处理函数中，决定是否需要讲该中断号对应的中断关闭，或者将某一个cpu的所有中断都关闭。那么中断处理函数中是关中断，还是开中断呢？
```c
void irq_exit(void)
{
  account_irq_exit_time(current);
  trace_hardirq_exit();
  sub_preempt_count(HARDIRQ_OFFSET);
  if (!in_interrupt() && local_softirq_pending())
    invoke_softirq();
  tick_irq_exit();
  rcu_irq_exit();
}
```
这个是irq_exit()函数，忽略trace函数，关键的代码如加粗所示。可以理解为sub_preempt_count函数后，即退出本次hardirq。所以在in_interrupt()函数中，如果没有其他的hardirq，或softirq，或preempt，则会检查是否有softirq待处理。

##操作函数
```c
open_softirq(TASKLET_SOFTIRQ, tasklet_action, NULL);     
使用该函数可以注册软中断。三个参数，第一个是软中断号，第二个是中断执行函数，action，第三个是中断执行函数的参数，data。
void fastcall raise_softirq(unsigned int nr)    引发一个软中断。raise_softirq_irqoff
local_bh_disable()                              关闭本地的软中断
local_bh_enable()                               打开本地的软中断
```
问题：是关闭当前cpu上所有的软中断吗？why
所谓关闭软中断，是指不能触发新的软中断，避免当前的操作被打断。

首先检查当前上下文是否在中断中，包括软中断或硬中断。然后保存当前本地硬件中断到flag中。检查是否有软中断被添加到pending字段下，如果有，则调用__do_softirq()。
```c
asmlinkage void do_softirq(void)
{
 __u32 pending;
 unsigned long flags;

 if (in_interrupt())
  return;

 local_irq_save(flags);

 pending = local_softirq_pending();

 if (pending)
  __do_softirq();

 local_irq_restore(flags);
}
```

执行具体的软中断的代码。
```c
#define MAX_SOFTIRQ_RESTART 10
asmlinkage void __do_softirq(void) 
{
 struct softirq_action *h;
 __u32 pending;
 int max_restart = MAX_SOFTIRQ_RESTART;
 int cpu;

 pending = local_softirq_pending();
 account_system_vtime(current);

 __local_bh_disable((unsigned long)__builtin_return_address(0));           
 trace_softirq_enter();

 cpu = smp_processor_id();
restart:
 /* Reset the pending bitmask before enabling irqs */
 set_softirq_pending(0);

 local_irq_enable();

 h = softirq_vec;

 do {
  if (pending & 1) {
   h->action(h);
   rcu_bh_qsctr_inc(cpu);
  }
  h++;
  pending >>= 1;
 } while (pending);

 local_irq_disable();

 pending = local_softirq_pending();
 if (pending && --max_restart)
  goto restart;

 if (pending)
  wakeup_softirqd();

 trace_softirq_exit();

 account_system_vtime(current);
 _local_bh_enable();
}
```
