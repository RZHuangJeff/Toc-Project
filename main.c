#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <wait.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "json/json.h"
#include "ac_sys/ac_sys.h"
#include "http_server/http_server.h"

char *secret, *actoken;
EVP_MAC_CTX *evp_mac_ctx;

void evp_ctx_init();
bool check_signature(const char *ver, const char *body);
void do_reply_msg(const char *userid, const char *message);
void do_reply_flex(const char *userid, json_ele_t *flex);

void post_root(http_request_t *req, http_response_t *res);

void first_use(char *userid, void *arg);
void valid_name(char *userid, void *arg);
void dup_name(char *userid, void *arg);
void wait_input(char *userid, void *arg);
void help_show_command(char *userid, void *arg);
void join_family(char *userid, void *arg);
void join_success(char *userid, void *arg);
void unlock_pick_door(char *userid, void *arg);
void unlock_success(char *userid, void *arg);
void unknown_cmd(char *userid, void *arg);
void unknown_id(char *userid, void *arg);
void no_permission(char *userid, void *arg);
void share_pick_door(char *userid, void *arg);
void share_to_who(char *userid, void *arg);
void share_pick_period(char *userid, void *arg);
void share_success(char *userid, void *arg);

int main(){
    secret = getenv("LINE_CHANNEL_SECRET_KEY");
    actoken = getenv("LINE_CHANNEL_ACCESS_TOKEN");
    
    evp_ctx_init();

    ac_sys_init();
    ac_sys_add_cback(REG_FIRST_USE, first_use);
    ac_sys_add_cback(REG_VALID_NAME, valid_name);
    ac_sys_add_cback(REG_DUP_NAME, dup_name);
    ac_sys_add_cback(WAIT_INPUT, wait_input);
    ac_sys_add_cback(HELP_SHOW_CMD, help_show_command);
    ac_sys_add_cback(JOIN_FAM, join_family);
    ac_sys_add_cback(JOIN_SUCC, join_success);
    ac_sys_add_cback(UNLOCK_PICK_DOOR, unlock_pick_door);
    ac_sys_add_cback(UNLOCK_SUCC, unlock_success);
    ac_sys_add_cback(SHARE_PICK_DOOR, share_pick_door);
    ac_sys_add_cback(SHARE_TO_WHO, share_to_who);
    ac_sys_add_cback(SHARE_PICK_PERIOD, share_pick_period);
    ac_sys_add_cback(SHARE_SUCC, share_success);
    ac_sys_add_cback(UNKNOWN_CMD, unknown_cmd);
    ac_sys_add_cback(UNKNOWN_ID, unknown_id);
    ac_sys_add_cback(NO_PERMISSION, no_permission);

    http_server_init(8080, stdout);
    http_server_add_route(POST, NULL, post_root);
    http_server_run();

    return 0;
}

void evp_ctx_init(){
    EVP_MAC *evp_mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
    evp_mac_ctx = EVP_MAC_CTX_new(evp_mac);

    OSSL_PARAM parms[] = {
        {"digest", OSSL_PARAM_UTF8_STRING, "SHA-256", 0, 0},
        {"key", OSSL_PARAM_OCTET_STRING, secret, strlen(secret), 0},
        OSSL_PARAM_END
    };
    EVP_MAC_CTX_set_params(evp_mac_ctx, parms);
}

void post_root(http_request_t *req, http_response_t *res){
    char *ver = http_header_get(req->header, "X-Line-Signature");
    
    if(!check_signature(ver, req->body))
        return;

    http_server_send_response(res);

    json_ele_t *events = json_object_at(json_parse(req->body), J_STR_TAG("events"));
    json_ele_t *event, *message, *source;
    char *event_type, *msg_type, *userid, *text;
    for(size_t i = 0; i < events->cont_len; i++){
        event = json_array_at(events, i);
        event_type = J_TO_STR(json_object_at(event, J_STR_TAG("type")));
        
        message = json_object_at(event, J_STR_TAG("message"));
        msg_type = J_TO_STR(json_object_at(message, J_STR_TAG("type")));
        userid = J_TO_STR(json_object_at(json_object_at(event, J_STR_TAG("source")), J_STR_TAG("userId")));

        printf("%s\n", json_to_string(event));
        if(!event_type)
            continue;
        else if(!strcmp(event_type, "follow"))
            text = NULL;
        else if(!strcmp(event_type, "message") && !strcmp(msg_type, "text"))
            text = J_TO_STR(json_object_at(message, J_STR_TAG("text")));
        else
            continue;

        ac_sys_do_command(userid, text);
    }
}

