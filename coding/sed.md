 问题：在接口up的时候，向一个配置文件中加入一些配置项，down的时候，将这些配置项从配置文件中删除。  
radvd.temp.conf 保存接口相关的配置项，radvd.conf是最终配置文件。$REALDEVICE是接口名称。  
  
up.sh  
sed -e "s/ppp0/$REALDEVICE/" radvd.temp.conf >> radvd.conf  
  
down.sh  
CFGNUM=`wc -l radvd.temp.conf | sed 's/[[:space:]].*$//g'`  
\#如果grep找不到，那么会导致脚本阻塞  
ARGNUM_B=`grep -n ${REALDEVICE} radvd.conf | sed 's/:.*$//g'`  
ARGNUM_E=`expr $CFGNUM + $ARGNUM_B - 1`  
sed -i "$ARGNUM_B, $ARGNUM_E"d radvd.conf  
  
命令说明：  
  
expr 算数计算（加减乘除）、逻辑运算（与或）、数值比较（大于小于等等）、字符串计算（match/index/substr/length）。  
  
sed 单行文本处理工具 sed [option] [[address]action] input_file  
option:  
    -n 静默方式，不打印sed执行过程  
    -f 从外部文件读取sed脚本  
    -e 指定命令。单条命令时会自动加上，多条命令需要指定。  
    -i 直接修改文件，而不是在屏幕上输出。  
    -r 使用扩展型正则表达式。  
address：  
    修改文件的位置。比如指定行。  
    num1,num2 从第num1行到num2行。num1可以为0，表示第一行。num2为$，表示最后一行。  
    /expr/，可以匹配expr的行。  
    num1,+N，从第num1行之后的N行。  
action:  
     a 新增。a后面可以接字符串。将该字符串添加到指定行。如果没有指定address，则对所有行执行该操作。 如：sed "3a abc" radvd.conf  
     i 插入。i后面接字符串。在指定行前面插入字符串  
     c 取代。c后面接字符串。用这个字符串替换address中指定的行。  
     d 删除。后面不接字符串的。 如：sed "/$REALDEVICE/"d radvdconf。$REALDEVICE是一个shell变量。删除所有匹配该变量的行。  
     s 修改。按照正则表示，修改某一行文本。 如：sed -e "s/ppp0/$REALDEVICE" radvd.temp.conf >> radvd.conf 。将radvd.temp.conf中的ppp0修改为$REALDEVICE，然后附加到radvd.conf  
     q 退出。比如sed 2q radvd.conf，找到文件的前两行后退出。  
     p 打印  
  
注意：  
1. 如果要在sed中使用$1这样的变量，需要使用双引号来把命令括起来。否则单双引号都可以用的。  
2. s命令，一般使用"/"作为分隔符，但其实可以其他字符如":"作为分隔符的。这样在对文件路径进行操作时会很方便，否则要进行转义。  
  
示例  
cspkernel中，有很多source "../../csp/cspkernel/linux/net/ax25/Kconfig"引用，需要指明Kconfig的路径。  
由于目录结构变化，需要将“../../”改为"../../../../",使用sed脚本完成。  
  
find csp/cspkernel/ -name "Kconfig" | xargs sed -i 's:../../:../../../../:g' {} ;  
  
  
