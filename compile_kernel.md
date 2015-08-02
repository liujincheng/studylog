#编译一个内核模块
可以使用内核模块的方式加速调试。

1） 在host主机上以内核的形式编译
```shell
make -C /usr/src/kernels/3.15.10-200.fc20.x86_64 M=`pwd` modules
```
-C  指明内核源码所在的目录。如果编译机上没有，需要去网上下载源码或者使用yum下载。源码根目录下需要由内核的Makefile。
-M 告诉内核的源码，需要编译的模块的路径。该路径需要包含一个Makefile，指明该模块需要的.o文件。一个最简单的Makefile如下所示。
```makefile
obj-m := test.o
```
obj-m，告诉编译系统以模块的形式编译，test.o，告诉该模块对应的.o文件名称。需要注意，该名称需要和所在的文件夹名称相同，否则会报错误。
如果编译涉及到对个文件，可以使用如下方式：
```makefile
obj-m := test.o
test-objs ：= hello.o
```

每次编译时，都输入这么一长串命令，也够累的。可以把这个活交给Makefile来做。可通过KERNELRELEASE宏判断是否通过第二次进入该Makefile。
```makefile
KERNELDIR ?= /usr/src/kernels/3.15.10-200.fc20.x86_64
PWD := $(shell pwd)

ifneq ($(KERNELRELEASE),)
 obj-m := test.o #无论是否有tab，都可以执行。
 test-objs := hello.o
endif

default:
 $(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
 $(MAKE) -C $(KERNELDIR) M=$(PWD) clean
```

2. 在target单板的编译环境中编译出模块
博通现有SDK编译框架中，定义了大量的默认编译目标，导致编译时间较长。希望修改，可以达到快速编译的目的。

由于博通SDK中定义了大量的宏变量，且为target单板编译模块，需要使用到交叉工具链，所以需要修改现有的编译框架。最初的打算，是修改内核源码Makefile，在modules的基础上，新增一个modules编译目标。但由于有很多目标是默认编译的，所以效果不明显。

之后修改SDK总的Makefile，新增一个testmod目标，即可完成任务。so easy。

```makefile
testmod:
 make C=$(KERNEL_DIR) M=/home/liu/src/416L02/test modules
```
