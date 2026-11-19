#include <assert.h>
#include <stdio.h>
#include "protocol.h"

int main(void) {
    uint8_t frame[FRAME_SIZE], window[FRAME_SIZE] = {0};
    reading_t input, output = {0};
    unsigned count = 0;
    for (int speed = 0; speed <= 60; speed++) {
        for (int cm = 2; cm <= 401; cm++) {
            input = (reading_t){cm == 401 ? NO_ECHO : (unsigned)cm, speed};
            encode_frame(frame, input);
            for (unsigned i = 0; i < FRAME_SIZE; i++) {
                assert(receive_byte(window, frame[i], &output) == (i == FRAME_SIZE - 1));
            }
            assert(input.cm == output.cm && input.mph == output.mph);
            warning_t expected = SAFE;
            if (cm <= 25 + speed) expected = CAUTION;
            if (cm <= 10 + speed / 2) expected = DANGER;
            if (cm == 401) expected = NO_TARGET;
            assert(warning_for(output, true) == expected);
            assert(warning_for(output, false) == LINK_LOST);
            count++;
        }
    }
    input = (reading_t){30, 60};
    encode_frame(frame, input);
    for (int bit = 0; bit < FRAME_SIZE * 8; bit++) {
        frame[bit / 8] ^= 1u << (bit % 8);
        for (unsigned i = 0; i < FRAME_SIZE; i++) assert(!receive_byte(window, frame[i], &output));
        frame[bit / 8] ^= 1u << (bit % 8);
    }
    for (int drop = 0; drop < FRAME_SIZE; drop++) {
        for (int i = 0; i < FRAME_SIZE; i++) {
            if (i != drop) assert(!receive_byte(window, frame[i], &output));
        }
        for (int i = 0; i < FRAME_SIZE; i++)
            assert(receive_byte(window, frame[i], &output) == (i == FRAME_SIZE - 1));
    }
    printf("%u readings passed, including risk, link-loss and no-echo cases. Corruption and resync passed.\n", count);
}
