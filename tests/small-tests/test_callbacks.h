#include <khc_socket_callback.h>
#include <functional>

typedef struct sock_ctx {
  std::function<khc_sock_code_t(void* socket_context, const char* host, unsigned int port)> on_connect;
  std::function<khc_sock_code_t(void* socket_context, const char* buffer, size_t length)> on_send;
  std::function<khc_sock_code_t(void* socket_context, char* buffer, size_t length_to_read, size_t* out_actual_length)> on_recv;
  std::function<khc_sock_code_t(void* socket_context)> on_close;
} sock_ctx;

typedef struct io_ctx {
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_read;
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_header;
  std::function<size_t(char *buffer, size_t size, size_t count, void *userdata)> on_write;
} io_ctx;

inline khc_sock_code_t cb_connect(void* socket_context, const char* host, unsigned int port) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_connect(socket_context, host, port);
}

inline khc_sock_code_t cb_send (void* socket_context, const char* buffer, size_t length) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_send(socket_context, buffer, length);
}

inline khc_sock_code_t cb_recv(void* socket_context, char* buffer, size_t length_to_read, size_t* out_actual_length) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_recv(socket_context, buffer, length_to_read, out_actual_length);
}

inline khc_sock_code_t cb_close(void* socket_context) {
  sock_ctx* ctx = (sock_ctx*)socket_context;
  return ctx->on_close(socket_context);
}

inline size_t cb_write(char *buffer, size_t size, size_t count, void *userdata) {
  io_ctx* ctx = (io_ctx*)(userdata);
  return ctx->on_write(buffer, size, count, userdata);
}

inline size_t cb_read(char *buffer, size_t size, size_t count, void *userdata) {
  io_ctx* ctx = (io_ctx*)(userdata);
  return ctx->on_read(buffer, size, count, userdata);
}

inline size_t cb_header(char *buffer, size_t size, size_t count, void *userdata) {
  io_ctx* ctx = (io_ctx*)(userdata);
  return ctx->on_header(buffer, size, count, userdata);
}
