将操作延期的一种手段，在内核守护线程上下文执行，与内核无关。

创建工作队列的时候，alloc_workqueue，可以在所有的cpu上创建一个worker线程，也可以只在某一个cpu上创建worker线程，由它来执行实际的操作。这是灵活所在，但是，也是局限。如果支持在多个cpu上运行，那么就得考虑同步问题。如果只在一个cpu上运行，那就要考虑负载的问题。所以还不如向博通那样，另外再起一个内核线程。虽然起一个内核线程的操作要复杂得多。

##数据结构
```c
struct work_struct {
 atomic_long_t data;       //指向执行func时需要的数据，atomic，可以原子操作。
 struct list_head entry;    //可以将多个任务链接在一起
 work_func_t func;         //执行函数
};
typedef void (*work_func_t)(struct work_struct *work);
```

##操作函数
queue_work(struct workqueue_struct *wq, struct work_struct *work)   //向等待队中添加任务
