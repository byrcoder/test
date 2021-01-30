//
// Created by weideng(邓伟) on 2021/1/24.
//

#include <sys/socket.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

namespace Tcp {

#ifndef TCP_CONGESTION
#define TCP_CONGESTION 13
#endif

#ifdef TCP_CONGESTION
    void test_cong() {
        char buf[256];
        socklen_t len;
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        if (sock == -1)
        {
            perror("socket");
            return ;
        }

        len = sizeof(buf);

        if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0)
        {
            perror("getsockopt");
            return;
        }

        printf("Current: %s\n", buf);

        strcpy(buf, "reno");

        len = strlen(buf);

        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, len) != 0)
        {
            perror("setsockopt");
            return;
        }

        len = sizeof(buf);

        if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &len) != 0)
        {
            perror("getsockopt");
            return;
        }

        printf("New: %s\n", buf);

        close(sock);
    }
#else
    void test_cong() {

    }
#endif

    void test() {
        test_cong();
    }

}