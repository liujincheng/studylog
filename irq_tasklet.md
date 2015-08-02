tasklet主要用在驱动中，搜索整个内核，基本只有drivers目录有用到tasklet。使用tasklet_schedule()将任务添加到队列中，但这个任务只会执行一次。如果需要再次执行，需要再次调度该函数。

在无线驱动wl_linux.c的中断收包函数wl_isr中，就有使用tasklet收包。

tasklet_schedule(&wl->tasklet);

tasklet使用TASKLET_SOFTIRQ中断来实现，因此需要利用普通的软中断的函数来触发，raise_softirq(TASKLET_SOFTIRQ)来触发。

##数据结构
```c
struct tasklet_struct
{
 struct tasklet_struct *next;         容许几个任务排队执行
 unsigned long state;                 TASKLET_STATE_SCHED/TASKLET_STATE_RUN
 atomic_t count;                         如果count不为0，则tasklet已经停用。
 void (*func)(unsigned long);    执行函数
 unsigned long data;                 执行时的私有数据
};
```

##tasklet操作函数
前面说到，tasklet采用软中断实现，所以它的注册方法、使用方法都类似于软中断。
1. 去取当前cpu上的tasklet_vec向量表。取之前，需要先设置当前cpu的中断屏蔽位，即禁用本地中断的情况下才会操作tasklet_vec数组。
1. 遍历list上所有的tasklet_struct，执行t->func(t->data)。但是需要检测几个条件：（1）tasklet_trylock(t)  检查tasklet的状态是否为TASKLET_STATE_RUN, 也就是说是否已经在另外一个cpu上运行了。（2）原子读t->count，若部位0，表示这个tasklet已经停用了。（3） 设置tasklet的状态，清除TASKLET_STATE_SCHED。一个tasklet只被执行一次，但是tasklet并不会把它给删掉，只是设置状态，避免再次被执行。
1. 将链表上剩余的未处理完成的tasklit放回到tasklet_vec向量表上。
```c
static void tasklet_action(struct softirq_action *a)
{
 struct tasklet_struct *list; 
 local_irq_disable(); 
 list = __get_cpu_var(tasklet_vec).list;
 __get_cpu_var(tasklet_vec).list = NULL;
 local_irq_enable();

 while (list) {
  struct tasklet_struct *t = list;
  list = list->next; 
  if (tasklet_trylock(t)) { 
   if (!atomic_read(&t->count)) {
    if (!test_and_clear_bit(TASKLET_STATE_SCHED, &t->state))
     BUG();
    t->func(t->data);
    tasklet_unlock(t);
    continue;
   }
   tasklet_unlock(t);
  }

  local_irq_disable();
  t->next = __get_cpu_var(tasklet_vec).list;
  __get_cpu_var(tasklet_vec).list = t;
  __raise_softirq_irqoff(TASKLET_SOFTIRQ);
  local_irq_enable();
 }
}
```
