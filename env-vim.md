# 基本编辑
## vim的移动命令

  >  h，j，k，l  
  >  C-W H、J、K、L  
  >  C-B、F 向上翻页、向下翻页  
  >  C-G 告诉当前位置  
  >  t、f、T、F 移动到指定字符  
  >  w、b、e、ge 按单词移动  
  >  H、M、L 在当前窗口中跳转到不同位置  
  >  zt、zz、zb 相对于光标滚屏  
  >  / n q/ 查找，及查找历史  
  >  0 ^ $  跳到行首，第一个字符，最后一个字符  
  >  { 上一段(以空白行分隔) - } 下一段(以空白行分隔) - 
  >  % 跳到当前对应的括号上(适用各种配对符号)  

## vim的跳转命令

  > %跳转到配对的括号去  
  > [[跳转到代码块的开头去(但要求代码块中'{'必须单独占一行)  
  > gD跳转到局部变量的定义处  
  > ''跳转到光标上次停靠的地方, 是两个', 而不是一个"  
  > mx设置书签,x只能是a-z的26个字母  
  > \`x跳转到书签处("\`"是1左边的键)  
  > >增加缩进,  
  > x>表示增加以下x行的缩进<减少缩进,  
  > x<表示减少以下x行的缩进  
 
## 高效编辑

  >  di" 光标在”“之间，则删除”“之间的内容  
  >  yi( 光标在()之间，则复制()之间的内容  
  >  vi[ 光标在  
  >  以上三种可以自由组合搭配，效率奇高，i(nner)  
  >  dtx 删除字符直到遇见光标之后的第一个 x 字符  
  >  ytx 复制字符直到遇见光标之后的第一个 x 字符  
 
## 标签和宏(macro)
宏操作是VIM最为神奇的操作之一，需要慢慢体会其强大之处；

  >  ma 将当前位置标记为a，26个字母均可做标记， mb 、 mc 等等；  
  >  'a 跳转到a标记的位置； - 这是一组很好的文档内标记方法，在文档中跳跃编辑时很有用；  
  >  qa 将之后的所有键盘操作录制下来，直到再次在命令模式按下 q ，并存储在 a 中；  
  >  @a 执行刚刚记录在 a 里面的键盘操作；  
  >  @@ 执行上一次的macro操作；  
  
## 前缀
在vim脚本中，有一些组合命令，比如qa表示录制宏，zz用于折叠等。实际实验下来，任意字母组合都可以用于快捷键。比如a通常用于插入，但是你仍然可以自定义一个ac之类的命令。但后果是，每次输入a的时候，vim都会等待大约2秒，如果没有后续输入，才会执行插入操作。因此，使用字母组合做前缀时，需要选用那些不太常用的字母。

需要切换模式才能生效的命令，比如a，i，o等，不适合做前缀。下面是一些常用的作为前缀的关键字。

  >  q   宏录制，后面跟的字母表示寄存器的编号。  
  >  z   折叠相关  
  >  d   删除相关。包括di，dt，dd，dw，d$，等  
  >  w   自定义，窗体相关的，我会使用该前缀。  
  >  f   搜索相关的命令。比如fr，ff，fc  
  >  m   标签命令
  
 打开光标下的折叠  – 循环打开光标下的折叠，也就是说，如果存在多级折叠，每一级都会被打开  – 关闭光标下的折叠  – 循环关闭光标下的折叠

## 刷新文件
  >  :e! 刷新文件  
  >  :file 显示当前文件名  
  >  :vimgrep xxx xxx.txt 在文件中搜索:字符串  
  >  :e!、:file、:vimgrep三个命令组合起来，可以实现类似于UE的搜索功能。该功能在调试android kernel下疯狂打印时，应该有有用。  

## vim中的复制与粘贴
  > :set paste/nopaste   在paste和智能缩进之间切换。方便复制粘贴。设置paste后，智能缩进等很多功能将失效。  
  > :set pastetoggle=<F11>  使用快捷键在paste和nopaste之间切换。  
 
# vim的窗口管理
##winManager
1. 窗口布局

  > let g:winManagerWindowLayout='FileExplorer|TagList'  
  > nmap wm :WMToggle<cr>  

如果对默认的窗口大小不满意，可以使用:ressize +n/:resesize -n/:vertical resesize +n 命令改变窗口的大小。
  > let g:netrw_winsize=20
  > nmap <silent> <leader>se :Sexployer!<CR>
  > nmap wj :resize -3<CR>
  > nmap wk :resize +3<CR>
  > nmap wh :vertical resize +3<CR>
  > nmap wl :vertical resize -3<CR>

4. 窗口分割
  > split window, Ctrl+W H, Ctrl+W J, Ctrl+W K, Ctrl+W L

FileExployer command

  >  c   : change pwd to the directory currently displayed  
  >  C  : change currently dissplayed dir to pwd.  

## minibuffer

  >  :bn—下一个文件   
  >  :bp—上一个文件  
  >  d  删除光标所在的buffer（需要光标切换到minibuffer）  
  >  :bd N  delete given number buffer   
  >  ctrl +^  jump between current and latest buffer   
  >  N ctrl+^  jump to the given number buffer   

另一个类似的工具，是buffexployer

## tab tabnew、tabclose等

## vim的会话管理
  >  :cd src                            "切换到/home/easwy/src/vim70/src目录  
  >  :set sessionoptions-=curdir        "在session option中去掉curdir  
  >  :set sessionoptions+=sesdir        "在session option中加入sesdir  
  >  :mksession vim70.vim               "创建一个会话文件  
  >  :wviminfo vim70.viminfo            "创建一个viminfo文件  
  >  :qa                                "退出vim  
  >  :source ~/src/vim70/src/vim70.vim  '载入会话文件  
  >  :rviminfo vim70.viminfo            '读入viminfo文件  

# VIM开发环境
本文的目标，使用VIM建立IDE环境。

* 支持在符号之间跳转，包括向前搜索调用者，向后搜索被调用者。
* 支持快速查找文件。 
* 支持相同codebase下创建多个项目

## 自定义shell命令，准备工程的环境变量。
本地有多个代码目录，需要切换工程，可将如下语句添加到.bashrc中，路径自定义，命令名称自定义。这些环境变量，主要是给vim准备的。

```bash
alias cdsyn="export PRJTOP=~/code1/share/MT6795_SYNC ; export TAGTOP=~/code1/share/MT6795_SYNC/tags ; cd ~/code1/share/MT6795_SYNC ; mkdir -p tags ; source build/envsetup.sh; source ~/.bashrc.loc"
.bashrc.loc只有一句话，自定义了一个快速查找文件的命令。 
alias loc="locate -d $TAGTOP/.dirlocate.db"
```

## 为工程创建数据库
 
1. 随便把脚本放到一个目录下，然后设置PATH。比如我放在~/code1/bin/下。
2. 使用自定义cd命令(如cdsyn)进入到工程中。 udb $(pwd)。完整的更新大概十几分钟就完成了。
3. 我使用 crontab -e ，添加如下代码， 让电脑每天凌晨时自动更新数据库。记得sudo service cron restart  
    > 0 0 * * * /home/liu/code1/bin/udb-repo /home/liu/code1/share/MT6795_SYNC

有的数据库是整个工程的，有的只索引部分目录。需要修改脚本，指定代码文件夹。
* locate db。在工程的几十万个文件中中找一个文件，只需要几秒钟。配合.bashrc.loc用。比如loc factory_init.rc。
* ctags db。vim中向下查函数和变量的代码。ctags和cscsope网上很多资料，不详细说了。
* cscope db。vim中向上找谁掉了这个函数。
* filelist。也是用来找文件的，但配合grep，可以更精细。比如这条命令， 在sepolicy相关.te类型文件中找factory字符串。在selinux开发中，这个数据库帮了我大忙。  
  > grep "sepolicy" tags/filelist | grep ".*.te$" | xargs grep factory
* lookupfile db。在vim中找文件。 http://easwy.com/blog/archives/advanced-vim-skills-lookupfile-plugin/

## 配置vim的环境变量
如果你有在Android众多目录下频繁切换文件夹，为频繁设置tags路径烦恼，那么这段内容你一定很感兴趣。核心点：在vimrc中可以使用shell的环境变量。你应该还记得，在cdsyn时，已经将工程环境变量导入到shell中了。

```vimrc
" ctags, 可以集中放在一个目录下。
set tags=$TAGTOP/tags
" cscope, 没办法，只能放在各自代码文件夹下，逐个添加。
:cs add $PRJTOP/kernel-3.10/cscope.out $PRJTOP/kernel-3.10
:cs add $PRJTOP/system/cscope.out $PRJTOP/system
:cs add $PRJTOP/vendor/cscope.out $PRJTOP/vendor
:cs add $PRJTOP/bootable/cscope.out $PRJTOP/bootable
:cs add $PRJTOP/device/cscope.out $PRJTOP/device
:cs add $PRJTOP/bionic/cscope.out $PRJTOP/bionic
" Lookup file的数据库路径
let g:LookupFile_TagExpr = printf('"%s/tags/filenametags"',$PRJTOP)
```

## 几种实现命令的方式：
1. 修改.bashrc脚本，使用alias命令，对一些常用命令重命名。
2. 写shell脚本。
3. 使用source为shell导入常用函数。

第二种方式更灵活，但是会打开子进程运行shell，所以可能没有办法把符号export到当前的shell中。而且，使用alias可以保留原有参数形式。缺点是使用这种方式产生的自定义命令无法在shell中使用。

1. 在.bashrc中自定义cdsyn命令，导出 当前工程使用的变量
alias cdsyn="cd ~/code1/share/MT6752_SYNC; mkdir -p tags; source build/envsetup.sh; export TAGTOP=~/code1/share/MT6752_SYNC/tags ; export PRJTOP=~/code1/share/MT6752_SYNC; source ~/.bashrc.loc"
在工程中执行cdsyn，表示进入到某一个工程下，同时为该工程准备环境变量

#常用插件

##Ctags
VIM中符号查找 ctags
修改/etc/vim/vimrc    set tags=$TAGTOP/tags
  >  :tag xxx 查找xxx符号   
  >  :tj  jump to only result, and if there is more than one result, let user choose   
  >  :tj /xxxx   seach any tag that include xxxx string   
  >  g ] use :ts command to jump, it will force user to select one match   
  >  g ctrl+j  use :tj command to jump   
  >    
  >  :tnext  :tn  查找下一个符号  
  >  :tprev :tp  查找上一个符号  
  >  let g:LookupFile_TagExpr = printf('"%s/tags/filenametags"',$PRJTOP)  
  >      <CR>          跳到光标下tag所定义的位置，用鼠标双击此tag功能也一样  
  >  o             在一个新打开的窗口中显示光标下tag  
  >  <Space>       显示光标下tag的原型定义  
  >  u             更新taglist窗口中的tag  
  >  s             更改排序方式，在按名字排序和按出现顺序排序间切换  
  >  x             taglist窗口放大和缩小，方便查看较长的tag  
  >  +             打开一个折叠，同zo  
  >  -             将tag折叠起来，同zc  
  >  *             打开所有的折叠，同zR  
  >  =             将所有tag折叠起来，同zM  
  >  [[            跳到前一个文件  
  >  ]]            跳到后一个文件  
  >  q             关闭taglist窗口  
  >  <F1>          显示帮助  
   
## vim与git log
经常需要查看git log，喜欢用传统的diff文件的方式，可以在查看diff的时候，直接跳转到相应的代码行。
[参考资料](http://blog.sina.com.cn/s/blog_601f224a01012wat.html)

首先，使用生成git原生的命令生成diff文件。
  >  git log --no-color   
  >  git log --author=.*@htc.com  
  >  git log --expr=ramdump  

可以直接在diff文件中使用跳转命令。但是有个问题，diff文件如果比较大，比较长，如果能够折叠着看，则比较好。如下自定义了一个快捷键，实现代码折叠。
```vimrc
"process diff file
function FoldDiff()
    :%s/^diff/}}}\rdiff
    :%s/^commit/}}}\rcommit
    :set foldmarker=diff,}}}
    :set foldmethod=marker
    :set filetype=diff
