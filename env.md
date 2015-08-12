# inotify
希望在web服务器根目录监控到有文件更改时，自动转换html文件。由于更新不会很频繁，且转换的工作量较小，因此不考虑增量转换了。

linux提供接口供用户态观测文件系统的变化，有现成的C库可以调用接口。比如inotify-tools，可以使用apt-get安装。它包含了inotifywait和inotifywatch两个工具，可以观测到文件的create/delete/modify/access等。这两个工具，前者在每次有event发生时向stdout输出一句打印，打印格式可以定制。inotifywatch则是输出统计信息。这里使用inotifywait实现我们的需求。

本文参考了[inotify监控linux系统下的目录变化](http://limimgjie.iteye.com/blog/747414)

```shell
inotifywait -mrq -emodify -ecreate -edelete -emove $1 | \
	while read file ;
	do
		/home/liu/bin/md2html $1 $1/Github.css
	done
```

# lighttpd
由于Jekyll使用ruby，要成功运行依赖于一大堆的库，安装起来很麻烦。而且，使用Jekyll写作，标题限制为year-month-day为前缀，不友好。所以放弃，转而使用专用的http服务器。

最有名的web服务器有lighttpd，nglix，apache。apache太重，lighttpd最轻量级。实际验证下来，lighttpd完全符合我的要求。安装简单，配置简单，够用。使用apt-get就可以完成所有的安装。
```
sudo apt-get install lighttpd
```
完成安装后，配置文件为/etc/lighttpd/lighttpd.conf，修改端口号和web服务器的根目录即可。在/etc/lighttpd/conf-available中有lighttpd支持的模块，没有需求可以不用理会。

如果在使用过程中出现问题，需要开log来debug，可以在配置文件中添加如下行。（没有实际验证，没有需求）
```
debug.log-request-handling = "enable"
```

# Pandoc
使用markdown完成写作后，需要查看效果，经过了几个阶段。

* 最开始只能git push到Github上查看，效率较低。
* 后来装了Chrome extension MarkDown Preview Plus Options，效果不过，可以安装Github的风格查看Markdown文件。但是也有不足，不方便共享。
* 最后选择了pandoc，可以将markdown转化为html，然后配合lihtttpd来查看。事实上，pandoc还可以在更多的格式之间转换，比如pandoc -> docx。

Pandoc支持将markdown转化为pdf，但是依赖Latex相关的库。如果要支持中文，需要额外配置。如果待转换的文档不是很多，可以用Chrome查看html格式，然后打印为pdf，也是一种转换方式。

## 安装
使用apt-get可以完成安装。如果需要转化为pdf，可能需要更多的Latex相关的库，根据提示安装即可。
```
sudo apt-get install pandoc

```

## 使用
最好的手册就是man pandoc。下面罗列常用的参数。更详细的可参考[使用pandoc markdown寫作後記]http://www.douban.com/note/330859852/。非常强大，博士论文都可以用pandoc完成。

* -s，如果转化出来的html不支持中文，需要加上这个参数。意思是standalone，我理解是将字库包含进来。
* -o，输出的文件。如果没有使用-t指定输出格式，则根据文件后缀名去猜测待转换的格式。
* -f，输入文件的格式，如果没有指定，根据后缀名猜测。
* -H，在创建html文件时，在\<header\>中插入指定的文件，可以用来插入css的路径。还可以用-B或-A在boday的前后插入内容。

如下是一个例子。使用指定的css文件生成html文件。注意，如果是在web服务器中查看，web服务器需要有访问css文件的权限，否则会出现403 Forbidden错误。
```
echo '<link rel="stylesheet" href="'$cssfile'" type="text/css" />' > $WWWDIR/header
pandoc -s -H $WWWDIR/header -o $WWWDIR/$htmlfile $file
```

# ubuntu
## 查看ubuntu下安装的软件的详细信息
dpkg --listfiles

#Jekyll
Jekyll是一套博客系统，可以使用markdown标记语言写作，然后由Jekyll自动转化为html。就是GitHub page使用Jekyll搭建。

## setup Jekyll
### 更新ruby到2.1.2
```
curl --progress http://cache.ruby-lang.org/pub/ruby/2.1/ruby-2.1.2.tar.gz | tar xz
cd ruby-2.1.2
./configure
make
sudo make install
```
默认ruby将会被安装到/usr/local/bin/目录下，该目录可能不在PATH默认路径下，可以为之在/usr/bin下创建一个软链接。

### 为ruby安装zlib
```
cd ext/zlib
ruby ./extconf.rb
make
make install
```

### 为rubyGem安装openssl
```
cd ext/zlib
ruby ./extconf.rb
修改Makefile，添加top_srcdir = ../../。自动生成的Makefile有点问题。
make
make install
```

### 安装并更新Gem
gem是ruby安装包的管理工具。
```
sudo apt-get install rubygems
sudo gem install rubygems-update
sudo gem install bundle
```

### 为gem安装taobao源
```
$ gem sources --remove https://rubygems.org/  可以先查看现有的source
$ gem sources -a https://ruby.taobao.org/
$ gem sources -l
*** CURRENT SOURCES ***

https://ruby.taobao.org
请确保只有 ruby.taobao.org
```

### 安装jekyll
gem install jekyll -V  
jekyll居然一共有31个依赖包。

sudo apt-get install nodejs  安装默认的js runtime

## setup Jekyll blog
jekyll new stability-blog
cd stability-blog





