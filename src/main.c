// main.c


// =========================================================================================== IMPORT

#include <stdio.h>
#include "driver/gpio.h"

#include "esp_rom_sys.h"
#include "esp_timer.h" // esp_timer_get_time()
#include "soc/rtc.h" // esp_cpu_get_cycle_count()
#include "esp_cpu.h"

#include "soc/gpio_reg.h"
#include "soc/gpio_struct.h"

#include <time.h>

#include <inttypes.h>

#include "driver/ledc.h"

// Async delay library
#include <my_libs/async_await/async_await.h>

// Button control library
#include <my_libs/button_control/button_control.h>

// Encoder control library
#include <my_libs/encoder_control/encoder_control.h>

// PWM library
#include <my_libs/pwm/pwm_by_ledc.h>

// Random PWM library
#include <my_libs/random_pwm_setup/random_pwm_setup.h>

// I2C display library
#include "esp_log.h"
#include <outer_libs/I2C_display/ssd1306.h>
#include <outer_libs/I2C_display/fonts.h>

// =========================================================================================== IMPORT


// =========================================================================================== RTOS OR NOT

// FreeRTOS or bare metal choose
#define USE_FREERTOS 0 // 0 for bare metal or 1 for RTOS

#if USE_FREERTOS

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"

#endif

// =========================================================================================== RTOS OR NOT


// =========================================================================================== DEFINES

// HCPL3120 optocoupler diode control pins

#define VCC_OD_DIODE GPIO_NUM_33
#define GND_OD_DIODE GPIO_NUM_32


// Interrupts by PWM to optocoupler counter
#define INTERRUPTS_COUNTER_PIN GPIO_NUM_35


// Random-PWM parameters regime-swich button pins
#define R_PWM_BUT_VCC GPIO_NUM_5


// Encoder 1 pins

#define ENC_DUTY_SW GPIO_NUM_13
#define ENC_DUTY_DT GPIO_NUM_12
#define ENC_DUTY_CLK GPIO_NUM_14


// Encoder 2 pins

#define ENC_FREQ_SW GPIO_NUM_27
#define ENC_FREQ_DT GPIO_NUM_26
#define ENC_FREQ_CLK GPIO_NUM_25


// 1 ms delay for FREERTOS loop stability

#define RTOS_LOOP_DELAY_MS 20

// =========================================================================================== DEFINES


// =========================================================================================== TYPES

// Frequency regulation step value
typedef enum {

    STEP_1HZ = 1,
    STEP_100HZ = 100,
    STEP_1KHZ = 1000,

} freq_step_t;


// Enum for the encoder rotation control logic in random PWM mode
typedef enum 
{

    DUTY,
    FREQ,
    REPEATS

} controlled_parameter;

// =========================================================================================== TYPES



// =========================================================================================== GLOBAL VARIABLES

// BUTTONS

button_ctx r_pwm_but;


// ENCODERS

encoder_ctx encoder_ds_ll;
encoder_ctx encoder_fs_hl;


// PWM

pwm_ctx u_c_pwm;

random_pwm_setup_ctx r_pwm_parameters;
random_pwm_pipeline_ctx r_pwm_pipeline;


// PWM PARAMETERS

uint8_t duty_cycle = 10;
uint8_t r_pwm_duty_cycle = 10;

uint8_t duty_cycle_step = 1;
uint8_t r_pwm_duty_cycle_step = 1;

uint8_t duty_cycle_min = 2;
uint8_t r_pwm_duty_cycle_min = 2;
uint8_t duty_cycle_max = 15;
uint8_t r_pwm_duty_cycle_max = 8;


uint32_t frequency = 1000;
uint32_t r_pwm_frequency = 1000;

uint32_t frequency_step = 1;
uint32_t r_pwm_frequency_step = 1;

uint32_t prev_frequency_step = 1;
uint32_t r_pwm_prev_frequency_step = 1;

uint32_t frequency_min = 50;
uint32_t r_pwm_frequency_min = 50;

