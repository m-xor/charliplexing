/*
 * charlie.h
 *
 *  Created on: 26 maj 2025
 *      Author: slawek
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <avr/io.h>
#include <stdbool.h>

/*
 * Wyjście na wyświetlacz na jednym porcie
 */
#define O_PORT	PORTD
#define O_DIR	DDRD
/*
 * Przypisanie pinów mikrokontrolera do pinów wyświetlacza (6 wyprowadzeń)
 * Oznakowanie pinów wyświetlacza: A-F od dołu do góry, przy właściwej orientacji kropli
 */
#define A_PIN 0
#define B_PIN 1
#define C_PIN 2
#define D_PIN 3
#define E_PIN 4
#define F_PIN 5

//piny inne niż powyżej
#define TEST_PIN_0 6
#define TEST_PIN_1 7

//inicjuje hardware procesora tj. timer 0 i odpowiednie przerwania
extern void display_init(void);
//zatrzymuje timer wyświetlacza i wygasza segmenty, nie modyfikuje bufora
//funkcja wywoływana przed głębokim uśpieniem urządzenia
extern void display_driver_off(void);
//wznawia pracę timera
//funkcja wywoływana po obudzeniu urządzenia z głebokiego uśpienia
extern void display_driver_on(void);
//ustawia poziom jasności w zakresie 0-100
extern void display_brigthness(uint8_t brightness);

//wygaszenie wskaźników na wyświetlaczu
extern void display_clear(void);
//zakres wyświetlanych liczb 0-99
//dla zakresu 100-127 wyświetla --
//dla zakresu 128-137 wyświetla E plus numer błędu od 0-9
//dla 138 wyświetla E
enum {
	NUMBER_DASH = 127,
	NUMBER_E0,
	NUMBER_E1,
	NUMBER_E2,
	NUMBER_E3,
	NUMBER_E4,
	NUMBER_E5,
	NUMBER_E6,
	NUMBER_E7,
	NUMBER_E8,
	NUMBER_E9,
	NUMBER_E,
};
extern void display_number(uint8_t val);
//wygaszenie wskaźnika numerycznego
extern void display_number_clear(void);
//wyświetla/wygasza symbol błyskawicy
extern void display_power(bool show);
//wyświetla/wygasza symbol %
extern void display_percent(bool show);
//wyświetla/wygasza elementy symbolu kropli
enum {
	//części obrysu kropli
	DROP_N,
	DROP_NE,
	DROP_SE,
	DROP_SW,
	DROP_NW,
	//ilość segmentów obrysu kropli
	DROP_SEGS_MAX,
	//włączenie wszystkich, wyłączenie wszystkich
	DROP_ALL = DROP_SEGS_MAX,
	DROP_BLANK,
};
extern void display_droplet(uint8_t level);
//wyświetla/wygasza elementy wypełnienia kropli
enum {
	FILL_LEVEL_1,
	FILL_LEVEL_2,
	FILL_LEVEL_3,
	FILL_LEVEL_4,
	//symbole alternatywne dla powyższych
	FILL_RED = FILL_LEVEL_1,
	FILL_BLUE,
	FILL_GREEN_1,
	FILL_GREEN_2,
	//ilość poziomów
	FILL_LEVEL_MAX,
	//włączenie wszystkich, i całkowite wyłączenie
	FILL_ALL = FILL_LEVEL_MAX,
	FILL_BLANK,
};
extern void display_filling(uint8_t level);


#endif /* DISPLAY_H_ */
