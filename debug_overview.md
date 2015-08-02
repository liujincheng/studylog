##Hacks4 获取coredump信息
ulimit是一个可以获取和设置进程线程的程序。在对设置某选项时，需要制定参数。get和set使用相同命令，区别在于是否带有limit参数。-c表示读取与设置core文件大小。可对core文件压缩，也可以控制不产生共享链接库的core文件已减少core文件的体积。

CTips 如何查看堆栈信息

在glibc头文件"execinfo.h"中声明了三个函数用于获取当前线程的函数调用堆栈。
```c
int backtrace(void **buffer,int size)
char ** backtrace_symbols (void *const *buffer, int size)
void backtrace_symbols_fd (void *const *buffer, int size, int fd) 
```

##Hacks5~10 GDB基础，打断点，查看栈帧，寄存器，变量
函数调用时，需要将返回地址、参数、上层FP保留在栈中，然后发射call指令跳转，再更新ebp栈顶指针。函数的返回值，保留在eax，然后ret指令回去后，再读这个寄存器。返回值并不会压栈。

if向下跳转

while向上跳转

compl指令，比较，如果false，则跳转到下

##ramdump
针对kernel issue，使用pc和lr定义出错异常点，使用sp找到出错的堆栈，理清上下文，并可向上反推，理清调用者的上下文等信息。
针对user issue，理论上也可以反推每个进程的堆栈，但实际上很难操作。因为user态的最后一条是系统调用，即使往上推，肯定也是工具链库函数。可以尝试使用crash提供的search函数，找到关键点。或者找到进程的mm，找到进程的虚拟地址，然后将该进程的所有memory打印出来。

