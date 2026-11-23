// random_pwm_setup.c


// =========================================================================================== IMPORT

#include "random_pwm_setup.h"

#include <assert.h>
#include <stdlib.h>

// =========================================================================================== IMPORT


// =========================================================================================== API

// Pipeline constructor initialization
random_pwm_pipeline_ctx random_pwm_pipeline(void)
{
    random_pwm_pipeline_ctx pipeline;

    // Memory allocation for the queue with MAX_QUEUE_SIZE elements
    pipeline.random_pwm_queue = malloc(MAX_QUEUE_SIZE * sizeof(random_pwm_setup_ctx));

    // Initialize queue with zeroed random_pwm_setup_ctx structs
    for (int i = 0; i < MAX_QUEUE_SIZE; i++)
    {
        pipeline.random_pwm_queue[i].frequency = 0;
        pipeline.random_pwm_queue[i].duty_cycle = 0;
        pipeline.random_pwm_queue[i].repeats = 0;
    }

    // Set initial queue size and limits
    pipeline.current_queue_size = 0;
    pipeline.max_queue_size = MAX_QUEUE_SIZE;

    // Initialize PWM limits to zeros
    pipeline.pwm_limits.min_freq = 0;
    pipeline.pwm_limits.max_freq = 0;
    pipeline.pwm_limits.min_duty = 0;
    pipeline.pwm_limits.max_duty = 0;
    pipeline.pwm_limits.min_repeats = 0;
    pipeline.pwm_limits.max_repeats = 0;

    return pipeline;
}


// Helper generator function for ESP32
// Generate a uniform random 32-bit unsigned integer in the given [min, max] range.
// Includes rejection sampling to remove modulo bias and ensure uniform distribution.
static inline uint32_t rand_range_uint_32(uint32_t min, uint32_t max)
{
    // Safety check: if range is invalid or zero-length, return min directly
    if (max <= min) return min;

    // Calculate the size of the range (inclusive)
    uint32_t range = max - min + 1;

    // Compute the largest integer limit that fits evenly into 2^32
    // Example: if 2^32 = 100 and range = 30, then limit = 90.
    // This ensures that when we take modulo, each result 0..29 appears equally often.
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range);

    uint32_t random_value;

    // Keep generating new random numbers until we get one below the limit
    // This avoids the small bias that comes from uneven division of 2^32 by 'range'.
    do { random_value = esp_random();
    } while (random_value >= limit);

    // Scale the value into the desired [min, max] range
    return min + (random_value % range);
}


// Triplet generator realization
random_pwm_setup_ctx generate_r_pwm_setup(

    uint32_t min_freq, uint32_t max_freq,
    uint8_t min_duty, uint8_t max_duty,
    uint32_t min_repeats, uint32_t max_repeats

)
{
    
    random_pwm_setup_ctx curr_pwm_ctx;

    // Helper-generator for ESP32
    curr_pwm_ctx.frequency = rand_range_uint_32(min_freq, max_freq);
    curr_pwm_ctx.duty_cycle = (uint8_t)rand_range_uint_32((uint32_t)min_duty, (uint32_t)max_duty);
    curr_pwm_ctx.repeats = rand_range_uint_32(min_repeats, max_repeats);

    return curr_pwm_ctx;
}


void update_pwm_setup_limits(

    random_pwm_pipeline_ctx* current_pwm_pipeline,
    uint32_t min_freq, uint32_t max_freq,
    uint8_t min_duty, uint8_t max_duty,
    uint32_t min_repeats, uint32_t max_repeats

)
{
    current_pwm_pipeline->pwm_limits.min_freq = min_freq;
    current_pwm_pipeline->pwm_limits.max_freq = max_freq;
    current_pwm_pipeline->pwm_limits.min_duty = min_duty;
    current_pwm_pipeline->pwm_limits.max_duty = max_duty;
    current_pwm_pipeline->pwm_limits.min_repeats = min_repeats;
    current_pwm_pipeline->pwm_limits.max_repeats = max_repeats;
}


void update_pwm_setup_queue(random_pwm_pipeline_ctx* current_pwm_pipeline)
{
    // Loop until the queue is full
    while (current_pwm_pipeline->current_queue_size < current_pwm_pipeline->max_queue_size)
    {
        // Generate one random PWM setup using current limits
        random_pwm_setup_ctx new_setup = generate_r_pwm_setup(

            current_pwm_pipeline->pwm_limits.min_freq,
            current_pwm_pipeline->pwm_limits.max_freq,
            current_pwm_pipeline->pwm_limits.min_duty,
            current_pwm_pipeline->pwm_limits.max_duty,
            current_pwm_pipeline->pwm_limits.min_repeats,
            current_pwm_pipeline->pwm_limits.max_repeats

        );

        // Add new random PWM setup to the queue and increment current_queue_size after this
        current_pwm_pipeline->random_pwm_queue[current_pwm_pipeline->current_queue_size++] = new_setup;
    }
}


// PWM parameters getter realization
random_pwm_setup_ctx get_random_pwm_parameters(random_pwm_pipeline_ctx* current_pwm_pipeline)
{
    // Error-handler
    if (current_pwm_pipeline == NULL || current_pwm_pipeline->random_pwm_queue == NULL)
    {
        printf("FATAL: R_PWM pipeline or queue is NULL! Aborting...\n");
        abort(); // Quit program
    }

    // If the queue is empty — just refill and return zero setup
    if (current_pwm_pipeline->current_queue_size == 0)
    {
        update_pwm_setup_queue(current_pwm_pipeline);
    }

    // TMP the first element (FIFO pop)
    random_pwm_setup_ctx first = current_pwm_pipeline->random_pwm_queue[0];

    // Shift all remaining elements one step down
    for (int i = 1; i < current_pwm_pipeline->current_queue_size; i++)
    {
        // Rewrite [i - 1] as [i]
        current_pwm_pipeline->random_pwm_queue[i - 1] = current_pwm_pipeline->random_pwm_queue[i];
    }

    // Decrease the current size
    current_pwm_pipeline->current_queue_size--;

    if (current_pwm_pipeline->current_queue_size < current_pwm_pipeline->max_queue_size)
        update_pwm_setup_queue(current_pwm_pipeline);

    return first;
}

// =========================================================================================== API
