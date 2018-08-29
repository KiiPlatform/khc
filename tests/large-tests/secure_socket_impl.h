#ifndef __KII_CORE_SOCKET
#define __KII_CORE_SOCKET

#include "khc_socket_callback.h"
#include <openssl/ssl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ssl_context
{
    SSL *ssl;
    SSL_CTX *ssl_ctx;
    int socket;
} ssl_context_t;

khc_sock_code_t
    s_connect_cb(void* socket_context, const char* host,
            unsigned int port);

khc_sock_code_t
    s_send_cb(void* socket_context,
            const char* buffer,
            size_t length);

khc_sock_code_t
    s_recv_cb(void* socket_context,
            char* buffer,
            size_t length_to_read,
            size_t* out_actual_length);

khc_sock_code_t
    s_close_cb(void* socket_context);


#ifdef __cplusplus
}
#endif

#endif /* __KII_CORE_SOCKET */
