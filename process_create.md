##数据结构
进程的管理结构包括几部分，一是task_struct，二是thread_info，三是stack, 四是pt_regs, 存放寄存器。其中，thread_info放与硬件上下文切换有关的数据，一般由arch相关代码初始化, 用于entry.S进程切换等。stack是进程在kernel的调用栈。一般thread和stack共用page。task_struct是kernel管理进程的结构。

下面这段代码大致描述了这些数据结构的初始化流程。在fork进程时，需要copy_process中创建该进程的task_struct和栈。可以看出，一个进程的task_struct可以到相应的slab中去搜索，而它的栈则是有alloc_pages_node随机分配的。
```c
copy_process()
    +--> p = dup_task_struct(current);
          +--> tsk = alloc_task_struct_node(node);   在cachep中申请task_struct
          +--> ti = alloc_thread_info_node(tsk, node);  使用alloc_pages_node分配0x8192字节。task->stack=ti
```

###[fork与vfork的区别](http://blog.csdn.net/jianchi88/article/details/6985326)
fork（）与vfock（）都是创建一个进程，那他们有什么区别呢？总结有以下三点区别： 
1. fork  （）：子进程拷贝父进程的数据段，代码段  
    vfork （ ）：子进程与父进程共享数据段 
2. fork （）父子进程的执行次序不确定  
    vfork 保证子进程先运行，在调用exec 或exit 之前与父进程数据是共享的,在它调用exec或exit 之后父进程才可能被调度运行。 
3. vfork （）保证子进程先运行，在她调用exec 或exit 之后父进程才可能被调度运行。如果在调用这两个函数之前子进程依赖于父进程的进一步动作，则会导致死锁。 


