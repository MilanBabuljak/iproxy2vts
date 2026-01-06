#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>

#include "config.h"
#include "log.h"
#include "notify.h"
#include "iphone.h"
#include "service.h"

// global flags
int daemon_mode = 0;
volatile int running = 1;
volatile int vts_ready = 0;
volatile int vts_data_detected = 0;

static void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        LOGMSG_INFO("Received signal %d, shutting down...", sig);
        running = 0;
    }
}

static void print_usage(const char *progname) {
    printf("Usage: %s [OPTIONS]\n", progname);
    printf("\nOptions:\n");
    printf("  -d, --daemon     Run as a background daemon\n");
    printf("  -f, --foreground Run in foreground (default)\n");
    printf("  -h, --help       Show this help message\n");
    printf("\nDescription:\n");
    printf("  iproxy2vts bridges VTS tracking data between the iPhone app and VTS on the PC.\n");
    printf("\n");
    printf("\nFor systemd service, use: systemctl --user start iproxy2vts\n");
}

static int daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid > 0) {
        _exit(0);
    }

    // child becomes daemon
    setsid();
    
    // ios mindset
    pid = fork();
    if (pid < 0) _exit(1);
    if (pid > 0) _exit(0);

    if (chdir("/") != 0) {
        perror("chdir");
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    (void)open("/dev/null", O_RDONLY);
    (void)open("/dev/null", O_WRONLY);
    (void)open("/dev/null", O_WRONLY);

    openlog("iproxy2vts", LOG_PID, LOG_DAEMON);
    
    return 0;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemon") == 0) {
            daemon_mode = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--foreground") == 0) {
            daemon_mode = 0;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    if (daemon_mode) {
        if (daemonize() < 0) {
            return 1;
        }
    } else {
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    LOGMSG_INFO("iproxy2vts service starting...");
    send_notification("iproxy2vts Started", "Monitoring for iPhone connection...", "low");

    service_loop();

    stop_iproxy();
    
    if (daemon_mode) {
        closelog();
    }

    LOGMSG_INFO("iproxy2vts service stopped.");
    return 0;
}
