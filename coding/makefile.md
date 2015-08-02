device/htc/common/prebuilt-subsys-images.mk  
在makefile中使用for循环，定义函数，调用函数的方法  
```makefile
include $(CLEAR_VARS)  
  
PREBUILT_PATH := $(strip $(wildcard \  
 $(HTC_PREBUILT_SUBSYS_PATH) \  
 $(TARGET_DEVICE_DIR)/prebuilt_images \  
 $(HTC_MAIN_PROJECT_PREBUILT_SUBSYS_PATH) \  
 device/htc/prebuilts/$(TARGET_BOARD_PLATFORM)))  
  
LOCAL_MODULE := htc-subsys-images  
  
IMG := \  
 *.img \  
  
IMG_OUT := $(PRODUCT_OUT)  
IMG_RESUFFIX :=  
  
SYM := \  
 *.elf \  
 *.map \  
 *.sym \  
  
SYM_OUT := $(PRODUCT_OUT)/symbols/subsys  
SYM_RESUFFIX :=  
  
_local_images :=  
define htc-copy-subsys-image  
$(foreach _path,$(PREBUILT_PATH), \  
$(foreach _src,$(wildcard $(addprefix $(_path)/,$(1))), \  
 $(eval _dst := $(addprefix $(2)/,$(notdir $(_src)))) \  
 $(eval _dst := $(if $(3),$(basename $(_dst)).$(3),$(_dst))) \  
 $(eval $(if $(filter $(_dst),$(_local_images)),, \  
  $(eval $(call copy-one-file,$(_src),$(_dst))) \  
  $(eval _local_images += $(_dst)) \  
  $(eval $(if $(filter true,$(4)), \  
   $(eval INSTALLED_RADIOIMAGE_TARGET += $(_dst)),)) \  
  $(eval $(LOCAL_MODULE): $(_dst)) \  
 )) \  
))  
endef  
  
$(call htc-copy-subsys-image,$(IMG),$(IMG_OUT),$(IMG_RESUFFIX),true)  
$(call htc-copy-subsys-image,$(SYM),$(SYM_OUT),$(SYM_RESUFFIX),false)  
  
.PHONY: $(LOCAL_MODULE)  
ALL_DEFAULT_INSTALLED_MODULES += $(LOCAL_MODULE)  
``` 
  
内核编译时，如何编译单个的文件？  
可以使用grep，获取编译单个文件的具体命令。  
  
-n 参数可以让我们观察到底会执行些什么命令行指令，而不真的去执行它们。这对调试复杂的 Makefile 非常有用。  
  
$@ target  
$< 第一个依赖  
$^ 所有依赖  
$* target，去掉后缀名  
  
VPATH 指定文件搜索路径  
vpath %.c src ，使用pattern去搜索。所有的.c文件都到src目录下面去找。  
  
%o:%c >>>>>>通配符  
 gcc -o $@ $<  
$(OBJFILES):%o:%c: >>>>>>通配符,只匹配$(OBJFILES)中的文件  
 gcc -o $@ $<  
  
:= 简单扩展赋值符，立即替换，有可能出现空值  
= 变量被使用时才扩展它  
?= 条件赋值。只有当左值没有赋值时，才会生效  
+= 使用:=做+=赋值。  
  
使用:=的方式定义一个函数，以后碰到这个函数时，用这些语句替换。  
define parent  
  echo "parent has two parameters: $1, $2"  
  $(call child,$1)  
endef  
  
define child  
  echo "child has one parameter: $1"  
  echo "but child can also see parent's second parameter: $2!"  
endef  
  
scoping_issue:  
        @$(call parent,one,two)  
  
