#include "csapp.c"

int open_listenfd(char *port) 
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    // 잠재적인 서버 주소 목록을 가져온다.
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             // 연결허용 TCP, UDP
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; // 모든 IP 주소에서 
    hints.ai_flags |= AI_NUMERICSERV;            // 포트 넘버를 사용해서
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    // 바인딩 할 수 있는 주소를 찾기 위해 목록을 순회한다.
    for (p = listp; p; p = p->ai_next) {
        // 소켓 디스크립터(기술자)를 생성한다.
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  // 소켓 생성 실패, 다음 주소를 기다린다.

        // bind 함수에서 발생한, (이미 사용중인 주소) 오류를 제거 한다.
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int));

        // 디스크립터를 주소에 바인딩한다.
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; // 성공
        if (close(listenfd) < 0) { // 바인딩 실패시, 다음 주소를 시도
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }


    // 정리 (메모리 해제)
    freeaddrinfo(listp);
    if (!p) // 작동하는 (사용 가능한) 주소가 없다.
        return -1;

    // 연결 요청을 받을 준비가 된 리스닝 소켓으로 만든다.
    if (listen(listenfd, LISTENQ) < 0) {
        close(listenfd);
	return -1;
    }
    return listenfd;
}