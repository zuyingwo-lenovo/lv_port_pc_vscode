#ifndef AGENT_PIPELINE_H
#define AGENT_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AGENT_STATE_WAITING = 0,
    AGENT_STATE_LISTENING,
    AGENT_STATE_PROCESSING,
    AGENT_STATE_SPEAKING
} agent_state_t;

void agent_pipeline_init(const char* tts_device);
void agent_pipeline_step(void); // Call this periodically from main loop or a thread
agent_state_t agent_get_state(void);
const char on* agent_get_last_user_text(void);
const char on* agent_get_last_agent_text(void);

// GUI -> Agent Requests
void agent_request_abort(void);
void agent_request_quit(void);
int agent_poll_abort(void); // Returns 1 if abort requested, 0 otherwise. Clears on call.
int agent_poll_quit(void);  // Returns 1 if quit requested, 0 otherwise. Clears on call.

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // AGENT_PIPELINE_H