endfunction
map <F12> :call FoldDiff()<CR>
```

## 快速文件查找  lookupfile 
```
(echo "!_TAG_FILE_SORTED 2 /2=foldcase/";          (find . -type f -printf "%f\t%p\t1\n" |          sort -f)) > ./filenametags
find . \( -name .svn -o -wholename ./classes \) -prune   -type f -printf "%f\t%p\t1\n" \
let g:LookupFile_TagExpr = string('./filenametags')
```
[参考资料](http://easwy.com/blog/archives/advanced-vim-skills-lookupfile-plugin/ ) [参考资料](
http://easwy.com/blog/archives/advanced-vim-skills-netrw-bufexplorer-winmanager-plugin/)

## cscope 
在自动更新脚本中，增加cscope -Rbqk -P $PRJTOP/$src, 在vimrc中，增加数据库添加命令：:cs add $PRJTOP/kernel-3.10/cscope.out $PRJTOP/kernel-3.10

对于java文件，然后在生成cscope文件时，使用-f指定文件。find . –name “*.java” > cscope.files

  >  nmap <C-_>s :cs find s <C-R>=expand("<cword>")<CR><CR>  
  >  nmap <C-_>g :cs find g <C-R>=expand("<cword>")<CR><CR>  
  >  nmap <C-_>c :cs find c <C-R>=expand("<cword>")<CR><CR>  
  >  nmap <C-_>t :cs find t <C-R>=expand("<cword>")<CR><CR>  
  >  nmap <C-_>e :cs find e <C-R>=expand("<cword>")<CR><CR>  
  >  nmap <C-_>f :cs find f <C-R>=expand("<cfile>")<CR><CR>  
  >  nmap <C-_>i :cs find i ^<C-R>=expand("<cfile>")<CR>$<CR>  
  >  nmap <C-_>d :cs find d <C-R>=expand("<cword>")<CR><CR>  

## Rgrep
https://github.com/yegappan/grep

## auto complete 
CTRL-N和 CTRL-P补全了。它们会在当前缓冲区、其它缓冲区，以及当前文件所包含的头文件中查找以光标前关键字开始的单词。 
  >  整行补全                        CTRL-X CTRL-L  
  >  根据当前文件里关键字补全        CTRL-X CTRL-N   
  >  根据字典补全                    CTRL-X CTRL-K   
  >  根据同义词字典补全              CTRL-X CTRL-T   
  >  根据头文件内关键字补全          CTRL-X CTRL-I   
  >  根据标签补全                    CTRL-X CTRL-]   
  >  补全文件名                      CTRL-X CTRL-F   
  >  补全宏定义                      CTRL-X CTRL-D   
  >  补全vim命令                     CTRL-X CTRL-V   
  >  用户自定义补全方式              CTRL-X CTRL-U   
  >  拼写建议                        CTRL-X CTRL-S   
  
## supertab 
SuperTab插件会记住你上次所使用的补全方式，下次再补全时，直接使用TAB，就可以重复这种类型的补全。比如，上次你使用 CTRL-X CTRL-F进行了文件名补全，接下来，你就可以使用TAB来继续进行文件名补全，直到你再使用上面列出的补全命令进行了其它形式的补全。 

g:SuperTabRetainCompletionType的值缺省为1，意为记住你上次的补全方式，直到使用其它的补全命令改变它；如果把它设成2，意味着记住上次的补全方式，直到按ESC退出插入模式为止；如果设为0，意味着不记录上次的补全方式。

g:SuperTabDefaultCompletionType的值设置缺省的补全方式，缺省为CTRL-P。

## 和Git的集成
gitv，类似于gitk，用来看log的。

fugitive，集成了很多的git命令。

# vim插件开发
  >  :scriptnames  查看vim已经加载的scripts  
  >  :command     查看所有直接的命令  
  >  :function 列出所有支持的函数  
  >  :verbose  列出详细过程  
  >  :let xxxx  查看或设置全局变量  
  >  :set mouse=  查看或设置默认参数  
  >  “$”——访问环境变量；  
  >  “&”——访问 Vim 选项；  
  >  “@”——访问寄存器。  

# 小技巧
[在vim中执行外部shell命令](http://blog.csdn.net/topgun_chenlingyun/article/details/8013115)

调整vim窗口大小

## Windows 回车符处理
dos回车符和unix回车符之间的转换

  >  dos2unix  filename
  >  :%s/^M$//g 后，回车即会自动删除该文件中的所有 ^M 字符。  
     ^M 注意要用 Ctrl + V Ctrl + M 来输入  
     linux下，如果需要在vim中查看^M，需要使用如下命令：e ++ff=unix %  
     隐藏^M,可以实用:e ++ff=dos %

[vim 高亮工具](http://www.vim.org/scripts/script.php?script_id=2666)
