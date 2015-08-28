#Git常用命令
http://www.liaoxuefeng.com/wiki/0013739516305929606dd18361248578c67b8067c8c017b000
高级git命令 http://blog.sina.com.cn/s/blog_63e603160101a4pe.html

#文件管理
  > git init 创建一个空的本地仓库。会创建一个隐藏文件夹.git
  > git add 标记待提交的文件，无论这个文件是新增，还是修改过，或者是删除，都是这个命令标记。
  > git reset [file]不加任何参数。与git add相对应的命令。告诉git不要提交这个修改。
  > git checkout 也可以从代码库中更新一个文件到本地工作目录，从而撤销之前的修改。不确定是否会发生冲突。
  > git commit 提交git add标记的所有修改的文件到版本库中。
  > git status    可以查看当前git仓库的状态。比如新增一个文件，git status，会告诉你有哪些文件不受控。git add 之后再使用git status，会告诉你有哪些文件待提交。
  > git diff   顾名思义，查看对文件的修改。即使有修改，但是没有用git add命令，标记文件待提交，使用git commit时仍然不会提交该文件。该功能很有用，可以防止将一些临时的修改提交到版本库中。
  > git log --pretty=oneline 查看本地仓库的log
  > 用git log --graph命令可以看到分支合并图。 -p 选项展开显示每次提交的内容差异
  > git diff 21d2753 423b828 >gpio_bring_up.diff   如何查看某一个节点具体修改了哪些文件？可以先git lg看下相邻的两个ci的ID，然后git diff他们。可以输出到diff文件中方便查看。
  > git diff -w 在查看log时忽略空白

# 版本回退
首先，Git必须知道当前版本是哪个版本，在Git中，用HEAD表示当前版本，也就是最新的提交“ 3628164...882e1e0”（注意我的提交ID和你的肯定不一样），上一个版本就是HEAD^，上上一个版本就是HEAD^^，当然往上100个版本写100个^比较容易数不过来，所以写成HEAD~100。

  > git reset --hard HEAD^  回退到上一个版本
  > git reset --hard 3628164  回退到指定版本。可使用git reflog，查看所有的修改记录。使用该命令，还可以放弃本地所有的修改。
   
# 远程库
  > git remote add origin  git@server-name :path/repo-name.git 关联一个远程库 
  > git push -u origin master  第一次推送master分支的所有内容，添加-u参数，将本地的master和远程的master关联起来。origin代表远程库 
  > git push origin master  此后，每次本地提交后，只要有必要，就可以使用命令推送最新修改；
  > git clone git@github.com:michaelliao/gitskills.git  从远程库clone一个最新的库
  > 1. 开发库完成修改后，需要同步到最新的库上去验证是否ok。
  > 2. 开发库的修改，需要定期备份。
  > cd build/
  > git init
  > git remote add lb /home/liu/code1/share/MT6752_GEP/build/.git   lb是remote库的在本地的名称
  > git pull lb refs/heads/l-rel-gep-mtk6752_r2   从lb库拉取l-rel-gep-mtk6752_r2上拉取代码。


# 分支合并
把别的分支合并到master时用merge，而把master合并到别的分支时会用到rebase。把某一个分支的部分修改同步过来，用cherry-pick。使用merge同步时，分支的ID不会变，但是使用cherry-pick相当于创建了一次新的提交。

合并某分支到当前分支：git merge name 

分支合并 git merge --no-ff -m "merge with no-ff" dev  不添加--no-ff，表示快速合并，完成后会删除的dev分支。将dev分支合并到当前分支。

当Git无法自动合并分支时，就必须首先解决冲 突。解决冲突后，再提交，合并完成。 

  > git checkout master  切换到出现问题的分支，这里假设是在master分支上出现的问题。
  > git checkout -b issue-101  在master分支上创建一个临时分支，用于bug修复。不在master分支上直接修改。
  > git checkout -b aaa remote/orign/aaaa  为远程分支创建别名。
  > git checkout master -- filename 从master分支上拉取filename文件到工作区，用于将一份代码porting到多个分支。
  > git add/git commit  bug修改并提交到issue-101分支。
  > git checkout master   切换回master分支，准备代码merge
  > git merge --no-ff -m "merged bug fix 101" issue-101  这里使用--no-ff的方式merge，便于后续可以切换到issue-101上继续分析。
  > git branch -d issue-101  删除bug分支，非必须。
  > git checkout dev  切换到dev分支，也即在之前的工作分支下。

# 跨文件夹的分支管理
1. 进到MTK 的kernel 
2. git push -fn 你电脑上HTC 6752的codebase/ kernel HEAD:MTK branch 名字
3. 进到 你电脑上HTC 6752的codebase/kernel 里
4. 切到目前test porting 的branch上
5. 做 git merge MTK branch 名字

使用git stash可以临时缓存。若长久缓存或多次缓存，则应该使用分支来管理，方便merge代码。
  > git stash  缓存当前工作区的代码内容，便于后续恢复。执行该命令后，git status看，当前工作区是干净的。
  > git stash list  查看缓存区中的工作区
  > git stash apply  stash@{0}  恢复指定工作区，缓存仍然保留。下次还可以继续用。
  > git stash pop  从缓存区中恢复一个工作区，并在缓存区中删除这个工作区。

如何回退代码。
方法1. 
  > git log  #记录关心的commit id
  > git reset --soft HEAD^  #回退修改，使用reset，使得本次的修改都编程待提交状态。
  > git reset xxx.c  # 将待修改的文件变为untrack状态。
  > xxx modify文件。
  > git add -up xxx.c  #将文件变为待提交状态。注意，-up，表示需要逐个确认修改是否必须。一些无意义的空白修改，不会被提交上去。
  > git commit -c CID   #这个ID就是待修改的那个ID。

方法2.
修改文件
  > git commit --amend   #官方推荐方法，但是不好用。因为提交之前不需要git add -up，导致一些无意义的回车符修改被添加进去了。

如何永久修改提交者的名字：
  > git config --global user.name "jincheng_liu" 
  > git config --global user.email " jincheng_liu@htc.com"

有用的全局命令
  > git config --global alias.lg "log --color --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit"
  > git config --global alias.st status  将git status缩写为git status
  > git config --global alias.co checkout
  > git config --global alias.ci commit
  > git config --global alias.br branch
  > git config --global alias.difflog=log --pretty=format:'commit %h %d %s | %cd %an(%ae)%n%b' --date=short -p --no-color
  > 
  > git blame filename.c 显示文件的每一行是在那个版本最后修改。

#repo
  > repo forall  -r "lk|kernel|ftm|nvram"  -p -c git lg --since "1 days ago" 查看最近一天repo库的修改。这个命令很有用。
  > repo forall -r "lk|kernel|ftm|nvram" -p -c git log --since "2 days ago" --full-diff   > xx-xx.log 将最新的修改写到log中。  注意这里使用了全拼log。

## 合patch
git am -3 -s

## 图形工具
gitk

## log生成工具
gitdiff2wiki --csv --author --main-url --since 6551d6e48f144dbad3825cc --until dd64130f75af95d87966fb47022c47f02dff296a


