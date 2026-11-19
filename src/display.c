#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "pins.h"
#include "protocol.h"
#include "lcd.h"
#include "audio.h"

static void output_pin(uint pin, bool high) {
    gpio_init(pin);
    gpio_put(pin, high);
    gpio_set_dir(pin, GPIO_OUT);
}

static void init_spi_lcd(void) {
    output_pin(TFT_CS, true);
    output_pin(TFT_RESET, true);
    output_pin(TFT_DC, false);
    spi_init(spi1, 12000000);
    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(TFT_SCK, GPIO_FUNC_SPI);
    gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);
    LCD_Setup();
    LCD_Clear(BLACK);
    LCD_DrawString(12, 20, WHITE, BLACK, "ProximityGuard", 16, 0);
    LCD_DrawString(12, 72, CYAN, BLACK, "Obstacle distance", 16, 0);
    LCD_DrawString(12, 140, CYAN, BLACK, "Simulated speed", 16, 0);
    LCD_DrawString(12, 208, CYAN, BLACK, "Warning", 16, 0);
}

static void show(reading_t reading, warning_t warning) {
    static const char *names[] = {"SAFE", "CAUTION", "DANGER", "NO ECHO", "CONNECTION ERROR"};
    static const uint16_t colors[] = {GREEN, YELLOW, RED, YELLOW, RED};
    char line[24] = "-- cm   ";
    if (warning != LINK_LOST && reading.cm != NO_ECHO)
        snprintf(line, sizeof line, "%3u cm  ", (unsigned)reading.cm);
    LCD_DrawString(12, 96, WHITE, BLACK, line, 16, 0);
    if (warning == LINK_LOST) snprintf(line, sizeof line, "-- mph  ");
    else snprintf(line, sizeof line, "%2u mph  ", (unsigned)reading.mph);
    LCD_DrawString(12, 164, WHITE, BLACK, line, 16, 0);
    snprintf(line, sizeof line, "%-16s", names[warning]);
    LCD_DrawString(12, 232, colors[warning], BLACK, line, 16, 0);
}

int main(void) {
    init_spi_lcd();
    init_audio();
    uart_init(uart1, LINK_BAUD);
    gpio_set_function(LINK_RX, GPIO_FUNC_UART);
    gpio_pull_up(LINK_RX);
    reading_t reading = {NO_ECHO, 0};
    uint8_t window[FRAME_SIZE] = {0};
    uint64_t last_frame = 0;
    bool received = false;
    int last_warning = -1;
    reading_t last_shown = {0, 0};
    while (true) {
        while (uart_is_readable(uart1)) {
            if (receive_byte(window, uart_getc(uart1), &reading)) {
                last_frame = time_us_64();
                received = true;
            }
        }
        uint64_t now = time_us_64();
        warning_t warning = warning_for(reading, received && now - last_frame < LINK_TIMEOUT_US);
        update_audio(warning, (uint32_t)(now / 1000));
        if (last_warning != (int)warning || last_shown.cm != reading.cm || last_shown.mph != reading.mph) {
            show(reading, warning);
            last_warning = warning;
            last_shown = reading;
        }
        sleep_ms(1);
    }
}
