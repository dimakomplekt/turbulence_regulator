// random_pwm_setup.h


// =========================================================================================== IMPORT

#include <stdint.h>

#include <stdio.h>

#include "esp_random.h"         // Pseudorandom values generator

// =========================================================================================== IMPORT


// =========================================================================================== DEFINES

#define MAX_QUEUE_SIZE 2        // Max size of the random PWM queue

// =========================================================================================== DEFINES


// =========================================================================================== STRUCTS

// Current PWM parameters limitations
typedef struct
{

    uint32_t min_freq; uint32_t max_freq;
    uint8_t min_duty; uint8_t max_duty;
    uint32_t min_repeats; uint32_t max_repeats;

} random_pwm_setup_limits_ctx;


// Current PWM parameters
typedef struct
{

    uint32_t frequency;
    uint8_t duty_cycle;
    uint32_t repeats;

} random_pwm_setup_ctx;


// Current PWM pipeline struct
typedef struct
{
    
    random_pwm_setup_ctx* random_pwm_queue;         // Parameters queue
    uint8_t current_queue_size;                     // Current queue objects quantity
    uint8_t max_queue_size;                         // Maximum queue objects quantity

    random_pwm_setup_limits_ctx pwm_limits;         // Current PWM parameters limitations

} random_pwm_pipeline_ctx;

// =========================================================================================== STRUCTS


// =========================================================================================== API

// Function: random_pwm_pipeline()
// Generates the pipeline with init values
random_pwm_pipeline_ctx random_pwm_pipeline(void);


// Function: generate_r_pwm_setup()
// Randomly generates 1 triplet of PWM settings by the current limitations values
random_pwm_setup_ctx generate_r_pwm_setup(
    
    uint32_t min_freq, uint32_t max_freq,
    uint8_t min_duty, uint8_t max_duty,
    uint32_t min_repeats, uint32_t max_repeats

);


// Function: update_pwm_setup_limits()
// Updates PWM pipeline limitations pwm_limits inside random_pwm_pipeline_ctx
// by the passed values
void update_pwm_setup_limits(
    
    random_pwm_pipeline_ctx* current_pwm_pipeline,
    uint32_t min_freq, uint32_t max_freq,
    uint8_t min_duty, uint8_t max_duty,
    uint32_t min_repeats, uint32_t max_repeats

);


// Function: update_pwm_setup_queue()
// Updates PWM pipeline (random_pwm_pipeline_ctx) queue by the current pwm_limits - generates PWM parameters
// by generate_r_pwm_setup() and put them to the random_pwm_queue, until the current_queue_size == max_queue_size
void update_pwm_setup_queue(random_pwm_pipeline_ctx* current_pwm_pipeline);

// Function: get_random_pwm_parameters()
// Returns the first PWM setup in queue (FIFO), shifts queue left and updates it with a new random setup
random_pwm_setup_ctx get_random_pwm_parameters(random_pwm_pipeline_ctx* current_pwm_pipeline);

// =========================================================================================== API
