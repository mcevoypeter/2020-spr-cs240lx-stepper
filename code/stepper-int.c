#include "stepper-int.h"
#include "timer-interrupt.h"
#include "cycle-count.h"

// you can/should play around with this
#define STEPPER_INT_TIMER_INT_PERIOD 100 

static int first_init = 1;

#define MAX_STEPPERS 16
static stepper_int_t * my_steppers[MAX_STEPPERS];
static unsigned num_steppers = 0;

void stepper_int_handler(unsigned pc) {
    // check and clear timer interrupt
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;
    dev_barrier();  

    stepper_int_t * stepper_int = my_steppers[num_steppers-1]; 
    Q_t * pos_Q = &stepper_int->positions_Q;

    if (!Q_empty(pos_Q)) {
        stepper_position_t * pos = Q_start(pos_Q);
        pos->status = STARTED;
        stepper_direction_t dir = (pos->goal_steps < 0 ? BACKWARD : FORWARD);

        // enough time has elapsed to take another step
        if (pos->usec_at_prev_step + pos->usec_between_steps < timer_get_usec()) {
            if (dir == BACKWARD) {
                stepper_step_backward(stepper_int->stepper);
                pos->goal_steps++;
            } else if (dir == FORWARD) {
                stepper_step_forward(stepper_int->stepper);
                pos->goal_steps--;
            } else
                panic("unknown step direction\n");
            pos->usec_at_prev_step = timer_get_usec();
        }

        // more steps to take so back onto the queue it goes
        if (pos->goal_steps == 0) {
            Q_pop(pos_Q);
            pos->status = FINISHED;
            kfree(pos);
        }
    } 
    dev_barrier();
    PUT32(arm_timer_IRQClear, 1);
}

static unsigned usec_at_prev_step;
stepper_int_t * stepper_init_with_int(unsigned dir, unsigned step){
    if(num_steppers == MAX_STEPPERS){
        return NULL;
    }
    kmalloc_init();

    gpio_set_output(dir);
    gpio_set_output(step);
    stepper_int_t * stepper_int = kmalloc(sizeof(stepper_int_t));
    *stepper_int = (stepper_int_t){
        .stepper = kmalloc(sizeof(stepper_t)),
        .status = NOT_IN_JOB,
        .positions_Q = (Q_t){
            .head = 0,
            .tail = 0,
            .cnt = 0
        }
    };

    *(stepper_int->stepper) = (stepper_t){
        .step_count = 0,
        .dir = dir,
        .step = step,
        .MS1 = UNUSED,
        .MS2 = UNUSED,
        .MS3 = UNUSED
    };
    my_steppers[num_steppers++] = stepper_int;

    //initialize interrupts; only do once, on the first init
    if(first_init){
        first_init = 0;
        int_init();
        cycle_cnt_init();
        timer_interrupt_init(STEPPER_INT_TIMER_INT_PERIOD);
        system_enable_interrupts();
    }
    usec_at_prev_step = timer_get_usec();
    return stepper_int;
}

stepper_int_t * stepper_int_init_with_microsteps(unsigned dir, unsigned step, unsigned MS1, unsigned MS2, unsigned MS3, stepper_microstep_mode_t microstep_mode){
    unimplemented();
}

/* retuns the enqueued position. perhaps return the queue of positions instead? */
stepper_position_t * stepper_int_enqueue_pos(stepper_int_t * stepper, int goal_steps, unsigned usec_between_steps){
    
    stepper_position_t * new_pos = kmalloc(sizeof(stepper_position_t));
    *new_pos = (stepper_position_t){
        .next = 0,
        .goal_steps = goal_steps,
        .usec_between_steps = usec_between_steps,
        .usec_at_prev_step = timer_get_usec(), 
        .status = NOT_STARTED
    };
    Q_append(&stepper->positions_Q, new_pos);


    return new_pos;
}

int stepper_int_get_position_in_steps(stepper_int_t * stepper){
    return stepper_get_position_in_steps(stepper->stepper);
}

int stepper_int_is_free(stepper_int_t * stepper){
    return Q_empty(&stepper->positions_Q);
}

int stepper_int_position_is_complete(stepper_position_t * pos){
    return pos->status == FINISHED;
}

