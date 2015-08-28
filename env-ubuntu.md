# Ubuntu
## 风扇狂转
进入 Ubuntu 14.04/14.10，在“系统设置”--“软件和更新”中找到“附加驱动”。默认使用的是开源的 Nouveau 驱动。我们试试软件源提供的私有（闭源）驱动--Nvidia 331 驱动。

## amule
更新kad网络
1. http://upd.emule-security.org/nodes.dat
2. http://kademlia.ru/download/nodes.dat
3. http://sn.im/nodes.dat

## 常用软件
* 为知笔记
  > $ sudo add-apt-repository ppa:wiznote-team
  > $ sudo apt-get update
  > $ sudo apt-get install wiznote

* wps，需要添加支持lib32 arch multiarch
* stardict
* xmind， v3.5.0.201410310637
* sougou
* gedit-plugins
* openconect，  Cicso openvpn connect
* evolution   mail client
* gconf-editor    配置系统

# SSH
* 产生SSH key。 https://help.github.com/articles/generating-ssh-keys/#platform-linux
* 使用SSH登录。 http://chenjianjx.iteye.com/blog/2035150
* ssh-keygen  产生密钥
* ssh-agent    启动后台进程，代理ssh认证
* ssh-add       添加密钥到ssh数据库

碰到的问题：
* 执行ssh-add时出现Could not open a connection to your authentication agent
* 若执行ssh-add /path/to/xxx.pem是出现这个错误:Could not open a connection to your authentication agent，则先执行如下命令即可：
  > ssh-agent bash

# minicom
secureCRT作串口工具，乱码太多，所以没有办法，尝试使用minicom。

1. sudo minicom -s   设置串口，特别是设置波特率。
2. sudo minicom 启动串口工具。
3. sudo rm /var/lock/LCK...ttyUSB0  去掉minicom的锁

在手机开发过程中，UART的打印太多，很难停住。导致看打印困难，输入困难。
1. 输入的问题，可借助secureCRT的交互窗口向minicom输入。
2. 查看打印的问题，可使用more命令解决。sudo more /dev/ttyUSB0，使用该命令，可用/查找字符串。

在minicom中自动换行：Ctrl+A Z W 组合键的用法是：先按Ctrl+A组合键，然后松开这两个键，再按Z键。另外还有一些常用的组合键。

* S键：发送文件到目标系统中；
* W键：自动卷屏。当显示的内容超过一行之后，自动将后面的内容换行。这个功能在查看内核的启动信息时很有用；

# NFS服务器
环境：Ubuntu12.04

##服务器侧：
需要安装的软件：

  > sudo apt-get install nfs-utils
  > sudo apt-get install nfs-kernel-server

对应的配置文件：
* cat /etc/export     共享文件夹的路径
    /home/kernel/HTC_Codebase/MTK *(insecure,rw,sync,no_subtree_check,no_root_squash)
    insecure表示允许nfs使用大于1024的端口号
    no_root_squash表示从client映射到服务器端使用的帐号
* /etc/default/nfs-kernel-server /etc/default/nfs-common    NFS服务器的设置，比如禁用NFS4

相关的命令：

  > sudo /etc/init.d/portmap restart
  > sudo /etc/init.d/nfs-kernel-server restart

## 客户端
需要安装的软件
  > sudo apt-get install nfs-common

使用的命令
  > sudo mount -t nfs4 -o nolock 10.33.137.246:/home/kernel/HTC_Codebase/export KERNEL_SERVER

或者添加这一行到/etc/fstab
  > 10.33.137.246:/home/kernel/HTC_Codebase /home/liu/KERNEL_SERVER nfs4 nolock    注意，这里后面没有0 0

再mount -a

## 出现的问题：
1.  mount.nfs4: access denied by server while mounting 10.33.137.113:/home/kernel/HTC_Codebase/MTK
原因：
可能与selinux、iptables、以及相应文件夹没有777权限有关。
但我查的时候，已经将待export的文件夹，以及它的子文件夹权限全部设置为777了，仍然不行。
最后尝试在其他目录export文件夹，OK。原来，因为 /home/kernel/HTC_Codebase是挂载的磁盘，对应的文件夹权限只有700，所以导致无法获取到superblock。

# Samba
使用到的命令
  > smbpasswd -a liu    添加samba用户liu，并配置密码
  > chkconfig smb on
  > chkconfig nmb on
  > service smb start
  > service nmb start
  > iptables -F  或  systemctl disable iptables
  > setenforce  0  或  vi /etc/selinux/config  把SELINUX=enforce 改成disabled就可以了。后者需要重启。
  > 可能还需要修改/etc/samba/smb.conf中。

强制使用某一个用户登录samba
  > http://wiki.ubuntu.org.cn/Samba
  > force user = nobody
  > force group = nogrou

# Virtual box
共享文件夹
  > net use H: \\vboxsvr\Downloads
  > mount -t vbox

# host 设置
国内访问drive.google.com
* PC端：找到“Windows/system32/drivers/etc/host”，用记事本打开，将ip和网址（74.125.224.231 drive.google.com）写进去。
* 安卓系统:  修改host首先需要root，然后用文件浏览器打开host。安卓系统host文件位于“根目录/etc/host”，修改方法与电脑一样，打开host文件，然后用文本编辑器将“74.125.224.231 drive.google.com”写进去就可以了。

