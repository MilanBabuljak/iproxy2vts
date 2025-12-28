#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

// libimobiledevice headers
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <usbmuxd.h>

// Flags
#define IPHONE_PORT 25561           // don't ask how i found this
#define SERVER_PORT 25565
#define BUF_SIZE 8192
#define MAX_EVENTS 16
#define ENABLE_HEARTBEAT_ECHO 1

pid_t iproxy_pid = -1;

int iphone_connected_and_paired() {
    char **udids = NULL;
    int count = 0;

    if (idevice_get_device_list(&udids, &count) != IDEVICE_E_SUCCESS || count == 0)
        return 0;

    idevice_t dev = NULL;
    lockdownd_client_t lockdown = NULL;
    
    if (idevice_new(&dev, udids[0]) != IDEVICE_E_SUCCESS) {
        idevice_device_list_free(udids);
        return 0;
    }

    if (lockdownd_client_new_with_handshake(dev, &lockdown, "iproxy2vts") != LOCKDOWN_E_SUCCESS) {
        idevice_free(dev);
        idevice_device_list_free(udids);
        return 0;
    }

    lockdownd_client_free(lockdown);
    idevice_free(dev);
    idevice_device_list_free(udids);
    return 1;
}

void start_iproxy() {
    if (iproxy_pid > 0) return;

    printf("[*] Launching iproxy...\n");
    iproxy_pid = fork();
    if (iproxy_pid == 0) {
        int null = open("/dev/null", O_WRONLY);
        dup2(null, 1);
        dup2(null, 2);
        execlp("iproxy", "iproxy", "25561", "25561", NULL);
        _exit(1);
    }
    usleep(500000); // Give it 0.5s to bind
}

void stop_iproxy() {
    if (iproxy_pid > 0) {
        kill(iproxy_pid, SIGTERM);
        waitpid(iproxy_pid, NULL, 0);
        iproxy_pid = -1;
    }
}

int connect_robust(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return -1;

    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&flag, sizeof(int));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
    
    return s;
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stdout, NULL, _IONBF, 0);

    while (1) {
        if (!iphone_connected_and_paired()) {
            printf("[!] Waiting for iPhone...\n");
            stop_iproxy();
            sleep(1);
            continue;
        }

        start_iproxy();
        printf("[+] iPhone found. Connecting sockets...\n");

        int iphone = -1, server = -1;
        
        for(int i=0; i<10; i++) {
            if (iphone < 0) iphone = connect_robust(IPHONE_PORT);
            if (server < 0) server = connect_robust(SERVER_PORT);
            if (iphone >= 0 && server >= 0) break;
            usleep(200000);
        }

        if (iphone < 0 || server < 0) {
            printf("[-] Socket connection failed. iPhone:%d Server:%d\n", iphone, server);
            if (iphone >= 0) close(iphone);
            if (server >= 0) close(server);
            stop_iproxy();
            sleep(1);
            continue;
        }

        printf("[+] Connected! Bridge is active.\n");

        int ep = epoll_create1(0);
        struct epoll_event ev1 = { .events = EPOLLIN, .data.fd = iphone };
        struct epoll_event ev2 = { .events = EPOLLIN, .data.fd = server };

        epoll_ctl(ep, EPOLL_CTL_ADD, iphone, &ev1);
        epoll_ctl(ep, EPOLL_CTL_ADD, server, &ev2);

        char buf[BUF_SIZE];
        struct epoll_event events[MAX_EVENTS];
        int active = 1;

        while (active) {
            int n = epoll_wait(ep, events, MAX_EVENTS, 3000);
            
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }

            for (int i = 0; i < n; i++) {
                int src = events[i].data.fd;
                int dst = (src == iphone) ? server : iphone;

                ssize_t r = recv(src, buf, BUF_SIZE, 0);

                if (r <= 0) {
                    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
                    printf("[-] Disconnected by %s\n", (src == iphone) ? "iPhone" : "Server");
                    active = 0;
                    break;
                }

                ssize_t sent = 0;
                while (sent < r) {
                    ssize_t w = send(dst, buf + sent, r - sent, 0);
                    if (w < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) { usleep(100); continue; }
                        active = 0; break;
                    }
                    sent += w;
                }

                if (src == iphone && ENABLE_HEARTBEAT_ECHO) {
                    send(iphone, buf, r, 0); 
                }
            }
        }

        printf("[-] Re-syncing...\n");
        close(iphone);
        close(server);
        close(ep);
        stop_iproxy();
        sleep(1);
    }
}