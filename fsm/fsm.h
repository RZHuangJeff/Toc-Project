#ifndef FSM_H
#define FSM_H

#include <stdbool.h>

typedef struct fsm_arg_t fsm_arg_t;

typedef bool (*fsm_trans_func_t)(fsm_arg_t *arg);
typedef void (*fsm_cback_func_t)(fsm_arg_t *arg);

struct fsm_arg_t {
    void *input;
};

typedef struct {
    long dest_state_id;
    fsm_trans_func_t trans_func;
} fsm_transition_t;

typedef struct {
    long state_id;
    long trans_cnt;
    fsm_transition_t *transitions;
    fsm_cback_func_t cback;
} fsm_state_t;

typedef struct {
    long cur_state_id;
    long state_cnt;
    fsm_state_t *states;
} fsm_t;

void fsm_step(fsm_t *fsm, void *input);

#endif