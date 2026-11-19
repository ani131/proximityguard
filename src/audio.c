#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "audio.h"
#include "pins.h"
#include "clips.inc"

static const uint8_t *samples;
static unsigned sample_index, sample_count;
static repeating_timer_t timer;

static bool play_sample(repeating_timer_t *unused) {
    (void)unused;
    if (sample_index < sample_count) {
        pwm_set_gpio_level(SPEAKER_PWM, samples[sample_index++]);
    } else {
        pwm_set_gpio_level(SPEAKER_PWM, 128);
    }
    return true;
}

void init_audio(void) {
    gpio_set_function(SPEAKER_PWM, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SPEAKER_PWM);
    pwm_set_wrap(slice, 255);
    pwm_set_gpio_level(SPEAKER_PWM, 128);
    pwm_set_enabled(slice, true);
    gpio_set_function(PIEZO_PWM, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(PIEZO_PWM);
    pwm_set_clkdiv(slice, (float)clock_get_hz(clk_sys) / 2000000);
    pwm_set_wrap(slice, 999);
    pwm_set_gpio_level(PIEZO_PWM, 0);
    pwm_set_enabled(slice, true);
    if (!add_repeating_timer_us(-125, play_sample, NULL, &timer)) panic("audio timer");
}

void update_audio(warning_t warning, uint32_t now_ms) {
    static int previous = -1;
    static uint32_t started;
    uint32_t period = 2000;
    if (warning == CAUTION) period = 1600;
    if (warning == DANGER) period = 800;
    if ((int)warning != previous || now_ms - started >= period) {
        uint32_t irq = save_and_disable_interrupts();
        sample_index = 0;
        sample_count = 0;
        if (warning == CAUTION) { samples = caution; sample_count = sizeof caution; }
        if (warning == DANGER) { samples = danger; sample_count = sizeof danger; }
        if (warning == LINK_LOST) { samples = lost; sample_count = sizeof lost; }
        restore_interrupts(irq);
        started = now_ms;
        previous = warning;
    }
    uint32_t phase = now_ms - started;
    bool beep = (warning == CAUTION && phase % 800 < 90) ||
                (warning == DANGER && phase % 200 < 90) ||
                (warning == LINK_LOST && phase < 400);
    pwm_set_gpio_level(PIEZO_PWM, beep ? 500 : 0);
}
