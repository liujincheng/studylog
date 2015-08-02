摘要：  
介绍内核线程的主要api，以及使用场景。  
1. 生命周期，创建、删除等，以及和创建者之间的交互。
2. 堆栈分析。如何查看一个内核线程的栈。栈是如何生成的。

---
##主要api：
```c
long kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)  创建线程，并返回pid。使用daemonize修改线程的名称。
task_struct *kthread_create(fn, data, flag)   创建内核线程，返回task_struct，需要手动wake_up_process
#define kthread_run(threadfn, data, namefmt, ...)   类似kthread_create，自动wake_up_process
```

##内核线程的创建流程
```c
kthread_run创建线程，并自动开始运行 
    |-->kthread_create, 继而调用kthread_create_on_node(threadfn, data, -1, namefmt, ##arg)  在指定node上创建内核线程
        |-->构建内核线程信息对象create，并添加到全局链表中list_add_tail(&create.list, &kthread_create_list);
        |-->wake_up_process(kthreadd_task) 唤醒创建内核线程的守护线程kthreadd
        |-->wait_for_completion(&create.done)  初始化完成量，等待kthreadd完成内核创建。在线程创建后，complete(&create->done);通知线程创建成功。
        |-->return create.result 返回内核线程的task_struct结构体。
    |-->wake_up_process(__k) 唤醒内核线程
```

为内核线程创建栈时，应该不会使用全新的page，而是以当前线程为基础创建。把完整的代码拷贝过来，需要注意spin_lock和线程的状态。
```c
int kthreadd(void *unused)  用于创建内核线程的函数
{
 struct task_struct *tsk = current;

/*为继承该线程的子线程创建一个干净的上下文环境。*/
 set_task_comm(tsk, "kthreadd");  //设置tsk->comm，修改线程的名称。
 ignore_signals(tsk);   //修改线程所有的信号处理函数SIG_IGN。不接收任何信号。因为在内核的原因？
 set_cpus_allowed_ptr(tsk, cpu_all_mask);    //设置该线程可以在哪些cpu上被执行到
 set_mems_allowed(node_states[N_HIGH_MEMORY]);   //设置该县城可以使用的内存区域。

 current->flags |= PF_NOFREEZE;

 for (;;) {  循环，该线程永不退出。
  set_current_state(TASK_INTERRUPTIBLE);    //可中断状态。
  if (list_empty(&kthread_create_list))    //只判断链表是否为空，不涉及链表操作，不需要加spin_lock
   schedule();
  __set_current_state(TASK_RUNNING);   //运行状态。如果被打断后，可以继续回来执行。

  spin_lock(&kthread_create_lock);
  while (!list_empty(&kthread_create_list)) {   
   struct kthread_create_info *create;

   create = list_entry(kthread_create_list.next,
         struct kthread_create_info, list);
   list_del_init(&create->list);
   spin_unlock(&kthread_create_lock);

   create_kthread(create);  //实际的线程创建函数。

   spin_lock(&kthread_create_lock);
  }
  spin_unlock(&kthread_create_lock);
 }

 return 0;
}
//在kthread函数中调用线程操作函数fn（存放在create对象中）。这里只是创建线程，需要手动wakeup
 pid = kernel_thread(kthread, create, CLONE_FS | CLONE_FILES | SIGCHLD);  
```


