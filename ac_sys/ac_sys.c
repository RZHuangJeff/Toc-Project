#include "ac_sys.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../fsm/fsm.h"
#include "../json/json.h"

#define CALLBACK(state, uid, arg) {if(cbacks[state]) cbacks[state](uid, arg);}

typedef enum {
    START = 16,
    UNLOCK_HAS_PERM,
    SHARE_HAS_PERM,
} private_state_t;

static bool is_to_first_use(fsm_arg_t *arg);
static bool is_to_dup_name(fsm_arg_t *arg);
static bool is_to_help(fsm_arg_t *arg);
static bool is_to_join_fam(fsm_arg_t *arg);
static bool is_to_join_succ(fsm_arg_t *arg);
static bool is_to_unlock(fsm_arg_t *arg);
static bool is_to_unlock_pick_door(fsm_arg_t *arg);
static bool is_to_unlock_succ(fsm_arg_t *arg);
static bool is_to_share(fsm_arg_t *arg);
static bool is_to_share_pick_door(fsm_arg_t *arg);
static bool is_to_share_to_who(fsm_arg_t *arg);
static bool is_to_share_pick_period(fsm_arg_t *arg);

static void do_first_use(fsm_arg_t *arg);
static void do_valid_name(fsm_arg_t *arg);
static void do_dup_name(fsm_arg_t *arg);
static void do_wait_input(fsm_arg_t *arg);
static void do_help(fsm_arg_t *arg);
static void do_join_fam(fsm_arg_t *arg);
static void do_join_succ(fsm_arg_t *arg);
static void do_unlock_pick_door(fsm_arg_t *arg);
static void do_unlock_succ(fsm_arg_t *arg);
static void do_share_pick_door(fsm_arg_t *arg);
static void do_share_to_who(fsm_arg_t *arg);
static void do_share_pick_period(fsm_arg_t *arg);
static void do_share_succ(fsm_arg_t *arg);
static void do_unknown_cmd(fsm_arg_t *arg);
static void do_unknown_id(fsm_arg_t *arg);
static void do_no_permission(fsm_arg_t *arg);

fsm_state_t states[] = {
    {
        .state_id = START,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {REG_FIRST_USE, is_to_first_use},
            {WAIT_INPUT, NULL}
        },
        .cback = NULL,
    },
    {
        .state_id = WAIT_INPUT,
        .trans_cnt = 5,
        .transitions = (fsm_transition_t[]){
            {HELP_SHOW_CMD, is_to_help},
            {JOIN_FAM, is_to_join_fam},
            {UNLOCK_HAS_PERM, is_to_unlock},
            {SHARE_HAS_PERM, is_to_share},
            {UNKNOWN_CMD, NULL}
        },
        .cback = do_wait_input,
    },
    {
        .state_id = REG_FIRST_USE,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {REG_DUP_NAME, is_to_dup_name},
            {REG_VALID_NAME, NULL}
        },
        .cback = do_first_use,
    },
    {
        .state_id = REG_VALID_NAME,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {START, NULL}
        },
        .cback = do_valid_name,
    },
    {
        .state_id = REG_DUP_NAME,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {REG_FIRST_USE, NULL}
        },
        .cback = do_dup_name,
    },
    {
        .state_id = HELP_SHOW_CMD,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {WAIT_INPUT, NULL}
        },
        .cback = do_help,
    },
    {
        .state_id = JOIN_FAM,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {JOIN_SUCC, is_to_join_succ},
            {UNKNOWN_ID, NULL}
        },
        .cback = do_join_fam,
    },
    {
        .state_id = JOIN_SUCC,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {WAIT_INPUT, NULL}
        },
        .cback = do_join_succ,
    },
    {
        .state_id = UNLOCK_HAS_PERM,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {UNLOCK_PICK_DOOR, is_to_unlock_pick_door},
            {NO_PERMISSION, NULL}
        },
        .cback = NULL,
    },
    {
        .state_id = UNLOCK_PICK_DOOR,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {UNLOCK_SUCC, is_to_unlock_succ},
            {NO_PERMISSION, NULL}
        },
        .cback = do_unlock_pick_door,
    },
    {
        .state_id = UNLOCK_SUCC,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {WAIT_INPUT, NULL}
        },
        .cback = do_unlock_succ,
    },
    {
        .state_id = SHARE_HAS_PERM,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {SHARE_PICK_DOOR, is_to_share_pick_door},
            {NO_PERMISSION, NULL}
        },
        .cback = NULL,
    },
    {
        .state_id = SHARE_PICK_DOOR,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {SHARE_TO_WHO, is_to_share_to_who},
            {NO_PERMISSION, NULL}
        },
        .cback = do_share_pick_door,
    },
    {
        .state_id = SHARE_TO_WHO,
        .trans_cnt = 2,
        .transitions = (fsm_transition_t[]){
            {SHARE_PICK_PERIOD, is_to_share_pick_period},
            {UNKNOWN_ID, NULL}
        },
        .cback = do_share_to_who,
    },
    {
        .state_id = SHARE_PICK_PERIOD,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {SHARE_SUCC, NULL}
        },
        .cback = do_share_pick_period,
    },
    {
        .state_id = SHARE_SUCC,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {WAIT_INPUT, NULL}
        },
        .cback = do_share_succ,
    },
    {
        .state_id = UNKNOWN_CMD,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {HELP_SHOW_CMD, NULL}
        },
        .cback = do_unknown_cmd,
    },
    {
        .state_id = UNKNOWN_ID,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {WAIT_INPUT, NULL}
        },
        .cback = do_unknown_id,
    },
    {
        .state_id = NO_PERMISSION,
        .trans_cnt = 1,
        .transitions = (fsm_transition_t[]){
            {WAIT_INPUT, NULL}
        },
        .cback = do_no_permission,
    },
};

