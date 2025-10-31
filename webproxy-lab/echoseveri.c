#include "csapp.h"

void echo(int connfd); /* echo.c 에 정의된 함수 프로토타입 */

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* 모든 주소 형식을 담을 수 있을 만큼 충분히 큼 */
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    /* 지정된 포트에서 듣기 소켓을 엶 */
    listenfd = Open_listenfd(argv[1]);

    printf("Server listening on port %s...\n", argv[1]);

    while (1) {
        clientlen = sizeof(struct sockaddr_storage); 
        /* 클라이언트의 연결 요청을 기다림 */
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        /* 클라이언트의 주소 정보를 문자열로 변환 */
        Getnameinfo((SA *) &clientaddr, clientlen, 
                    client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        /* 연결된 클라이언트와 통신 (echo 함수 호출) */
        echo(connfd);

        /* 통신이 끝나면 연결 소켓을 닫음 */
        Close(connfd);
        printf("Connection closed for (%s, %s)\n", client_hostname, client_port);
    }
    exit(0);
}