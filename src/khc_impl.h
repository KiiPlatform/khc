#ifndef __khc_impl
#define __khc_impl

#ifdef __cplusplus
extern "C"
{
#endif

#include "khc.h"

void khc_state_idle(khc* khc);
void khc_state_connect(khc* khc);
void khc_state_req_line(khc* khc);
void khc_state_req_header(khc* khc);
void khc_state_req_header_send(khc* khc);
void khc_state_req_header_send_crlf(khc* khc);
void khc_state_req_header_end(khc* khc);
void khc_state_req_body_read(khc* khc);
void khc_state_req_body_send(khc* khc);
void khc_state_resp_headers_alloc(khc* khc);
void khc_state_resp_headers_realloc(khc* khc);
void khc_state_resp_headers_read(khc* khc);
void khc_state_resp_status_parse(khc* khc);
void khc_state_resp_headers_callback(khc* khc);
void khc_state_resp_body_flagment(khc* khc);
void khc_state_resp_body_read(khc* khc);
void khc_state_resp_body_callback(khc* khc);
void khc_state_close(khc* khc);

typedef void (*KHC_STATE_HANDLER)(khc* khc);

extern const KHC_STATE_HANDLER state_handlers[];

#ifdef __cplusplus
}
#endif

#endif //__khc_impl