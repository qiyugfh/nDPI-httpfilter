使用C++连接ActiveMQ总是复杂一些，下面是环境搭建的步骤（至少耗时30分钟(☄⊙ω⊙)☄），以及测试代码。


### 安装activeMQ 服务端
```
wget http://mirrors.shu.edu.cn/apache//activemq/5.15.4/apache-activemq-5.15.4-bin.tar.gz
tar xvf apache-activemq-5.15.4-bin.tar -C /opt/
cd /opt
mv apache-activemq-5.15.4/ activemq
vim activemq/bin/activemq
```

```
#!/bin/sh

### BEGIN INIT INFO
# Provides:          activemq
# Required-Start:    $remote_fs $network $syslog
# Required-Stop:     $remote_fs $network $syslog
# Default-Start:     3 5
# Default-Stop:      0 1 6
# Short-Description: Starts ActiveMQ
# Description:       Starts ActiveMQ Message Broker Server
### END INIT INFO

```
#### 创建软连接并启动
```
ln -s /opt/activemq/bin/activemq /etc/init.d/
/etc/init.d/activemq start
/etc/init.d/activemq status
```

#### 在web上查看

http://192.168.80.130:8161/admin 或 http://localhost:8161/admin/  
账号密码：admin/admin


### 安装activeMQ cpp
#### 参考编译教程  
https://blog.csdn.net/chenxun_2010/article/details/52709277  
https://blog.csdn.net/github_30605157/article/details/60468727  
cnblogs.com/gongxijun/p/4789479.html?ptvd  


#### 安装CppUnit
```
wget    https://sourceforge.net/projects/cppunit/files/cppunit/1.12.1/cppunit-1.12.1.tar.gz
tar xvf cppunit-1.12.1.tar
cd cppunit-1.12.1
./configure LDFLAGS='-ldl' --prefix=/usr/local/cppunit
make
make install
```
#### 安装apr、apr-util 和apr-iconv   
下载地址 http://apr.apache.org/ 
```
tar xvf apr-1.6.3.tar
cd apr-1.6.3
./configure  --prefix=/usr/local/apr/
make
make install

tar xvf apr-util-1.6.1.tar
cd apr-util-1.6.1
./configure  --prefix=/usr/local/apr-util  --with-apr=/usr/local/apr/
make
make install

tar xvf apr-iconv-1.2.2.tar
cd apr-iconv-1.2.2
./configure  --prefix=/usr/local/apr-iconv/  --with-apr=/usr/local/apr/
make
make install
```
#### 安装activemq-cpp  
下载地址 https://github.com/apache/activemq-cpp
```
wget http://mirrors.shu.edu.cn/apache/activemq/activemq-cpp/3.9.4/activemq-cpp-library-3.9.4-src.tar.gz  
gzip -dv activemq-cpp-library-3.9.4-src.tar.gz  
tar xvf activemq-cpp-library-3.9.4-src.tar
cd activemq-cpp-library-3.9.4
./configure  --prefix=/usr/local/ActiveMQ-CPP --with-apr=/usr/local/apr/  --with-apr-util=/usr/local/apr-util/  --with-cppunit=/usr/local/cppunit  --disable-ssl
make
make install
```
 
#### ActiveMQ-CPP API  
http://activemq.apache.org/cms/api_docs/activemqcpp-3.9.0/html/  
https://wenku.baidu.com/view/5eb31ea1284ac850ad024271.html  



### 测试代码
https://download.csdn.net/download/m0_37406679/10551733