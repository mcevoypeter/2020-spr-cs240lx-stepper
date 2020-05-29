#ifndef __STEPPER_H__
#define __STEPPER_H__

// for microstepping extension
typedef enum {
    FULL_STEP,
    HALF_STEP,
    QUARTER_STEP,
    EIGHTH_STEP,
    SIXTEENTH_STEP
} stepper_microstep_mode_t;

typedef enum {
    BACKWARD = 0,
    FORWARD = 1
} stepper_direction_t;

// see images/a4988_pinout.png
enum { UNUSED = 0 };
typedef unsigned gpio_pin_t;
typedef struct {
    int step_count;
    gpio_pin_t dir;
    gpio_pin_t step;
    gpio_pin_t MS1;
    gpio_pin_t MS2;
    gpio_pin_t MS3;
} stepper_t;

stepper_t * stepper_init(unsigned dir, unsigned step);

// for microstepping extension
stepper_t * stepper_init_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode);

// for microstepping extension
void stepper_set_microsteps(stepper_t * stepper, stepper_microstep_mode_t microstep_mode);

void stepper_step_forward(stepper_t * stepper);

void stepper_step_backward(stepper_t * stepper);

int stepper_get_position_in_steps(stepper_t * stepper);

#endif 
