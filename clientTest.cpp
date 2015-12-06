/*************************************************************************
	> File Name: m
	> Author: ysg
	> Mail: ysgcreate@gmail.com
	> Created Time: 2015年09月22日 星期二 22时59分58秒
 ************************************************************************/
#define _GNU_SOURCE 1
#include<string.h>
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<fcntl.h>
#include<poll.h>
#include <iostream>
using namespace std;
#define BUF_SIZE 2048
int main(int argc,char* argv[]){
    if(argc<=2){
        printf("usage:%s \nip_address port_number\n",argv[0]);
        return 1;
    }
    const char* ip=argv[1];
    int port=atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address,sizeof(server_address));
    inet_pton(AF_INET,ip,&server_address.sin_addr);
    server_address.sin_port=htons(port);
    server_address.sin_family=AF_INET;

    int sockfd=socket(PF_INET,SOCK_STREAM,0);
    assert(sockfd>=0);

    if(connect(sockfd,(struct sockaddr*)&server_address,sizeof(server_address))<0){
        printf("connect fail\n");
        close(sockfd);
        return 1;
    }

    pollfd fds[2];
    fds[0].fd=0;
    fds[0].events=POLLIN;
    fds[0].revents=0;
    fds[1].fd=sockfd;
    fds[1].events=POLLIN|POLLRDHUP;
    fds[1].revents=0;

    char read_buf[BUF_SIZE];
    int pipefd[2];
    int ret=pipe(pipefd);
    assert(ret!=-1);

    while(1){
        ret=poll(fds,2,-1);
        if(ret<0){
            printf("poll fail\n");
            break;
        }

        if(fds[1].revents&POLLRDHUP){
            printf("server close\n");
            break;
        }else if(fds[1].revents&POLLIN){
            memset(read_buf,'\0',sizeof(read_buf));
            int bytes_read=0;
            bytes_read=recv(fds[1].fd,read_buf,BUF_SIZE-1,0);
            printf("%s\n",read_buf);
        }
        if(fds[0].revents&POLLIN){
            ret=splice(0,NULL,pipefd[1],NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
            ret=splice(pipefd[0],NULL,sockfd,NULL,32768,SPLICE_F_MORE|SPLICE_F_MOVE);
        }
    }
    close(sockfd);
    return 0;
}
