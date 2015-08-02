#RCU

RCU的基本理念，将析构操作拆分为两个部分，首先阻止新的使用者看到待析构指针，然后对该指针实行析构操作。按照RCU的操作逻辑，要改变一个指针的地址，首先不让操作者可以看到这个指针，然后重新赋值。需要注意的是，在第一步中，只是新的使用者看不到待析构指针了，但是这个指针原本的使用者还是可以继续用。基于这个理由，如果只是读操作，不需要理会这个指针是否最新，不需要同步原语，这样将会提高效率。reader在检测到rcu_read_unlock后，或者cpu core进入idle状况，或者切换到user mode后，就可以释放这个指针。无论是read还是write，被RCU保护的临界代码不能睡眠，这一点和spinlock类似。被RCU读操作者释放掉的指针，需要有回收机制。该工作可以在softirq中完成，也在进程切换时完成。

CONFIG_TREE_PREEMPT_RCU是用于实时操作系统中的RCU。

##应用场景1，链表，只有add/delete操作，没有原地update的操作。
适用于读操作很多的list_head。 RCU的宏本身已经考虑到了memory barriers，使用者不再需要考虑。  
1.  将原有的read_lock/read_unlock替换为rcu_read_lock()/rcu_read_unlock()
2.  将list_for_each替换为list_for_each_rcu
3.  将write_lock/write_unlock去掉，并使用list_add_rcu和list_del_rcu。使用spinlock保护临界区域。

##应用场景2，链表，原地更新
RCU的逻辑，为待更新的指针创建一份拷贝，在该拷贝中更新，完成之后再替换原有的指针。在更新过程中，允许reader同步操作原有指针。如下是在listRCU.txt中的一份实例。涉及到list_replace_rcu和call_rcu两个函数。同样，在更新操作前，需要使用spinlock或其他锁同步。  
```c
list_for_each_entry(e, list, list) {
	if (!audit_compare_rule(rule, &e->rule)) {
		ne = kmalloc(sizeof(*entry), GFP_ATOMIC);
		if (ne == NULL)
			return -ENOMEM;
		audit_copy_rule(&ne->rule, &e->rule);
		ne->rule.action = newaction;
		ne->rule.file_count = newfield_count;
		list_replace_rcu(&e->list, &ne->list);
		call_rcu(&e->rcu, audit_free_rule);
		return 0;
	}
```

##应用场景3，链表，消除脏数据
在应用场景2中，在更新过程中，原有的reader可能同时在操作原有指针。但是在某些场景下这是无法忍受的。从RCU本身的逻辑，无法处理这种场景。但是可以使用其他方式规避。一种方法，在链表条目中加入deleted字段标记当前条目的状况。当删除条目的时候，在spinlock的保护下更新这个字段。而当reader在RCU的两个步骤之间尝试去read这个条目，按照RCU的逻辑，可以正常读取，但是reader却可以检查deleted，来获取该条目的真实状况，从而决定是否采信该条目。。在listRCU.txt的example 3给出了一个实例。

需要注意，在上述过程中，如果涉及到写操作，都需要使用同步机制保护，比如用spinlock保护。这是因为虽然rcu lock能够保护读和写不冲突，但如果有多个写操作同时发生，仍然无能为力。

##应用场景4，保护单个指针
在上述rcu list的底层实现中，必然是基于对单个指针的保护实现，然后辅以list原本的操作包装。但有的时候，可以使用rcu lock保护一些全局的指针变量。比如某些驱动模块初始化时会使用全局指针指向起数据模块，而一旦初始化好后就很少再更新。此种情况可以使用rcu lock保护是合适的。

关键的api包括：
```c
entries = rcu_dereference(ids->entries);
rcu_assign_pointer()
```
这两个接口实际上是指针赋值，但是加上了内存屏障。这两个接口都需要在rcu_read_lock()和rcu_read_unlock()保护下。


