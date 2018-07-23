#### 编译安装
```
wget https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.gz
gzip -dv boost_1_67_0.tar.gz
tar -zxvf boost_1_67_0.tar

./boostrap.h --help
./bootstrap.sh
./b2 install

```
默认安装在/usr/local的lib, include中

#### 示例

g++ -o first first.cpp -lboost_timer -lboost_system

first.cpp:
```

#include <vector>
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/assign.hpp>
#include <boost/timer.hpp>

using namespace std;
using namespace boost;


int main(){
 timer t;
 vector<int> v = (assign::list_of(1), 2, 3, 4, 5);

 BOOST_FOREACH(int x, v){
   cout<<x<<",";
 }

 cout<<endl;

 cout<<t.elapsed()<<"s"<<endl;
 cout<<"hello world!"<<endl;
 return 0;

}

```

#### 博文教程
https://blog.csdn.net/qq2399431200/article/details/45621921


#### 遇到的问题

```
./second: error while loading shared libraries: libboost_serialization.so.1.67.0: cannot open shared object file: No such file or directory
```
http://www.51testing.com/html/54/n-3708154.html  

https://www.cnblogs.com/diyunpeng/p/3663201.html

可以使用下面的命令查看依赖库的加载过程
```
LD_DEBUG=libs /home/fanghua/workspace/cfiles/testboost/second -h
```
看到详细的加载过程：
```
root@ubuntu:/home/fanghua/workspace/cfiles/testboost#  LD_DEBUG=libs /home/fanghua/workspace/cfiles/testboost/second -h
      1458:    find library=libboost_serialization.so.1.67.0 [0]; searching
      1458:     search cache=/etc/ld.so.cache
      1458:     search path=/lib/x86_64-linux-gnu/tls/x86_64:/lib/x86_64-linux-gnu/tls:/lib/x86_64-linux-gnu/x86_64:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu/tls/x86_64:/usr/lib/x86_64-linux-gnu/tls:/usr/lib/x86_64-linux-gnu/x86_64:/usr/lib/x86_64-linux-gnu:/lib/tls/x86_64:/lib/tls:/lib/x86_64:/lib:/usr/lib/tls/x86_64:/usr/lib/tls:/usr/lib/x86_64:/usr/lib        (system search path)
      1458:      trying file=/lib/x86_64-linux-gnu/tls/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/lib/x86_64-linux-gnu/tls/libboost_serialization.so.1.67.0
      1458:      trying file=/lib/x86_64-linux-gnu/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/lib/x86_64-linux-gnu/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/x86_64-linux-gnu/tls/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/x86_64-linux-gnu/tls/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/x86_64-linux-gnu/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/x86_64-linux-gnu/libboost_serialization.so.1.67.0
      1458:      trying file=/lib/tls/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/lib/tls/libboost_serialization.so.1.67.0
      1458:      trying file=/lib/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/lib/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/tls/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/tls/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/x86_64/libboost_serialization.so.1.67.0
      1458:      trying file=/usr/lib/libboost_serialization.so.1.67.0
      1458:    
/home/fanghua/workspace/cfiles/testboost/second: error while loading shared libraries: libboost_serialization.so.1.67.0: cannot open shared object file: No such file or directory

```
解决办法
```
ln -s /usr/local/lib/libboost_serialization.so.1.67.0  /usr/lib/libboost_serialization.so.1.67.0  
cd /etc/
vim ld.so.conf
# 在里面添加一行 /usr/lib
ldconfig
```
