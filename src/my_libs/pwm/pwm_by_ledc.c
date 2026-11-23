// pwm_by_ledc.c


// =========================================================================================== IMPORT

#include "pwm_by_ledc.h"

// =========================================================================================== IMPORT


// =========================================================================================== DEFINES

// This library works only with 4 different PWMs, every of those uses it owns timer and channel
#define MAX_PWMS 4

// Linked list for PWMs realtime change (stop using on of 4 initialized before and create the new one in loop)
static pwm_ctx* pwm_list_head = NULL;

// Counter for the PWM list orientation 
static int pwm_instance_count = 0;

// =========================================================================================== DEFINES


// =========================================================================================== API

// PWM initialization function
void init_pwm(pwm_ctx *pwm, gpio_num_t pin, uint32_t freq, uint8_t duty_percent)
{

    // Already exists error handler by struct variable
    if (pwm->channel_initialized) {
        printf("PWM already initialized on pin %d\n", pwm->pin);
        return;
    }
    // Already exists error handler by pin variable
    if (find_pwm_by_pin(pin))
    {
        printf("PWM on pin %d is already registered\n", pin);
        return;
    }

    // Max PWMs value overflow error handler
    if (pwm_instance_count >= MAX_PWMS)
    {
        printf("PWM limit exceeded (max %d)\n", MAX_PWMS);
        return;
    }


    // Initialization flag change
    pwm->channel_initialized = true;


    // Unique timer and channel, depends on current pwm_instance_count value
    pwm->channel = pwm_instance_count;
    pwm->timer = pwm_instance_count;


    // PWM SETTINGS

    pwm->pin = pin;
    pwm->freq = freq;
    pwm->duty = duty_percent;

    // PWM SETTINGS


    // TIMER SETTINGS

    pwm->timer_config.duty_resolution = LEDC_TIMER_10_BIT;
    pwm->timer_config.freq_hz = freq;
    pwm->timer_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    pwm->timer_config.timer_num = pwm->timer;
    ledc_timer_config(&pwm->timer_config);

    // TIMER SETTINGS


    // CHANNEL SETTINGS

    pwm->channel_config.channel = pwm->channel;
    pwm->channel_config.duty = (1023 * duty_percent) / 100;
    pwm->channel_config.gpio_num = pin;
    pwm->channel_config.speed_mode = LEDC_HIGH_SPEED_MODE;
    pwm->channel_config.hpoint = 0;
    pwm->channel_config.timer_sel = pwm->timer;
    ledc_channel_config(&pwm->channel_config);

    // CHANNEL SETTINGS


    // PWM registration in the linked list

    pwm->next = NULL;

    if (!pwm_list_head) pwm_list_head = pwm;
    else
    {
        pwm_ctx* curr = pwm_list_head;
        while (curr->next) curr = curr->next;
        curr->next = pwm;
    }

    // Control variable incrementation
    pwm_instance_count++;
}


// Helper-function to define the PWM through the linked list, by it's setted pin
pwm_ctx* find_pwm_by_pin(gpio_num_t pin) 
{
    pwm_ctx* curr = pwm_list_head;

    while (curr) 
    {
        if (curr->pin == pin) return curr;

        curr = curr->next;
    }

    return NULL;
}

// PWM delete function
void unregister_pwm(pwm_ctx* pwm) 
{
    if (!pwm_list_head || !pwm) return;

    pwm_ctx* curr = pwm_list_head;
    pwm_ctx* prev = NULL;

    while (curr) 
    {
        if (curr == pwm) 
        {
            // Delete from the list with timers and channels change for other PWMs 
            if (prev) prev->next = curr->next;
            else pwm_list_head = curr->next;

            // Stop the PWM
            ledc_stop(curr->timer_config.speed_mode, curr->channel, 0);

            // Control variable decrementation 
            pwm_instance_count--;
            

            // Timers and channels change for other PWMs 
            curr = (prev) ? prev->next : pwm_list_head;


            int index = 0;

            while (curr) 
            {
                curr->channel = index;
                curr->timer = index;

                curr->timer_config.timer_num = curr->timer;
                ledc_timer_config(&curr->timer_config);

                curr->channel_config.duty = (1023 * curr->duty) / 100;

                curr->channel_config.channel = curr->channel;
                curr->channel_config.timer_sel = curr->timer;
                ledc_channel_config(&curr->channel_config);

                index++;

                curr = curr->next;
            }


            // Reset the PWM control values
            pwm->pin = GPIO_NUM_NC;
            pwm->freq = 0;
            pwm->duty = 0;
            pwm->channel_initialized = false;
            pwm->next = NULL; // на всякий случай

            return;
        }

        prev = curr;
        curr = curr->next;
    }
}


// Pwm parameters update function
void update_pwm(pwm_ctx *pwm, uint32_t freq, uint8_t duty_percent)
{

    if (pwm->channel_initialized == true)
    {
        pwm->freq = freq;
        pwm->duty = duty_percent;
    
        // Frequency value update
        set_pwm_freq(pwm, freq);
    
        // Duty value update
        set_pwm_duty(pwm, duty_percent);
    }

    else return;
}


// Duty control function
void set_pwm_duty(pwm_ctx *pwm, uint8_t duty_percent) 
{
    uint32_t max_duty = (1 << pwm->timer_config.duty_resolution) - 1;

    pwm->channel_config.duty = (max_duty * duty_percent) / 100;

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, pwm->channel_config.channel, pwm->channel_config.duty);

    ledc_update_duty(LEDC_HIGH_SPEED_MODE, pwm->channel_config.channel);
}


// Frequency control function
void set_pwm_freq(pwm_ctx *pwm, uint32_t freq) 
{
    pwm->freq = freq;
    pwm->timer_config.freq_hz = freq;

    ledc_timer_config(&(pwm->timer_config));
}


// PWM start function
void start_pwm(pwm_ctx *pwm) 
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, pwm->channel_config.channel, pwm->channel_config.duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, pwm->channel_config.channel);

    pwm->channel_initialized = true;
}

// PWM stop function
void stop_pwm(pwm_ctx *pwm)
{
    ledc_stop(LEDC_HIGH_SPEED_MODE, pwm->channel_config.channel, 0);

    pwm->channel_initialized = false;
}

// =========================================================================================== API
