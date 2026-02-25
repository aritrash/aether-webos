#include "drivers/ethernet/tcp_state.h"
#include "kernel/timer.h"
#include "drivers/uart.h"
#include "common/utils.h"
#include "config.h"

struct tcp_control_block tcb_table[MAX_TCP_CONN];

/* ===================================================== */
/* Initialize TCP Table                                  */
/* ===================================================== */

void tcp_init_stack(void)
{
    for (int i = 0; i < MAX_TCP_CONN; i++)
    {
        memset(&tcb_table[i], 0, sizeof(struct tcp_control_block));
        tcb_table[i].state = TCP_CLOSED;
    }

    uart_puts("[OK] TCP State Table Initialized.\r\n");
}

/* ===================================================== */
/* Allocate TCB                                          */
/* ===================================================== */

struct tcp_control_block* tcp_allocate_tcb(void)
{
    for (int i = 0; i < MAX_TCP_CONN; i++)
    {
        if (tcb_table[i].state == TCP_CLOSED)
        {
            memset(&tcb_table[i], 0,
                   sizeof(struct tcp_control_block));

            tcb_table[i].rcv_wnd = 8192;
            tcb_table[i].last_activity =
                get_system_uptime_ms();

            return &tcb_table[i];
        }
    }

    uart_puts("[WARN] TCP: No free TCB slot.\r\n");
    return 0;
}

/* ===================================================== */
/* Find TCB (match remote endpoint)                      */
/* ===================================================== */

struct tcp_control_block*
tcp_find_tcb(uint32_t remote_ip,
             uint16_t remote_port)
{
    for (int i = 0; i < MAX_TCP_CONN; i++)
    {
        if (tcb_table[i].state != TCP_CLOSED &&
            tcb_table[i].remote_ip == remote_ip &&
            tcb_table[i].remote_port == remote_port)
        {
            return &tcb_table[i];
        }
    }

    return 0;
}

/* ===================================================== */
/* Free TCB                                              */
/* ===================================================== */

void tcp_free_tcb(struct tcp_control_block *tcb)
{
    if (!tcb)
        return;

    memset(tcb, 0, sizeof(struct tcp_control_block));
    tcb->state = TCP_CLOSED;
}

/* ===================================================== */
/* Active Connection Count                               */
/* ===================================================== */

int tcp_get_active_count(void)
{
    int count = 0;

    for (int i = 0; i < MAX_TCP_CONN; i++)
    {
        if (tcb_table[i].state == TCP_ESTABLISHED)
            count++;
    }

    return count;
}

/* ===================================================== */
/* Optional SYN Timeout Watchdog                         */
/* ===================================================== */

void tcp_reap_syn_timeouts(uint64_t timeout_ms)
{
    uint64_t now = get_system_uptime_ms();

    for (int i = 0; i < MAX_TCP_CONN; i++)
    {
        if (tcb_table[i].state == TCP_SYN_RCVD)
        {
            if ((now - tcb_table[i].last_activity) >
                timeout_ms)
            {
                uart_puts("[TCP] Reaping stale SYN_RCVD.\r\n");
                tcp_free_tcb(&tcb_table[i]);
            }
        }
    }
}