uint32_t frequency_max = 75000;
uint32_t r_pwm_frequency_max = 50000;

uint32_t r_pwm_repeats = 0;

uint32_t r_pwm_repeats_step = 1;

uint32_t r_pwm_repeats_min = 2;
uint32_t r_pwm_repeats_max = 10;


// RANDOM PWM LIMITS

uint32_t r_pwm_frequency_low_limit = 1000;
uint32_t r_pwm_frequency_high_limit = 15000;

uint8_t r_pwm_duty_cycle_low_limit = 3;
uint8_t r_pwm_duty_cycle_high_limit = 6;

uint32_t r_pwm_repeats_low_limit = 2;
uint32_t r_pwm_repeats_high_limit = 5;

controlled_parameter curr_controlled_parameter = DUTY;


// PWM CONTROL DATA

static int prev_frequency = -1;
static int prev_duty_cycle = 255;

static int r_pwm_prev_frequency_low_limit = -1;
static int r_pwm_prev_frequency_high_limit = -1;

static int r_pwm_prev_duty_cycle_low_limit = 255;
static int r_pwm_prev_duty_cycle_high_limit = 255;

static int r_pwm_prev_repeats_low_limit = -1;
static int r_pwm_prev_repeats_high_limit = -1;

uint32_t r_pwm_prev_repeats = 0;


// INTERRUPTS COUNTER
static volatile uint32_t pulse_count = 0;

// BUTTON PRESS FLAGS

bool r_pwm_but_pressed;
bool r_pwm_but_pressed_prev = true;


// ENCODER PRESS FLAGS

bool encoder_ds_ll_pressed = false;
bool encoder_ds_ll_prev_pressed = false;

bool r_pwm_encoder_ds_ll_pressed = false;
bool r_pwm_encoder_ds_ll_prev_pressed = false;
bool r_pwm_step_change_onetime_flag = true;

bool r_pwm_encoder_ds_ll_lt_pressed = false;
bool r_pwm_encoder_ds_ll_prev_lt_pressed = true;

bool encoder_fs_hl_pressed = false;
bool encoder_fs_hl_prev_pressed = false;
bool r_pwm_freq_change_onetime_flag = true;

bool onetime_press_sw_to_change_curr_parameter = false;
bool onetime_press_sw_to_change_curr_parameter_onetime_flag = true;

bool r_pwm_encoder_fs_hl_pressed = false;
bool r_pwm_encoder_fs_hl_pressed_onetime_flag = true;
bool r_pwm_encoder_fs_hl_prev_pressed = false;
bool r_pwm_encoder_fs_hl_change_onetime_flag = true;

bool u_c_pwm_enabled = false;
bool r_pwm_enabled = false;

bool pwm_repeats_overflow = false;



// DISPLAY
char display_buf[32]; // Буфер для форматирования текста

// =========================================================================================== GLOBAL VARIABLES


// =========================================================================================== PREDECLARE

void initialization();
void main_loop(void *pvParameter);

void update_display_u_c_pwm();
void update_display_r_pwm();

static void pwm_pulse_isr_handler(void* arg); // Interrupts counter

// =========================================================================================== PREDECLARE


// =========================================================================================== SETUP

