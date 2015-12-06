/*************************************************************************
	> File Name: http_conn main
	> Author: ysg
	> Mail: ysgcreate@gmail.com
	> Created Time: 2015年11月11日 星期三 22时59分58秒
 ************************************************************************/
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <exception>
#include <semaphore.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <map>

extern void addfd(int epollfd,int fd,bool one_shot);
extern void removefd(int epollfd,int fd);
extern void modfd(int epollfd,int fd,int ev);
extern int setnonblocking(int fd);
extern bool readData(char* file);
extern void SplitData(int M,int k);
extern void UserSimilarity();
