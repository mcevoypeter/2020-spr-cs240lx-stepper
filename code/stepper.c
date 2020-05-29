#include "stepper.h"
#include "rpi.h"
#include "math-helpers.h"

stepper_t * stepper_init(unsigned dir, unsigned step){
    kmalloc_init();
    stepper_t * stepper = kmalloc(sizeof(stepper_t));
    
    // XXX
    gpio_set_output(dir);
    gpio_set_output(step);
    *stepper = (stepper_t){
        .step_count = 0,
        .dir = dir,
        .step = step,
        .MS1 = UNUSED,
        .MS2 = UNUSED,
        .MS3 = UNUSED
    };

    return stepper;
}

// If you want to do microstep extension:
void stepper_set_microsteps(stepper_t * stepper, stepper_microstep_mode_t microstep_mode){
    unimplemented();
}

// If you want to do microstep extension:
stepper_t * stepper_init_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode){
    unimplemented();
}

// how many gpio writes should you do?
void stepper_step_forward(stepper_t * stepper){
    gpio_write(stepper->dir, FORWARD);
    gpio_write(stepper->step, 1);
    delay_us(1);
    gpio_write(stepper->step, 0);
    stepper->step_count++;
}

void stepper_step_backward(stepper_t * stepper){
    gpio_write(stepper->dir, BACKWARD);
    gpio_write(stepper->step, 1);
    delay_us(1);
    gpio_write(stepper->step, 0);
    stepper->step_count--;
}

int stepper_get_position_in_steps(stepper_t * stepper){
    return stepper->step_count;
}
