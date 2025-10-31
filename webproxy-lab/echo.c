#include "csapp.h"

void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    /* 클라이언트로부터 데이터(줄바꿈 포함)를 읽음 */
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) { 
        printf("server received %d bytes\n", (int)n);
        /* 읽은 데이터를 그대로 클라이언트에게 다시 보냄 */
        Rio_writen(connfd, buf, n);
    }
}