#include <stdint.h>
#include <stddef.h>

#include "drivers/ethernet/socket.h"
#include "drivers/ethernet/tcp/tcp.h"
#include "drivers/uart.h"

/* ============================================================
 *                  STATIC HTML CONTENT
 * ============================================================ */

static const char html_body[] =
"<html>\r\n"
"<head><title>Aether WebOS</title></head>\r\n"
"<body>\r\n"
"<h1>Welcome to Aether WebOS</h1>\r\n"
"<p>Custom TCP Stack Running Bare Metal AArch64</p>\r\n"
"</body>\r\n"
"</html>\r\n";

static const char http_header_prefix[] =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Content-Length: ";

/* ============================================================
 *                  SIMPLE HTTP CHECK
 * ============================================================ */

static int is_http_get(uint8_t *data, uint16_t len)
{
    if (len < 3)
        return 0;

    if (data[0] == 'G' &&
        data[1] == 'E' &&
        data[2] == 'T')
        return 1;

    return 0;
}

/* ============================================================
 *                  SOCKET DISPATCH
 * ============================================================ */

void socket_dispatch(tcp_tcb_t *tcb,
                     uint8_t *payload,
                     uint16_t len)
{
    uart_debugps("[SOCKET] Packet received\n");

    uart_debugps("[SOCKET] First bytes: ");
    uart_debugpc(payload[0]);
    uart_debugpc(payload[1]);
    uart_debugpc(payload[2]);
    uart_debugps("\n");

    uart_debugps("[SOCKET] HTTP GET detected\n");

    /* Build HTTP response in stack buffer */
    char response[1024];
    int offset = 0;

    /* Copy header prefix */
    for (int i = 0; http_header_prefix[i] != 0; i++)
        response[offset++] = http_header_prefix[i];

    /* Calculate body length */
    int body_len = sizeof(html_body) - 1;

    /* Convert body_len to decimal string */
    int temp = body_len;
    char digits[10];
    int digit_count = 0;

    do {
        digits[digit_count++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);

    for (int i = digit_count - 1; i >= 0; i--)
        response[offset++] = digits[i];

    /* End of headers */
    response[offset++] = '\r';
    response[offset++] = '\n';
    response[offset++] = '\r';
    response[offset++] = '\n';

    /* Copy HTML body */
    for (int i = 0; i < body_len; i++)
        response[offset++] = html_body[i];

    uart_debugps("[SOCKET] Sending HTTP response\n");

    tcp_send_data(tcb, (uint8_t *)response, offset);

    tcp_close(tcb);
}