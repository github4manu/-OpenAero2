//***********************************************************
//* menu_autolevel.c
//***********************************************************

//***********************************************************
//* Includes
//***********************************************************

#include <avr/pgmspace.h> 
#include <avr/io.h>
#include <stdlib.h>
#include <stdbool.h>
#include <util/delay.h>
#include "..\inc\io_cfg.h"
#include "..\inc\init.h"
#include "..\inc\mugui.h"
#include "..\inc\glcd_menu.h"
#include "..\inc\menu_ext.h"
#include "..\inc\glcd_driver.h"
#include "..\inc\main.h"
#include "..\inc\eeprom.h"

//************************************************************
// Prototypes
//************************************************************

// Menu items
void menu_al_control(void);

//************************************************************
// Defines
//************************************************************

#define AUTOITEMS 9 	// Number of menu items
#define AUTOSTART 50 	// Start of Menu text items
#define AUTOTEXT 61 	// Start of value text items

//************************************************************
// AUTO menu items
//************************************************************

const uint8_t AutoMenuOffsets[AUTOITEMS] PROGMEM = {75, 75, 75, 75, 75, 75, 75, 75, 75};
const uint8_t AutoMenuText[AUTOITEMS] PROGMEM = {AUTOTEXT, 0, 0, 0, 0, 0, 0, 0, 0};
const menu_range_t auto_menu_ranges[] PROGMEM = 
{
	{DISABLED,ALWAYSON,1,1,AUTOCHAN}, 	// Min, Max, Increment, Style, Default
	{0,250,1,0,60},
	{0,250,1,0,0},
	{0,250,1,0,0},
	{0,250,1,0,100},
	{0,250,1,0,0}, 
	{0,250,1,0,0},
	{-127,127,1,0,0}, 
	{-127,127,1,0,0}
};

//************************************************************
// Main menu-specific setup
//************************************************************

void menu_al_control(void)
{
	uint8_t cursor = LINE0;
	uint8_t button = 0;
	uint8_t top = AUTOSTART;
	uint8_t temp = 0;
	int16_t values[AUTOITEMS];
	menu_range_t range;
	uint8_t text_link = 0;
	
	while(button != BACK)
	{
		// Clear buffer before each update
		clear_buffer(buffer);	

		// Load values from eeprom
		values[0] = Config.AutoMode;		// DISABLED = 0, AUTOCHAN, STABCHAN, THREEPOS, ALWAYSON
		values[1] = Config.G_level.P_mult;
		values[2] = Config.G_level.I_mult;
		values[3] = Config.G_level.D_mult;
		values[4] = Config.A_level.P_mult;
		values[5] = Config.A_level.I_mult;
		values[6] = Config.A_level.D_mult;
		values[7] = Config.AccRollZeroTrim - 127;
		values[8] = Config.AccPitchZeroTrim - 127;

		// Print menu
		print_menu_items(top, AUTOSTART, &values[0], (prog_uchar*)auto_menu_ranges, (prog_uchar*)AutoMenuOffsets, (prog_uchar*)AutoMenuText, cursor);

		// Poll buttons when idle
		button = poll_buttons();
		while (button == NONE)					
		{
			button = poll_buttons();
		}

		// Handle menu changes
		update_menu(AUTOITEMS, AUTOSTART, button, &cursor, &top, &temp);
		range = get_menu_range ((prog_uchar*)auto_menu_ranges, temp - AUTOSTART);

		if (button == ENTER)
		{
			text_link = pgm_read_byte(&AutoMenuText[temp - AUTOSTART]);
			values[temp - AUTOSTART] = do_menu_item(temp, values[temp - AUTOSTART], range, 0, text_link);
		}

		// Update value in config structure
		Config.AutoMode = values[0];
		Config.G_level.P_mult = values[1];
		Config.G_level.I_mult = values[2];
		Config.G_level.D_mult = values[3];
		Config.A_level.P_mult = values[4];
		Config.A_level.I_mult = values[5];
		Config.A_level.D_mult = values[6];
		Config.AccRollZeroTrim = values[7] + 127;
		Config.AccPitchZeroTrim = values[8] + 127;

		if (button == ENTER)
		{
			Save_Config_to_EEPROM(); // Save value and return
		}
	}
	menu_beep(1);
	_delay_ms(200);
}