typedef struct {
    char *userid;
    char *command;
} ac_sys_input_t;

typedef struct ac_sys_user_t {
    char *name;
    char *family_id;
    long door_cnt;
    ac_sys_door_t *door_list;
    ac_sys_door_t *s_door;
    struct ac_sys_user_t *who;
    fsm_t *fsm;
} ac_sys_user_t;

json_ele_t *user_table;
ac_sys_cback_t cbacks[16];

void ac_sys_init(){
    user_table = json_new_ele(JSON_OBJ);

    memset(cbacks, 0, sizeof(ac_sys_cback_t)*16);
}

void ac_sys_add_cback(public_state_t state, ac_sys_cback_t cback){
    cbacks[state] = cback;
}

void ac_sys_do_command(char *userid, char *command){
    ac_sys_input_t input = {
        .userid = userid,
        .command = command
    };

    ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(userid));
    if(!user){
        user = (ac_sys_user_t*)malloc(sizeof(ac_sys_user_t));
        user->name = NULL;
        user->family_id = NULL;
        user->door_list = NULL;
        user->fsm = (fsm_t*)malloc(sizeof(fsm_t));
        user->fsm->cur_state_id = START;
        user->fsm->state_cnt = 19;
        user->fsm->states = states;

        json_object_add(user_table, J_STR_TAG(userid), (json_ele_t*)user);
    }

    fsm_step(user->fsm, &input);
}



static bool is_to_first_use(fsm_arg_t *arg){
    ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

    ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));

    return !user->name;
}

static bool is_to_dup_name(fsm_arg_t *arg){
    ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

    json_kvpair_t *pairs = J_TO_OBJ(user_table);
    for(size_t i = 0; i < user_table->cont_len; i++){
        ac_sys_user_t *user = (ac_sys_user_t*)pairs->value;
        if(user->name && !strcmp(user->name, input->command))
            return true;
    }

    return false;
}

static bool is_to_help(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        return !strcasecmp("help", input->command);
    }
    return false;
}

static bool is_to_join_fam(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        return !strcasecmp("join", input->command);
    }
    return false;
}

static bool is_to_join_succ(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        return !strcasecmp("test-family-id", input->command);
    }
    return false;
}

static bool is_to_unlock(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        return !strcasecmp("unlock", input->command);
    }
    return false;
}

static bool is_to_unlock_pick_door(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        time_t cur_time = time(NULL);
        if(user->door_list){
            for(ac_sys_door_t **ptr = &(user->door_list); *ptr;){
                if((*ptr)->valid_time && cur_time > *(*ptr)->valid_time){
                    ac_sys_door_t *del = *ptr;
                    *ptr = (*ptr)->next;
                    free(del->valid_time);
                    free(del);
                }
                else{
                    ptr = &((*ptr)->next);
                }
            }
        }

        return user->door_list;
    }
    return false;
}

static bool is_to_unlock_succ(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;
    
        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        for(ac_sys_door_t *ptr = user->door_list; ptr; ptr = ptr->next)
            if(!strcmp(ptr->door_id, input->command))
                return true;
    }
    return false;
}

static bool is_to_share(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        return !strcasecmp("share", input->command);
    }
    return false;
}

static bool is_to_share_pick_door(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        
        return user->family_id;
    }
    return false;
}

static bool is_to_share_to_who(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        for(ac_sys_door_t *ptr = user->door_list; ptr; ptr = ptr->next)
            if(!strcmp(ptr->door_id, input->command)){
                user->s_door = ptr;
                return true;
            }
    }
    return false;
}

