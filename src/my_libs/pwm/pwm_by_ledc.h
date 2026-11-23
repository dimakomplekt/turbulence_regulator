// pwm_by_ledc.h


// =========================================================================================== IMPORT

// LEDC include 
#include "driver/ledc.h"

// =========================================================================================== IMPORT

#ifndef LEDC_PWM_H
#define LEDC_PWM_H

// =========================================================================================== STRUCTS

// PWM structure
typedef struct pwm_ctx {
    
    gpio_num_t pin;
    uint32_t freq;
    uint8_t duty;
    ledc_timer_t timer;
    ledc_channel_t channel;
    ledc_timer_config_t timer_config;
    ledc_channel_config_t channel_config;
    bool channel_initialized;

    struct pwm_ctx* next;                   // Correct link to itself

} pwm_ctx;

// =========================================================================================== STRUCTS


// =========================================================================================== API

// Function: init_pwm
// Purpose: Initialize the PWM output pin by the pin, frequency and duty 
void init_pwm(pwm_ctx *pwm, gpio_num_t pin, uint32_t freq, uint8_t duty_percent);

// Function: find_pwm_by_pin
// Purpose: Helper-function to find pwm by pin number
pwm_ctx* find_pwm_by_pin(gpio_num_t pin);

// Function: unregister_pwm
// Purpose: Free the pwm slot and renumber the other PWMs inside the list
void unregister_pwm(pwm_ctx* pwm);

// Function: update_pwm
// Purpose: Update the PWM frequency and duty values 
void update_pwm(pwm_ctx *pwm, uint32_t freq, uint8_t duty_percent);


// Function: set_pwm_duty
// Purpose: Update the PWM duty value 
void set_pwm_duty(pwm_ctx *pwm, uint8_t duty_percent);


// Function: set_pwm_freq
// Purpose: Update the PWM frequency value 
void set_pwm_freq(pwm_ctx *pwm, uint32_t freq);


// Function: start_pwm
// Purpose: Starts the PWM
void start_pwm(pwm_ctx *pwm);


// Function: stop_pwm
// Purpose: Stops the PWM
void stop_pwm(pwm_ctx *pwm);

// =========================================================================================== API

#endif // LEDC_PWM_H
