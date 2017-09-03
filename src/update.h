#ifndef __UPDATE_H__
#define __UPDATE_H__

const uint8_t TYPE_SET = 1;
const uint8_t TYPE_INC = 1;

struct Update {
    char key[256];
    uint8_t type;
    int64_t data;
};

#endif