# pptp
1. yum install net-tools  支持ifconfig

2. yum install pptpd  安装pptpd，用于在虚拟主机上创建一个ppp进程。

3. 修改/etc/pptpd.conf
这个配置文件内容很简单，主要需要修改的就是文件末尾的localip和remoteip
  > #localip 192.168.0.1
  > #remoteip 192.168.0.234-238,192.168.0.245

4. 修改/etc/ppp/chap-secrets
这个文件名中保存了访问VPN的用户名密码，格式如下：

 > username pptpd password *

 Username和password都是明文；pptpd代表服务名，和/etc/ppp/options.pptpd里的name对应，通常默认值就是pptpd；最后一项是分配这个用户的ip，*代表随机分配。

5. 设置dns地址/etc/ppp/options.pptpd 或/etc/ppp/options
  > ms-dns 8.8.8.8
  > ms-dns 4.4.4.4

6. 重启pptpd
  > chkconfig pptpd on  设置开启启动pptpd
  > ps aux可以看到进程已经启动。

7. 开启网络转发
  > echo "net.ipv4.ip_forward=1" >> /etc/sysctl.conf
  > sysctl -p
  > cat /proc/sys/net/ipv4/ip_forward
  > echo 1 > /proc/sys/net/ipv4/ip_forward
可以看到，ip转发功能已经打开

8. 配置iptables转发规则
  > iptables -A FORWARD -s 192.168.0.0/24 -j ACCEPT
  > iptables -t nat -A POSTROUTING -s 192.168.0.0/24 -o eth0 -j MASQUERADE
  > iptables-save > /etc/iptables-rules
转发来自192.168.0.0网段的报文，并设置SNAT地址转换。并保存当前配置到配置文件中。

9. 将iptables规则添加到ppp的启动脚本/etc/ppp/ip-up中，这样每次ppp连接up的时候，都会配置iptables规则。
  > iptables-restore < /etc/iptables-rules

10. 在电脑上验证：Windows XP
控制面板 -> 网络连接 -> 新建连接向导 -> 连接到我的工作场所的网络 -> 虚拟专用网络连接 -> 公司名，随意取，作为连接名称 -> 不初始化连接 -> 填写远程虚拟主机的IP地址 -> 不使用智能卡 -> 完成。

之后在网络连接中出现虚拟专用网络，点击刚创建的网络连接，输入用户名和密码，连接成功。在电脑右下角，可以看到新创建的ppp连接。

在Windows XP命令行下ipconfig，可以看到新创建的ppp连接，地址为192.168.0.234。

在虚拟主机命令行下ifconfig，可以看到localip ppp0，地址为192.168.0.1。

11. 在手机上验证：Android MX3
网络 -> 添加VPN -> PPTP，基本步骤和电脑上类似。

12. PPTP上网后，智能路由选择
使用PPTP vpn登录，导致访问国内网站会很慢，如csdn.net基本打不开。可配置国内网段的IP路由不经过vpn连接。
如果是使用goagent代理来翻墙，可以使用switchysharp，配合autoproxy-gfwlist提供的脚本，可以完成智能代理的功能。
http://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest记录了所有已经分配的网段。
比如apnic|CN|ipv4|58.18.0.0|65536|20050301|allocated
可以写一个脚本将之转化为route的配置命令。在网上找到一个现成的，chnroutes可生成windows、mac、linux、android下的路由配置脚本。

#使用代理上网
使用pptp vpn代理，会申请一个私网IP来和server相连，这种方式下会影响当前系统的默认路由。在公司网络环境下就会不太好用，可能导致无法使用公司内部的IP地址和dns解析服务器。需要有复杂的配置。其实如果只是希望使用代理上网，可以使用ssh代理。

在linux下，
  > ssh -qTfnN -D 7070 root@45.79.68.100

设置浏览器的代理服务器, 注意，ssh使用socket5代理，所以在代理服务器的配置项中，http，https的都不要填，只配置socket5。

[Chrome下使用SwitchyOmega](https://github.com/FelisCatus/SwitchyOmega/wiki/GFWList.bak)

GitHub上有专门的project管理gfw list，FelisCatus，可以配合SwitchyOmega

安装goagent，

可以在windows下使用，在linux下还是有些问题
https://github.com/goagent/goagent/blob/wiki/InstallGuide.md

而且，即便是Goagent代理上网，成功过一次，之后再也连不上。总的来说，很不稳定。

新建一个appid，appengine1, 2015-8-2

# 后台执行命令
nohup make &
setsid make &
(make &)

# mail
配置ubuntu的本地mail，将repo每天的changelog推送到终端，方便提醒自己查看。

按照这种方式配置后，可以发送邮件到外部邮箱了。就是发送得很慢。

http://mars914.iteye.com/blog/1470961

# vsftpd
lamp http://wiki.qcloud.com/wiki/CentOS%E7%8E%AF%E5%A2%83%E9%85%8D%E7%BD%AE

ftp    http://wiki.qcloud.com/wiki/%E9%83%A8%E7%BD%B2%E4%BB%A3%E7%A0%81%E5%88%B0CentOS%E4%BA%91%E6%9C%8D%E5%8A%A1%E5%99%A8

#杂项
##linux特殊字符
ctrl+v+m
