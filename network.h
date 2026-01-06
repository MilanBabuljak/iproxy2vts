#ifndef NETWORK_H
#define NETWORK_H


int connect_nonblocking(int port, int timeout_ms);
int check_vts_available(void);
int probe_iphone_for_vts_data(int timeout_sec);

#endif // NETWORK_H
