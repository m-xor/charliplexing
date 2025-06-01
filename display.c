/*
 * charlie.c
 *
 *  Created on: 26 maj 2025
 *      Author: slawek
 */

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "display.h"

//testowanie ISR
#ifdef DEBUG_ISR_TEST

#define TEST_PIN_0_INIT O_DIR |= _BV(TEST_PIN_0);
#define TEST_PIN_0_HIGH	O_PORT |= _BV(TEST_PIN_0);
#define TEST_PIN_0_LOW	O_PORT &= ~_BV(TEST_PIN_0);

#define TEST_PIN_1_INIT	O_DIR |= _BV(TEST_PIN_1);
#define TEST_PIN_1_HIGH	O_PORT |= _BV(TEST_PIN_1);
#define TEST_PIN_1_LOW	O_PORT &= ~_BV(TEST_PIN_1);

#else

#define TEST_PIN_0_INIT
#define TEST_PIN_0_HIGH
#define TEST_PIN_0_LOW

#define TEST_PIN_1_INIT
#define TEST_PIN_1_HIGH
#define TEST_PIN_1_LOW

#endif

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
/*
 * Maski dla linii wejściowych wyświetlacza
 */
#define LINE_A _BV(A_PIN)
#define LINE_B _BV(B_PIN)
#define LINE_C _BV(C_PIN)
#define LINE_D _BV(D_PIN)
#define LINE_E _BV(E_PIN)
#define LINE_F _BV(F_PIN)
#define DISPLAY_LINES_MASK (LINE_A|LINE_B|LINE_C|LINE_D|LINE_E|LINE_F)
#define DISPLAY_LINES_NEG_MASK (uint8_t)~DISPLAY_LINES_MASK

/*
 * Opis segmentów wyświetlacza
 */
//makro initializer dla typu segment_type
//dla aktywnego segmentu parametry określają które piny mają być ustawione w push-pull
//anode HIGH, cathode LOW
#define SEGMENT_DEF(anode,cathode) { LINE_##anode | LINE_##cathode, LINE_##anode }
//dziesiątki
#define SEG_1 SEGMENT_DEF(B,D)	//D
#define SEG_2 SEGMENT_DEF(B,E)	//E
#define SEG_3 SEGMENT_DEF(C,E)	//F
#define SEG_4 SEGMENT_DEF(C,B)	//A
#define SEG_5 SEGMENT_DEF(B,C)	//B
#define SEG_6 SEGMENT_DEF(C,D)	//C
#define SEG_7 SEGMENT_DEF(D,E)	//G
//jednostki
#define SEG_8 SEGMENT_DEF(A,C)	//D
#define SEG_9 SEGMENT_DEF(D,A)	//E
#define SEG_10 SEGMENT_DEF(A,D)	//F
#define SEG_11 SEGMENT_DEF(B,A)	//A
#define SEG_12 SEGMENT_DEF(A,B)	//B
#define SEG_13 SEGMENT_DEF(C,A)	//C
#define SEG_14 SEGMENT_DEF(A,E)	//F
//znak %
#define SEG_15 SEGMENT_DEF(D,B)
//znak błyskawicy
#define SEG_16 SEGMENT_DEF(D,C)
//wypełnienie kropli od góry
#define SEG_17 SEGMENT_DEF(E,A)	//zielony
#define SEG_18 SEGMENT_DEF(E,B)	//zielony
#define SEG_19 SEGMENT_DEF(E,C)	//niebieski
#define SEG_20 SEGMENT_DEF(E,D)	//czerwony
//kropla od godziny 4 do godziny 1 zgodnie z ruchem wskazówek zegara
#define SEG_21 SEGMENT_DEF(F,A)
#define SEG_22 SEGMENT_DEF(F,B)
#define SEG_23 SEGMENT_DEF(F,C)
#define SEG_24 SEGMENT_DEF(F,D)
#define SEG_25 SEGMENT_DEF(F,E)

#define SEG_MAX 25

//największa liczba dziesiętna jaką wyświetlacz moze wyświetlić
#define MAX_NUMBER 99

/*
 * typ opisu segmentu - piny aktywne oraz położenie anody
 */
