#ifndef SPI_FUNCTIONS
#define SPI_FUNCTIONS

unsigned char rw_spi_byte(unsigned char a);
void SPI_Device_Select(unsigned char dev);
void LCD_w(unsigned char a);
void LCD_w_string(char* str);
void LCD_w_stringROM(const rom char* str);
void LCD_snd_cmd(unsigned char cmd);
void UART_w(unsigned char a);
void UART_w_string(ram char* str);
void UART_w_stringROM(const rom char* str);
void config_SPI_devs(void);
char GPS_r(void);
char UART_r(void);
#endif