built-in函数  
字符串相关  
$(filter pattern . . . ,text) 例如$(filter ui/%.o,$(objects))，在$(objects)中匹配ui/xxx.o。可以使用filter来做字符串比较。has-duplicates = $(filter $(words $1) $(words $(sort $1))))  
$(filter-out pattern . . . ,text) 和filter相反  
$(findstring string,text) 在text中查找string，如果找到，那么返回string，否则返回空。  
$(subst search-string,replace-string,text) ，例如$(subst .c,.o,$(sources))。注意该函数的执行顺序是从右向左。也就是，先处理text，再处理replace-text，最后处理search-string  
$(patsubst search-pattern,replace-pattern,text) ，支持通配符的替换函数，例如$(patsubst %/,%,$(directory-path))将会删除目录名后的斜线  
$(words text) 统计词语个数  
$(word n,text) 返回第n个词语  
$(firstword text) 返回text中的第一个词语  
$(wordlist start,end,text) ，返回从start到end的词语列表  
$(sort list) 对list中的词语排序，并去掉重复  
$(shell command) 执行外部shell命令  
文件名相关  
$(dir test)返回test的文件夹名称。source_list=$(sort $(dir $(shell find ../ -name "*")))  
$(notdir test)返回去掉路径后的文件名。如$(sort $(notdir $(shell find . -name "*")))  
$(basename test)返回去掉后缀后的文件名称，注意可能包含路径。  
$(suffix test)返回文件的后缀名  
$(addsuffix suffix, name...) 添加后缀名 $(sort $(addsuffix .bak, $(basename $(shell find . -name "*"))))将所有文件的后缀名都替换为.bak  
流控制，if、for  
$(if condition, then-part, else-part)各个部分之间用","分割。iftest=$(if $(filter $(MAKE_VERSION), 3.80), "ok", $(error wrong make version))  
$(foreach varable, list, body)各个部分用","分割。 $(foreach var, $(sort $(notdir $(shell find . -name "*.h"))), -i $(var) )  
其他  
$(strip text) 去除text文本前后的空白  
$(error text)打印出错语句。相当于$(warning text) exit 1两个语句的组合  
  
  
1. make是基于字符串来构造依赖关系，而不是基于文件。所以在写依赖关系的时候，文件名是否有前缀是很很重要的。  
2. 一个.o文件有两行依赖语句是可行的。例如下面的情况：  
basic.o: basic.c /home/sdb/liu/webd/mhttpd/include/httpd.h \  
  /home/sdb/liu/webd/mhttpd/include/ctypes.h  
%.o : %.c  
 $(CC) $(CFLAGS) -c $< -o $@  
  
include表示包含外部文件  
-include表示如果包含的外部文件不存在时不报错。  
  
  
递归调用make  
  
首先说明一下makefile的执行步骤：  
1. 读入所有的Makefile。  
2. 读入被include的其它Makefile。  
3. 初始化文件中的变量。  
4. 推导隐晦规则，并分析所有规则。  
5. 为所有的目标文件创建依赖关系链。  
6. 根据依赖关系，决定哪些目标要重新生成。  
7. 执行生成命令  
  
但是一个简单的问题就卡壳了很简单的一个问题  
  
|-- Makefile  
|-- include  
| `-- hello.h  
`-- src  
    |-- hello.cpp  
    `-- main.cpp  
  
首先是Makefile 文件和include文件夹还有src文件夹在同一个目录下  
  
头文件hello.h在include目录下  
  
源文件main.cpp和hello.cpp在src目录下  
  
////////////////////////////////  
  
hello.h:  
  
#ifndef _HELLO_H__  
  
#define _HELLO_H__  
  
void hello();  
  
#endif  
  
///////////////////////////  
  
hello.cpp:  
  
#include<iostream>  
  
#include"hello.h"  
  
using namespace std;  
  
void hello()  
  
{  
  
        cout<<"Hello world"<<endl;  
  
}  
  
/////////////////////////////  
  
main.cpp:  
  
#include<iostream>  
  
#include"hello.h"  
  
using namespace std;  
  
int main()  
  
{  
  
          hello();  
  
          return 0;  
  
}  
  
/////////////////////////////////  
  
Makfile:  
```makefile 
#VPATH=include:src  
vpath %.cpp src  
vpath %.h include  
test:main.o hello.o  
 g++ -o $@ main.o hello.o  
main.o:main.cpp hello.h  
 g++ -c $< -Iinclude  
hello.o:hello.cpp hello.h  
 g++ -c $< -Iinclude  
.PHONY:clean  
clean:  
 -rm test hello.o  
