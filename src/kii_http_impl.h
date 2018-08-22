#ifndef __kii_http_impl
#define __kii_http_impl

#ifdef __cplusplus
extern "C"
{
#endif

#include "kii_http.h"

void kii_state_idle(kii_http* kii_http);
void kii_state_connect(kii_http* kii_http);
void kii_state_req_line(kii_http* kii_http);
void kii_state_req_header(kii_http* kii_http);
void kii_state_req_header_send(kii_http* kii_http);
void kii_state_req_header_send_crlf(kii_http* kii_http);
void kii_state_req_header_end(kii_http* kii_http);
void kii_state_req_body_read(kii_http* kii_http);
void kii_state_req_body_send(kii_http* kii_http);
void kii_state_resp_headers_alloc(kii_http* kii_http);
void kii_state_resp_headers_realloc(kii_http* kii_http);
void kii_state_resp_headers_read(kii_http* kii_http);
void kii_state_resp_headers_callback(kii_http* kii_http);
void kii_state_resp_body_flagment(kii_http* kii_http);
void kii_state_resp_body_read(kii_http* kii_http);
void kii_state_resp_body_callback(kii_http* kii_http);
void kii_state_close(kii_http* kii_http);

typedef void (*KII_STATE_HANDLER)(kii_http* kii_http);

extern const KII_STATE_HANDLER state_handlers[];

#ifdef __cplusplus
}
#endif

#endif //__kii_http_impl