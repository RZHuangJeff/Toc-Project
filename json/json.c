#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define BUFF_SIZE 128

#define ISBLANK(c) ((*c) ==' ' || (*c) == '\n' || (*c) == '\t')
#define TOUNBLANK(c) {while((*c) != '\0' && ISBLANK(c)){(c)++;}}

static json_ele_t *json_gen_ele(const char *ptr, const char **eptr);

static json_ele_t *json_gen_object(const char *ptr, const char **eptr);
static json_ele_t *json_gen_array(const char *ptr, const char **eptr);
static json_ele_t *json_gen_string(const char *ptr, const char **eptr);
static json_ele_t *json_gen_bool(const char *ptr, const char **eptr);
static json_ele_t *json_gen_null(const char *ptr, const char **eptr);
static json_ele_t *json_gen_number(const char *ptr, const char **eptr);

static char *json_to_string_r(json_ele_t *jele, char *ptr);

json_ele_t *json_parse(const char *str){
    json_ele_t *jele = NULL;
    const char *eptr;

    if(str[0] == '{')
        jele = json_gen_object(str + 1, &eptr);
    else if(str[0] == '[')
        jele = json_gen_array(str + 1, &eptr);

    return jele;
}

char *json_to_string(json_ele_t *jele){
    char *result = (char*)malloc(json_get_str_length(jele) + 1);

    char *last = json_to_string_r(jele, result);
    *last = '\0';

    return result;
}

json_ele_t *json_new_ele(json_type_t type, ...){
    va_list args;
    va_start(args, type);

    char *str;
    json_ele_t *jele = (json_ele_t*)malloc(sizeof(json_ele_t));
    jele->type = type;
    switch(type){
        case JSON_STR:
            str = va_arg(args, char*);
            jele->cont_len = strlen(str);
            jele->content.str = (char*)malloc(jele->cont_len + 1);
            memcpy(jele->content.str, str, jele->cont_len + 1);
            break;
        case JSON_NUM:
            jele->content.num = va_arg(args, double);
            break;
        case JSON_BOOL:
            jele->content.boo = va_arg(args, int);
            break;
        case JSON_OBJ:
            jele->cont_len = 0;
            jele->content.obj = (json_kvpair_t*)malloc(sizeof(json_kvpair_t)*10);
            break;
        case JSON_ARR:
            jele->cont_len = 0;
            jele->content.arr = (json_ele_t**)malloc(sizeof(json_ele_t*)*10);
            break;
    }

    va_end(args);

    return jele;
}

void json_free_ele(json_ele_t *jele){
    if(!jele)
        return;
    
    switch(jele->type){
        case JSON_OBJ:
            for(size_t i = 0; i < jele->cont_len; i++){
                json_free_ele(jele->content.obj[i].key);
                json_free_ele(jele->content.obj[i].value);
            }
            jele->cont_len = 0;
        case JSON_ARR:
            for(size_t i = 0; i < jele->cont_len; i++)
                json_free_ele(jele->content.arr[i]);
            jele->cont_len = 0;
        case JSON_STR:
            free(jele->content.str);
        default:
            free(jele);
    }
}

json_ele_t *json_object_at(json_ele_t *jobj, json_tag_t tag){
    if(!J_TO_OBJ(jobj))
        return NULL;

    json_ele_t *jele = NULL;
    json_kvpair_t *pairs = jobj->content.obj;
    if(tag.type == JSON_STR){
        for(size_t i = 0; i < jobj->cont_len; i++)
            if(!strcmp(pairs[i].key->content.str, tag.tag.str_tag)){
                jele = pairs[i].value;
                break;
            }
    }
    else if(tag.type == JSON_NUM){
        for(size_t i = 0; i < jobj->cont_len; i++)
            if(pairs[i].key->content.num == tag.tag.num_tag){
                jele = pairs[i].value;
                break;
            }
    }

    return jele;
}

void json_object_add(json_ele_t *jobj, json_tag_t tag, json_ele_t *value){
    if(jobj->type != JSON_OBJ)
        return;
    
    json_ele_t *key = (json_ele_t*)malloc(sizeof(json_ele_t));
    if(tag.type == JSON_STR){
        size_t len = strlen(tag.tag.str_tag);

        key->type = JSON_STR;
        key->content.str = (char*)malloc(len + 1);
        memcpy(key->content.str, tag.tag.str_tag, len + 1);
    }
    else{
        key->type = JSON_NUM;
        key->content.num = tag.tag.num_tag;
    }

    if(jobj->cont_len && jobj->cont_len%10 == 0)
        jobj->content.obj = (json_kvpair_t*)realloc(jobj->content.obj, (jobj->cont_len + 10)*sizeof(json_kvpair_t));

    jobj->content.obj[jobj->cont_len].key = key;
    jobj->content.obj[jobj->cont_len].value = value;
    jobj->cont_len++;
}

