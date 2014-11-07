#include "put_bytes.h"
#include <inttypes.h>
#include <cpu.h>

struct char_format format;

/*
  Params :
            lig : (between 0-25) value of the line
            col : (between 0-80) value of the column
  returns : a pointer to the memory address corresponding to the coordinates 
            given as parameter.
*/
uint16_t *ptr_mem(uint32_t lig, uint32_t col)
{
	return (uint16_t*)0xB8000 + (lig * 80 + col);
	// arithmétique des pointeurs fait qu'on avance de 2 octets	
}

/*
  warranty : Writes the char c at the given coordinates, with the given colors.
*/
void ecrit_car(uint32_t lig, uint32_t col, char c, struct char_format *format)
{
	// The first Byte contains the ASCII code of the char
	// The second Byte contains the format of the char.
	*ptr_mem(lig, col) = (format->blink << 15) | (format->bg_color << 12) | (format->fg_color << 8) | (c);
}

/*
  Warranty : moves the cursor to the given position
 */
void place_curseur(uint32_t lig, uint32_t col)
{
	uint16_t pos = lig * 80 + col;
	// Sends the Low part of the cursor position : 
	outb(0x0F, PORT_CURSOR_CMD);
	outb((uint8_t)pos, PORT_CURSOR_DATA);
	// Sends the High part of the cursor position : 
	outb(0x0E, PORT_CURSOR_CMD);
	outb((uint8_t)(pos >> 8), PORT_CURSOR_DATA);
}

/*
  Warranty: Set the variables given as parameters with the current position
            of the cursor.
*/
void get_curseur(uint32_t* lig, uint32_t* col)
{
	// Get the High part of the cursor position : 
	outb(0x0E, PORT_CURSOR_CMD);
	uint16_t pos = inb(PORT_CURSOR_DATA) << 8;
	// Get the Low part of the cursor position : 
	outb(0x0F, PORT_CURSOR_CMD);
        pos |= inb (PORT_CURSOR_DATA);
	// Works out the coordinates :
	*lig = pos / SCREEN_NB_COLS;
	*col = pos % SCREEN_NB_COLS;
}

/*
  Warranty : Clears the screen
*/
void efface_ecran(void)
{
	// We set all Bytes to 0 (char 0 + black + not blinking)
	memset((uint16_t*)0xB8000, 0, SCREEN_SIZE);
}

/*
  Warranty : Set the pointeur given as parameter with the format global variable
*/
void get_text_format(struct char_format * f)
{
	*f= format;
	// TODO : should I make a copy of the format ?
}

/*
  Params :
          blink: boolean value to say wether the text should blink or not.
	  bg_color: background color.
	  fg_color: foreground color.
  Warranty: Sets the value of the global variable "format" with the values given
            as parameters.
 */
void set_text_format(bool blink, uint8_t bg_color, uint8_t fg_color)
{
	format.blink = blink;
	format.bg_color = bg_color;
	format.fg_color = fg_color;
}

/*
  Warranty : executes the correct action given the char in parameter
             if the char is printable : it is printed - at the cursor position -, 
	     else the attended effect is executed
 */
void traite_car(char c)
{
	// At firtst, we get the cursor position
	uint32_t lig, col;
	get_curseur(&lig, &col);
	uint16_t* addr = ptr_mem(lig, col);

	// printable chars :
	if(c >= 32 && c <= 126){
	      	// We print the character at the cursor position
		ecrit_car(lig, col, c, &format);
		// The we update the cursor position
		if(++col >= 80){
			col = 0;
			if(++lig >= 25){
				--lig;
				defilement();
			}
		}
 	// control chars :
	} else {
		switch(c){
		case '\b': // moves the cursor backward if it's not in first column
			if(col != 0)
				--col;
			break;
		case '\t': // jumps to the next tab
			if(col != 80){
				while((++col % 8) != 0);
				memset(addr, 0, (ptr_mem(lig, col) - addr)*2);
			}
			break;
		case '\n': // Move the cursor to the next line, first col.
			col = 0;
			lig = (lig<79) ? lig + 1 : 0;
			memset(addr, 0, (ptr_mem(lig, col) - addr)*2);
			if(lig >= SCREEN_NB_LINES){
				--lig;
				defilement();
			}
			break;
		case '\f': // Clear the screen + put the cursor in first line/col
			efface_ecran();
			lig = col = 0;
			break;
		case '\r': // Move the cursor at the beginning of the current line.
			col = 0;
			break;
		}
	}	
	place_curseur(lig, col);	
}

/*
  Warranty : moves all the content of the screen of one line upward.
*/
void defilement(void)
{
	memmove(ptr_mem(0, 0), ptr_mem(1, 0), SCREEN_SIZE);
}

/*
  Warranty: Displays the string given as parameter 
 */
void console_putbytes(char *chaine, int32_t taille)
{
	int32_t i = 0;
	while(chaine[i] && i < taille){
		traite_car(chaine[i++]);
	}
}

// QUESTION, do we store the cursor position ? Or do we "ask" it (with inb) ?
// |--> Answer : It's better to ask it.

// QUESTION, when doing a defilement, should we clear the new line or not ?
