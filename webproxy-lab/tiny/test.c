#include "csapp.h"

void serve_static(int fd, char* filename, int filesize)
{
    int srcfd;
    char *srcp;
    char filetype[MAXLINE], buf[MAXBUF];

    get_filetype(filename, filetype);
    
    sprintf(buf, "HTTP://...\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "서버정보\r\n");
    Rio_writen(fd, buf, strlen(buf));
    
    sprintf(buf, "연결 유지 안해\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "파일 사이즈\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "타입\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    srcfd = Open(filename, O_RDONLY, 0);

    srcp = Mmap(0, filesize, PROT_BTI,MAP_PRIVATE, srcfd, 0);

    Close(srcfd);

    Rio_writen(fd, srcp, filesize);
    
    Munmap(srcp, filesize);
}