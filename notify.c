#include "notify.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

void send_notification(const char *title, const char *message, const char *urgency) {
    (void)urgency; 
    
    pid_t pid = fork();
    if (pid == 0) {
       
        char *display = getenv("DISPLAY");
        char *dbus = getenv("DBUS_SESSION_BUS_ADDRESS");
        
        if (!display) setenv("DISPLAY", ":0", 0);
        if (!dbus) {
            char dbus_path[256];
            snprintf(dbus_path, sizeof(dbus_path), "/run/user/%d/bus", getuid());
            if (access(dbus_path, F_OK) == 0) {
                char dbus_addr[300];
                snprintf(dbus_addr, sizeof(dbus_addr), "unix:path=%s", dbus_path);
                setenv("DBUS_SESSION_BUS_ADDRESS", dbus_addr, 0);
            }
        }

        // I DONT CARE THAT THE APP DOESNT HAVE ID OR WHATEVER 
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        
        execlp("notify-send", "notify-send", title, message, NULL);
        _exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, WNOHANG);
    }
}
