==
worklog/2015/07/0724_issue_A50CML_ITS#1890_coldboot_ramdump_fail
1. last_kmsg_shutdown_1.txt和kmsg_shutdown_1.txt，在384s的时候，kmsg中有cpu_ntf相关log，但是last_kmsg中没有。
2. 很多RCMS的log看下来，发现cpu shutdown的时候，在last_kmsg中都没有plug off的打印。两者使用相同的打印级别。
---
说明last kmsg和dmesg中的打印有区别。


