#include <Arduino.h>
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define HALF_BIT 500                                                    // for 2000 baudrate, half bit is 250us

void timer_init(){
    // configure timer with 0.5us clock, up counting, freee running
    TIMERG0.hw_timer[0].config.enable = 0;                              // Stop timer before setting
    TIMERG0.hw_timer[0].config.divider = 41;                            // 80 MHz / 40 = 2 MHz (0.5 µs)
    TIMERG0.hw_timer[0].config.increase = 1;                            // Count up
    TIMERG0.hw_timer[0].config.autoreload = 0;                          // free running mode
    TIMERG0.hw_timer[0].config.alarm_en = 0;                            // no alarm value
    TIMERG0.int_ena.val = 0;                                            // Disable interrupts explicitly
    TIMERG0.hw_timer[0].load_low  = 0;                                  // Load counter = 0
    TIMERG0.hw_timer[0].load_high = 0;
    TIMERG0.hw_timer[0].reload    = 1;
    TIMERG0.hw_timer[0].config.enable = 1;                              // Start timer
}

static inline uint64_t get_timer_cnt(){
    // update counter value an read
    TIMERG0.hw_timer[0].update = 1;
    uint32_t lo = TIMERG0.hw_timer[0].cnt_low;
    uint32_t hi = TIMERG0.hw_timer[0].cnt_high;
    return ((uint64_t)hi << 32) | lo;
}

static inline void reset_timer(){
    // reset timer
    TIMERG0.hw_timer[0].load_low  = 0;
    TIMERG0.hw_timer[0].load_high = 0;
    TIMERG0.hw_timer[0].reload = 1;
}

void verify_config(){
    // verify timer register setting
    uint32_t cfg = TIMERG0.hw_timer[0].config.val;
    printf("TIMG_T0CONFIG = 0x%08X\n", cfg);
    printf(" enable: %d", (cfg >> 31) & 1);
    printf(", increase: %d", (cfg >> 30) & 1);
    printf(", autoreload: %d", (cfg >> 29) & 1);
    printf(", alarm_en: %d", (cfg >> 10) & 1);
    printf(", divider: %d\n", (cfg >> 13) & 0xFFFF);
}

uint16_t bits_cnt;
uint16_t sec_cnt;

void setup(){
    Serial.begin(115200);
    timer_init();
}

void loop(){
    if(get_timer_cnt() > HALF_BIT){
        reset_timer();
        bits_cnt += 1;
        if (bits_cnt > 4000){;                                           // for 0.25us half bit time -> 1s
            Serial.println(sec_cnt);
            sec_cnt += 1;
            bits_cnt = 0;
        }
    }
}