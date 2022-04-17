#pragma once

#include <stdlib.h>

#include "swap.h"

#define NBT_TAG_End 0
#define NBT_TAG_Byte 1
#define NBT_TAG_Short 2
#define NBT_TAG_Int 3
#define NBT_TAG_Long 4
#define NBT_TAG_Float 5
#define NBT_TAG_Double 6
#define NBT_TAG_Byte_Array 7
#define NBT_TAG_String 8
#define NBT_TAG_List 9
#define NBT_TAG_Compound 10
#define NBT_TAG_Int_Array 11
#define NBT_TAG_Long_Array 12

#define NBT_BUF_TO_SMALL -1
#define NBT_OK 1

typedef struct nbt_state {
    u_int8_t* buf;
    int ptr;
} nbt_state;

typedef struct nbt_list {
    int len;
    int type;
} nbt_list;

int8_t nbt_read_byte(nbt_state* nbt) {
    int8_t ret = nbt->buf[nbt->ptr];
    nbt->ptr += 1;

    return ret;
}

int16_t nbt_read_short(nbt_state* nbt) {
    int16_t ret = swap_int16(*(int16_t*)(nbt->buf + nbt->ptr));
    nbt->ptr += 2;

    return ret;
}

int32_t nbt_read_int(nbt_state* nbt) {
    int32_t ret = swap_int32(*(int32_t*)(nbt->buf + nbt->ptr));
    nbt->ptr += 4;

    return ret;
}

int64_t nbt_read_long(nbt_state* nbt) {
    int64_t ret = swap_int64(*(int64_t*)(nbt->buf + nbt->ptr));
    nbt->ptr += 8;

    return ret;
}

float nbt_read_float(nbt_state* nbt) {
    float ret = swap_uint32(*(uint32_t*)(nbt->buf + nbt->ptr));
    nbt->ptr += 4;

    return ret;
}

double nbt_read_double(nbt_state* nbt) {
    float ret = swap_uint64(*(uint64_t*)(nbt->buf + nbt->ptr));
    nbt->ptr += 8;

    return ret;
}

void nbt_skip_byte_array(nbt_state* nbt) {
    int32_t size = nbt_read_int(nbt);
    nbt->ptr += size;
}

void nbt_skip_int_array(nbt_state* nbt) {
    int32_t size = nbt_read_int(nbt);
    nbt->ptr += size * 4;
}

void nbt_skip_long_array(nbt_state* nbt) {
    int32_t size = nbt_read_int(nbt);
    nbt->ptr += size * 4;
}

void nbt_skip_string(nbt_state* nbt) {
    u_int16_t size = (nbt->buf[nbt->ptr] << 8) + nbt->buf[nbt->ptr + 1];
    nbt->ptr += size + 2;
}

int nbt_read_string(nbt_state* nbt, u_int8_t* out, size_t out_size) {
    u_int16_t size = (nbt->buf[nbt->ptr] << 8) + nbt->buf[nbt->ptr + 1];

    nbt->ptr += 2;

    if (size + 1 > out_size) {
        return NBT_BUF_TO_SMALL;
    }

    for (int i = 0; i < size; i++) {
        out[i] = nbt->buf[nbt->ptr + i];
    }

    out[size] = '\0';

    nbt->ptr += size;

    return size;
}

void nbt_read_list(nbt_state* nbt, nbt_list* list) {
    int8_t type = nbt_read_byte(nbt);
    int32_t size = nbt_read_int(nbt);
    list->len = size;
    list->type = type;
}
