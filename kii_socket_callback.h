#ifndef _KII_SOCKET_CALLBACK
#define _KII_SOCKET_CALLBACK

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum kii_http_error_t {
    /** No error. */
    KII_HTTP_ERROR_NONE,
    /** Invalid response. */
    KII_HTTP_ERROR_INVALID_RESPONSE,
    /** Response buffer overflow. */
    KII_HTTP_ERROR_INSUFFICIENT_BUFFER,
    /** Socket functions returns false. */
    KII_HTTP_ERROR_SOCKET
} kii_http_error_t;

typedef struct kii_socket_context_t {
    /** Application specific context object.
     * Used by socket callback implementations.
     */
    void* app_context;

    /** Application specific socket.
     * Used by socket callback implementations.
     */
    int socket;

    /** HTTP client error. */
    kii_http_error_t http_error;
} kii_socket_context_t;

typedef enum kii_socket_code_t {
    /** Retrun this code when operation succeed. */
    KII_SOCKETC_OK,

    /** Return this code when operation failed. */
    KII_SOCKETC_FAIL,

    /** Return this code when operation is in progress.
     *
     *  SDK calls the callback again until the callbacks returns
     *  KII_SOCKETC_OK or KII_SOCKETC_FAIL.
     */
    KII_SOCKETC_AGAIN
} kii_socket_code_t;

/** Callback for connecting socket to server.
 * Applications must implement this callback in the target enviroment.
 *
 * @param [in] socket_context context object.
 * @param [in] host host name.
 * @param [in] port port number.
 * @return If connection is succeeded, applications need to return
 * KII_SOCKETC_OK. If connection is failed, applications need to
 * return KII_SOCKETC_FAIL. If applications want to pend returning
 * success or fail, applications need to return KII_SOCKETC_AGAIN.
 */
typedef kii_socket_code_t
    (*KII_SOCKET_CONNECT_CB)
    (kii_socket_context_t* socket_context, const char* host, unsigned int port);

/** Callback for sending data to server.
 * Applications must implement this callback in the target enviroment.
 *
 * @param [in] socket_context context object.
 * @param [in] buffer data to send server.
 * @param [in] length length of buffer.

 * @return If applications succeed to send data, applications need to
 * return KII_SOCKETC_OK. If connection is failed. applications need
 * to return KII_SOCKETC__FAIL. If applications don't want to send
 * data, applications need to return KII_SOCKETC_AGAIN. In this case,
 * KiiThingSDK Embedded Core pass same data to this callback again.
 */
typedef kii_socket_code_t
    (*KII_SOCKET_SEND_CB)
    (kii_socket_context_t* socket_context, const char* buffer, size_t length);

/** Callback for receiving data from server.
 * Applications must implement this callback in the target enviroment.
 *
 * @param [in] socket_context context object.
 * @param [out] buffer buffer to set receiving data.
 * @param [in] length_to_read buffer size.
 * @param [out] out_actual_length actual set data size.
 * @return If applications succeed to receive data and set the data to
 * buffer, applications need to return KII_SOCKETC_OK. Applications
 * also set data size to out_actual_length. If applications fail,
 * applications need to return KII_HTTPC_FAIL. If applications want to
 * wait to receive data, applications need to return
 * KII_HTTPC_AGAIN. In this case, applications must not set receving
 * data to buffer if some data is received.
 */
typedef kii_socket_code_t
    (*KII_SOCKET_RECV_CB)
    (kii_socket_context_t* socket_context, char* buffer, size_t length_to_read,
     size_t* out_actual_length);

/** Callback for closing socket.
 * Applications must implement this callback in the target enviroment.
 *
 * @param [in] socket_context context object.
 *
 * @return If applications succeed to close socket, applications need
 * to return KII_SOCKETC_OK. If applications fail to close socket,
 * applications need to return KII_SOCKETC_FAIL. If applications want
 * to pend returning success or fail, applications need to return
 * KII_SOCKETC_AGAIN.
 */
typedef kii_socket_code_t
    (*KII_SOCKET_CLOSE_CB)(kii_socket_context_t* socket_context);


#ifdef __cplusplus
}
#endif

#endif /* _KII_SOCKET_CALBACK */
/* vim:set ts=4 sts=4 sw=4 et fenc=UTF-8 ff=unix: */