static bool is_to_share_pick_period(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        
        json_kvpair_t *pairs = J_TO_OBJ(user_table);
        for(size_t i = 0; i < user_table->cont_len; i++){
            ac_sys_user_t *touser = (ac_sys_user_t*)pairs[i].value;
            if(!strcmp(touser->name, input->command)){
                user->who = touser;
                return true;
            }
        }
    }
    return false;
}



static void do_first_use(fsm_arg_t *arg){
    CALLBACK(REG_FIRST_USE, ((ac_sys_input_t*)arg->input)->userid, NULL);
    arg->input = NULL;
}

static void do_valid_name(fsm_arg_t *arg){
    ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

    ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
    printf("%s\n", input->command);
    user->name = (char*)malloc(strlen(input->command) + 1);
    memcpy(user->name, input->command, strlen(input->command) + 1);

    CALLBACK(REG_VALID_NAME, input->userid, input->command);
}

static void do_dup_name(fsm_arg_t *arg){
    CALLBACK(REG_DUP_NAME, ((ac_sys_input_t*)arg->input)->userid, NULL);
}

static void do_wait_input(fsm_arg_t *arg){
    CALLBACK(WAIT_INPUT, ((ac_sys_input_t*)arg->input)->userid, NULL);
    arg->input = NULL;
}

static void do_help(fsm_arg_t *arg){
    CALLBACK(HELP_SHOW_CMD, ((ac_sys_input_t*)arg->input)->userid, NULL);

}

static void do_join_fam(fsm_arg_t *arg){
    CALLBACK(JOIN_FAM, ((ac_sys_input_t*)arg->input)->userid, NULL);
    arg->input = NULL;
}

static void do_join_succ(fsm_arg_t *arg){
    ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

    static char *doors[] = {
        "door3", "door2", "door1"
    };

    ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
    user->family_id = (char*)malloc(strlen(input->command) + 1);
    memcpy(user->family_id, input->command, strlen(input->command) + 1);

    user->door_cnt = 3;
    for(int i = 0; i < 3; i++){
        ac_sys_door_t *door = (ac_sys_door_t*)malloc(sizeof(ac_sys_door_t));
        door->door_id = doors[i];
        door->valid_time = NULL;

        door->next = user->door_list;
        user->door_list = door;
    }

    CALLBACK(JOIN_SUCC, ((ac_sys_input_t*)arg->input)->userid, NULL);
}

static void do_unlock_pick_door(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;
        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        CALLBACK(UNLOCK_PICK_DOOR, input->userid, user->door_list);
    }

    arg->input = NULL;
}

static void do_unlock_succ(fsm_arg_t *arg){
    ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

    CALLBACK(UNLOCK_SUCC, input->userid, input->command);
}

static void do_share_pick_door(fsm_arg_t *arg){
    if(arg->input){
        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;
        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        CALLBACK(SHARE_PICK_DOOR, input->userid, user->door_list);
    }

    arg->input = NULL;
}

static void do_share_to_who(fsm_arg_t *arg){
    CALLBACK(SHARE_TO_WHO, ((ac_sys_input_t*)arg->input)->userid, NULL);
    arg->input = NULL;
}

static void do_share_pick_period(fsm_arg_t *arg){
    CALLBACK(SHARE_PICK_PERIOD, ((ac_sys_input_t*)arg->input)->userid, NULL);
    arg->input = NULL;
}

static void do_share_succ(fsm_arg_t *arg){
    if(arg->input){

        ac_sys_input_t *input = (ac_sys_input_t*)arg->input;

        printf("%ld\n", atol(input->command));
        ac_sys_user_t *user = (ac_sys_user_t*)json_object_at(user_table, J_STR_TAG(input->userid));
        ac_sys_user_t *touser = user->who;

        ac_sys_door_t *door = (ac_sys_door_t*)malloc(sizeof(ac_sys_door_t));
        door->door_id = user->s_door->door_id;
        door->valid_time = (time_t*)malloc(sizeof(time_t));
        *door->valid_time = time(NULL) + atol(input->command)*60;

        door->next = touser->door_list;
        touser->door_list = door;

        CALLBACK(SHARE_SUCC, input->userid, NULL);
    }
}

static void do_unknown_cmd(fsm_arg_t *arg){
    CALLBACK(UNKNOWN_CMD, ((ac_sys_input_t*)arg->input)->userid, NULL);
}

static void do_unknown_id(fsm_arg_t *arg){
    CALLBACK(UNKNOWN_ID, ((ac_sys_input_t*)arg->input)->userid, NULL);
}

static void do_no_permission(fsm_arg_t *arg){
    CALLBACK(NO_PERMISSION, ((ac_sys_input_t*)arg->input)->userid, NULL);
}