#进程间通信
进程间通信，通俗讲，就是进程A告诉进程B一个事情。这个事情，可以是进程的业务状态变化，也可以一段数据。需要强调，因为进程的调度，进程A和进程B是同时在运行的。进程B对进程A的任意假设都是不可靠的。虽然站在cpu的角度，进程的执行是串行的。但站在用户的角度，进程的执行就是并行的。更何况，在多核环境中，A和B同时运行的概率更高。要深刻意识到这一点，否则在开发过程中就会犯错。比如当年调试的ramdump fail的issue，没有考虑到debuggerd是否已经将其状态写入到property中，就开始去读，导致出错。

最简单的一种进程间通讯，就是进程A在全局文件或全局property中写一个值，然后进程B去反复地读，从而得知进程A的状态变化。

##1. 管道和具名管道
###概念
管道（Pipe）及有名管道（named pipe）：管道可用于具有亲缘关系进程间的通信，有名管道克服了管道没有名字的限制，因此，除具有管道所具有的功能外，它还允许无亲缘关系进程间的通信；

管道两端可分别用描述字fd[0]以及fd[1]来描述，需要注意的是，管道的两端是固定了任务的。即一端只能用于读，由描述字fd[0]表示，称其为管道读端。且读端需要关闭用于写的fd；另一端则只能用于写，由描述字fd[1]来表示，称其为管道写端。写端需要关闭用于读的fd。如果试图从管道写端读取数据，或者向管道读端写入数据都将导致错误发生。一般文件的I/O函数都可以用于管道，如close、read、write等等。

由于pipe的缓存区大小有限，因此是有可能因为写满，导致数据丢失的。实际操作中，如果缓存区满，则进程会阻塞。（为什么不返回实际写入的字符数，然后由应用程序判断是否继续写入呢？我觉得效果一致，写进程会不停尝试去写。还不如由系统调用本身完成这个事情）

###关键接口函数：
int pipe(int fd[2]) ; 创建管道后，fork子进程，父子进程可实现通讯。
int mkfifo(const char * pathname, mode_t mode)  具名管道创建后，在读和写的进程中分别open这个管道，并设置flag为O_RDONLY或者O_WRONLY。管道只需要创建一次。
FILE *popen(const char *command, const char *type);  实用/bin/sh –c执行command的命令，创建子进程，打开管道。type只能选择为”r”或者”w”。

###管道的局限性：
管道的主要局限性正体现在它的特点上：
* 只支持单向数据流；
* 只能用于具有亲缘关系的进程之间；
* 没有名字；
* 管道的缓冲区是有限的（管道制存在于内存中，在管道创建时，为缓冲区分配一个页面大小）；
* 管道所传送的是无格式字节流，这就要求管道的读出方和写入方必须事先约定好数据的格式，比如多少字节算作一个消息（或命令、或记录）等等； 

##2. 信号
信号是比较复杂的通信方式，用于通知接受进程有某种事件发生，除了用于进程间通信外，进程还可以发送信号给进程本身；linux除了支持Unix早期信号语义函数sigal外，还支持语义符合Posix.1标准的信号函数sigaction（实际上，该函数是基于BSD的，BSD为了实现可靠信号机制，又能够统一对外接口，用sigaction函数重新实现了signal函数）；

不可靠信号，信号值小于SIGRTMIN。每次信号处理函数结尾都需要实用signal重新安装信号处理函数，且信号有可能丢失。

可靠信号，信号值在SIGRTMIN和SIGRTMAX之间，支持在发送信号的时候带参数。发送信号的主要函数有：kill()、raise()、 sigqueue()、alarm()、setitimer()以及abort()。

