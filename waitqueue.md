wait_queue，内核通知其他进程或线程有事件发生，并将事件放到队列中，放入过程中，需要检查队列的flags，决定放入的方式，需要调用队列的回调函数。

##数据结构
```c
struct __wait_queue {
 unsigned int flags; 队列的标志位，表明应该如何放。
#define WQ_FLAG_EXCLUSIVE0x01
 void *private; 队列的私有数据，比如可以用于记录队列所属进程。这里用void*表示，便于扩展。
 wait_queue_func_t func; 应该是队列的回调函数
 struct list_head task_list; 应该是事件的节点，被添加到链表头_wait_queue_head中。
};
struct __wait_queue_head {
 spinlock_t lock;
 struct list_head task_list;
};
```

##使用场景
等待队列示例，中断上下文和等待队列的交互：
```c
#define wake_up_interruptible(x) __wake_up(x, TASK_INTERRUPTIBLE, 1, NULL)
```
激活等待队列上的进程，属于进程调度的范围。

##主要函数
```
void __wake_up(wait_queue_head_t *q, unsigned int mode,  int nr_exclusive, void *key)
{
 unsigned long flags;

 spin_lock_irqsave(&q->lock, flags);
 __wake_up_common(q, mode, nr_exclusive, 0, key);
 spin_unlock_irqrestore(&q->lock, flags);
}

static void __wake_up_common(wait_queue_head_t *q, unsigned int mode,
   int nr_exclusive, int wake_flags, void *key)
{
 wait_queue_t *curr, *next;

 list_for_each_entry_safe(curr, next, &q->task_list, task_list) {    //会遍历等待队列下的所有等待者，如果等待者不具备拍它特性，且调用该函数的上下文也不要求排他，则会一次性将所有的进程都唤醒。在以太驱动收发包的流程中，该字段被设置为1，也就是说，只会唤醒一个等待者。事实上，也只有一个等待者。
  unsigned flags = curr->flags;

  if (curr->func(curr, mode, wake_flags, key) &&  (flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
   break;
 }
}

默认的func为autoremove_wake_function
int autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
 int ret = default_wake_function(wait, mode, sync, key);    //最终调用调度函数try_to_wake_up将进程放到可执行的队列上。mode为可被打断的调度。
 if (ret)
  list_del_init(&wait->task_list);
 return ret;
}
```

##在内核线程上下文中等待事件发生。
```c
#define wait_event_interruptible(wq, condition)wait_queue \
({wait_queue \
 int __ret = 0;wait_queue \
 if (!(condition))wait_queue \     //判断条件，使用者虽然调用了，但其实并不想等待。比如以太驱动收包流程。
  __wait_event_interruptible(wq, condition, __ret);wait_queue\    //返回值在这里没有用上。
 __ret;wait_queue \
})

#define __wait_event_interruptible(wq, condition, ret)wait_queue \
do {wait_queue \
 DEFINE_WAIT(__wait);wait_queue \              //展开这个宏，使用默认处理函数autoremove_wake_function创建一个等待队列的实例。
         \
 for (;;) {wait_queue \
  prepare_to_wait(&wq, &__wait, TASK_INTERRUPTIBLE);wait_queue\    //向等待队列头部中添加刚创建的_wait，flag为~WQ_FLAG_EXCLUSIVE，并设置当前线程状态为TASK_INTERRUPTIBLE
  if (condition)wait_queue \                           //再次condition条件。因为这个宏不只有一个地方调用。
   break;wait_queue \
  if (!signal_pending(current)) {wait_queue \    //当前线程从循环中被激活，但是却没有待处理的信号，则调用schdule()放弃执行权，继续等待。
   schedule();wait_queue \
   continue;wait_queue \
  }wait_queue \
  ret = -ERESTARTSYS;wait_queue \
  break;wait_queue \
 }wait_queue \
 finish_wait(&wq, &__wait);wait_queue \        //设置当前线程为TASK_RUNNING，并将临时申请的等待队列的实例从队列中删除。为防止被中断打断，需要使用spin_lock_irqsave。
} while (0)
```

##等待队列的三个主要接口
void fastcall add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)  
void fastcall add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *wait)  
给定等待队列的链表头部，将等待队列条目添加到队列中。
当该接口带有_exclusive后缀，表示等待队列的flag将会被置上WQ_FLAG_EXCLUSIVE位。

void fastcall prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state)

void fastcall finish_wait(wait_queue_head_t *q, wait_queue_t *wait)

wake_up_interruptible(sk->sk_sleep);
唤醒等待队列

```c
#define __wait_event(wq, condition) \
do {wait_queue \
 DEFINE_WAIT(__wait);wait_queue \
         \
 for (;;) {wait_queue \
  prepare_to_wait(&wq, &__wait, TASK_UNINTERRUPTIBLE);wait_queue\
  if (condition)wait_queue \
   break;wait_queue \
  schedule();wait_queue \ 当条件不满足的时候，就让其他进程执行。
 }wait_queue \
 finish_wait(&wq, &__wait);wait_queue \
} while (0)

#define wait_event(wq, condition) \
do {wait_queue \
 if (condition)wait_queue \ 避免被调度丢失时间片，先检查一次condition
  break;wait_queue \
 __wait_event(wq, condition);wait_queue \
} while (0)
```

##等待队列示例，用户态进程和等待队列的交互：
###以socket fs的poll函数为例
用户态调用select函数，指定fd个数、用于read、write和error的fd_set，以及timeout参数。通过该函数，当指定的fd_set中有事件发生的时候，进程能够被唤醒，并通知用户态退出阻塞状态。由于select是同步函数，所以在执行过程中会一直处于等待状态。如果超时，进程也需要能够被唤醒。这里用的是schedule\_timeout(\_\_timeout)，执行该函数后，时间片会被切走。

select传递到内核态后，首先调用sys_select，最终调用do_select执行。它初始化一个用于执行poll的等待队列事件列表，然后将列表中的等待队列事件添加到各个文件的wait_queue_head下。当事件发生时，当前进程会被唤醒。
```c
 poll_initwait(&table); table的类型为poll_wqueues。
     +-->init_poll_funcptr(&pwq->pt, __pollwait); 该函数会将poll等待队列的poll_table初始化为_poll_wait。 
        每个poll_wqueues可以存(WQUEUES_STACK_ALLOC / sizeof(struct poll_table_entry))个poll_table。
 wait = &table.pt; //poll_table *wait，
 if (!*timeout)
  wait = NULL;
```

static void __pollwait(struct file *filp, wait_queue_head_t *wait_address, poll_table *p)  
该函数首先找poll_table所对应的poll_wqeues申请一个poll_table_entry。所有entry以数组的形式存放在poll_wqueue中。
然后初始化这个poll_table_entry，并将它添加到调用该函数的wait_queue中。add_wait_queue(wait_address, &entry->wait); 现在的问题核心在于，wait_address是谁，也就是等待队列头在哪里。

mask = (*f_op->poll)(file, retval ? NULL : wait); 遍历传入的所有fd，得到fd的file参数，执行该函数。通过mask判断，是否有事件发生。
在tcp_poll中，将前面申请的poll_table添加到文件的sk_sleep链表头下。

poll_wait(file, sk->sk_sleep, wait);