void initialization()
{
    // HCLP3120 LED-diode control pins initialization

    gpio_set_direction(VCC_OD_DIODE, GPIO_MODE_OUTPUT);
    gpio_set_level(VCC_OD_DIODE, 1);

    gpio_set_direction(GND_OD_DIODE, GPIO_MODE_OUTPUT);
    gpio_set_level(GND_OD_DIODE, 0);


    // Encoders initialization

    encoder_initialization(

        &encoder_ds_ll,
        GPIO_PIN_NONE,                // Choose to use the default 5V pin for enc VCC
        GPIO_PIN_NONE,                // Choose to use the default GND pin for the enc GND
        ENC_DUTY_SW,                  // SW by the defined pin name
        ENC_DUTY_DT,                  // DT by the defined pin name
        ENC_DUTY_CLK                  // CLK by the defined pin name

    );

    encoder_initialization(
        
        &encoder_fs_hl,
        GPIO_PIN_NONE,               // Choose to use the default 5V pin for enc VCC
        GPIO_PIN_NONE,               // Choose to use the default GND pin for the enc GND
        ENC_FREQ_SW,                 // SW by the defined pin name
        ENC_FREQ_DT,                 // DT by the defined pin name
        ENC_FREQ_CLK                 // CLK by the defined pin name

    );


    init_pwm(&u_c_pwm, VCC_OD_DIODE, frequency, duty_cycle);

    printf("Initialized frequency = %lu Hz\n", u_c_pwm.freq);


    // Display initialization

    if (ssd1306_init(0, 22, 21))
    {
        ESP_LOGI("OLED", "oled inited");

        ssd1306_clear(0);

        ssd1306_draw_rectangle(0, 10, 30, 20, 20, SSD1306_COLOR_WHITE);
        ssd1306_select_font(0, 0);
        ssd1306_draw_string(0, 0, 0, "glcd_5x7_font_info", SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);
        ssd1306_select_font(0, 1);
        ssd1306_draw_string(0, 0, 18, "tahoma_8pt_font_info", SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);
        ssd1306_draw_string(0, 55, 30, "Hello ESP32!", SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

        ssd1306_refresh(0, true);
    }
    else
    {
        ESP_LOGE("OLED", "oled init failed");
    }



    // Random PWM INIT
    // Random PWM parameters button initialization

    r_pwm_but = button_initialization(R_PWM_BUT_VCC, GPIO_PULLUP_ONLY, NO_FIX);


    // Interrupts counter

    gpio_set_direction(INTERRUPTS_COUNTER_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INTERRUPTS_COUNTER_PIN, GPIO_PULLDOWN_ONLY);

    // Interrupts permission
    gpio_install_isr_service(0);


    // Interrupts counter setup

    gpio_set_intr_type(INTERRUPTS_COUNTER_PIN, GPIO_INTR_POSEDGE); // Front reaction
    gpio_isr_handler_add(INTERRUPTS_COUNTER_PIN, pwm_pulse_isr_handler, NULL); // Counter callback

    // R_PWM pipeline initialization
    r_pwm_pipeline = random_pwm_pipeline();


    // R_PWM limits update
    update_pwm_setup_limits(

        &r_pwm_pipeline,
        r_pwm_frequency_low_limit, r_pwm_frequency_high_limit,
        r_pwm_duty_cycle_low_limit, r_pwm_duty_cycle_high_limit,
        r_pwm_repeats_low_limit, r_pwm_repeats_high_limit

    );

    // R_PWM queue update
    update_pwm_setup_queue(&r_pwm_pipeline);

    // Get the random triplet
    r_pwm_parameters = get_random_pwm_parameters(&r_pwm_pipeline);

    // Update repeats value
    r_pwm_repeats = r_pwm_parameters.repeats;

    stop_pwm(&u_c_pwm);
}

// =========================================================================================== SETUP


// =========================================================================================== MAIN LOOP

void main_loop(void *pvParameter)
{
    while (1)
    {
        // LOOP:

        // Regime choosing (USER-controlled PWM or random PWM)
        flag_control_by_but_onetime_press(&r_pwm_but, &r_pwm_but_pressed);

        // USER-controlled PWM
        if (!r_pwm_but_pressed) 
        {
            // Stop the random PWM
            if (r_pwm_enabled || !r_pwm_enabled) 
            {
                if (r_pwm_enabled)
                    stop_pwm(&u_c_pwm);
                    
                r_pwm_enabled = false;
                r_pwm_encoder_ds_ll_lt_pressed = false;
            }


            // DISPLAY AND PWM PARAMETERS UPDATE IN LOOP

            if (
                
                frequency != prev_frequency ||
                duty_cycle != prev_duty_cycle ||
                encoder_ds_ll_pressed != encoder_ds_ll_prev_pressed ||
                frequency_step != prev_frequency_step ||
                r_pwm_but_pressed_prev
            )           
            {

                r_pwm_but_pressed_prev = false;

                update_pwm(&u_c_pwm, frequency, duty_cycle);

                update_display_u_c_pwm();

                prev_frequency = frequency;
                prev_duty_cycle = duty_cycle;

                prev_frequency_step = frequency_step;
                encoder_ds_ll_prev_pressed = encoder_ds_ll_pressed;
            }


            // Freq encoder press control
            // Double click to change ()
            flag_control_by_but_onetime_press(&encoder_fs_hl.sw_button, &encoder_fs_hl_pressed);


            // Control frequency regulation step
            if (!encoder_fs_hl_pressed) r_pwm_freq_change_onetime_flag = true;

            if (encoder_fs_hl_pressed && r_pwm_freq_change_onetime_flag)
            {
                switch ((freq_step_t)frequency_step)
                {
                    case STEP_1HZ: frequency_step = (int)STEP_100HZ; break;
                    case STEP_100HZ: frequency_step = (int)STEP_1KHZ; break;
                    case STEP_1KHZ: frequency_step = (int)STEP_1HZ; break;
                }

                printf("Frequency step value changed to = %lu Hz\n", frequency_step);

                r_pwm_freq_change_onetime_flag = false;
            }


            // Control the duty cycle parameter by the encoder rotation
            enc_rotation_value_control(
                
                &encoder_ds_ll,
                CLOCKWISE, 
                LIMITATION,
                &duty_cycle,
                TYPE_UINT_8,
                &duty_cycle_step,
                &duty_cycle_min,
                &duty_cycle_max

            );

            if (frequency == 0) frequency = frequency_min; // 0 division defense 


            // Control the frequency parameter by the encoder rotation 
            enc_rotation_value_control(
                
                &encoder_fs_hl,
                CLOCKWISE, 
                LIMITATION,
                &frequency,
                TYPE_UINT_32,
                &frequency_step,
                &frequency_min,
                &frequency_max

            );

            // Duty encoder press control
            flag_control_by_but_onetime_press(&encoder_ds_ll.sw_button, &encoder_ds_ll_pressed);


            if (encoder_ds_ll_pressed && !u_c_pwm_enabled)
            {
                // PWM ON
                start_pwm(&u_c_pwm);
                u_c_pwm_enabled = true;
            }
            else if (!encoder_ds_ll_pressed && u_c_pwm_enabled)
            {
                // PWM OFF
                stop_pwm(&u_c_pwm);
                u_c_pwm_enabled = false;
            }
        }  


        // Random PWM

        else if (r_pwm_but_pressed)
        {
            if (u_c_pwm_enabled) 
            {
                stop_pwm(&u_c_pwm);
                encoder_ds_ll_pressed = false;
                u_c_pwm_enabled = false;
            }

            // DISPLAY AND PWM LIMITS UPDATE IN LOOP if some settings changed by encoders rotation
            // or SW presses
            if (

                // There could be signals from other functions and only 1 condition
                // but that arch was faster to implement -_-

                r_pwm_duty_cycle_low_limit != r_pwm_prev_duty_cycle_low_limit ||
                r_pwm_duty_cycle_high_limit != r_pwm_prev_duty_cycle_high_limit ||
                r_pwm_frequency_low_limit != r_pwm_prev_frequency_low_limit ||
                r_pwm_frequency_high_limit != r_pwm_prev_frequency_high_limit ||
                r_pwm_repeats_low_limit != r_pwm_prev_repeats_low_limit ||
                r_pwm_repeats_high_limit != r_pwm_prev_repeats_high_limit ||
                r_pwm_prev_frequency_step != r_pwm_frequency_step ||
                r_pwm_encoder_ds_ll_pressed != r_pwm_encoder_ds_ll_prev_pressed ||
                r_pwm_encoder_fs_hl_pressed != r_pwm_encoder_fs_hl_prev_pressed ||
                r_pwm_encoder_ds_ll_lt_pressed != r_pwm_encoder_ds_ll_prev_lt_pressed ||
                !r_pwm_but_pressed_prev

            ) 
            {
            
                r_pwm_but_pressed_prev = true;
                r_pwm_encoder_ds_ll_prev_lt_pressed = r_pwm_encoder_ds_ll_lt_pressed;


                // R_PWM limits update
                update_pwm_setup_limits(

                    &r_pwm_pipeline,
                    r_pwm_frequency_low_limit, r_pwm_frequency_high_limit,
                    r_pwm_duty_cycle_low_limit, r_pwm_duty_cycle_high_limit,
                    r_pwm_repeats_low_limit, r_pwm_repeats_high_limit

                );

                update_display_r_pwm();


                r_pwm_prev_frequency_low_limit = r_pwm_frequency_low_limit;
                r_pwm_prev_frequency_high_limit = r_pwm_frequency_high_limit;
                r_pwm_prev_duty_cycle_low_limit = r_pwm_duty_cycle_low_limit;
                r_pwm_prev_duty_cycle_high_limit = r_pwm_duty_cycle_high_limit;
                r_pwm_prev_repeats_low_limit = r_pwm_repeats_low_limit;
                r_pwm_prev_repeats_high_limit = r_pwm_repeats_high_limit;

                r_pwm_frequency_step = r_pwm_prev_frequency_step;

                encoder_ds_ll_prev_pressed = encoder_ds_ll_pressed;
                r_pwm_encoder_fs_hl_prev_pressed = r_pwm_encoder_fs_hl_pressed;
            }


            // Update the PWM settings triplet if there were r_pwm_repeat
            // pulses with previous settings
            if (pulse_count >= r_pwm_repeats)
            {
                pwm_repeats_overflow = true;
            }
            if (pwm_repeats_overflow)
            {
                // Get the new random triplet
                r_pwm_parameters = get_random_pwm_parameters(&r_pwm_pipeline);
                
                // Update repeats value
                r_pwm_repeats = r_pwm_parameters.repeats;

                // Init random PWM on same pin as user controlled PWM
                update_pwm(&u_c_pwm, r_pwm_parameters.frequency, r_pwm_parameters.duty_cycle);

                // Reset the counter
                pulse_count = 0;

                // Close the overflow logic way
                pwm_repeats_overflow = false;
            }
            

            // Select the parameter for control by the encoder_fs_hl SW 1 longtime press
            flag_control_by_but_onetime_press(&encoder_fs_hl.sw_button, &onetime_press_sw_to_change_curr_parameter);

            if (onetime_press_sw_to_change_curr_parameter && onetime_press_sw_to_change_curr_parameter_onetime_flag)
            {   
                switch ((controlled_parameter)curr_controlled_parameter)
                {
                    case DUTY: curr_controlled_parameter = (int)FREQ; break;
                    case FREQ: curr_controlled_parameter = (int)REPEATS; break;
                    case REPEATS: curr_controlled_parameter = (int)DUTY; break;
                }

                onetime_press_sw_to_change_curr_parameter = false;
                onetime_press_sw_to_change_curr_parameter_onetime_flag = false;
            }

            if (!onetime_press_sw_to_change_curr_parameter) onetime_press_sw_to_change_curr_parameter_onetime_flag = true;

            // Selected parameter control by the encoder rotation
            if (curr_controlled_parameter == DUTY)
            {
                // Control the low limit of the duty parameter by the encoder rotation 
                enc_rotation_value_control(
                    
                    &encoder_ds_ll,
                    CLOCKWISE, 
                    LIMITATION,
                    &r_pwm_duty_cycle_low_limit,
                    TYPE_UINT_8,
                    &duty_cycle_step,
                    &r_pwm_duty_cycle_min,
                    &r_pwm_duty_cycle_high_limit

                );

                // Control the high limit of the duty parameter by the encoder rotation 
                enc_rotation_value_control(
                    
                    &encoder_fs_hl,
                    CLOCKWISE, 
                    LIMITATION,
                    &r_pwm_duty_cycle_high_limit,
                    TYPE_UINT_8,
                    &duty_cycle_step,
                    &r_pwm_duty_cycle_low_limit,
                    &r_pwm_duty_cycle_max

                );
            }
            else if (curr_controlled_parameter == FREQ)
            {
                // Control the low limit of the frequency parameter by the encoder rotation 
                enc_rotation_value_control(
                    
                    &encoder_ds_ll,
                    CLOCKWISE, 
                    LIMITATION,
                    &r_pwm_frequency_low_limit,
                    TYPE_UINT_32,
                    &frequency_step,
                    &r_pwm_frequency_min,
                    &r_pwm_frequency_high_limit

                );


                // 0 division defense 
                if (r_pwm_frequency_low_limit == 0) r_pwm_frequency_high_limit = r_pwm_frequency_min;

                // Control the high limit of the frequency pparameter by the encoder rotation 
                enc_rotation_value_control(
                    
                    &encoder_fs_hl,
                    CLOCKWISE, 
                    LIMITATION,
                    &r_pwm_frequency_high_limit,
                    TYPE_UINT_32,
                    &frequency_step,
                    &r_pwm_frequency_low_limit,
                    &r_pwm_frequency_max

                );

                // 0 division defense 
                if (r_pwm_frequency_high_limit == 0) r_pwm_frequency_high_limit = r_pwm_frequency_min;
            }
            else if (curr_controlled_parameter == REPEATS)
            {
                // Control the low limit of the duty parameter by the encoder rotation 
                enc_rotation_value_control(
                    
                    &encoder_ds_ll,
                    CLOCKWISE, 
                    LIMITATION,
                    &r_pwm_repeats_low_limit,
                    TYPE_UINT_32,
                    &r_pwm_repeats_step,
                    &r_pwm_repeats_min,
                    &r_pwm_repeats_high_limit

                );

                // Control the high limit of the duty parameter by the encoder rotation 
                enc_rotation_value_control(
                    
                    &encoder_fs_hl,
                    CLOCKWISE, 
                    LIMITATION,
                    &r_pwm_repeats_high_limit,
                    TYPE_UINT_32,
                    &r_pwm_repeats_step,
                    &r_pwm_repeats_low_limit,
                    &r_pwm_repeats_max

                );
            }

            // Контроль шага частоты одинарным нажатием на duty enc
            if (curr_controlled_parameter == FREQ)
            {
                // Frequency step control by onetime press on encoder_fs_hl SW button
                flag_control_by_but_onetime_press(&encoder_ds_ll.sw_button, &r_pwm_encoder_ds_ll_pressed);


                // Control frequency regulation step
                if (r_pwm_encoder_ds_ll_pressed && r_pwm_step_change_onetime_flag)
                {
                    switch ((freq_step_t)frequency_step)
                    {
                        case STEP_1HZ: frequency_step = (int)STEP_100HZ; break;
                        case STEP_100HZ: frequency_step = (int)STEP_1KHZ; break;
                        case STEP_1KHZ: frequency_step = (int)STEP_1HZ; break;
                    }

                    printf("Frequency step value changed to = %lu Hz\n", frequency_step);

                    onetime_press_sw_to_change_curr_parameter = false;
                    r_pwm_encoder_ds_ll_pressed= false;
                }

                if (!r_pwm_encoder_ds_ll_pressed) r_pwm_step_change_onetime_flag  = true;
            }


            // Duty encoder press control - ON / OFF
            flag_control_by_but_longtime_press(&encoder_ds_ll.sw_button, &r_pwm_encoder_ds_ll_lt_pressed);

            if (r_pwm_encoder_ds_ll_lt_pressed) r_pwm_encoder_ds_ll_pressed = false;

            if (r_pwm_encoder_ds_ll_lt_pressed && !r_pwm_enabled) 
            {
                // PWM ON
                start_pwm(&u_c_pwm);
                r_pwm_enabled = true;

                r_pwm_encoder_ds_ll_prev_lt_pressed = !r_pwm_encoder_ds_ll_lt_pressed;
            }
            else if (!r_pwm_encoder_ds_ll_lt_pressed && r_pwm_enabled)
            {
                // PWM OFF
                r_pwm_encoder_ds_ll_pressed = false;
                stop_pwm(&u_c_pwm);
                r_pwm_enabled = false;

                r_pwm_encoder_ds_ll_prev_lt_pressed = !r_pwm_encoder_ds_ll_lt_pressed;
            }
        }
    
        // LOOP END

        
        // Delay for correct loop
        #if USE_FREERTOS
            // vTaskDelay(pdMS_TO_TICKS(RTOS_LOOP_DELAY_MS));
        #else
            // esp_rom_delay_us(1000);

            await(1, TIME_UNIT_US); // Ending_await
        #endif
    }
}

// =========================================================================================== MAIN LOOP


// =========================================================================================== APP

void app_main()
{
    // INITIALIZATION:

    initialization();

    // INITIALIZATION END


    // PRE-CYCLE:

    // PRE-CYCLE END


    // CYCLE:

    #if USE_FREERTOS
        xTaskCreate(main_loop, "Main Loop", 2048, NULL, 5, NULL);
    #else
        main_loop(NULL);
    #endif

    // CYCLE END
}

// =========================================================================================== APP


// =========================================================================================== REALIZATION

// Display update
void update_display_u_c_pwm()
{
    ssd1306_clear(0);

    ssd1306_select_font(0, 1);

    snprintf(display_buf, sizeof(display_buf), "Duty: %u %%", duty_cycle);
    ssd1306_draw_string(0, 0, 0, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

    snprintf(display_buf, sizeof(display_buf), "Freq: %lu Hz", frequency);
    ssd1306_draw_string(0, 0, 15, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

    ssd1306_draw_string(0, 0, 30, u_c_pwm_enabled ? "PWM: ON " : "PWM: OFF", SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

    snprintf(display_buf, sizeof(display_buf), "Step: %lu Hz", frequency_step);
    ssd1306_draw_string(0, 0, 45, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

    ssd1306_refresh(0, true);
}


// Display update
void update_display_r_pwm()
{
    ssd1306_clear(0);

    ssd1306_select_font(0, 1);


    snprintf(display_buf, sizeof(display_buf), "D_l: %u %%", r_pwm_duty_cycle_low_limit);
    ssd1306_draw_string(0, 0, 0, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

    snprintf(display_buf, sizeof(display_buf), "D_h: %u %%", r_pwm_duty_cycle_high_limit);
    ssd1306_draw_string(0, 60, 0, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);


    snprintf(display_buf, sizeof(display_buf), "F_l: %lu Hz", r_pwm_frequency_low_limit);
    ssd1306_draw_string(0, 0, 16, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

    snprintf(display_buf, sizeof(display_buf), "F_h: %lu Hz", r_pwm_frequency_high_limit);
    ssd1306_draw_string(0, 60, 16, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);


    snprintf(display_buf, sizeof(display_buf), "R_l: %lu", r_pwm_repeats_low_limit);
    ssd1306_draw_string(0, 0, 32, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);

    snprintf(display_buf, sizeof(display_buf), "R_h: %lu", r_pwm_repeats_high_limit);
    ssd1306_draw_string(0, 60, 32, display_buf, SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);


    ssd1306_draw_string(0, 0, 48, r_pwm_enabled ? "R_PWM: ON " : "R_PWM: OFF", SSD1306_COLOR_WHITE, SSD1306_COLOR_BLACK);


    ssd1306_refresh(0, true);
}

// Interrupts counter
static void IRAM_ATTR pwm_pulse_isr_handler(void* arg)
{
    pulse_count++;
}

// =========================================================================================== REALIZATION