json_ele_t *json_array_at(json_ele_t *jarr, const size_t ind){
    json_ele_t **arr;
    if(!(arr = J_TO_ARR(jarr)) || ind >= jarr->cont_len)
        return NULL;

    return arr[ind];
}

size_t json_array_add(json_ele_t *jarr, json_ele_t *ele){
    if(jarr->type != JSON_ARR)
        return 0;

    if(jarr->cont_len && jarr->cont_len%10 == 0)
        jarr->content.arr = (json_ele_t**)realloc(jarr->content.arr, (jarr->cont_len + 10)*sizeof(json_ele_t*));

    jarr->content.arr[jarr->cont_len] = ele;

    return jarr->cont_len++;
}

size_t json_get_length(json_ele_t *jele){
    if(jele->type == JSON_STR || jele->type == JSON_OBJ || jele->type == JSON_ARR)
        return jele->cont_len;

    return 0;
}

static json_ele_t *json_gen_ele(const char *ptr, const char **eptr){
    while(*ptr != '\0' && ISBLANK(ptr))
        ptr++;

    if(*ptr == '\0')
        return NULL;

    if(*ptr == '{')
        return json_gen_object(ptr + 1, eptr);

    if(*ptr == '[')
        return json_gen_array(ptr + 1, eptr);

    if(*ptr == '\"')
        return json_gen_string(ptr + 1, eptr);

    if(*ptr == 't' || *ptr == 'T' || *ptr == 'f' || *ptr == 'F')
        return json_gen_bool(ptr, eptr);

    if(*ptr == 'n' || *ptr == 'N')
        return json_gen_null(ptr, eptr);

    return json_gen_number(ptr, eptr);
}

static json_ele_t *json_gen_object(const char *ptr, const char **eptr){
    const char *nc;
    bool succ = true, end = false;

    json_ele_t *jele = NULL;
    json_ele_t *key, *value;
    json_kvpair_t *pairs = (json_kvpair_t*)malloc(10*sizeof(json_kvpair_t));
    size_t pind = 0, size = 10;

    TOUNBLANK(ptr);
    if(*ptr == '}'){
        *eptr = ptr + 1;
        end = true;
    }

    while(!end){
        key = json_gen_ele(ptr, eptr);
        if(!key || (key->type != JSON_STR && key->type != JSON_NUM)){
            succ = false;
            break;
        }

        nc = *eptr;
        TOUNBLANK(nc);
        if(*nc != ':'){
            succ = false;
            break;
        }

        value = json_gen_ele(nc + 1, eptr);
        if(!value){
            json_free_ele(key);
            succ = false;
            break;
        }

        if(pind == size){
            size += 10;
            pairs = (json_kvpair_t*)realloc(pairs, size*sizeof(json_kvpair_t));
        }

        pairs[pind].key = key;
        pairs[pind].value = value;
        pind++;

        nc = *eptr;
        TOUNBLANK(nc);
        if(*nc == '}'){
            *eptr = nc + 1;
            break;
        }
        else if(*nc != ','){
            succ = false;
            break;
        }
        ptr = nc + 1;
    }

    if(succ){
        jele = (json_ele_t*)malloc(sizeof(json_ele_t));
        jele->type = JSON_OBJ;
        jele->content.obj = pairs;
        jele->cont_len = pind;
    }
    else{
        for(size_t i = 0; i < pind; i++){
            json_free_ele(pairs[i].key);
            json_free_ele(pairs[i].value);
        }
        free(pairs);
    }

    return jele;
}

static json_ele_t *json_gen_array(const char *ptr, const char **eptr){
    json_ele_t *jele = NULL, *nele;
    json_ele_t **arr = (json_ele_t**)malloc(sizeof(json_ele_t*)*10);
    size_t pind = 0;
    size_t size = 10;

    bool succ = true, end = false;

    TOUNBLANK(ptr);
    if(*ptr == ']'){
        end = true;
        *eptr = ptr + 1;
    }

    while(!end){
        nele = json_gen_ele(ptr, eptr);
        if(!nele){
            succ = false;
            break;
        }

        if(pind == size){
            size += 10;
            arr = (json_ele_t**)realloc(arr, sizeof(json_ele_t*)*size);
        }
        arr[pind++] = nele;

        ptr = *eptr;
        TOUNBLANK(ptr);

        if(*ptr == ']'){
            *eptr = ptr + 1;
            break;
        }
        else if(*ptr != ','){
            succ = false;
            break;
        }
        ptr++;
    }

    if(succ){
        jele = (json_ele_t*)malloc(sizeof(json_ele_t));
        jele->type = JSON_ARR;
        jele->content.arr = arr;
        jele->cont_len = pind;
    }
    else{

    }

    return jele;
}

