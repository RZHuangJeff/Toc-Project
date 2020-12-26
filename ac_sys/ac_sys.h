#ifndef AC_SYS_H
#define AC_SYS_H

#include <time.h>

typedef enum {
    WAIT_INPUT,

    REG_FIRST_USE,
    REG_VALID_NAME,
    REG_DUP_NAME,

    HELP_SHOW_CMD,

    JOIN_FAM,
    JOIN_SUCC,

    UNLOCK_PICK_DOOR,
    UNLOCK_SUCC,

    SHARE_PICK_DOOR,
    SHARE_TO_WHO,
    SHARE_PICK_PERIOD,
    SHARE_SUCC,

    UNKNOWN_CMD,
    UNKNOWN_ID,
    NO_PERMISSION,
} public_state_t;

typedef void (*ac_sys_cback_t)(char *userid, void *arg);

typedef struct ac_sys_door_t {
    char *door_id;
    time_t *valid_time;

    struct ac_sys_door_t *next;
} ac_sys_door_t;

void ac_sys_init();
void ac_sys_add_cback(public_state_t state, ac_sys_cback_t cback);
void ac_sys_do_command(char *userid, char *command);

#endif