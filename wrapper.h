#include<sys/types.h> /* predefined types */
#include<unistd.h> /* include unix standard library */
#include<arpa/inet.h> /* IP addresses conversion utililites */
#include<sys/socket.h> /* socket library */
#include<string.h>
#include<time.h>
#include<netdb.h>
#include<errno.h>
#include<sys/stat.h>
#include<pthread.h>
#include<semaphore.h>
#include<fcntl.h>
#include<signal.h>


ssize_t FullWrite(int fd, const void *buf, size_t count);
ssize_t FullRead(int fd, void *buf, size_t count);
int Accept(int socket,struct sockaddr *addr, socklen_t *addr_len);
void Listen(int socket,int backlog);
void Bind(int socket,struct sockaddr *addr,socklen_t addr_len);
void Connect(int socket,struct sockaddr *addr,socklen_t addr_len);
pid_t Fork();
int Socket(int domain,int type,int protocol)
{
    int fd;
    if((fd=socket(domain,type,protocol))<0){
        printf("socket error\n");
        exit(1);
    }
    return fd;
}
void Connect(int socket,struct sockaddr *addr,socklen_t addr_len)
{
    if(connect(socket,addr,addr_len)<0){
        perror("connect error");
        exit(1);
    }
}
void Bind(int socket,struct sockaddr *addr,socklen_t addr_len)
{
    if(bind(socket,addr,addr_len)<0){
        printf("bind error\n");
        exit(1);
    }
}
void Listen(int socket,int backlog)
{
    if(listen(socket,backlog)<0){
        printf("listen error\n");
        exit(1);
    }
}
int Accept(int socket,struct sockaddr *addr, socklen_t *addr_len)
{
    int connfd;
    if((connfd=accept(socket,addr,addr_len))<0){
        printf("accept error\n");
        exit(1);
    }
}
ssize_t FullRead(int fd, void *buf, size_t count)
{
    size_t nleft;
    ssize_t nread;
    nleft = count;
    while (nleft > 0) {             /* repeat until no left */
        if((nread = read(fd,buf,nleft))<0){
            if(errno == EINTR){
                continue;
            } else{
                exit(nread);
            }
        }else if (nread==0){
            nleft = -1;
            break;
        }
        nleft-=nread;
        buf+=nread;
    }
    buf = 0;
    return (nleft);
}
ssize_t FullWrite(int fd, const void *buf, size_t count)
{
    size_t nleft;
    ssize_t nwritten;
    nleft = count;
    while (nleft > 0) {             /* repeat until no left */
        if((nwritten = write(fd,buf,nleft))<0){
            if(errno==EINTR){
                continue;
            }else{
                exit(nwritten);
            }
        }
        nleft -= nwritten;
        buf+=nwritten;
    }
    return (nleft);
}

pid_t Fork(){
    pid_t pid;
    if((pid= fork())<0)
        {
            perror ("fork error: ");
            exit ( -1);
        }
    return pid;
}
