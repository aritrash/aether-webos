#ifndef AETHER_KERNEL_MODE_H
#define AETHER_KERNEL_MODE_H

typedef enum {
    MODE_PORTAL = 0,
    MODE_CONFIRM_SHUTDOWN,
    MODE_NET_STATS,
    MODE_DEBUG
} kernel_mode_t;

/* Global current mode */
extern kernel_mode_t current_mode;

#endif