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

    for (unsigned i = 0; i < num_steppers; i++) {
        stepper_int_t * stepper_int = my_steppers[i]; 
        Q_t * pos_Q = &stepper_int->positions_Q;
        if (Q_empty(pos_Q))
            continue;

        stepper_position_t * pos = Q_start(pos_Q);
        pos->status = STARTED;
        stepper_direction_t dir = (pos->goal_steps < 0 ? BACKWARD : FORWARD);

        // enough time has elapsed to take another step
        if (pos->usec_at_prev_step + pos->usec_between_steps < timer_get_usec()) {
            if (dir == BACKWARD) 
                stepper_step_backward(stepper_int->stepper);
            else if (dir == FORWARD)
                stepper_step_forward(stepper_int->stepper);
            else
                panic("unknown step direction\n");
            pos->usec_at_prev_step = timer_get_usec();
        }

        // reached our step goal
        if (stepper_int->stepper->step_count == pos->goal_steps) {
            Q_pop(pos_Q);
            pos->status = FINISHED;
            kfree(pos);
        }
    }
    dev_barrier();
    PUT32(arm_timer_IRQClear, 1);
}

stepper_int_t * stepper_init_with_int(unsigned dir, unsigned step){
    if(num_steppers == MAX_STEPPERS){
        return NULL;
    }
    kmalloc_init();
    //initialize interrupts; only do once, on the first init
    if(first_init){
        first_init = 0;
        int_init();
        cycle_cnt_init();
        timer_interrupt_init(STEPPER_INT_TIMER_INT_PERIOD);
        system_enable_interrupts();
    }

    stepper_t * stepper = stepper_init(dir, step);
    stepper_int_t * stepper_int = kmalloc(sizeof(stepper_int_t));
    *stepper_int = (stepper_int_t){
        .stepper = stepper,
        .status = NOT_IN_JOB,
        .positions_Q = (Q_t){
            .head = 0,
            .tail = 0,
            .cnt = 0
        }
    };
    my_steppers[num_steppers++] = stepper_int;

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

