# changxiaoyuanLibRecommend
畅校园图书推荐系统

#### main.cpp
55-58 配置ip,端口,日志文件.线上运行时只需要更改这几个配置

80 取消注释会输出准确率,召回率,覆盖率,新颖度

#### locker.h
条件变量等封装

#### threadpool.hpp
线程池

#### userIIF.hpp
基于用户的协同过滤

#### global.h
全局变量定义

#### clientTest.cpp
测试连接类,编译g++ clientTest.cpp -o client

./client 127.0.0.1 13000

#### cjson为json库

#### 测试使用方法:
main.cpp的80行的会输出准确率,召回率,覆盖率,新颖度,实际环境可以关闭,只会在启动时计算,会耗时,但不大

g++ main.cpp cJSON.c -lpthread -o main -std=c++11

g++ clientTest.cpp -o client

一个终端 ./main

等main初始化以后,另一个终端./client 127.0.0.1 13000,并输出1,可以得到结果

以下为终端结果:

###### main
```c
[root@localhost changxiaoyuanLibRecommend]# g++ main.cpp cJSON.c -lpthread -o main -std=c++11
main.cpp: 在函数‘int main(int, char**)’中:
main.cpp:58:13: 警告：不建议使用从字符串常量到‘char*’的转换 [-Wwrite-strings]
  char* file="./ml-1m/rat.dat";
[root@localhost changxiaoyuanLibRecommend]# ./main
create the 0th thread
create the 1th thread
create the 2th thread
create the 3th thread
create the 4th thread
create the 5th thread
create the 6th thread
create the 7th thread
召回率:0.10 准确率:0.20 覆盖率:0.17 新颖度:5.00
```

###### client
```c
[root@localhost changxiaoyuanLibRecommend]# g++ clientTest.cpp -o client
[root@localhost changxiaoyuanLibRecommend]# ./client 127.0.0.1 13000
1
{
	"items":	["1196", "2858", "480", "2355", "2081", "2571", "364", "2078", "2096", "2396"],
	"code":	1
}
```
