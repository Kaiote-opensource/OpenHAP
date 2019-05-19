
#ifndef __UTF_8_H_
#define __UTF_8_H_

#include <stddef.h>
#include <stdint.h>

typedef enum 
{
    UTF8_ACCEPT = 0,
    UTF8_REJECT = 1,

    UTF8_END = 255
} utf8_error;

int check_count_valid_UTF8(char* _bytestring, size_t* _count);

uint32_t* utf8string_create(uint32_t* _codepointbuffer, char* _bytestring);

#endif