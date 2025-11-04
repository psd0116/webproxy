/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize); // 내가 하기
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  
  // 1. 첫번째 헤더 라인을 읽는다.
  // (예: "Host: localhost:8080\r\n")
  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf); // [11.6A 수정] 첫 번째 헤더도 출력

  // 2. 방금 읽은 줄(buf)이 헤더의 끝을 알리는 빈 줄인지 검사한다.
  while (strcmp(buf, "\r\n"))
  {
    // 3. 다음 헤더 라인을 읽는다.
    Rio_readlineb(rp, buf, MAXLINE);
    
    // 4. 방금 읽은 "User-Agent" 라인을 서버 콘솔에 출력한다.
    printf("%s", buf);
  }
  // 5. 루프 반복
  // 6. 함수가 종료된다.
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  
  // uri 문자열에 cgi-bin이 포함되어 있지 않으면 정적 컨텐츠로 간주한다.
  if (!strstr(uri, "cgi-bin"))
  {
    // cgiargs 버퍼를 빈 문자열로 설정한다.
    strcpy(cgiargs, "");
    
    // filename 버퍼에 "." (현재 디렉터리)를 복사한다.
    strcpy(filename, ".");
    
    // uri 문자열을 filename 버퍼 뒤에 *이어붙인다*.
    strcat(filename, uri);  // <--- 이 부분을 strcpy에서 strcat으로 수정!
    
    // uri의 마지막 문자가 
    if (uri[strlen(uri) - 1] == '/')
        // 맞다면, filename 뒤에 home.html을 이어 붙인다.
        strcat(filename, "home.html");

        // 1을 반환하여 정적 콘텐츠임을 알린다.
        return 1;
  }
  else {
    ptr = index(uri, '?');
    
    if (ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0'; // [추가 수정 권장] URI에서 CGI 인자 부분을 잘라냅니다.
    }
    else {
        strcpy(cgiargs, ""); // [추가 수정 권장] 인자가 없을 때 cgiargs를 비웁니다.
    }

    strcpy(filename, ".");
    strcat(filename, uri); // <--- 동적 콘텐츠 처리 시에도 strcat으로 수정!

    return 0;
  }
}

// // get_filetype 함수 -> 사진
// void get_filetype(char *filename, char *filetype)
// {
//     // 1. filename 문자열에 ".html"이 포함되어 있는지 검사합니다.
//     if (strstr(filename, ".html"))
//         // 2. 만약 ".html"이 있다면, filetype 버퍼에 "text/html"을 복사합니다.
//         strcpy(filetype, "text/html");
//     // 3. ".html"이 없다면, ".gif"가 있는지 검사합니다.
//     else if (strstr(filename, ".gif"))
//         strcpy(filetype, "image/gif");
//     // 4. (..PNG, JPG에 대해 반복..)
//     else if (strstr(filename, ".png"))
//         strcpy(filetype, "image/png");
//     else if (strstr(filename, ".jpg"))
//         strcpy(filetype, "image/jpeg");
//     // 5. 위 4가지 확장자에 모두 해당하지 않으면, 
//     //    기본값으로 "text/plain" (일반 텍스트)을 사용합니다.
//     else
//         strcpy(filetype, "text/plain");
// }
// get_filetype 함수 -> 동영상
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif"); // gif 추가
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) // jpg/jpeg 통합
        strcpy(filetype, "image/jpeg");
        
    else if (strstr(filename, ".mpg") || strstr(filename, ".mpeg"))
        strcpy(filetype, "video/mpeg");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    /* ---------------------- */
    
    else
        strcpy(filetype, "text/plain");
}



// // serve_static 함수의 핵심 역할은 클라이언트가 요청한 정적 파일을 찾아서 HTTP 응답으로 보내주는 것.
// // int fd -> 클라이언트(브라우저)와 연결된 소켓(통신 파이프)의 번호
// // char *filename -> (파일 이름), 서버가 클라이언트에게 보내줘야할 파일의 로컬 경로이다.
// // int filesize -> (파일 크기), filename에 해당하는 파일의 정확한 크기(바이트 단위)이다.
// // doit 함수에서 읽을 수 있는지 확인한 후 서빙
// void serve_static(int fd, char *filename, int filesize)
// {
//   int srcfd; // 소스 파일 디스크립터
//   char *srcp; // 소스 파일이 매핑될 메모리 주소 포인터
//   char filetype[MAXLINE], buf[MAXBUF]; // 파일의 MIME 타입을 저장할 버퍼, 헤더 한 줄을 만들때 사용할 임시 버퍼

//   // 응답 헤더 전송 : 지금부터 보낼 파일은 이런 종류이고, 크기는 이 정도야라고 알려주는 메타데이터를 전송한다.
  
//   // filename 기반으로 filetype 변수에 MIME 타입을 채운다.
//   get_filetype(filename, filetype);

//   // 헤더를 누적하지 않고 개별적으로 전송
//   // buf 버퍼에 (성공) 상태 코드를 쓴.다
//   sprintf(buf, "HTTP/1.0 200 OK\r\n");
//   Rio_writen(fd, buf, strlen(buf));

//   // 응답은 Tiny Web Server가 보낸 거야"라는 헤더를 만들어 전송한다.
//   sprintf(buf, "Server: Tiny Web Server\r\n");
//   Rio_writen(fd, buf, strlen(buf));

//   // 이 응답이 끝나면 연결을 닫을 거야 라고 알려준다.
//   sprintf(buf, "Connection: close\r\n"); // Tiny는 연결 유지를 지원 안 함
//   Rio_writen(fd, buf, strlen(buf));

