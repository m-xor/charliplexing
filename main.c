/*
 * main.c
 *
 *  Created on: 28 maj 2025
 *      Author: slawek
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "software_timer.h"
#include <stdlib.h>
#include "display.h"

Timer counter_period;
Timer power_period;
Timer droplet_period;
Timer filling_period;
//Timer error_showing;
//Timer dash_showing;

#define COUNTER_UP_DELAY_MS		20
#define COUNTER_DOWN_DELAY_MS	90
#define POWER_DELAY_MS			250
#define DROPLET_DELAY_MS		100
#define FILLING_DELAY_MS		500

#define TICK_MS 8

#define COUNTER_UP_TICKS 	TICKS(COUNTER_UP_DELAY_MS,TICK_MS)
#define COUNTER_DOWN_TICKS	TICKS(COUNTER_DOWN_DELAY_MS,TICK_MS)
#define POWER_TICKS			TICKS(POWER_DELAY_MS,TICK_MS)
#define DROPLET_TICKS		TICKS(DROPLET_DELAY_MS,TICK_MS)
#define FILLING_TICKS		TICKS(FILLING_DELAY_MS,TICK_MS)


extern void do_counter(void);
extern void do_power(void);
extern void do_droplet(void);
extern void do_filling(void);

#pragma GCC diagnostic ignored "-Wmain"
void main() {

//timer 2 - licznik 5ms
TIMSK2 = _BV(TOIE2);
TCCR2B = _BV(CS22)|_BV(CS21); //preskaler 256 -> okres 8 ms -> TICK_MS


display_init();

sei();
//zapalenie całości
display_number(88);
display_power(true);
display_percent(true);
display_droplet(5);
display_filling(4);

//jasność
for(uint8_t i=100; i>0; i--) {
	_delay_ms(20);
	display_brigthness(i-1);
}

for(uint8_t i=0; i<100; i++) {
	_delay_ms(20);
	display_brigthness(i+1);
}

//test zatrzymania/wznowienia pracy drivera - DEEP SLEEP
display_driver_off();
_delay_ms(1000);
display_driver_on();
_delay_ms(1000);

display_clear();

//test symboli dla kropli
for(uint8_t i=2; i; i--) {
display_droplet(DROP_NW);
_delay_ms(100);
display_droplet(DROP_SW);
_delay_ms(100);
display_droplet(DROP_SE);
_delay_ms(100);
display_droplet(DROP_NE);
_delay_ms(100);
display_droplet(DROP_N);
_delay_ms(100);
};
display_droplet(DROP_BLANK);

_delay_ms(300);

//test symboli wypełnienia
display_filling(FILL_LEVEL_1);
_delay_ms(200);
display_filling(FILL_LEVEL_2);
_delay_ms(200);
display_filling(FILL_LEVEL_3);
_delay_ms(200);
display_filling(FILL_LEVEL_4);
_delay_ms(200);
display_filling(FILL_BLANK);
_delay_ms(300);
display_filling(FILL_GREEN_2);
_delay_ms(200);
display_filling(FILL_GREEN_1);
_delay_ms(200);
display_filling(FILL_BLUE);
_delay_ms(200);
display_filling(FILL_RED);
_delay_ms(200);
display_filling(FILL_BLANK);
_delay_ms(300);
display_filling(FILL_ALL);
_delay_ms(300);
display_filling(FILL_BLANK);

_delay_ms(300);

//test symboli wyświetlania numerów
display_number(NUMBER_E);
_delay_ms(200);
display_number(NUMBER_E0);
_delay_ms(300);
display_number(NUMBER_E1);
_delay_ms(300);
display_number(NUMBER_E2);
_delay_ms(300);
display_number(NUMBER_E3);
_delay_ms(300);
display_number(NUMBER_E4);
_delay_ms(300);
display_number(NUMBER_E5);
_delay_ms(300);
display_number(NUMBER_E6);
_delay_ms(300);
display_number(NUMBER_E7);
_delay_ms(300);
display_number(NUMBER_E8);
_delay_ms(300);
display_number(NUMBER_E9);
_delay_ms(300);
display_number(NUMBER_DASH);
_delay_ms(300);
display_number_clear();


Timer_setPeriod(&counter_period, TICKS(1000,TICK_MS));
Timer_setPeriod(&power_period, TICKS(2000,TICK_MS));
Timer_setPeriod(&droplet_period, TICKS(100,TICK_MS));
Timer_setPeriod(&filling_period, TICKS(500,TICK_MS));

for(;;) {
	if(Timer_isTime(&counter_period)) {
		do_counter();
	}
	if(Timer_isTime(&power_period)) {
		do_power();
	}
	if(Timer_isTime(&droplet_period)) {
		do_droplet();
	}
	if(Timer_isTime(&filling_period)) {
		do_filling();
	}
}

}

ISR(TIMER2_OVF_vect)
{
	Timer_count(&counter_period);
	Timer_count(&power_period);
	Timer_count(&droplet_period);
	Timer_count(&filling_period);
}


void do_counter(void)
{
	static uint8_t counter;
	static bool direction = true;
	Counter ticks = COUNTER_UP_TICKS;

	display_number(counter);
	if(direction) {
		counter++;
	} else {
		counter--;
	}
	if(counter == 100){
		counter = 99;
		direction = false;
	} else if(counter == 0) {
		direction = true;
	} else if(direction) {
		ticks = COUNTER_UP_TICKS;
	} else {
		ticks = COUNTER_DOWN_TICKS;
	}
	if(counter > 40 && counter < 75) {
		display_percent(true);
	} else {
		display_percent(false);
	}
	Timer_setPeriod(&counter_period, ticks);
}

void do_power(void)
{
	static bool show = true;
	display_power(show);
	show = !show;
	Timer_setPeriod(&power_period, POWER_TICKS);
}


void do_droplet(void)
{
	static uint8_t segment;

	segment %= DROP_SEGS_MAX;
	display_droplet(segment++);
	Timer_setPeriod(&droplet_period, DROPLET_TICKS);
}

void do_filling(void)
{
	static uint8_t level=1;

	uint8_t dir = rand() & 1;
	level += dir ? +1 : -1;
	if(level == 0) {
		level = 4;
	} else if(level == 5) {
		level = 1;
	}
	display_filling(level-1);
	Timer_setPeriod(&filling_period, FILLING_TICKS);
}
