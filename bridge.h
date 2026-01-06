#ifndef BRIDGE_H
#define BRIDGE_H

// Global state flags (defined in main.c)
extern volatile int running;
extern volatile int vts_ready;
extern volatile int vts_data_detected;

void run_bridge(void);
void drain_iphone_data(void);

#endif // BRIDGE_H
