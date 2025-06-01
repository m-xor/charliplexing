/*
 * soft_timer/software_timer_port.h
 *
 *  Created on: 31 sty 2021
 *      Author: slawek
 */

#ifndef SOFTWARE_TIMER_PORT_H_
#define SOFTWARE_TIMER_PORT_H_

#include <stdint.h>

/*! \typedef
 *  \brief Type of timer counter
 *
 * Look out capacity of counter. In this case it is 256 ticks. Overflowing is not
 * checked.
 */
typedef uint8_t Counter;

/*! \def
 *  \brief Interrupt guard
 *
 *  In case of uint8_t there is no need to guard critical sections.
 */
#define CRITICAL_SECTION_BEGIN
#define CRITICAL_SECTION_END


#endif /* SOFTWARE_TIMER_PORT_H_ */