typedef struct segment_tag {
	uint8_t active_mask;	//wskazuje które linie są aktywne dla danego segmentu, pozostałe -> HiZ
	uint8_t anode_mask;		//wskazuje które z aktywnych linii mają być HIGH dla zapalenia segmentu, pozostałe -> LOW
} segment_type;

/*
 * Pamięć ekranu
 * Obszary bufora są przypisane na stałe dla danego obiektu wyświetlacza (jedności, dzisiątki,
 * kropla, wypełnienie kropli, błyskawica, procent) ale nie muszą być w określonej kolejności
  */

#define DIGIT_SEGS_NO	7
#define FLAG_SEGS_NO	1
#define FILL_SEGS_NO	4
#define DROP_SEGS_NO	5

static union {
	struct areas{ //
		segment_type tens[ DIGIT_SEGS_NO ];
		segment_type units[ DIGIT_SEGS_NO ];
		segment_type power[ FLAG_SEGS_NO ];
		segment_type percent[ FLAG_SEGS_NO ];
		segment_type fill[ FILL_SEGS_NO ];
		segment_type droplet[ DROP_SEGS_NO ];
	} areas;
	segment_type buffer[sizeof(struct areas)/sizeof(segment_type)];
//	uint8_t buffer[sizeof(struct areas)];
} display;

/*
 * Opisy obiektów
 */
enum units_entities_tag {
	UNITS_0,
	UNITS_1,
	UNITS_2,
	UNITS_3,
	UNITS_4,
	UNITS_5,
	UNITS_6,
	UNITS_7,
	UNITS_8,
	UNITS_9,
	UNITS_BLANK,
	UNITS_DASH,
	UNITS_MAX,
};
__flash static segment_type const units_entities[][ARRAY_SIZE(display.areas.units)] ={

		[UNITS_0] = {SEG_8, SEG_9, SEG_10, SEG_11, SEG_12, SEG_13},
		[UNITS_1] = {SEG_12, SEG_13},
		[UNITS_2] = {SEG_8, SEG_9, SEG_11, SEG_12, SEG_14},
		[UNITS_3] = {SEG_8, SEG_11, SEG_12, SEG_13, SEG_14},
		[UNITS_4] = {SEG_10, SEG_12, SEG_13, SEG_14},
		[UNITS_5] = {SEG_8, SEG_10, SEG_11, SEG_13, SEG_14},
		[UNITS_6] = {SEG_8, SEG_9, SEG_10, SEG_11, SEG_13, SEG_14},
		[UNITS_7] = {SEG_11, SEG_12, SEG_13},
		[UNITS_8] = {SEG_8, SEG_9, SEG_10, SEG_11, SEG_12, SEG_13, SEG_14},
		[UNITS_9] = {SEG_8, SEG_10, SEG_11, SEG_12, SEG_13, SEG_14},
		[UNITS_BLANK] = {},
		[UNITS_DASH] = {SEG_14},

};
_Static_assert(ARRAY_SIZE(units_entities)==UNITS_MAX, "nieprawidowa tablica units");

enum tens_entities_tag {
	TENS_0,
	TENS_1,
	TENS_2,
	TENS_3,
	TENS_4,
	TENS_5,
	TENS_6,
	TENS_7,
	TENS_8,
	TENS_9,
	TENS_ERROR,
	TENS_DASH,
	TENS_MAX,
	TENS_BLANK = TENS_0, //zera wiodącego nie wyświetlamy
};
__flash static segment_type const tens_entities[][ARRAY_SIZE(display.areas.tens)] ={

//		[TENS_0] = {SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6},
		[TENS_0] = {},
		[TENS_1] = {SEG_5, SEG_6},
		[TENS_2] = {SEG_1, SEG_2, SEG_4, SEG_5, SEG_7},
		[TENS_3] = {SEG_1, SEG_4, SEG_5, SEG_6, SEG_7},
		[TENS_4] = {SEG_3, SEG_5, SEG_6, SEG_7},
		[TENS_5] = {SEG_1, SEG_3, SEG_4, SEG_6, SEG_7},
		[TENS_6] = {SEG_1, SEG_2, SEG_3, SEG_4, SEG_6, SEG_7},
		[TENS_7] = {SEG_4, SEG_5, SEG_6},
		[TENS_8] = {SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7},
		[TENS_9] = {SEG_1, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7},
		[TENS_ERROR] = {SEG_1, SEG_2, SEG_3, SEG_4, SEG_7},
		[TENS_DASH] = {SEG_7},

};
_Static_assert(ARRAY_SIZE(tens_entities)==TENS_MAX, "nieprawidowa tablica tens");