//   // doit에서 받아온 filesize 값을 이용해 네가 받을 파일의 총 크기는 ()바이트야 라고 알려준다.
//   sprintf(buf, "Content-length: %d\r\n", filesize); // 파일 크기
//   Rio_writen(fd, buf, strlen(buf));

//   sprintf(buf, "Content-type: %s\r\n\r\n", filetype); // MIME 타입과 헤더의 끝
//   Rio_writen(fd, buf, strlen(buf));
  
//   // 서버 콘솔에 보낸 헤더 내용을 로깅(출력)한다.
//   // (디버깅 목적으로 남겨두지만, 실제로는 위에서 buf의 내용을 실시간으로 출력해야 함)
//   printf("Response headers sent (see individual lines above).\n");


//   // 응답 본문 전송 : 파일의 실제 내용(알맹이) 전송

//   // 전송할 파일을 읽기 전용(O_RDONLY)으로 연다. -> 수정을 하지 않기 때문에 읽기 전용으로 사용해도 충분하다.
//   srcfd = Open(filename, O_RDONLY, 0);

//   // 파일을 메모리에 매핑한다.
//   srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

//   // 파일이 메모리에 매핑되었으므로 파일 디스크립터는 더 이상 필요없다. 즉시 닫아서 리소스 누수를 방지한다.
//   Close(srcfd);

//   // 파일을 클라이언트로 전송한다.
//   Rio_writen(fd, srcp, filesize);

//   // 매핑했던 메모리를 해제한다.
//   Munmap(srcp, filesize);
// }
// In serve_static() 함수:

/*
 * serve_static - 정적 파일을 서빙합니다.
 * [11.9 문제: Malloc/Rio_readn/Rio_writen 방식으로 수정됨]
 */

void serve_static(int fd, char *filename, int filesize)
{
    int srcfd; // 소스 파일 디스크립터
    char *srcp; // [11.9] mmap 포인터 대신 malloc 포인터로 재활용
    char filetype[MAXLINE], buf[MAXBUF]; // 헤더 전송용 버퍼

    /* 1. 응답 헤더 전송 */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    
    /* (참고: 헤더를 이렇게 누적해서 한 번에 보내는 것이 더 효율적입니다) */
    Rio_writen(fd, buf, strlen(buf)); 
    printf("Response headers sent.\n");

    /* 2. 응답 본문 전송 (11.9 수정된 부분) */

    // /* [오류 수정] 'int srcfd'가 아니라, 이미 선언된 'srcfd'에 할당 */
    // srcfd = Open(filename, O_RDONLY, 0);

    // /* [오류 수정] 헤더용 'buf' 대신 'srcp' 포인터 사용 */
    // srcp = Malloc(filesize); // 'filesize'만큼 힙 메모리 할당
    
    // Rio_readn(srcfd, srcp, filesize); // 파일 내용을 힙(srcp)으로 읽기
    // Close(srcfd); // 파일 닫기
    
    // Rio_writen(fd, srcp, filesize); // 힙(srcp)의 내용을 클라이언트로 전송
    
    // Free(srcp); // 할당했던 힙 메모리 해제
      // 전송할 파일을 읽기 전용(O_RDONLY)으로 연다. -> 수정을 하지 않기 때문에 읽기 전용으로 사용해도 충분하다.
    srcfd = Open(filename, O_RDONLY, 0);

    // 파일을 메모리에 매핑한다.
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);

    // 파일이 메모리에 매핑되었으므로 파일 디스크립터는 더 이상 필요없다. 즉시 닫아서 리소스 누수를 방지한다.
    Close(srcfd);

    // 파일을 클라이언트로 전송한다.
    Rio_writen(fd, srcp, filesize);

    // 매핑했던 메모리를 해제한다.
    Munmap(srcp, filesize);
}


void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {  /* Child process */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);  /* Redirect stdout to client */
        Execve(filename, emptylist, environ);  /* Run CGI program */
    }

    Wait(NULL);  /* Parent waits for and reaps child */
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* -------------------- Read request line and headers -------------------- */
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n%s", buf);

    sscanf(buf, "%s %s %s", method, uri, version);

    /* Only GET method is implemented */
    if (strcasecmp(method, "GET")) { // head 메소드도 허용하도록 허용
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }

    read_requesthdrs(&rio);

    /* -------------------- Parse URI from GET request ----------------------- */
    is_static = parse_uri(uri, filename, cgiargs);

    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn’t find this file");
        return;
    }

    /* -------------------- Serve static or dynamic content ------------------ */
    if (is_static) {
        /* Check read permission */
        if (!S_ISREG(sbuf.st_mode) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn’t read the file");
            return;
        }

        serve_static(fd, filename, sbuf.st_size);
    } 
    else {
        /* Check execute permission */
        if (!S_ISREG(sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn’t run the CGI program");
            return;
        }

        serve_dynamic(fd, filename, cgiargs);
    }
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body - [수정된 부분] */
    
    // 1. sprintf로 버퍼를 초기화합니다. (첫 줄)
    sprintf(body, "<html><title>Tiny Error</title>");
    
    // 2. strcat을 이용해 안전하게 문자열을 뒤에 이어 붙입니다.
    strcat(body, "<body bgcolor=\"ffffff\">\r\n");

    // 3. 포맷 스트링이 필요하면 sprintf(body + strlen(body), ...)를 사용합니다.
    sprintf(body + strlen(body), "%s: %s\r\n", errnum, shortmsg);
    sprintf(body + strlen(body), "<p>%s: %s\r\n", longmsg, cause);

    // 4. 다시 strcat을 사용합니다.
    strcat(body, "<hr><em>The Tiny Web server</em>\r\n");


    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));

    Rio_writen(fd, body, strlen(body));
}