bool check_signature(const char *ver, const char *body){
    unsigned char result[512], buff[512];
    size_t out_size;
    bool pass = false;

    EVP_MAC_CTX *ctx = EVP_MAC_CTX_dup(evp_mac_ctx);

    EVP_MAC_init(ctx);
    EVP_MAC_update(ctx, body, strlen(body));

    if(EVP_MAC_final(ctx, result, &out_size, 512)){
        BIO *b64, *bio;
        FILE *fp = fmemopen(buff, 512, "w");

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_fp(fp, BIO_CLOSE);
        BIO_push(b64, bio);
        BIO_write(b64, result, out_size);
        BIO_flush(b64);
        BIO_free(bio);
        BIO_free(b64);

        pass = !strncmp(ver, buff, strlen(ver));
    }

    EVP_MAC_CTX_free(ctx);
    return pass;
}

void do_reply_msg(const char *userid, const char *message){
    size_t uid_len = strlen(userid);
    size_t msg_len = strlen(message);

    char content[uid_len + msg_len + 50], header[200];

    sprintf(content, "{\"to\":\"%s\",\"messages\":[{\"type\":\"text\",\"text\":\"%s\"}]}", userid, message);
    sprintf(header, "Authorization: Bearer %s", actoken);

    char *argv[] = {
        "curl",
        "-v",
        "-X",
        "POST",
        "https://api.line.me/v2/bot/message/push",
        "-H",
        "Content-Type: application/json",
        "-H",
        header,
        "-d",
        content,
        NULL
    };

    pid_t pid;
    if(!(pid = fork())){
        execv("/bin/curl", argv);
    }
    else if(pid != -1){
        waitpid(pid, NULL, 0);
        printf("\n");
    }
}

void do_reply_flex(const char *userid, json_ele_t *flex){
    size_t uid_len = strlen(userid);
    size_t msg_len = json_get_str_length(flex);

    char content[uid_len + msg_len + 70], header[200];

    sprintf(content, "{\"to\":\"%s\",\"messages\":[{\"type\":\"flex\",\"altText\":\"flex\",\"contents\":%s}]}", userid, json_to_string(flex));
    sprintf(header, "Authorization: Bearer %s", actoken);

    char *argv[] = {
        "curl",
        "-v",
        "-X",
        "POST",
        "https://api.line.me/v2/bot/message/push",
        "-H",
        "Content-Type: application/json",
        "-H",
        header,
        "-d",
        content,
        NULL
    };

    pid_t pid;
    if(!(pid = fork())){
        execv("/bin/curl", argv);
    }
    else if(pid != -1){
        waitpid(pid, NULL, 0);
        printf("\n");
    }
}

void first_use(char *userid, void *arg){
    do_reply_msg(userid, "Hello there!");
    do_reply_msg(userid, "You are new here");
    do_reply_msg(userid, "Just give me your name and I will remember you.");
}

void valid_name(char *userid, void *arg){
    char greet[128];
    sprintf(greet, "Hello, %s.", (char*)arg);
    do_reply_msg(userid, greet);
}

void dup_name(char *userid, void *arg){
    do_reply_msg(userid, "Oh...");
    do_reply_msg(userid, "It seems that this name has already been used.");
    do_reply_msg(userid, "Maybe pick another one?");
}

void wait_input(char *userid, void *arg){
    do_reply_msg(userid, "What do you want to do?");
    do_reply_msg(userid, "If you don't know what to do, just type help.");
}

void help_show_command(char *userid, void *arg){
    const char *msg = "Following are the command list:\\n"
                    "JOIN: to join a existed family.\\n"
                    "UNLOCK: to unlock a door.\\n"
                    "SHARE: to share permission to others.\\n"
                    "HELP: to show this message";

    do_reply_msg(userid, msg);
}

void join_family(char *userid, void *arg){
    do_reply_msg(userid, "Please give me the family-ID that you want to join.");
}

