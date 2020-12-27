#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    JSON_STR,
    JSON_NUM,
    JSON_BOOL,
    JSON_NULL,
    JSON_OBJ,
    JSON_ARR
} json_type_t;

typedef struct json_ele_t json_ele_t;

typedef struct {
    json_ele_t *key;
    json_ele_t *value;
} json_kvpair_t;

struct json_ele_t {
    json_type_t type;

    union {
        char *str;
        double num;
        bool boo;
        json_ele_t **arr;
        json_kvpair_t *obj;
    } content;

    size_t cont_len;
};

typedef struct { 
    json_type_t type;

    union {
        const char *str_tag;
        const double num_tag;
    } tag;
}json_tag_t;

#define J_NUM_TAG(t) ((json_tag_t){.type = JSON_NUM, .tag.num_tag = t})
#define J_STR_TAG(t) ((json_tag_t){.type = JSON_STR, .tag.str_tag = t})

#define J_TO_NUM(jptr) ((jptr) && (jptr)->type == JSON_NUM ? (jptr)->content.num : 0)
#define J_TO_STR(jptr) ((jptr) && (jptr)->type == JSON_STR ? (jptr)->content.str : NULL)
#define J_TO_BOOL(jptr) ((jptr) && (jptr)->type == JSON_BOOL ? (jptr)->content.boo : 0)
#define J_TO_OBJ(jptr) ((jptr) && (jptr)->type == JSON_OBJ ? (jptr)->content.obj : NULL)
#define J_TO_ARR(jptr) ((jptr) && (jptr)->type == JSON_ARR ? (jptr)->content.arr : NULL)

json_ele_t *json_parse(const char *str);
char *json_to_string(json_ele_t *jele);

json_ele_t *json_new_ele(json_type_t type, ...);
void json_free_ele(json_ele_t *jele);

json_ele_t *json_object_at(json_ele_t *jobj, json_tag_t tag);
void json_object_add(json_ele_t *jobj, json_tag_t tag, json_ele_t *value);

json_ele_t *json_array_at(json_ele_t *jarr, const size_t ind);
size_t json_array_add(json_ele_t *jarr, json_ele_t *ele);

size_t json_get_length(json_ele_t *jele);
size_t json_get_str_length(json_ele_t *jele);

#endif