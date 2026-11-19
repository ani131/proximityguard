#include "protocol.h"
#include <string.h>

static uint8_t checksum(const uint8_t *frame) {
    uint8_t crc = 0;
    for (int i = 0; i < FRAME_SIZE - 1; i++) {
        crc ^= frame[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
        }
    }
    return crc;
}

void encode_frame(uint8_t frame[FRAME_SIZE], reading_t reading) {
    frame[0] = 0xA5;
    frame[1] = 0x5A;
    frame[2] = reading.cm >> 8;
    frame[3] = reading.cm & 255;
    frame[4] = reading.mph;
    frame[5] = checksum(frame);
}

bool receive_byte(uint8_t window[FRAME_SIZE], uint8_t byte, reading_t *reading) {
    memmove(window, window + 1, FRAME_SIZE - 1);
    window[FRAME_SIZE - 1] = byte;
    if (window[0] != 0xA5 || window[1] != 0x5A || checksum(window) != window[5]) {
        return false;
    }
    uint16_t cm = ((uint16_t)window[2] << 8) | window[3];
    if ((cm != NO_ECHO && (cm < 2 || cm > 400)) || window[4] > 60) {
        return false;
    }
    reading->cm = cm;
    reading->mph = window[4];
    return true;
}

warning_t warning_for(reading_t reading, bool connected) {
    if (!connected) return LINK_LOST;
    if (reading.cm == NO_ECHO) return NO_TARGET;
    if (reading.cm <= 10 + reading.mph / 2) return DANGER;
    if (reading.cm <= 25 + reading.mph) return CAUTION;
    return SAFE;
}
