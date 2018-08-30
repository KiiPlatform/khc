#ifndef __KHC_CORE_SOCKET
#define __KHC_CORE_SOCKET

#include "khc_socket_callback.h"
#include <openssl/ssl.h>

namespace khct {
namespace ssl {

struct SSLData
{
    SSL *ssl;
    SSL_CTX *ssl_ctx;
    int socket;
};

khc_sock_code_t
    cb_connect(void* socket_context, const char* host,
            unsigned int port);

khc_sock_code_t
    cb_send(void* socket_context,
            const char* buffer,
            size_t length);

khc_sock_code_t
    cb_recv(void* socket_context,
            char* buffer,
            size_t length_to_read,
            size_t* out_actual_length);

khc_sock_code_t
    cb_close(void* socket_context);

}
}

#endif /* __KHC_CORE_SOCKET */