void join_success(char *userid, void *arg){
    do_reply_msg(userid, "Joined successfully.");
}

void unlock_pick_door(char *userid, void *arg){
    json_ele_t *content = json_new_ele(JSON_ARR);
    for(ac_sys_door_t *ptr = (ac_sys_door_t*)arg; ptr; ptr = ptr->next){
        json_ele_t *action = json_new_ele(JSON_OBJ);
        json_object_add(action, J_STR_TAG("type"), json_new_ele(JSON_STR, "message"));
        json_object_add(action, J_STR_TAG("label"),json_new_ele(JSON_STR, ptr->door_id));
        json_object_add(action, J_STR_TAG("text"),json_new_ele(JSON_STR, ptr->door_id));

        json_ele_t *btn = json_new_ele(JSON_OBJ);
        json_object_add(btn, J_STR_TAG("type"), json_new_ele(JSON_STR, "button"));
        json_object_add(btn, J_STR_TAG("action"), action);

        json_array_add(content, btn);
    }

    json_ele_t *footer = json_new_ele(JSON_OBJ);
    json_object_add(footer, J_STR_TAG("type"), json_new_ele(JSON_STR, "box"));
    json_object_add(footer, J_STR_TAG("layout"), json_new_ele(JSON_STR, "vertical"));
    json_object_add(footer, J_STR_TAG("contents"), content);

    json_ele_t *flex = json_new_ele(JSON_OBJ);
    json_object_add(flex, J_STR_TAG("type"), json_new_ele(JSON_STR, "bubble"));
    json_object_add(flex, J_STR_TAG("footer"), footer);

    do_reply_msg(userid, "Which door to unlock?");
    do_reply_flex(userid, flex);

    json_free_ele(flex);
}

void unlock_success(char *userid, void *arg){
    char msg[128];
    sprintf(msg, "%s unlock successfully.", (char*)arg);
    do_reply_msg(userid, msg);
}

void share_pick_door(char *userid, void *arg){
    json_ele_t *content = json_new_ele(JSON_ARR);
    for(ac_sys_door_t *ptr = (ac_sys_door_t*)arg; ptr; ptr = ptr->next){
        json_ele_t *action = json_new_ele(JSON_OBJ);
        json_object_add(action, J_STR_TAG("type"), json_new_ele(JSON_STR, "message"));
        json_object_add(action, J_STR_TAG("label"),json_new_ele(JSON_STR, ptr->door_id));
        json_object_add(action, J_STR_TAG("text"),json_new_ele(JSON_STR, ptr->door_id));

        json_ele_t *btn = json_new_ele(JSON_OBJ);
        json_object_add(btn, J_STR_TAG("type"), json_new_ele(JSON_STR, "button"));
        json_object_add(btn, J_STR_TAG("action"), action);

        json_array_add(content, btn);
    }

    json_ele_t *footer = json_new_ele(JSON_OBJ);
    json_object_add(footer, J_STR_TAG("type"), json_new_ele(JSON_STR, "box"));
    json_object_add(footer, J_STR_TAG("layout"), json_new_ele(JSON_STR, "vertical"));
    json_object_add(footer, J_STR_TAG("contents"), content);

    json_ele_t *flex = json_new_ele(JSON_OBJ);
    json_object_add(flex, J_STR_TAG("type"), json_new_ele(JSON_STR, "bubble"));
    json_object_add(flex, J_STR_TAG("footer"), footer);

    do_reply_msg(userid, "Which door to share?");
    do_reply_flex(userid, flex);

    json_free_ele(flex);
}

void share_to_who(char *userid, void *arg){
    do_reply_msg(userid, "Who do you want to share to?");
}

void share_pick_period(char *userid, void *arg){
    do_reply_msg(userid, "How many minutes do you want to share?");
}

void share_success(char *userid, void *arg){
    do_reply_msg(userid, "Share successfully.");
}

void unknown_cmd(char *userid, void *arg){
    do_reply_msg(userid, "Hum...");
    do_reply_msg(userid, "I don't know what you are talking about.");
}

void unknown_id(char *userid, void *arg){
    do_reply_msg(userid, "Oh, it seems that this ID is not valid.");
}

void no_permission(char *userid, void *arg){
    do_reply_msg(userid, "You have no permission to do that.");
}