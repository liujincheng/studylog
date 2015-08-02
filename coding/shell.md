http://www.cnblogs.com/chengmo/archive/2010/10/02/1841355.html
#编译
需求：某一个repo库长期没有维护，导致很多编译失败。想回退所有的branch到某一个时期去编译。生成两个脚本，一个用于co到某一个历史版本，一个用于恢复到master HEAD

repo forall -p -c git log --no-color  --pretty=format:"%h" --since="2014-10-28" | ./repo_history.sh 20141028
```sh
#!/bin/sh

last_project=""
last_cid=""
PWD=$(pwd)
branch="dev"
if [ $# > 1 ]
then
        branch=$1
fi
FILE1=$PWD/"$branch.sh"
FILE2=$PWD/"$branch-master.sh"

echo "#!/bin/sh" > $FILE1
chmod a+x $FILE1
echo "#!/bin/sh" > $FILE2
chmod a+x $FILE2

while read commitid project
do
        if [  "$project" = "" ]
        then
                last_cid=$commitid
        else
                echo "echo "----------$last_project----------" ;cd $PWD/$last_project ; git branch $branch $last_cid^ ; git checkout $branch " >>$FILE1
                echo "echo "----------$last_project----------" ;cd $PWD/$last_project ; git branch master remotes/m/htc; git checkout master " >>$FILE2
                last_project=$project
        fi
done

echo "echo "----------$last_project----------" ;cd $PWD/$last_project ; git branch $branch $last_cid^ ; git checkout $branch " >>$FILE1
echo "echo "----------$last_project----------" ;cd $PWD/$last_project ; git branch master remotes/m/htc; git checkout master " >>$FILE2
```

##字符串查找
功能：从输入读取字符串，在参数指定的文件中查找该字符串，如果找到，则打印该行。

用"分割add_56.txt中的代码行，找到第4个。
```awk
awk -F"revision=" '{print $1 }' xiugai.xml | while read i ; do grep "$i" CL.xml; done
awk -F"\"" '{print $4}'  add_56.txt | grep "^[a-z]"  | ./test.sh
```
```sh
#!/bin/shj
while read i
do
        grep "$i" $1
done
```

解决repo sync时遇到的error
cat repo_error.txt | awk -F" " '{print $2 }' | awk -F":" '{print $1}' | while read i ; do cd $i ; git stash ; git co m/htc
; git stash pop ; cd - ; done
codereview.sh  对比svn库，生成修改的文件。方便对比。
```sh
#!/bin/sh

if [ $# = 0 ] ; then
    echo "correct command is:"
    echo " ./codereview [FILE|DIR] [FILE|DIR] ..."
    echo ""
    exit
fi

DIR=review_$(date '+%y%m%d_%H%M')

mkdir -p $DIR
mkdir -p $DIR/old
mkdir -p $DIR/new
touch $DIR/readme.txt
URL=`svn info | grep "URL: " | sed "s/URL: //"`

for i in $*
do
    FILEDIR=$(expr $i)
    REVIEWDIR=$(basename $FILEDIR)
    LIST=`svn status $FILEDIR | grep "^M" | sed "s/M //" `
    for file in $LIST
    do
        REVE=`svn info $file 2>/dev/null | grep "Revision: " | sed "s/Revision: //"`
        BASENAME=$(basename $file)
        echo $file
        if [ "$BASENAME" != "$REVIEWDIR" ] ; then
             mkdir -p $DIR/old/$REVIEWDIR/
             mkdir -p $DIR/new/$REVIEWDIR/
             cp $file $DIR/new/$REVIEWDIR 2>/dev/null
             svn cat -r $REVE $URL/$file > $DIR/old/$REVIEWDIR/$BASENAME 2>/dev/null
        else
             cp $file $DIR/new 2>/dev/null
             svn cat -r $REVE $URL/$file > $DIR/old/$BASENAME 2>/dev/null
        fi
        echo $file >> $DIR/readme.txt
    done
done
```

##在字符串中过滤
```sh
commit_author=${commit_author%%@}
```

取commit_author原始值@字符以前的内容

