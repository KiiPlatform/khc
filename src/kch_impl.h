#ifndef __kch_impl
#define __kch_impl

#ifdef __cplusplus
extern "C"
{
#endif

#include "kch.h"

void kch_state_idle(kch* kch);
void kch_state_connect(kch* kch);
void kch_state_req_line(kch* kch);
void kch_state_req_header(kch* kch);
void kch_state_req_header_send(kch* kch);
void kch_state_req_header_send_crlf(kch* kch);
void kch_state_req_header_end(kch* kch);
void kch_state_req_body_read(kch* kch);
void kch_state_req_body_send(kch* kch);
void kch_state_resp_headers_alloc(kch* kch);
void kch_state_resp_headers_realloc(kch* kch);
void kch_state_resp_headers_read(kch* kch);
void kch_state_resp_headers_callback(kch* kch);
void kch_state_resp_body_flagment(kch* kch);
void kch_state_resp_body_read(kch* kch);
void kch_state_resp_body_callback(kch* kch);
void kch_state_close(kch* kch);

typedef void (*KII_STATE_HANDLER)(kch* kch);

extern const KII_STATE_HANDLER state_handlers[];

#ifdef __cplusplus
}
#endif

#endif //__kch_impl