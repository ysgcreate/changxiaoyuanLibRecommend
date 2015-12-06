/*************************************************************************
	> File Name: http_conn main
	> Author: ysg
	> Mail: ysgcreate@gmail.com
	> Created Time: 2015年11月11日 星期三 22时59分58秒
 ************************************************************************/
#include "global.h"
#include "threadpool.hpp"
#include "userIIF.hpp"
#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000
#define random(x) (rand()%x)
int setnonblocking(int fd){
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
void addfd(int epollfd,int fd,bool one_shot){
    epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN|EPOLLET|EPOLLRDHUP;//ET模式,用EPOLLREHUP
    if (one_shot) {
        event.events|=EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}
void removefd(int epollfd,int fd) {
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}
void modfd(int epollfd,int fd,int ev) {
    epoll_event event;
    event.data.fd=fd;
    event.events=ev|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}
void addsig(int sig,void (handler)(int),bool restart=true){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler=handler;
    if (restart) {
        sa.sa_flags|=SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}
void show_error(int connfd,const char* info){
    printf("%s",info);
    send(connfd,info,strlen(info),0);
    close(connfd);
}
int main(int argc,char* argv[]){
    const char* ip="127.0.0.1";
    int port=atoi("13000");
    //init read data
	char* file="./ml-1m/rating.dat";
    //rat.dat为5万小样本 rating.dat为中等样本,ratings.dat为大样本,大样本运行载入太慢
    //忽略SIGPIPE信号
    addsig(SIGPIPE,SIG_IGN);


	int M=8;
	if(!readData(file)){
		printf("file error\n");
		return 1;
	}
	int rands=random(M);
	SplitData(M,rands);
	UserSimilarity();
    //创建线程池
    threadpool<UserIIF>* pool;
    try{
        pool=new threadpool<UserIIF>();
    }catch(...){
        return 1;
    }
    /*预先为每个可能的客户连接分配一个http_conn对象*/
    UserIIF* users=new UserIIF[MAX_FD];
    users->testRes(10);
    //上一句会输出准确率,召回率,覆盖率,新颖度
    assert(users);
    int user_count=0;

    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    assert(listenfd>=0);
    struct linger tmp={1,0};
    setsockopt(listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

    int ret=0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family=AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr);
    address.sin_port=htons(port);

    ret=bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    assert(ret>=0);

    ret=listen(listenfd,5);
    assert(ret>=0);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd=epoll_create(5);
    assert(epollfd!=-1);
	UserIIF::m_epollfd=epollfd;
    addfd(epollfd,listenfd,false);

    while(true){
        int number=epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if ((number<0)&&(errno!=EINTR)) {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++) {
            int sockfd=events[i].data.fd;
            if (sockfd==listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlen=sizeof(client_address);
                int connfd=accept(listenfd,(struct sockaddr*)&client_address,&client_addrlen);
                if (connfd<0) {
                    printf("errno is:%d\n",errno );
                    continue;
                }
				users[connfd].init(connfd,true);//debug
            }else if((events[i].events&EPOLLRDHUP)||(events[i].events&EPOLLERR)||(events[i].events&EPOLLRDHUP)){
                //有一场,直接关闭客户连接
                printf("%d stop\n",sockfd);
                users[sockfd].close();
            }else if(events[i].events&EPOLLIN){
                //根据读的结果,决定时将任务添加到线程池,还是关闭连接
				if (users[sockfd].readfrom()==true) {
					pool->append(users+sockfd);
				}else{
					users[sockfd].close();
				}
            }else if(events[i].events&EPOLLOUT){
                //根据写的结果,决定是否关闭连接
				if (!users[sockfd].writeto()) {
                    users[sockfd].close();
                }
            }else{}
        }
    }
    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;
    return 0;
}
