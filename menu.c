#include "menu.h"
#include "spi_funcs.h"
#include "defs.h"
#include <delays.h>
#define MEN_ITEMS 9

extern void draw_time(void);
unsigned int cur_men_item=0;

rom struct m_item item_list[MEN_ITEMS] = {		//This is the star object list
								{"Arcturus",213.915416667,19.181555556,"0.15"},
								{"Pollux",116.32875,28.025888889,"1.15"},
								{"Regulus",152.092916667,11.967333333,"1.35"},
								{"Procyon",114.825416667,5.224805556,"0.4"},
								{"Betelgeuse",88.792916667,7.407055556,"0.45"},
								{"Fomalhaut",-29.622361111,344.412916667,"1.15"},
								{"Denebola",177.265,14.572027778,"2.10"},
								{"Spica",201.298333333,-11.161333333,"0.95"},
								{"Saturn",166.958333333,7.952777778,"1.07"}
								};


void m_display_item(rom struct m_item* mi)
{
	LCD_snd_cmd(LCD_CLEAR);
	LCD_w_stringROM(mi->descriptor);
	LCD_w(254);						//Resposistion Cursor
	Delay10KTCYx(1);
	LCD_w(192);
	LCD_w_stringROM("m=");
	LCD_w_stringROM(mi->magnitude);
	LCD_w(254);					//Resposistion Cursor
	Delay10KTCYx(1);
	LCD_w(141);
	if(cur_men_item != 0)
		LCD_w('<');
	else
		LCD_w(' ');
	Delay10KTCYx(1);
	LCD_w('-');
	Delay10KTCYx(1);
	if(cur_men_item != (MEN_ITEMS-1))
		LCD_w('>');
		
	draw_time();
}



void m_up(void)
{
	if(cur_men_item < MEN_ITEMS - 1)
		cur_men_item++;
	m_display_item(&item_list[cur_men_item]);

}

void m_down(void)
{
	if(cur_men_item)
		cur_men_item--;
	m_display_item(&item_list[cur_men_item]);

}

void m_initialize(void)
{
	cur_men_item = 0;
	m_display_item(&item_list[cur_men_item]);
}

rom struct m_item* m_get_cur_item(void)
{
	return &item_list[cur_men_item];
}