###关键接口：
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);  改变信号的默认处理函数  
int sigqueue(pid_t pid, int sig, const union sigval value);  向指定进程发送一个信号，value是枚举值参数。  
int sigemptyset(sigset_t *set);  初始化  initializes the signal set given by set to empty, with all signals excluded from the set  
 
 
##3. 消息队列
消息队列是消息的链接表，包括Posix消息队列system V消息队列。有足够权限的进程可以向队列中添加消息，被赋予读权限的进程则可以读走队列中的消息。消息队列克服了信号承载信息量少，管道只能承载无格式字节流以及缓冲区大小受限等缺点。

消息队列和具名管道有一定类似，都基于一个通信双方可见的path创建通道，从而保证双方可知。也即通信双方需要在相同的namespace中，可以有相同的身份标志，比如path，端口号，property，字符设备文件等。

###关键接口：
key_t ftok (char*pathname, char proj)；返回与路径pathname相对应的一个键值。  
int msgget(key_t key, int msgflg)；返回和key关联的msgqueue的id  
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);  
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);   从msgqid指定的消息队列中读或写消息。使用msgtyp，可以控制操作的msg类型，反映为优先级。
int msgctl(int msqid, int cmd, struct msqid_ds *buf);  控制消息队列，读设置消息队列的状态。  

##4. 共享内存

使得多个进程可以访问同一块内存空间，是最快的可用IPC形式。是针对其他通信机制运行效率较低而设计的。往往与其它通信机制，如信号量结合使用，来达到进程间的同步及互斥。mmap()系统调用使得进程之间通过映射同一个普通文件实现共享内存。普通文件被映射到进程地址空间后，进程可以向访问普通内存一样对文件进行访问，不必再调用read()，write（）等操作。
```c
void* mmap ( void * addr , size_t len , int prot , int flags , int fd , off_t offset ) 将fd对应的文件映射到用户态，便于直接操作。proto表示可读写状态，flags表示MAP_SHARED或MAP_PRIVATE等。特别的，这个文件，可以是/dev/mem，可以直接操作内存设备。  
int msync ( void * addr , size_t len, int flags)  如果映射的是一个普通文件，只有调用该函数，才会将内存中的数据同步到磁盘中。  
```

##5. 信号量
主要作为进程间以及同一进程不同线程之间的同步手段。
```
key_t ftok (char*pathname, char proj)；和消息队列相同的接口，获取path对应的键值
int semget(key_t key, int nsems, int semflg)  返回和key关联的信号集的id
int semctl(int semid, int semnum, int cmd, ...);  控制semid对应的信号集
```

##6. 套接字
更为一般的进程间通信机制，可用于不同机器之间的进程间通信。起初是由Unix系统的BSD分支开发出来的，但现在一般可以移植到其它类Unix系统上：Linux和System V的变种都支持套接字。

基于Unix通信域（调用socket时指定通信域为PF_LOCAL即可）的套接口可以实现单机之间的进程间通信。采用Unix通信域套接口有几个好处：Unix通信域套接口通常是TCP套接口速度的两倍；另一个好处是，通过Unix通信域套接口可以实现在进程间传递描述字。

套接字通信，包括以下过程：  
1. 使用socket()创建一个套接字。指定AF_INET/AF_INET6/AF_UNIX，指定type为SOCK_STREAM，或者SOCK_DGRAM。
2. 使用bind()将套接字绑定到一个地址上。对于SOCK_STREAM类型的socket，在服务器和客户端，都需要绑定操作。而无连接的socket，则只需要在服务器端绑定地址，客户端的地址在connect()时动态分配。
3. connect()，有连接的socket，此时需要握手。无连接的socket，只是设置一下地址，避免每次都设置。
4. listen()  监听是否有新的连接到来。由于它只能监听一个socket，也可以被select替换。
5. accept() 服务器端调用该函数后阻塞，等待客户端的连接。如果有连接，则返回一个表示新的连接的fd，用于客户端和服务器的通信。在客户端无须调用该函数。
6. select()  监听多个fd上是否有r/w/e发生。

##Open binder
在Android中实用的进程间通讯方式。

##各种进程间通信方式优劣比较




 

 
