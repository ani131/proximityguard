#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FRAME_SIZE 6
#define NO_ECHO 65535u
#define LINK_TIMEOUT_US 500000u

typedef enum { SAFE, CAUTION, DANGER, NO_TARGET, LINK_LOST } warning_t;
typedef struct {
    uint16_t cm;
    uint8_t mph;
} reading_t;

void encode_frame(uint8_t frame[FRAME_SIZE], reading_t reading);
bool receive_byte(uint8_t window[FRAME_SIZE], uint8_t byte, reading_t *reading);
warning_t warning_for(reading_t reading, bool connected);
