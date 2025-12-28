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

// libimobiledevice headers
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <usbmuxd.h>

#define IPHONE_PORT 25561
#define SERVER_PORT 25565
#define BUF_SIZE 8192
#define MAX_EVENTS 4

pid_t iproxy_pid = -1;


int iphone_connected_and_paired() {
    char **udids = NULL;
    int count = 0;

    if (idevice_get_device_list(&udids, &count) != IDEVICE_E_SUCCESS || count == 0)
        return 0;

    idevice_t dev = NULL;
    lockdownd_client_t lockdown = NULL;
    lockdownd_error_t lerr;

    if (idevice_new(&dev, udids[0]) != IDEVICE_E_SUCCESS)
        goto fail;

    lerr = lockdownd_client_new_with_handshake(dev, &lockdown, "iproxy2vts");
    if (lerr != LOCKDOWN_E_SUCCESS)
        goto fail;

    lockdownd_client_free(lockdown);
    idevice_free(dev);
    idevice_device_list_free(udids);
    return 1;

fail:
    if (lockdown) lockdownd_client_free(lockdown);
    if (dev) idevice_free(dev);
    idevice_device_list_free(udids);
    return 0;
}


void start_iproxy() {
    if (iproxy_pid > 0) return;

    iproxy_pid = fork();
    if (iproxy_pid == 0) {
        // Starting iProxy process to forward ports
        // Unfortunately, i dont have time to re-make it here, as this is still just PoC
        // So i'm just gonna leave it like this, sorry :(
        execlp("iproxy", "iproxy", "25561", "25561", NULL);
        _exit(1);
    }

    usleep(500000); // let it bind
}

void stop_iproxy() {
    if (iproxy_pid > 0) {
        kill(iproxy_pid, SIGTERM);
        waitpid(iproxy_pid, NULL, 0);
        iproxy_pid = -1;
    }
}

// Opakujem IPK, nechajte ma tak 

int make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int connect_async(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    make_nonblocking(s);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    connect(s, (struct sockaddr *)&addr, sizeof(addr));
    return s;
}


int main() {
    printf("[*] Init bridge.\n");

    while (1) {
        if (!iphone_connected_and_paired()) {
            printf("[!] No paired iPhone. Waiting...\n");
            stop_iproxy();
            sleep(1);
            continue;
        }

        printf("[+] iPhone detected & paired. Locking connection.\n");
        start_iproxy();

        int iphone = connect_async(IPHONE_PORT);
        int server = connect_async(SERVER_PORT);

        int ep = epoll_create1(0);

        struct epoll_event ev1 = { .events = EPOLLIN, .data.fd = iphone };
        struct epoll_event ev2 = { .events = EPOLLIN, .data.fd = server };

        epoll_ctl(ep, EPOLL_CTL_ADD, iphone, &ev1);
        epoll_ctl(ep, EPOLL_CTL_ADD, server, &ev2);

        char buf[BUF_SIZE];
        struct epoll_event events[MAX_EVENTS];

        while (1) {
            int n = epoll_wait(ep, events, MAX_EVENTS, -1);
            if (n <= 0) break;

            for (int i = 0; i < n; i++) {
                int src = events[i].data.fd;
                int dst = (src == iphone) ? server : iphone;

                ssize_t r = recv(src, buf, BUF_SIZE, 0);
                if (r <= 0) goto reset;

                send(dst, buf, r, 0);
            }
        }

reset:
        // This happens often, I dont know why yet
        printf("[-] Connection lost. Re-syncing.\n");
        close(iphone);
        close(server);
        close(ep);
        stop_iproxy();
    }
}
