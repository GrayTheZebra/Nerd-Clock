#pragma once
#define GPT_TIMER 1
#define TIMER_MODE_PERIODIC 1
#define TIMER_EVENT_CYCLE_END 1
struct timer_callback_args_t{int event;};
struct FspTimer{bool stop(){return true;}static int get_available_timer(uint8_t&,bool=false){return 0;} static void force_use_of_pwm_reserved_timer(){} bool begin(int,uint8_t,uint8_t,float,float,void(*)(timer_callback_args_t*)){return true;} bool setup_overflow_irq(){return true;} bool open(){return true;} bool start(){return true;}};
