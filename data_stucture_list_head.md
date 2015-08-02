内核的通用双向链表，与业务无关，可以嵌套在任意数据结构中。

##接口api：
static LIST_HEAD(kthread_create_list);
list_empty(&kthread_create_list)  检测链表是否为空。
list_entry(kthread_create_list.next,  struct kthread_create_info, list)  取出链表的第一个对象，强制类型转换为struct kthread_create_info
list_add_tail(&create.list, &kthread_create_list)   向链表的尾部添加对象。

##原理分析：
```c
#define container_of(ptr, type, member) ({             /
         const typeof( ((type *)0)->member ) *__mptr = (ptr);     /
         (type *)( (char *)__mptr - offsetof(type,member) );})
container_of的作用在于，给定某结构体中字段的地址，能够获取该结构体的起始地址。
```

##演示代码：
```c
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#define container_of(ptr, type, member) ({             /
         const typeof( ((type *)0)->member ) *__mptr = (ptr);     /
         (type *)( (char *)__mptr - offsetof(type,member) );})
struct test1
{
    char *pchar ;
    int idata ;
    int *pint ;
};
 
int main()
{
    struct test1 ptest1 ;
    int *p1 = ptest1.pint ;
    printf("the address of ptest1 is %p/n", &ptest1) ;
    printf("the address of p1 is %p/n", p1) ;
    ptest1.pint = 0x55555555 ;
    printf("offsetof(struct test1, pint)=%d/n", offsetof(struct test1, pint)) ;
    printf("container_of(p1, struct test1, pint) = %p/n", container_of(p1, struct test1, pint)) ;  //1
    printf("containerof(ptest1.pint, struct test1, pint)=%p/n", container_of(&ptest1.pint, struct test1, pint)) ;  //2
    printf("containerof(ptest1.pint, struct test1, pint)=%p/n", container_of(ptest1.pint, struct test1, pint)) ;    //3
    return 0 ;
}
``` 
##运行结果
```
the address of ptest1 is 0xbfe5ce38
the address of p1 is 0xb7faeff4
offsetof(struct test1, pint)=8
container_of(p1, struct test1, pint) = 0xb7faefec      //p1的地址往前移8个字节
containerof(&ptest1.pint, struct test1, pint)=0xbfe5ce38    //使用&ptest1.pint作为第一个参数，获得ptest1的地址
containerof(ptest1.pint, struct test1, pint)=0x5555554d    //使用ptest1.pint做第一个参数，只是在该地址的基础上前移8个字节
```

