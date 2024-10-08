#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define BUF_SIZE 1024
static int connfd;

void sig_urg(int sig)
{
    int save_errno = errno;
    char buf[BUF_SIZE];
    memset(buf, '\0', BUF_SIZE);
    int ret = recv(connfd, buf, BUF_SIZE-1, MSG_OOB);
    printf("got %d bytes of oob data '%s'\n", ret ,buf);
    errno = save_errno;
}

void addsig(int sig, void (*sig_handler)(int))
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[])
{
    if(argc <= 2)
    {
        printf("usage: %s ip_address port_number\n",basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);
    
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    connfd = accept(sockfd, (struct sockaddr*)&client, &client_len);
    if(connfd < 0)
    {
        printf("errno is: %d\n",errno);
    }
    else
    {
        addsig(SIGURG, sig_urg);
        //设置socket的宿主进程或进程组
        fcntl(connfd, F_SETOWN, getpid());

        char buffer[BUF_SIZE];
        while(1)
        {
            memset(buffer, '\0', BUF_SIZE);
            ret = recv(connfd, buffer, BUF_SIZE-1, 0);
            if(ret <= 0)
            {
                break;
            }
            printf("got %d bytes normal data '%s'\n", ret, buffer);
        }
        close(connfd);
    }
    close(sockfd);
    return 0;
}