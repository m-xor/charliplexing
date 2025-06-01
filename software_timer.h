/*
 * soft_timer/software_timer.h
 *
 *  Created on: 31 sty 2021
 *      Author: slawek
 */

#ifndef SOFTWARE_TIMER_H_
#define SOFTWARE_TIMER_H_

#include "software_timer_port.h"

/*! \class Timer
 *  \brief Software timer
 *
 * 	Conveys foreground (ISR) time events to background loop
 *
 */
typedef struct Timer {
	_Bool active;
	volatile Counter cnt;
} Timer;

/*! \def   Ticks
 *  \brief Ticks calculator
 *
 *  Calculates timer event periods into timer ticks
 *
 *  @param period       length of period in milliseconds
 *  @param timer_period length of hardware timer period in milliseconds
 *
 */
#define TICKS(period,timer_period) ((period)/(timer_period)+(((period)%(timer_period))?1:0))

/*! \fn    Constructor
 *  \brief Class initializer
 *
 *  Initializes software timer instance. Not needed when instance is allocated
 *  in BSS section.
 *
 *  @param me pointer to software timer instance
 */
static inline void Timer_ctor(Timer * const me)
{
	me->active = 0;
	CRITICAL_SECTION_BEGIN
	me->cnt = 0;
	CRITICAL_SECTION_END
}

/*! \fn    Setter
 *  \brief Set period and start counting
 *
 * @param me    pointer to software timer instance
 * @param ticks number of ticks to go, usually computed by TICKS macro
 */
static inline void Timer_setPeriod(Timer * const me, Counter ticks)
{
	CRITICAL_SECTION_BEGIN
	me->cnt = ticks;
	CRITICAL_SECTION_END
	me->active = 1;
}

/*! \fn    Activator
 *  \brief Activates/decactivates software timer
 *
 *  Deactivation of timer which is set (!=0) doesn't stop counting ticks.
 *  Activation of timer which is not set (==0) fires next isTime function.
 *
 * @param me         pointer to software timer instance
 * @param activeness bool state of timer to be set
 */
static inline void Timer_activate(Timer * const me, _Bool activeness)
{
	me->active = activeness;
}

/*! \fn    Checker
 *  \brief checks if timer expired
 *
 *  Function is to be called from background loop.
 *  First call after expiry deactivates software timer
 *
 * @param  me pointer to software timer instance
 * @return 1 if timer expired and active, 0 otherwise
 */
static inline _Bool Timer_isTime(Timer * const me)
{
	Counter cnt;
	_Bool result;
	CRITICAL_SECTION_BEGIN
	cnt = me->cnt;
	CRITICAL_SECTION_END
	result = !cnt && me->active && !(me->active=0);
	return result;
}

/*! \fn Counter
 *  \brief Counts time
 *
 *  Function should be called from within timer interrupt
 *
 * @param me pointer to software timer instance
 */
static inline void Timer_count(Timer * const me) __attribute__((always_inline));
static inline void Timer_count(Timer * const me)
{
	Counter tmp = me->cnt;
	if(tmp) {
		me->cnt=--tmp;
	}
}

#endif /* SOFTWARE_TIMER_H_ */