enum power_entities_tag {
	POWER_BLANK,
	POWER,
	POWER_MAX,
};
__flash static segment_type const power_entities[][ARRAY_SIZE(display.areas.power)] = {
		[POWER] = {SEG_16},
		[POWER_BLANK] = {},
};
_Static_assert(ARRAY_SIZE(power_entities)==POWER_MAX, "nieprawidowa tablica power");

enum percent_entities_tag {
	PERCENT_BLANK,
	PERCENT,
	PERCENT_MAX,
};
__flash static segment_type const percent_entities[][ARRAY_SIZE(display.areas.percent)] = {
		[PERCENT] = {SEG_15},
		[PERCENT_BLANK] = {},
};
_Static_assert(ARRAY_SIZE(percent_entities)==PERCENT_MAX, "nieprawidowa tablica percent");

enum fill_entities_tag {
		FILL_ENT_1,
		FILL_ENT_2,
		FILL_ENT_3,
		FILL_ENT_4,
		FILL_ENT_ALL,
		FILL_ENT_BLANK,
		FILL_ENT_MAX,
};
__flash static segment_type const fill_entities[][ARRAY_SIZE(display.areas.fill)] = {
		[FILL_ENT_1] = {SEG_20},
		[FILL_ENT_2] = {SEG_19},
		[FILL_ENT_3] = {SEG_18},
		[FILL_ENT_4] = {SEG_17},
		[FILL_ENT_ALL] = {SEG_17, SEG_18, SEG_19, SEG_20},
		[FILL_ENT_BLANK] = {},
};
_Static_assert(ARRAY_SIZE(fill_entities)==FILL_ENT_MAX, "nieprawidowa tablica fill");

enum drop_entities_tag {
	DROP_ENT_1,
	DROP_ENT_2,
	DROP_ENT_3,
	DROP_ENT_4,
	DROP_ENT_5,
	DROP_ENT_ALL,
	DROP_ENT_BLANK,
	DROP_ENT_MAX
};
__flash static segment_type const drop_entities[][ARRAY_SIZE(display.areas.droplet)] = {
		[DROP_ENT_1] = {SEG_24},
		[DROP_ENT_2] = {SEG_25},
		[DROP_ENT_3] = {SEG_21},
		[DROP_ENT_4] = {SEG_22},
		[DROP_ENT_5] = {SEG_23},
		[DROP_ENT_ALL] = {SEG_24, SEG_25, SEG_21, SEG_22, SEG_23},
		[DROP_ENT_BLANK] = {},

};
_Static_assert(ARRAY_SIZE(drop_entities)==DROP_ENT_MAX, "nieprawidowa tablica drop");


/*
 * Funkcje
 */
void display_clear(void)
{
	memset(&display, 0, sizeof(display));
}

/*
 * dla liczb większych niż maksymalna wyświetla --
 * dla liczb z ustawionym najstarszym bitem wyświetla E i numer błędu
 */
void display_number(uint8_t val)
{
	if(val&0x80) {
		memcpy_P( display.areas.tens, &tens_entities[TENS_ERROR], sizeof(display.areas.tens));
		memcpy_P( display.areas.units, &units_entities[(val & 0x0f) % 11], sizeof(display.areas.units));
		return;
	}
	if(val>MAX_NUMBER) {
		memcpy_P( display.areas.tens, &tens_entities[TENS_DASH], sizeof(display.areas.tens));
		memcpy_P( display.areas.units, &units_entities[UNITS_DASH], sizeof(display.areas.units));
		return;
	}
	uint8_t tval = val / 10;
	memcpy_P( display.areas.tens, &tens_entities[tval], sizeof(display.areas.tens));
	tval = val % 10;
	memcpy_P( display.areas.units, &units_entities[tval], sizeof(display.areas.units));
}

void display_number_clear(void)
{
	memset( display.areas.units, 0, sizeof(display.areas.units));
	memset( display.areas.tens, 0, sizeof(display.areas.tens));
}