static json_ele_t *json_gen_string(const char *ptr, const char **eptr){
    *eptr = strchr(ptr, '\"');

    if(!(*eptr))
        return NULL;

    size_t len = *eptr - ptr;
    (*eptr)++;

    json_ele_t *jele = (json_ele_t*)malloc(sizeof(json_ele_t));
    jele->type = JSON_STR;
    jele->content.str = (char*)malloc(len + 1);
    strncpy(jele->content.str, ptr, len);
    jele->content.str[len] = '\0';
    jele->cont_len = len;

    return jele;
}

static json_ele_t *json_gen_bool(const char *ptr, const char **eptr){
    json_ele_t *jele = NULL;

    if(!strncasecmp(ptr, "true", 4)){
        jele = (json_ele_t*)malloc(sizeof(json_ele_t));
        jele->type = JSON_BOOL;
        jele->content.boo = true;

        *eptr = ptr + 4;
    }
    else if(!strncasecmp(ptr, "false", 5)){
        jele = (json_ele_t*)malloc(sizeof(json_ele_t));
        jele->type = JSON_BOOL;
        jele->content.boo = false;

        *eptr = ptr + 5;
    }
    else
        *eptr = ptr + 1;

    return jele;
}

static json_ele_t *json_gen_null(const char *ptr, const char **eptr){
    json_ele_t *jele = NULL;

    if(!strncasecmp(ptr, "null", 4)){
        jele = (json_ele_t*)malloc(sizeof(json_ele_t));
        jele->type = JSON_NULL;
        
        *eptr = ptr + 4;
    }
    else
        *eptr = ptr + 1;

    return jele;
}

static json_ele_t *json_gen_number(const char *ptr, const char **eptr){
    json_ele_t *jele = NULL;

    double n = strtod(ptr, (char**)eptr);
    if(ptr != *eptr){
        jele = (json_ele_t*)malloc(sizeof(json_ele_t));
        jele->type = JSON_NUM;
        jele->content.num = n;
    }

    return jele;
}

static char *json_to_string_r(json_ele_t *jele, char *ptr){
    switch(jele->type){
        case JSON_STR:
            return ptr + sprintf(ptr, "\"%s\"", J_TO_STR(jele));
        case JSON_NUM:
            return ptr + sprintf(ptr, "%f", J_TO_NUM(jele));
        case JSON_BOOL:
            return ptr + sprintf(ptr, "%s", J_TO_BOOL(jele) ? "true" : "false");
        case JSON_NULL:
            return ptr + sprintf(ptr, "null");
        case JSON_OBJ:
            ptr += sprintf(ptr, "{");
            json_kvpair_t *pairs = J_TO_OBJ(jele);
            for(size_t i = 0; i < jele->cont_len && (!i || (ptr += sprintf(ptr, ","))); i++){
                ptr = json_to_string_r(pairs[i].key, ptr);
                ptr += sprintf(ptr, ":");
                ptr = json_to_string_r(pairs[i].value, ptr);
            }
            return ptr + sprintf(ptr, "}");
        case JSON_ARR:
            ptr += sprintf(ptr, "[");
            json_ele_t **arr = J_TO_ARR(jele);
            for(size_t i = 0; i < jele->cont_len && (!i || (ptr += sprintf(ptr, ","))); i++)
                ptr = json_to_string_r(arr[i], ptr);
            return ptr + sprintf(ptr, "]");
    }
}

size_t json_get_str_length(json_ele_t *jele){
    size_t size;
    char tmp[64], *str;

    switch(jele->type){
        case JSON_STR:
            str = J_TO_STR(jele);
            return strlen(str) + 2;
        case JSON_NUM:
            sprintf(tmp, "%f", J_TO_NUM(jele));
            return strlen(tmp);
        case JSON_BOOL:
            return (size_t)(J_TO_BOOL(jele) ? 4 : 5);
        case JSON_NULL:
            return (size_t)4;
        case JSON_OBJ:
            size = 2;
            json_kvpair_t *pairs = J_TO_OBJ(jele);
            for(size_t i = 0; i < jele->cont_len; i++){
                size += 2;
                size += json_get_str_length(pairs[i].key);
                size += json_get_str_length(pairs[i].value);
            }
            return size - 1;
        case JSON_ARR:
            size = 2;
            json_ele_t **arr = J_TO_ARR(jele);
            for(size_t i = 0; i < jele->cont_len; i++)
                size += 1 + json_get_str_length(arr[i]);
            return size - 1;
    }
}