#ifndef KHICAS_AI_USB_H
#define KHICAS_AI_USB_H
#define KHICAS_AI_REQUEST_MAX 1024
#define KHICAS_AI_REPLY_MAX 2048
#define KHICAS_AI_CANCELLED (-2)
#ifdef __cplusplus
extern "C" {
#endif
/* Returns 0 for text, 1 for a remote error, -1 for a transport error, -2 for cancellation.
   Reply requires KHICAS_AI_REPLY_MAX + 1 bytes and is always NUL terminated. */
int khicas_ai_exchange(const char * request, unsigned size, char * reply);
#ifdef __cplusplus
}
#endif
#endif