void display_power(bool show)
{
	memcpy_P( display.areas.power, power_entities[show], sizeof(display.areas.power));
}

void display_percent(bool show)
{
	memcpy_P( display.areas.percent, percent_entities[show], sizeof(display.areas.percent));
}

void display_droplet(uint8_t level)
{
	level %= DROP_ENT_MAX;
	memcpy_P( display.areas.droplet, drop_entities[level], sizeof(display.areas.droplet));
}

void display_filling(uint8_t level)
{
	level %= FILL_ENT_MAX;
	memcpy_P( display.areas.fill, fill_entities[level], sizeof(display.areas.fill));
}

//włączenie cyfr
ISR(TIMER0_OVF_vect)
{
	static uint8_t counter;
	uint8_t tmp;

	TEST_PIN_0_HIGH

	tmp = O_DIR & DISPLAY_LINES_NEG_MASK;
	O_DIR = display.buffer[counter].active_mask | tmp;
	tmp = O_PORT & DISPLAY_LINES_NEG_MASK;
	O_PORT = display.buffer[counter].anode_mask | tmp;
	if(++counter>=SEG_MAX) {
		counter = 0;
	}

	TEST_PIN_0_LOW
}

//sterowanie jasnością - wyłączenie
ISR(TIMER0_COMPB_vect, ISR_NAKED)
{
	TEST_PIN_1_HIGH

	O_PORT &= ~LINE_A;
	O_PORT &= ~LINE_B;
	O_PORT &= ~LINE_C;
	O_PORT &= ~LINE_D;
	O_PORT &= ~LINE_E;
	O_PORT &= ~LINE_F;

	TEST_PIN_1_LOW

	asm volatile( "reti" );
}


//50Hz x 25 segmentów = 1250 Hz
//16MHz/1250Hz -> preskaler 64 OCRA 200
//8MHz/1250Hz -> preskaler 64 OCRA 100
//
//70Hz x 25 segmentów = 1750 Hz
//16MHz/1750Hz -> preskaler 64 OCRA 142
//8MHz/1750Hz -> preskaler 64 OCRA 71
#define PRESKALER_MASK (_BV(CS01) | _BV(CS00))
#define TIMER_MAX 72

void display_init(void)
{
	TEST_PIN_0_INIT
	TEST_PIN_1_INIT

	//timer0 tryb 7 fast pwm - potrzebujemy podwójnego buforowania OCR0y oraz ustawienia MAX timera
	TCCR0A = _BV(WGM00) | _BV(WGM01);
	TCCR0B = _BV(WGM02);
	//maksymalna wartość licznika
	OCR0A = TIMER_MAX;
	//początkowo jasność wyświetlania ==max
	OCR0B = TIMER_MAX;
	TIMSK0 = _BV(OCIE0B) | _BV(TOIE0); //przerwanie compare match i przepełnienie
	//lec goł
	TCCR0B |= PRESKALER_MASK;
}

void display_driver_on(void)
{
	//wyczyszczenie ewentualnie wiszących przerwań
	TIFR0 = _BV(OCF0B) | _BV(TOV0);
	//włączenie przerwań
//	TIMSK0 = _BV(OCIE0B) | _BV(TOIE0);
	//ustawienie preskalera
	TCCR0B |= PRESKALER_MASK;
}

void display_driver_off(void)
{
	//zatrzymanie timera i wyłączenie przerwań
	TCCR0B &= ~PRESKALER_MASK;
//	TIMSK0 &= ~(_BV(OCIE0A) | _BV(TOIE0));

	//wyłączenie wyświetlania
	O_PORT &= ~LINE_A;
	O_PORT &= ~LINE_B;
	O_PORT &= ~LINE_C;
	O_PORT &= ~LINE_D;
	O_PORT &= ~LINE_E;
	O_PORT &= ~LINE_F;
	O_DIR &= ~LINE_A;
	O_DIR &= ~LINE_B;
	O_DIR &= ~LINE_C;
	O_DIR &= ~LINE_D;
	O_DIR &= ~LINE_E;
	O_DIR &= ~LINE_F;
}

void display_brigthness(uint8_t brightness)
{
	if(brightness > 100) {
		OCR0B = TIMER_MAX;
	} else {
		OCR0B = brightness * TIMER_MAX / 100;
	}
}
