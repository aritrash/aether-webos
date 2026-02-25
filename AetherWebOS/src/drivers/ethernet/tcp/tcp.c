#include <stdint.h>
#include <stddef.h>

#include "drivers/ethernet/tcp/tcp.h"
#include "drivers/ethernet/tcp/tcp_internal.h"
#include "kernel/memory.h"
#include "drivers/uart.h"
#include "kernel/timer.h" // For ISN generation

/* ============================================================
 * GLOBAL TCP STATE
 * ============================================================ */

tcp_tcb_t *tcp_tcb_list = NULL;

/* * aether_ip must be Host Order as defined in kernel.c 
 */
extern uint32_t aether_ip;

/* ============================================================
 * TCB MANAGEMENT
 * ============================================================ */

/**
 * Generates an Initial Sequence Number based on system clock.
 * Prevents sequence overlap across reboots/reconnects.
 */
static uint32_t tcp_generate_isn(void) {
    return (uint32_t)(get_system_uptime_ms() * 250);
}

tcp_tcb_t *tcp_allocate_tcb(void) {
    tcp_tcb_t *tcb = (tcp_tcb_t *)kmalloc(sizeof(tcp_tcb_t));
    if (!tcb) {
        uart_debugps("[TCP] FATAL: TCB allocation failed\n");
        return NULL;
    }

    // Zero entire TCB for safety
    memset(tcb, 0, sizeof(tcp_tcb_t));

    tcb->state = TCP_STATE_CLOSED;
    tcb->rcv_wnd = TCP_DEFAULT_WINDOW; // Usually 4096-8192
    tcb->snd_wnd = TCP_DEFAULT_WINDOW;
    tcb->local_ip = aether_ip;

    /* Link into the global connection list */
    tcb->next = tcp_tcb_list;
    tcp_tcb_list = tcb;

    return tcb;
}

void tcp_remove_tcb(tcp_tcb_t *tcb) {
    if (!tcb) return;

    tcp_tcb_t *prev = NULL;
    tcp_tcb_t *cur = tcp_tcb_list;

    while (cur) {
        if (cur == tcb) {
            if (prev) prev->next = cur->next;
            else tcp_tcb_list = cur->next;

            kfree(cur);
            uart_debugps("[TCP] TCB purged from memory\n");
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

/**
 * Strict 4-tuple lookup: Source IP, Source Port, Dest IP, Dest Port.
 * This is vital because Chrome opens multiple ports simultaneously.
 */
tcp_tcb_t *tcp_find_tcb(uint32_t src_ip, uint16_t src_port, 
                        uint32_t dst_ip, uint16_t dst_port) {
    tcp_tcb_t *cur = tcp_tcb_list;

    while (cur) {
        if (cur->remote_ip   == src_ip   &&
            cur->remote_port == src_port &&
            cur->local_ip    == dst_ip   &&
            cur->local_port  == dst_port) 
        {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/* ============================================================
 * CONNECTION CONTROL
 * ============================================================ */

void tcp_send_data(tcp_tcb_t *tcb, const uint8_t *data, uint16_t len) {
    if (!tcb || tcb->state != TCP_STATE_ESTABLISHED) {
        uart_debugps("[TCP] Send failed: Connection not ESTABLISHED\n");
        return;
    }

    // PSH (Push) flag ensures Chrome processes the HTML immediately
    tcp_send_segment(tcb, TCP_FLAG_PSH | TCP_FLAG_ACK, data, len);
}

void tcp_close(tcp_tcb_t *tcb) {
    if (!tcb) return;

    // Standard state machine transition for active close
    if (tcb->state == TCP_STATE_ESTABLISHED || tcb->state == TCP_STATE_CLOSE_WAIT) {
        tcb->state = TCP_STATE_LAST_ACK;
        tcp_send_fin(tcb);
        uart_debugps("[TCP] Active close -> LAST_ACK\n");
    }
}

void tcp_init(void) {
    tcp_tcb_list = NULL;
    uart_debugps("[TCP] Core Stack Ready\n");
}