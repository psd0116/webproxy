#include <stdio.h>
#include "csapp.h"

void request_parse(int fd);
int parse_uri(char *uri, char *hostname, char *port, char *path);

int main(int argc, char** argv)
{
    // 포트번호 맞는지 확인
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    // 리스닝 소켓 생성
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM,0)) < 0)
    {
        printf("socket 생성 실패");
        exit(1);
    }
    
    // 만들어진 listenfd는 단지 socket 디스크립터일 뿐이다.
    // 이 소켓에 주소를 할당하고 port 번호를 할당해서 커널에 등록해야함
    // 커널에 등록해야 다른 시스템과 통신할 수 있는 상태가 된다.
    // sockaddr_in 구조체 사용
    // 서버 주소 구조체 설정 OS에 포트 정보를 알려주기 위함
    int port = atoi(argv[1]);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // 사용할 포트 번호는 8000
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 주소를 지정해 주는 것으로 아무 주소나 다 받겠다 선언

    if ((bind(listenfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0))
    {
        printf("bind 실행 에러\n");
        exit(1);
    }
    
    // listen 함수를 호출하면 클라이언트의 접속 요청이 올 때까지 대기상태가 된다. -> 블록된 모습
    // 에러없이 함수가 복귀했다면 클라이언트의 접속 요청이다.
    // listen은 큐를 5까지만 허용
    if (listen(listenfd, 5) < 0)
    {
        printf("대시 상태 모드 설정 실패");
        close(listenfd);
        exit(1);
    }

    // 클라이언트 연결 수락을 위한 무한 루프
    while(1)
    {
        int clientfd; // 클라인언트와 실제 통신할 소켓
        struct sockaddr_in client_addr; // 연결될 클라이언트의 주소 정보
        socklen_t client_len = sizeof(client_addr);

        // accept -> 클라이언트 연결 요철리 올 때까지 대기
        // 연결이 되면, 클라이언트와 통신할 수 있는 새로운 소켓(clientfd)를 반환
        clientfd = accept(listenfd, (struct sockaddr*) &client_addr, &client_len);

        if (clientfd < 0)
        {
            printf("accept 실패!");
            continue;
        }
        // accept로 받은 clientfd 파싱
        request_parse(clientfd);
        close(clientfd);
    }
}

// 클라이언트의 HTTP 요청을 읽고 파싱한다! + 서버에 중계
void request_parse(int clientfd)
{
    rio_t rio_client; // RIO 버퍼 구조체
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    // RIO 버퍼를 초기화 하자 ->clientfd와 rio 버퍼 연결
    Rio_readinitb(&rio_client, clientfd);
    
    // 요청 첫째줄 읽기
    if (Rio_readlineb(&rio_client, buf, MAXLINE) <= 0)
    {
        // 읽을 것이 없거나 오류가 발생한 경우 함수 종료
        return;
    }

    // 첫째 줄 파싱 
    if (sscanf(buf, "%s %s %s", method, uri, version) != 3)
    {
        fprintf(stderr, "잘못된 요청 라인 : %s\n", buf);
        return;
    }

    // Uri 파싱 함수를 호출해서 hostname, port, path를 채운다.
    if (parse_uri(uri, hostname, port, path) < 0)
    {
        fprintf(stderr, "파싱 실패 : %s\n", uri);
        return;
    }

    // 파싱한 hosrname과 port로 리얼 서버에 연결
    int serverfd; // 서버와 연결될 소켓
    rio_t rio_server; // 서버와 연결될 RIO 버퍼
    
    if ((serverfd = Open_clientfd(hostname, port)) < 0)
    {
        fprintf(stderr, "서버 연결 실패 : %s :%s\n", hostname, port);
        return; // 서버 연결 실패 시 함수 종료
    }
    
    Rio_readinitb(&rio_server, serverfd);

    // 서버로 보낼 새로운 HTTP 요청 라인 생성
    char new_request_line[MAXLINE];
    // 서버에게 HTTP/1.0으로 요청
    sprintf(new_request_line, "%s %s HTTP/1.0\r\n", method, path);

    Rio_writen(serverfd, new_request_line, strlen(new_request_line));
    
    // 클라이언트의 헤더를 읽으면서 즉시 서버로 전달
    int n;
    while ((n = Rio_readlineb(&rio_client, buf, MAXLINE)) > 0)
    {
        // \r\n을 만나면 헤더 끝
        if (strcmp(buf, "\r\n") == 0)
        {
            break;
        }
        
        // 우리가 직접 만들어서 보낼 것이므로 클라이언트가 보낸것은 무시
        if (strstr(buf, "Host:") || strstr(buf, "Connection:") || strstr(buf, "Proxy-Connection:"))
        {
            continue;
        }
        // 읽은 헤더를 서버로 전송
        Rio_writen(serverfd, buf, n);
    }

    if (n < 0)
    {
        fprintf(stderr, "헤더 못 읽어요\n");
        return;
    }
    // 프록시가 서버로 보낼 필수 헤더를 만들어서 전송
    char new_headers[MAXLINE];
    sprintf(new_headers, "Host: %s\r\n", hostname);
    Rio_writen(serverfd, new_headers, strlen(new_headers));
    Rio_writen(serverfd, "Connection: close\r\n", 19);
    Rio_writen(serverfd, "Proxy-Connection: close\r\n", 25);
    Rio_writen(serverfd, "\r\n", 2); // 헤더의 끝 (빈 줄)

    // ----- 클라이언트 -> 프록시 -> 서버 -------------

    // ----- 서버 -> 프록시 -> 클라이언트 -------------
    while ((n = Rio_readnb(&rio_server, buf, MAXLINE)) > 0)
    {
        Rio_writen(clientfd, buf, n);
    }
    close(serverfd);
}


int parse_uri(char *uri, char *hostname, char* port, char* path)
{
    // "http://" 부분 건너뛰기
    char *ptr = strstr(uri, "://");
    if (ptr == NULL) {
        fprintf(stderr, "http 요청이 아님");
        return -1;
    }
    ptr += 3; // "host.com:80/path"
    char *host_start = ptr;

    // 경로(/) 시작 위치 찾기
    char *path_start = strchr(host_start, '/');
    if (path_start == NULL) {
        // 경로 없음 (예: http://host.com:8080)
        strcpy(path, "/"); // 기본 경로
    } else {
        strcpy(path, path_start); // 경로 복사 (예: "/path")
        *path_start = '\0'; // 호스트 부분과 분리 (ptr이 "host.com:8080"이 됨)
    }

    // 포트(:) 시작 위치 찾기
    char *port_start = strchr(host_start, ':');
    if (port_start == NULL) {
        // 포트 없음 (예: host.com)
        strcpy(port, "80"); // 기본 포트
        strcpy(hostname, host_start); // 호스트명 복사
    } else {
        // 포트 있음 (예: host.com:8080)
        *port_start = '\0'; // 호스트명과 분리 (host_start가 "host.com"이 됨)
        strcpy(hostname, host_start);
        strcpy(port, port_start + 1); // 포트 번호 복사 (예: "8080")
    }
    
    return 0;
}