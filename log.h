#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <syslog.h>

extern int daemon_mode;

// 3 = LOG_ERR, 4 = LOG_WARNING, 5 = LOG_NOTICE, 6 = LOG_INFO

#define LOGMSG_INFO(fmt, ...) do { \
    if (daemon_mode) syslog(6, fmt, ##__VA_ARGS__); \
    else printf("[*] " fmt "\n", ##__VA_ARGS__); \
} while(0)

#define LOGMSG_WARN(fmt, ...) do { \
    if (daemon_mode) syslog(4, fmt, ##__VA_ARGS__); \
    else printf("[!] " fmt "\n", ##__VA_ARGS__); \
} while(0)

#define LOGMSG_ERR(fmt, ...) do { \
    if (daemon_mode) syslog(3, fmt, ##__VA_ARGS__); \
    else printf("[-] " fmt "\n", ##__VA_ARGS__); \
} while(0)

#define LOGMSG_OK(fmt, ...) do { \
    if (daemon_mode) syslog(5, fmt, ##__VA_ARGS__); \
    else printf("[+] " fmt "\n", ##__VA_ARGS__); \
} while(0)

#endif // LOG_H
