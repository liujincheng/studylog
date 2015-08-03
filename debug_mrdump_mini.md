ITS988, [    7.175353]<0>.(0)[832:htcserviced]EXT4-fs error (device mmcblk0p46): __ext4_journal_start_sb
kedump: invalid header[0x0454c46],  
587 void mrdump_mini_ipanic_done(void)
588 {
589   mrdump_mini_ehdr->ehdr.e_ident[0] = 0;
590 } 
CL564389
该问题实质上包含了三个子问题：
为什么在dbg文件中解析出来的各个文件的大小为0
kedump_to_expdb中没有对offset赋初值，导致offset为0x141e6a808，无法写emmc。
http://git.htc.com:8081/574443
为什么mrdump_mini_ipanic_done会被调用到，也即ipanic_mem_write的返回值为什么有错。
Kernel中的相关debug log都没有印出来
出错的代码函数流程：
ipanic() 使用ipanic_header_to_sd()和ipanic_data_to_sd()两种函数将ipanic的信息写到expdb partition，ipanic_header()函数初始化ipanic_header变量，并使用ipanic_msdc_info初始化存储设备，这里是expdb emmc partition 
    --->写ipanic_header到expdb partition，errno = ipanic_header_to_sd(0);  
        ---> 因为换入的参数header为NULL，所以执行errno = ipanic_mem_write(ipanic_hdr, 0, sizeof(struct ipanic_header), 0);  该函数会在offset为0的位置写入ipanic_header，从这个结构体定义可以看出，它包含了expdb partition的很多信息，比如blksize、partsize，还包含了ipanic相关的信息，比如32个ipanic_data_header结构体，bufsize、buf等。如果传入的参数不为NULL，则保存ipanic_data_header到ipanic_header的头部中。事实上，所有ipanic_data_to_sd()都会call到这种情况 
             ----》 ipanic_next_write(ipanic_memory_buffer, &mem_info, off, len, encrypt);  该函数使用 ipanic_memory_buffer，将ipanic_hdr的待写的data拷贝到ipanic_buffer，即iheader->buf，每次拷贝的长度为iheader->bufsize。之后尝试对ipanic_buffer加密。再之后使用ipanic_write_size写emmc。如果buf长度超过了iheader->bufsize，则上述过程可能执行多次，直到将所有data都写到emmc。  
                   ----》card_dump_func_write(((unsigned char *)buf, len, off, DUMP_INTO_BOOT_CARD_IPANIC)))   
                        ----》调用simp_emmc_dump_write写emmc。
errno有以下几种情况：
使用next fn_handle中执行memcpy出错
data oversize，也即要写的data太多，emmc装不下了。expdb的大小为10M。目前的代码逻辑，10M完全足够。
ipanic_write_size写emmc失败，导致error。
综合上述出错的代码流程，不管是写ipanic_header，还是写ipanic_data，最终都会call到simple_emmc_dump_write函数。而且从整个流程看来，最有可能出问题的也是这里。
