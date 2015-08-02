本文主要记录cpu，cmp，hotplug相关的知识

##判断cpu online的方法
```
cpu_online(cpu)
static DECLARE_BITMAP(cpu_online_bits, CONFIG_NR_CPUS) __read_mostly; 
const struct cpumask *const cpu_online_mask = to_cpumask(cpu_online_bits);
```
可以通过读取bitmap对应的memory来判断最终所有cpu core online状态，但是却抓不到现场中cpu的状况。因此watchdog生效时，会将所有的cpu core offline掉。

##cpu shutdown流程
smp.c::__cpu_die()中打印“[107:hps_main]CPU5: shutdown”
```c
_cpu_down()
    +--> BUG_ON(cpu_online(cpu));
    +--> __cpu_die()
    +--> cpu_notify_nofail(CPU_DEAD | mod, hcpu);
          +--> cpu_notify(val, v)
                 +--> __cpu_notify(val, v, -1, NULL);
                       +-->__raw_notifier_call_chain(&cpu_chain, val, v, nr_to_call,nr_calls); 
                             +--> notifier_call_chain(&nh->head, val, v, nr_to_call, nr_calls); 
                                    +--> 在while循环中执行，遍历cpu_chain上注册的所有的notifier
```
while循环中，检测到NOTIFY_STOP_MASK会退出循环。  
#define NOTIFY_STOP   (NOTIFY_OK|NOTIFY_STOP_MASK) 

MTK在此文件中添加打印，检测到(nl == &cpu_chain.head)就打印notifier的信息

```c
/* Requires cpu_add_remove_lock to be held */
static int __ref _cpu_down(unsigned int cpu, int tasks_frozen)
{
  int err, nr_calls = 0;
  void *hcpu = (void *)(long)cpu;
  unsigned long mod = tasks_frozen ? CPU_TASKS_FROZEN : 0;
  struct take_cpu_down_param tcd_param = { 
    .mod = mod,
    .hcpu = hcpu,
  };
  if (num_online_cpus() == 1)
    return -EBUSY;
  if (!cpu_online(cpu))
    return -EINVAL;
  cpu_hotplug_begin();
  err = __cpu_notify(CPU_DOWN_PREPARE | mod, hcpu, -1, &nr_calls);
  if (err) {
    nr_calls--;
    __cpu_notify(CPU_DOWN_FAILED | mod, hcpu, nr_calls, NULL);
    printk("%s: attempt to take down CPU %u failed\n",
        __func__, cpu);
    goto out_release;
  }
  smpboot_park_threads(cpu);
  err = __stop_machine(take_cpu_down, &tcd_param, cpumask_of(cpu));
  if (err) {
    /* CPU didn't die: tell everyone.  Can't complain. */
    smpboot_unpark_threads(cpu);
    cpu_notify_nofail(CPU_DOWN_FAILED | mod, hcpu);
    goto out_release;
  }
  BUG_ON(cpu_online(cpu));
  /*
   * The migration_call() CPU_DYING callback will have removed all
   * runnable tasks from the cpu, there's only the idle task left now
   * that the migration thread is done doing the stop_machine thing.
   *
   * Wait for the stop thread to go away.
   */
  while (!idle_cpu(cpu))
    cpu_relax();
#ifdef CONFIG_MT_LOAD_BALANCE_PROFILER
  mt_lbprof_update_state(cpu, MT_LBPROF_HOTPLUG_STATE);
#endif
  /* This actually kills the CPU. */
  __cpu_die(cpu);
  /* CPU is completely dead: tell everyone.  Too late to complain. */
  cpu_notify_nofail(CPU_DEAD | mod, hcpu);
  check_for_tasks(cpu);
out_release:
  cpu_hotplug_done();
  if (!err)
    cpu_notify_nofail(CPU_POST_DEAD | mod, hcpu);
  return err;
}
```