#关键接口：
##rcu_read_lock()/rcu_read_unlock()
前面提到，reader必须一次执行完读操作，在此过程中不能休眠，也不能切换到用户态，所以在rcu_read_lock()函数中需要禁用抢占。preempt_disable()或local_bh_disable()。该函数的目的是保护临界数据只读，最可靠的是从汇编指令的层面，保障当rcu_read_lock保护的临界数据只读，但这不具备可操作性。首先，编译器需要能够识别出临界数据，局部变量放在栈中不是临界数据，入参不一定是临界数据，全局变量是临界数据，编译器没有统一的规则去判断哪些数据是临界数据。其次，即使编译器能够识别，也需要以汇编指令配置cpu，保护某些内存不可写。目前只能由MMU控制某个page是只读的，而无法控制某几个字节只读。另外，配置cpu也需要额外的指令，将会降低cpu的性能，从而失去RCU免锁算法的初衷。所以，进入读保护区域，是否真的只有读操作，我理解，应该由程序员去保证，编译器检查告警。rcu_read_lock真正的操作，只需要禁用抢占，防止被抢占后，写操作进入这段临界区域代码。因为要进入抢占，所以进入的时间应该尽量的短。

##synchronize_rcu()
阻塞写操作。

##call_rcu(struct rcu_head*, func) 
call_rcu(struct rcu_head*, func) 写者在使用新指针完成对旧的指针的替换的时候，使用call_rcu通知其他的reader，在他们完成read操作之时，需要调用析构函数完成对指针的释放。
```c
	list_replace_rcu(&e->list, &ne->list);
	call_rcu(&e->rcu, audit_free_rule);
```

内核中有很多的结构体，需要使用专门的析构函数去释放。比如网络子系统中的sk_buff。在RCU中，当writer使用新的指针替换旧指针的时候，原有的reader仍然在使用旧的指针。使用完毕后，由kernel在进程切换/切换到用户态/cpu idle时负责释放这些指针。也就是说，writer在指针替换的时候，需要将析构函数以某种方式通知kernel，否则kernel就不会直到怎么去释放。这种通知机制由rcu_call(struct rcu_head*, func)来完成。实际的实现，参考rcupdate.h中对rcu_call的注释。kernel在一个full grace period释放后去出发该析构函数。因为按照RCU的设计，最多一个full grace period，reader就应该完成对旧的指针的操作。这个full grace period，可以是一个HZ。

rcu_call()功能，包括两个部分，一是将析构函数注册到kernel，二是kernel在合适的时机去调用。需要注意，可能cpu A注册析构函数，而析构函数则由cpu B调用。在这种情况下，就需要有内存屏障，保障将cache刷到memory中。

注册过程：
```c
#define rcu_head callback_head
rcu_call(head, func) 
    +--> __call_rcu(head, func, &rcu_preempt_state, -1, 0);
        +--> __call_rcu_core(rsp, rdp, head, flags);
            +--> invoke_rcu_core()   rcu_is_cpu_idle()以及cpu online
                +--> raise_softirq(RCU_SOFTIRQ);   触发专用的RCU softirq，需要禁用中断
                    +--> raise_softirq_irqoff(nr)  
                        +--> __raise_softirq_irqoff(nr);
                            +--> or_softirq_pending(1UL << nr);  local_softirq_pending |= (x)
                        +--> wakeup_softirqd();  需要检查是否在硬中断/软中断/抢占上下文。 in_interrupt()
open_softirq(RCU_SOFTIRQ, rcu_process_callbacks);
    +--> invoke_rcu_callbacks(rsp, rdp);
        +--> rcu_do_batch(rsp, rdp);
            +--> __rcu_reclaim(rsp->name, list)
                +--> head->func(head);
```

#参考资料：
1. [Linux 2.6内核中新的锁机制--RCU](http://www.ibm.com/developerworks/cn/linux/l-rcu/)  from IBM developerworks


