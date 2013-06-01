#include <p18f2550.h>
#include <delays.h>
#include "defs.h"
#include "spi_funcs.h"
unsigned char SPI_CUR_SEL	=	SPI_GPS;



unsigned char rw_spi_byte(unsigned char a)
{
	
	unsigned char ret;
	//INTCONbits.GIEH = 0;
	//INTCONbits.GIEL = 0;
	SSPCON1bits.WCOL = 0;
	ret = SSPBUF;
	do {
		SSPCON1bits.WCOL = 0;
		SSPBUF = a;
	}while(SSPCON1bits.WCOL);
	while(!SSPSTATbits.BF);
	ret = SSPBUF;
	//INTCONbits.GIEH = 1;
	//INTCONbits.GIEL = 1;
	return ret;
}

void LCD_w(unsigned char a)
{

	unsigned char cs = SPI_CUR_SEL;
	unsigned char ret=1;
	if(cs == SPI_LCD) {
		SPI_Device_Select(SPI_MEM);
		Delay1KTCYx(1);
	}
	while(ret) {
		SPI_Device_Select(SPI_LCD);
		Delay1KTCYx(1);
		ret = rw_spi_byte(MAX3100_WRITE);
		if(ret & 64) {
			rw_spi_byte(a);
			ret = 0;
		} else
			ret = 1;
		if(cs != SPI_LCD)
			SPI_Device_Select(cs);
		else
			SPI_Device_Select(SPI_GPS);
	}
	
}
void LCD_w_string(ram char* str)
{
	while(*str) {
		Delay10KTCYx(1);
		LCD_w(*str);
		str++;
	}
	
}

void LCD_w_stringROM(const rom char* str)
{
	char a;
	while(*str) {
		Delay10KTCYx(1);
		a = *str;
		LCD_w(a);
		str++;
	}
	
}

void LCD_snd_cmd(unsigned char cmd)
{
	if(cmd == LCD_TGLSC)
		LCD_w(LCD_CMDB);
	else
		LCD_w(LCD_CMDA);
	LCD_w(cmd);
	

}
void UART_w(unsigned char a)
{

	unsigned char cs = SPI_CUR_SEL;
	unsigned char ret=1;
	if(cs == SPI_UART) {
		SPI_Device_Select(SPI_MEM);
		Delay1KTCYx(1);
	}
	while(ret) {
		SPI_Device_Select(SPI_UART);
		ret = rw_spi_byte(MAX3100_WRITE);
		if(ret & 64) {
			rw_spi_byte(a);
			ret = 0;
		} else
			ret = 1;
		if(cs != SPI_UART)
			SPI_Device_Select(cs);
		else
			SPI_Device_Select(SPI_MEM);
	}
	
}

char UART_r(void)
{
	unsigned char cs = SPI_CUR_SEL;
	unsigned char ret;
	if(cs == SPI_UART) {
		SPI_Device_Select(SPI_MEM);
		Delay1KTCYx(1);
	}
	
		SPI_Device_Select(SPI_UART);
		ret = rw_spi_byte(MAX3100_READ);
		if(ret & 128) {
			ret = rw_spi_byte(0);
		
		} else
			ret = 0;
		if(cs != SPI_UART)
			SPI_Device_Select(cs);
		else
			SPI_Device_Select(SPI_MEM);
	
	return ret;

}

void UART_w_string(ram char* str)
{
	while(*str) {
		Delay10KTCYx(1);
		UART_w(*str);
		str++;
	}
	
}
void UART_w_stringROM(const rom char* str)
{
	char a;
	while(*str) {
		Delay10KTCYx(1);
		a = *str;
		UART_w(a);
		str++;
	}
	
}

char GPS_r(void)
{

	unsigned char cs	= SPI_CUR_SEL;
	unsigned char ret;
	char gval;
	if(cs == SPI_GPS) {
		SPI_Device_Select(SPI_MEM);
		Delay1KTCYx(1);
	}
	
	SPI_Device_Select(SPI_GPS);
	ret = rw_spi_byte(MAX3100_READ);
	if(ret & READ_READY) {
		gval = rw_spi_byte(0);
	} else
		gval = 0;
	if(cs != SPI_GPS)
		SPI_Device_Select(cs);
	else
		SPI_Device_Select(SPI_MEM);
			
	return gval;
	
}

void SPI_Device_Select(unsigned char dev)
{
	switch(dev) {
		case SPI_GPS:
			SPI_CS_A = SPI_CS_B = 0;
			break;
		case SPI_UART:
			SPI_CS_A = 1;
			SPI_CS_B = 0;
			break;
		case SPI_LCD:
			SPI_CS_A = 0;
			SPI_CS_B = 1;
			break;
		case SPI_MEM:
			SPI_CS_A = SPI_CS_B = 1;
			break;
		default:
			Nop();
			break;
	}
	SPI_CUR_SEL = dev;

}

void config_SPI_devs(void)
{
	char a = 0;
	if(SPI_CUR_SEL == 0) {
		SPI_Device_Select(1);
		Delay1KTCYx(10);
	}
	for(a=0;a<4;a++) {
		
		SPI_Device_Select(a);
		Delay1KTCYx(10);
		switch(a) {
			case SPI_GPS:
			rw_spi_byte(MAX3100_WCONFIGR);
			rw_spi_byte(UART_4800);
			break;
		case SPI_UART:
			rw_spi_byte(MAX3100_WCONFIG);
			rw_spi_byte(UART_115200); 
			break;
		case SPI_LCD:
			rw_spi_byte(MAX3100_WCONFIG);
			rw_spi_byte(UART_9600);
			break;
		case SPI_MEM:
			rw_spi_byte(MAX3100_WCONFIG);
			rw_spi_byte(UART_9600); //Change Speed Later
			break;
		default:
			Nop();
			break;
		}
	}
	SPI_Device_Select(SPI_GPS); //Unselect last device to let config take place
}