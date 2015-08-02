#内核同步机制概述
主要介绍内核中的锁，用户态的锁，以及进程间通信的实现方法。  
多个线程、多个进程、或多个处理器、或单个处理器但支持抢占的情况下，进入临界区，多个处理单元同时执行一段代码操作全局或共用数据，导致数据不一致，引发异常。其核心原因在于，cpu操作数据指令不是原子的。可能在C语言看来，只有一句代码，但是转化为汇编，则需要几句指令配合执行。因此如果多个使用者同时（对单个cpu，实质是抢占）执行该行代码，则发生同步问题。
* A处理器删除了链表中的一个节点，但B处理器不知情，数据不同步。
* A进程对一个字段做算数写操作，被调度，B进程再来写这个字段。导致异常。  

出现竟态的原因：处理器执行指令的粒度很小，哪怕一个最简单的i++，也需要分成三个步骤：loadw，ALU、savew。如果在执行过程中被打断，就会出现数据错误。比数据异常更严重的，在操作链表或指针的时候，A处理器已经释放掉指针，但B处理器不知情，有可能会对空指针操作，导致系统挂死。

内核中的锁机制，需要保证最大程度的效率。由于内核对锁的需求是多样的，所以单纯的一种锁无法在性能上满足所有场景的需求，所以发展出多种锁。


##1. 原子操作  atomic_t
由处理器提供原子操作指令，完成对原子变量加1等操作。有处理器保证其原子性。所以这个锁是依赖于处理器架构的。不过目前的处理器大都已提供了该指令。相关的操作接口包括
```
atomic_read(atomic_t *v)
atomic_set(atomic_t *v, int i)
atomic_inc(atomic_t *v)
atomic_dec(atomic_t *v)
```

##2. per-CPU变量。
前面提到，多个cpu同时操作一个共用的数据，导致发生数据同步异常。一个改进，是避免使用共用数据，为每个cpu分配一块私有的数据区，保持物理上的隔离。一个典型的应用场景，记录网络模块的收发包统计数据。各个cpu单独计数，只有当有api查询总的统计数据时，才汇总各个cpu的数据。  
kernel中实现使用per-cpu标记私有数据，kernel编译期间会将声明的这些per-cpu变量放在一块单独的section中。用户不用考虑cpu的个数，因为系统初始化时会根据cpu的个数来为各个cpu分配私有数据资源。理论上，只要系统在为per-cpu分配内存时，除了编译期间根据per-cpu声明计算出来的占用内存大小，再多分配一些，也可以做到per-cpu的动态分配。但因为这样做，很难控制多预留出来的内存的大小，将会增加设计上的复杂度，因此kernel实际上有没有实现per-cpu变量的动态分配，尚未可知。  
若需要操作某个cpu的私有数据，需要使用专有的接口，并指定cpu的id。  

##3. 信号量。
类似于用户态的信号量，进程在尝试进入一段已被加锁的临界区，会被阻塞，并加入到阻塞队列中，直到之前的进程从临界区出来，才会从等待队列中挑选一个进程唤醒。由于需要加入等待队列，唤醒，这种锁的开销是比较大的，适合与对一段较大的代码加锁。
```c
struct semphore{
    atomic_t count ;  //允许进入临界区的进程的数目，一般为1，用于实现互斥。进入临界区down(&mutex)，该值减1为0，其他进程就不能再进入了。从临界区出来前，up(&mutex)，该值增加1为1，其他进程可以再次进入该进程。
    int sleepers ;        //允许等待进入临界区的进程的数目
    wait_queue_head_t wait_list ;    //等待队列。进入等待队列后，进程进入TASK_UNINTERRUPTIBLE状态，无法被其他信号唤醒。
}
```
信号量可以动态申请，或者声明
> DECLARE_MUTEX(mux) 申请一个信号量变量。    

