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

void agent_pipeline_init(void);
void agent_pipeline_step(void); // Call this periodically from main loop or a thread
agent_state_t agent_get_state(void);
const char* agent_get_last_user_text(void);
const char* agent_get_last_agent_text(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // AGENT_PIPELINE_H