```
//////////////////////////////////  
  
因为Makefile、hello.h hello.cpp main.cpp没有在同一个路径，所以要考虑路径的问题  
  
同时，路径也有两种，一种是针对Makefile来说在执行make命令的时候，要寻找目标文件和依赖文件的路径  
  
另一个就是源文件所要包含的头文件等相关文件的路径。  
  
对于第一种来说，Makefile 提供了两种方式，一种是设置全局访问路径VAPTH:即在执行make命令时可以从该路径中查询目标和依赖make可识别一个特殊变量“VPATH”。通过变量“VPATH”可以指定依赖文件的搜索路径，  
在规则的依赖文件在当前目录不存在时，make会在此变量所指定的目录下去寻找这些依赖文件。一般我们都是用此变量来说明规则中的依赖文件的搜索路径。  
  
Makefile中所有文件的搜索路径，包括依赖文件和目标文件。  
变量“VPATH”的定义中，使用空格或者冒号（:）将多个目录分开。make 搜索的目录顺序  
按照变量“VPATH”定义中顺序进行（当前目录永远是第一搜索目录）。  
  
例如：  
  
VPATH = src:../headers  
  
它指定了两个搜索目录，“src”和“../headers”。对于规则“foo:foo.c”如果“foo.c”在“src”  
目录下，此时此规则等价于“foo:src:/foo.c”  
  
对于第二种来说：当需要为不类型的文件指定  
不同的搜索目录时需要这种方式  
  
vpath：关键字  
  
它所实现的功能和上一小节提到的“VPATH”变量很类似，但是  
它更为灵活。它可以为不同类型的文件（由文件名区分）指定不同的搜索目录。它的使用方法有三  
种  
  
1、vpath PATTERN DIRECTORIES  
为符合模式“PATTERN”的文件指定搜索目录“DIRECTORIES”。多个目录使用空格或者  
冒号（：）分开。类似上一小节的“VPATH”  
2、vpath PATTERN  
清除之前为符合模式“PATTERN”的文件设置的搜索路径  
  
3、vpath  
  
清除所有已被设置的文件搜索路径。  
  
对于vpath的详细说明待续。  
  
在执行make命令的时候，根据makefile执行步骤，首先读入所有的makefile文件，那么  
  
VPATH = include：src //指定了makefile的搜索路径  
  
或者  
  
vpath %.h include //指定.h类型文件的搜索路径是include  
  
vpath %.cpp src //指定.cpp类型文件的搜索路径是src  
  
这仅仅是对于makefile来说搜索目标和依赖文件的路径，但是对于命令行来说是无效的，也就是说  
  
在执行g++或者gcc时不会自动从VPATH 或者vpath中自动搜索要包含的头文件等信息文件  
  
此时要用到了 -I 或者--incude +路径  
  
例如依赖是：  
  
main.o:main.cpp hello.h  
  
即g++ -c $< -Iinclude,这时候，g++会自动从include目录中搜索要包含的hello.h头文件  
 
--- 
  
在Makefile中可以使用函数来处理变量，从而让我们的命令或是规则更为的灵活和具有智能。make所支持的函数也不算很多，不过已经足够我们的操作了。函数调用后，函数的返回值可以当做变量来使用。  
一、函数的调用语法  
函数调用，很像变量的使用，也是以“$”来标识的，其语法如下：  
    $(  
)  
或是  
    ${  
}  
这里，就是函数名，make支持的函数不多。是函数的参数，参数间以逗号“,”分隔，而函数名和参数之间以“空格”分隔。函数调用以“$”开头，以圆括号或花括号把函数名和参数括起。感觉很像一个变量，是不是？函数中的参数可以使用变量，为了风格的统一，函数和变量的括号最好一样，如使用“$(subst  
a,b,$(x))”这样的形式，而不是“$(subst a,b,${x})”的形式。因为统一会更清楚，也会减少一些不必要的麻烦。  
还是来看一个示例：  
    comma:= ,  
    empty:=  
    space:= $(empty) $(empty)  
    foo:= a b c  
    bar:= $(subst $(space),$(comma),$(foo))  
在这个示例中，$(comma)的值是一个逗号。$(space)使用了$(empty)定义了一个空格，$(foo)的值是“a b c”，$(bar)的定义用，调用了函数“subst”，这是一个替换函数，这个函数有三个参数，第一个参数是被替换字串，第二个参数是替换字串，第三个参数是替换操作作用的字串。这个函数也就是把$(foo)中的空格替换成逗号，所以$(bar)的值是“a,b,c”。  
二、字符串处理函数  
$(subst  
,,)  
    名称：字符串替换函数——subst。  
    功能：把字串中的字符串替换成。  
    返回：函数返回被替换过后的字符串。  
    示例：  
          
        $(subst ee,EE,feet on the street)，  
          
        把“feet on the street”中的“ee”替换成“EE”，返回结果是“fEEt  
on the strEEt”。  
$(patsubst ,,)  
    名称：模式字符串替换函数——patsubst。  
    功能：查找中的单词（单词以“空格”、“Tab”或“回车”“换行”分隔）是否符合模式，如果匹配的话，则以替换。这里，可以包括通配符“%”，表示任意长度的字串。如果中也包含“%”，那么，中的这个“%”将是中的那个“%”所代表的字串。（可以用“\”来转义，以“\%”来表示真实含义的“%”字符）  
    返回：函数返回被替换过后的字符串。  
    示例：  
         
$(patsubst %.c,%.o,x.c.c bar.c)  
        把字串“x.c.c  
bar.c”符合模式[%.c]的单词替换成[%.o]，返回结果是“x.c.o  
bar.o”  
    备注：  
        这和我们前面“变量章节”说过的相关知识有点相似。如：  
        “$(var:=)”  
         相当于  
        “$(patsubst  
,,$(var))”，  
         而“$(var:  
=)”  
         则相当于  
         “$(patsubst  
%,%,$(var))”。  
         例如有：objects = foo.o  
bar.o baz.o，  
         那么，“$(objects:.o=.c)”和“$(patsubst  
%.o,%.c,$(objects))”是一样的。  
$(strip )  
    名称：去空格函数——strip。  
    功能：去掉字串中开头和结尾的空字符。  
    返回：返回被去掉空格的字符串值。  
    示例：  
          
        $(strip a b c )  
        把字串“a b  
c ”去到开头和结尾的空格，结果是“a b c”。  
$(findstring ,)  
    名称：查找字符串函数——findstring。  
    功能：在字串中查找字串。  
    返回：如果找到，那么返回，否则返回空字符串。  
    示例：  
         
$(findstring a,a b c)  
        $(findstring a,b c)  
        第一个函数返回“a”字符串，第二个返回“”字符串（空字符串）  
$(filter  
,)  
    名称：过滤函数——filter。  
    功能：以模式过滤字符串中的单词，保留符合模式的单词。可以有多个模式。  
    返回：返回符合模式的字串。  
    示例：  
        sources  
:= foo.c bar.c baz.s ugh.h  
        foo: $(sources)  
                 
cc $(filter %.c %.s,$(sources)) -o foo  
         
$(filter %.c %.s,$(sources))返回的值是“foo.c bar.c baz.s”。  
$(filter-out  
,)  
    名称：反过滤函数——filter-out。  
    功能：以模式过滤字符串中的单词，去除符合模式的单词。可以有多个模式。  
    返回：返回不符合模式的字串。  
    示例：  
         
objects=main1.o foo.o main2.o bar.o  
        mains=main1.o main2.o  
          
        $(filter-out $(mains),$(objects)) 返回值是“foo.o  
bar.o”。  
          
$(sort )  
    名称：排序函数——sort。  
    功能：给字符串中的单词排序（升序）。  
    返回：返回排序后的字符串。  
    示例：$(sort foo bar lose)返回“bar foo lose”。  
    备注：sort函数会去掉中相同的单词。  
$(word ,)  
    名称：取单词函数——word。  
    功能：取字符串中第个单词。（从一开始）  
    返回：返回字符串中第个单词。如果比中的单词数要大，那么返回空字符串。  
    示例：$(word 2, foo bar baz)返回值是“bar”。  
$(wordlist  
,,)    
    名称：取单词串函数——wordlist。  
    功能：从字符串中取从开始到的单词串。和是一个数字。  
    返回：返回字符串中从到的单词字串。如果比中的单词数要大，那么返回空字符串。如果大于的单词数，那么返回从开始，到结束的单词串。  
    示例： $(wordlist 2, 3, foo bar baz)返回值是“bar  
baz”。  
$(words )  
    名称：单词个数统计函数——words。  
    功能：统计中字符串中的单词个数。  
    返回：返回中的单词数。  
    示例：$(words, foo bar baz)返回值是“3”。  
    备注：如果我们要取中最后的一个单词，我们可以这样：$(word  
$(words ),)。  
$(firstword )  
    名称：首单词函数——firstword。  
    功能：取字符串中的第一个单词。  
    返回：返回字符串的第一个单词。  
    示例：$(firstword foo bar)返回值是“foo”。  
    备注：这个函数可以用word函数来实现：$(word  
1,)。  
以上，是所有的字符串操作函数，如果搭配混合使用，可以完成比较复杂的功能。这里，举一个现实中应用的例子。我们知道，make使用“VPATH”变量来指定“依赖文件”的搜索路径。于是，我们可以利用这个搜索路径来指定编译器对头文件的搜索路径参数CFLAGS，如：  
    override CFLAGS += $(patsubst  
%,-I%,$(subst :, ,$(VPATH)))  
    如果我们的“$(VPATH)”值是“src:../headers”，那么“$(patsubst  
%,-I%,$(subst :, ,$(VPATH)))”将返回“-Isrc -I../headers”，这正是cc或gcc搜索头文件路径的参数。  
三、文件名操作函数  
下面我们要介绍的函数主要是处理文件名的。每个函数的参数字符串都会被当做一个或是一系列的文件名来对待。  
$(wildcard )  
名称：文件名展开函数——wildcard 。  
    功能：功能是展开成一列所有符合由其参数描述的文件名，文件间以空格间隔。  
    返回：函数返回展开后的文件名列表。  
      
示例：  
          
        $(wildcard *.c)  
          
        返回结果是一个所有以 '.c' 结尾的文件的列表。   
$(dir )  
    名称：取目录函数——dir。  
    功能：从文件名序列中取出目录部分。目录部分是指最后一个反斜杠（“/”）之前的部分。如果没有反斜杠，那么返回“./”。  
    返回：返回文件名序列的目录部分。  
    示例： $(dir src/foo.c hacks)返回值是“src/  
./”。  
$(notdir )  
    名称：取文件函数——notdir。  
    功能：从文件名序列中取出非目录部分。非目录部分是指最后一个反斜杠（“/”）之后的部分。  
    返回：返回文件名序列的非目录部分。  
    示例： $(notdir src/foo.c hacks)返回值是“foo.c  
hacks”。  
  
$(suffix )  
      
    名称：取后缀函数——suffix。  
    功能：从文件名序列中取出各个文件名的后缀。  
    返回：返回文件名序列的后缀序列，如果文件没有后缀，则返回空字串。  
    示例：$(suffix src/foo.c src-1.0/bar.c hacks)返回值是“.c  
.c”。  
$(basename )  
    名称：取前缀函数——basename。  
    功能：从文件名序列中取出各个文件名的前缀部分。  
    返回：返回文件名序列的前缀序列，如果文件没有前缀，则返回空字串。  
    示例：$(basename src/foo.c src-1.0/bar.c hacks)返回值是“src/foo  
src-1.0/bar hacks”。  
$(addsuffix  
,)  
    名称：加后缀函数——addsuffix。  
    功能：把后缀加到中的每个单词后面。  
    返回：返回加过后缀的文件名序列。  
    示例：$(addsuffix .c,foo bar)返回值是“foo.c  
bar.c”。  
$(addprefix  
,)  
    名称：加前缀函数——addprefix。  
    功能：把前缀加到中的每个单词后面。  
    返回：返回加过前缀的文件名序列。  
    示例：$(addprefix src/,foo bar)返回值是“src/foo  
src/bar”。  
$(join ,)  
    名称：连接函数——join。  
    功能：把中的单词对应地加到的单词后面。如果的单词个数要比的多，那么，中的多出来的单词将保持原样。如果的单词个数要比多，那么，多出来的单词将被复制到中。  
    返回：返回连接过后的字符串。  
    示例：$(join aaa bbb , 111 222 333)返回值是“aaa111  
bbb222 333”。  
  
四、foreach 函数  
  
foreach函数和别的函数非常的不一样。因为这个函数是用来做循环用的，Makefile中的foreach函数几乎是仿照于Unix标准Shell（/bin/sh）中的for语句，或是C-Shell（/bin/csh）中的foreach语句而构建的。它的语法是：  
  
    $(foreach  
,,)  
  
这个函数的意思是，把参数中的单词逐一取出放到参数所指定的变量中，然后再执行所包含的表达式。每一次会返回一个字符串，循环过程中，的所返回的每个字符串会以空格分隔，最后当整个循环结束时，所返回的每个字符串所组成的整个字符串（以空格分隔）将会是foreach函数的返回值。  
  
所以，最好是一个变量名，可以是一个表达式，而中一般会使用这个参数来依次枚举中的单词。举个例子：  
  
    names := a b c d  
    files := $(foreach n,$(names),$(n).o)  
  
上面的例子中，$(name)中的单词会被挨个取出，并存到变量“n”中，“$(n).o”每次根据“$(n)”计算出一个值，这些值以空格分隔，最后作为foreach函数的返回，所以，$(files)的值是“a.o b.o c.o d.o”。  
  
注意，foreach中的参数是一个临时的局部变量，foreach函数执行完后，参数的变量将不在作用，其作用域只在foreach函数当中。  
  
  
五、if 函数  
  
if函数很像GNU的make所支持的条件语句——ifeq（参见前面所述的章节），if函数的语法是：  
  
    $(if ,)   
  
或是  
  
    $(if  
,,)  
  
可见，if函数可以包含“else”部分，或是不含。即if函数的参数可以是两个，也可以是三个。参数是if的表达式，如果其返回的为非空字符串，那么这个表达式就相当于返回真，于是，会被计算，否则会被计算。  
  
而if函数的返回值是，如果为真（非空字符串），那个会是整个函数的返回值，如果为假（空字符串），那么会是整个函数的返回值，此时如果没有被定义，那么，整个函数返回空字串。  
  
所以，和只会有一个被计算。  
  
  
六、call函数  
  
call函数是唯一一个可以用来创建新的参数化的函数。你可以写一个非常复杂的表达式，这个表达式中，你可以定义许多参数，然后你可以用call函数来向这个表达式传递参数。其语法是：  
  
    $(call  
,,,...)  
  
当make执行这个函数时，参数中的变量，如$(1)，$(2)，$(3)等，会被参数，，依次取代。而的返回值就是call函数的返回值。例如：  
    reverse =  $(1) $(2)  
    foo = $(call reverse,a,b)  
那么，foo的值就是“a b”。当然，参数的次序是可以自定义的，不一定是顺序的，如：  
  
    reverse =   
$(2) $(1)  
    foo = $(call reverse,a,b)  
此时的foo的值就是“b a”。  
  
  
七、origin函数  
origin函数不像其它的函数，他并不操作变量的值，他只是告诉你你的这个变量是哪里来的？其语法是：  
  
    $(origin )  
  
注意，是变量的名字，不应该是引用。所以你最好不要在中使用“$”字符。Origin函数会以其返回值来告诉你这个变量的“出生情况”，下面，是origin函数的返回值:  
  
“undefined”  
      如果从来没有定义过，origin函数返回这个值“undefined”。  
  
“default”  
      如果是一个默认的定义，比如“CC”这个变量，这种变量我们将在后面讲述。  
  
“environment”  
      如果是一个环境变量，并且当Makefile被执行时，“-e”参数没有被打开。  
  
“file”  
      如果这个变量被定义在Makefile中。  
  
“command  
line”  
      如果这个变量是被命令行定义的。  
  
“override”  
      如果是被override指示符重新定义的。  
  
“automatic”  
      如果是一个命令运行中的自动化变量。关于自动化变量将在后面讲述。  
  
这些信息对于我们编写Makefile是非常有用的，例如，假设我们有一个Makefile其包了一个定义文件Make.def，在Make.def中定义了一个变量“bletch”，而我们的环境中也有一个环境变量“bletch”，此时，我们想判断一下，如果变量来源于环境，那么我们就把之重定义了，如果来源于Make.def或是命令行等非环境的，那么我们就不重新定义它。于是，在我们的Makefile中，我们可以这样写：  
  
    ifdef bletch  
    ifeq "$(origin bletch)"  
"environment"  
    bletch = barf, gag, etc.  
    endif  
    endif  
  
当然，你也许会说，使用override关键字不就可以重新定义环境中的变量了吗？为什么需要使用这样的步骤？是的，我们用override是可以达到这样的效果，可是override过于粗暴，它同时会把从命令行定义的变量也覆盖了，而我们只想重新定义环境传来的，而不想重新定义命令行传来的。  
  
  
八、shell函数  
  
shell函数也不像其它的函数。顾名思义，它的参数应该就是操作系统Shell的命令。它和反引号“`”是相同的功能。这就是说，shell函数把执行操作系统命令后的输出作为函数返回。于是，我们可以用操作系统命令以及字符串处理命令awk，sed等等命令来生成一个变量，如：  
  
    contents := $(shell cat foo)  
  
    files := $(shell echo *.c)  
  
注意，这个函数会新生成一个Shell程序来执行命令，所以你要注意其运行性能，如果你的Makefile中有一些比较复杂的规则，并大量使用了这个函数，那么对于你的系统性能是有害的。特别是Makefile的隐晦的规则可能会让你的shell函数执行的次数比你想像的多得多。  
  
