#include "fsm.h"

#include <stddef.h>

static fsm_state_t *get_cur_state(fsm_t *fsm);

void fsm_step(fsm_t *fsm, void *input){
    fsm_state_t *cur_state = get_cur_state(fsm);
    fsm_arg_t arg = {
        .input = input
    };
    
    do{
        fsm_transition_t *trans = cur_state->transitions;

        long next_state_id = fsm->cur_state_id;
        for(long i = 0; i < cur_state->trans_cnt; i++){
            if(!trans[i].trans_func || trans[i].trans_func(&arg)){
                next_state_id = trans[i].dest_state_id;
                break;
            }
        }

        if(next_state_id == fsm->cur_state_id)
            break;

        fsm->cur_state_id = next_state_id;
        cur_state = get_cur_state(fsm);

        if(cur_state->cback){
            cur_state->cback(&arg);
            
            if(!arg.input)
                break;
        }
    }while(1);
}

static fsm_state_t *get_cur_state(fsm_t *fsm){
    for(long i = 0; i < fsm->state_cnt; i++)
        if(fsm->states[i].state_id == fsm->cur_state_id)
            return fsm->states + i;

    return NULL;
}