一个典型示例
```c
struct semaphore semMibShared;
sema_init( &semMibShared,1);  也可以初始化为0
up(&mutex); 
down(&mutex)
down_interruptible(&mutex)  可以被其他信号唤醒。通过返回值区分实际被唤醒的原因。
down_trylock(&mutex)  尝试返回，如果失败，继续等待。使用该接口，可以避免进程进入等待状态。
```

##4. 互斥量
和信号量类似，但不使用等待队列阻塞进程，只是将等待进程加入到等待队列中。注意到它数据结构中的定义，mutex使用spin lock，而semphore没有。可以猜想，当使用信号量保护的数据已经被持有时，会使用spic lock开始忙等待。这里wait_list的作用待分析。    
优先级反转：低优先级进程占有互斥量，高优先级进程等待的优先级队列执行。
```c
struct mutex{
    atomic_t count;
    spinlock_t  wait_lock ;
    struct list_head  wait_list ;
}
```
典型接口：
```c
DEFINE_MUTEX(mux)定义一个互斥量，
mutex_lock
mutex_unlock
```

##5. 自旋锁
当只有较少的几行代码处理临界代码区，需要被保护，如果使用信号量，则有点不划算。与其将被阻塞的调用者加入到等待队列，不如让它空转一小会，也就几个指令的功夫，当前使用者就从临界代码区退出了。这种让处理器空转等待的锁机制，就是spinlock。  
注意，如果相同的使用者，两次对一个自旋锁加锁，将会很危险，发生死锁。  
```c 
static DEFINE_SPINLOCK(lock);  定义自旋锁
spin_lock(&lock)/spin_unlock(&lock)，普通的自旋锁等待。
spin_lock_irqsave()  获取自旋锁，并且停掉本处理器的硬件中断。。相应的释放接口是spin_unlock_irqsave()
spin_lock_bh()  获取自旋锁，停掉本地处理器的软件中断，即softIRQ。相应的释放函数为spin_unlock_bh()
spin_trylock()
spin_trylock_bh()
```
在SMP中，spinlock可以让处理器空转。而在单处理器中，如果不支持内核抢占，那么相关接口为空。如果支持抢占，那么该接口其实等同于preempt_disable和preempt_enable。

##5. rcu机制
rcu机制，reay-copy-update，支持并发的读写。如果使用者要尝试修改被rcu保护的数据结构，那么首先需要创建一个副本，用于修改，其他使用者结束对旧的数据结构修改后，修改指针，指向修改后的副本。
该机制主要针对指针特别有效，进一步，可被广泛用在list_head，hlist_head中。相关接口为list_add_rcu()，list_add_tail_rcu()

使用场景：
进程维护一个数组，保存它所打开得fd。每个进程可以打开的fd可以很大，比如4096。但是绝大多数的fd都不会这么大。所以最好的方式是运行期间动态扩展，且不影响原有fd的使用。最适合使用rcu机制。
```c
rcu_read_lock()
p = rcu_reference(ptr) ;  if(p == NULL) ...
list_for_each_entry_rcu(answer, &inetsw[sock->type], list) 
rcu_read_unlock
rcu_assign_pointer()。修改指针的对象
```

##6. 读写锁
任意进程都可以读数据结构，但只有一个进程可以写数据结构。数据结构类型为rwlock_t，支持_irq_irqsave，_bh后缀
```c
 read_lock(&nl_table_lock);
 atomic_inc(&nl_table_users);
 read_unlock(&nl_table_lock);
```

##7. 内存屏障
编译器过度优化，或者是处理器乱序发射指令，都有可能导致时序混乱。如果编译器和处理器足够智能，应该不会出现这种情况。但这毕竟是理想状态。
mb(), rmb(), wmb()，保障屏障之前的所有指令的读或写或读写操作都已经完成。也就是让编译器优化的时候，不要将读或写或读写指令放到屏障之后。
barriar(), 插入该屏障之后的代码，编译器或处理器都不要做优化。用于preempt_disable()和preempt_enable中。内核使用者看不到屏障，但却是存在的。  

其他还包括smp_mb(), read_barriar_depends()等接口.
















