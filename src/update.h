#ifndef __UPDATE_H__
#define __UPDATE_H__

const uint8_t TYPE_SET = 1;
const uint8_t TYPE_INC = 2;

union Number {
    int64_t i;
    double d;
};

struct Update {
    char key[256];
    uint8_t type;
    Number data;
};

#endif
