#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "pins.h"
#include "protocol.h"

static uint16_t read_distance(void) {
    if (gpio_get(ECHO)) return NO_ECHO;
    gpio_put(TRIGGER, 1);
    busy_wait_us_32(10);
    gpio_put(TRIGGER, 0);
    uint32_t sent = time_us_32();
    while (!gpio_get(ECHO)) {
        if (time_us_32() - sent >= 30000) return NO_ECHO;
    }
    uint32_t start = time_us_32();
    while (gpio_get(ECHO)) {
        if (time_us_32() - start >= 30000) return NO_ECHO;
    }
    uint32_t cm = (time_us_32() - start + 29) / 58;
    if (cm < 2 || cm > 400) return NO_ECHO;
    return cm;
}

int main(void) {
    gpio_init(TRIGGER);
    gpio_set_dir(TRIGGER, GPIO_OUT);
    gpio_init(ECHO);
    gpio_set_dir(ECHO, GPIO_IN);
    adc_init();
    adc_gpio_init(SPEED_ADC);
    adc_select_input(SPEED_CHANNEL);
    uart_init(uart1, LINK_BAUD);
    gpio_set_function(LINK_TX, GPIO_FUNC_UART);
    while (true) {
        absolute_time_t next = make_timeout_time_ms(100);
        unsigned sum = 0;
        for (int i = 0; i < 8; i++) sum += adc_read();
        reading_t reading = {read_distance(), (sum * 60 + 16380) / 32760};
        uint8_t frame[FRAME_SIZE];
        encode_frame(frame, reading);
        uart_write_blocking(uart1, frame, FRAME_SIZE);
        sleep_until(next);
    }
}
