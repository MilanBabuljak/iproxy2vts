#include "iphone.h"
#include "log.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

static pid_t iproxy_pid = -1;

int iphone_connected_and_paired(void) {
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

// will be eventually rewritten to use libimobiledevice directly
void start_iproxy(void) {
    if (iproxy_pid > 0) return;

    LOGMSG_INFO("Launching iproxy...");
    iproxy_pid = fork();
    if (iproxy_pid == 0) {
        int null = open("/dev/null", O_WRONLY);
        dup2(null, STDOUT_FILENO);
        dup2(null, STDERR_FILENO);
        close(null);
        execlp("iproxy", "iproxy", "25561", "25561", NULL);
        _exit(1);
    }
    usleep(500000); // Give it 0.5s to bind
}

void stop_iproxy(void) {
    if (iproxy_pid > 0) {
        kill(iproxy_pid, SIGTERM);
        waitpid(iproxy_pid, NULL, 0);
        iproxy_pid = -1;
    }
}
