#ifndef M_DEFS
#define M_DEFS

#define PI				3.141592654

#define MAG_SET			LATBbits.LATB4
#define TRIS_MAG_SET	TRISBbits.TRISB4
#define TRIS_MAG_IN_A	TRISAbits.TRISA0
#define TRIS_MAG_IN_B	TRISAbits.TRISA1

#define SPI_SCK			LATBbits.LATB1
#define SPI_DIN			LATBbits.LATB0 
#define SPI_DOUT		LATCbits.LATC7
#define TRIS_SPI_SCK	TRISBbits.TRISB1
#define TRIS_SPI_DIN	TRISBbits.TRISB0
#define TRIS_SPI_DOUT	TRISCbits.TRISC7

#define SPI_CS_A		LATCbits.LATC1
#define SPI_CS_B		LATCbits.LATC2
#define TRIS_SPI_CS_A	TRISCbits.TRISC1
#define TRIS_SPI_CS_B	TRISCbits.TRISC2

#define TRIS_SERV_A		TRISAbits.TRISA6
#define TRIS_SERV_B		TRISCbits.TRISC0
#define SERV_A			LATAbits.LATA6
#define SERV_B			LATCbits.LATC0

#define SPI_GPS			0
#define SPI_UART		1
#define SPI_LCD			2
#define SPI_MEM			3
/*
const ROM unsigned char MAX3100_WCONFIG		=	0b11000000;
const ROM unsigned char MAX3100_WCONFIGR	=	0b11000100;

const ROM unsigned char MAX3100_RCONFIG	=	0b01000000;
const ROM unsigned char MAX3100_WRITE	=	0b10000000;
const ROM unsigned char MAX3100_READ	=	0;

const ROM unsigned char UART_9600		=	0b00001010;
const ROM unsigned char UART_4800		=	0b00001011;
*/

#define LCD_CLEAR		0x01
#define LCD_SCROLL_R	0x1C
#define LCD_SCROLL_L	0x18
#define LCD_CMDA		0xFE
#define LCD_CMDB		0x7C
#define LCD_TGLSC		0x09
#define LCD_CUR_ON		0x0D
#define LCD_CUR_OFF		0x0D
#define LCD_SET_POS		0x80

#define MAX3100_WCONFIG		0b11000000
#define MAX3100_WCONFIGR	0b11000100
#define MAX3100_RCONFIG		0b01000000
#define MAX3100_WRITE		0b10000000
#define MAX3100_READ		0

#define READ_READY			0x80
#define WRITE_READY			0x40

#define UART_115200			0b00000000
#define UART_57600			0b00000001
#define UART_38400			0b00001000
#define UART_9600			0b00001010
#define UART_4800			0b00001011

//const ROM char obj_lst[3][64] = {"Jupiter","Saturn","Neptune"};

#define LED_S_A			LATAbits.LATA4
#define LED_S_B			LATAbits.LATA5
#define TRIS_LED_S_A	TRISAbits.TRISA4
#define TRIS_LED_S_B	TRISAbits.TRISA5

#define LED_GPSA	0
#define LED_GPSB	1
#define	LED_GPSC	2
#define LED_ERR		3

#define TRIS_SW_A	TRISBbits.TRISB7
#define TRIS_SW_B	TRISBbits.TRISB6
#define TRIS_SW_C	TRISBbits.TRISB5

#define SW_A		PORTBbits.RB7
#define SW_B		PORTBbits.RB6
#define SW_C		PORTBbits.RB5

#define TRIS_GPS_IRQ	TRISBbits.TRISB3
#define GPS_IRQ			PORTBbits.RB3

#define TRIS_UART_IRQ	TRISBbits.TRISB2
#define UART_IRQ		PORTBbits.RB2

#define GPS_ST_RESET	0
#define GPS_ST_WAIT		1
#define GPS_ST_MSGID	2
#define GPS_ST_TIME		3
#define GPS_ST_STATUS	4
#define GPS_ST_LAT		5
#define GPS_ST_NS		6
#define GPS_ST_LON		7
#define GPS_ST_EW		8
#define GPS_ST_SPD		9
#define GPS_ST_CRS		10
#define GPS_ST_DATE		11
#define GPS_ST_MVAR		12
#define GPS_ST_CKSUM	13
#define GPS_ST_VALIDATE	14


union byte_access{
		unsigned int val;
		struct {
			unsigned char val_L;
			unsigned char val_H;
		}v;
};

struct servo {

	union byte_access DS;
	union byte_access PER;
	union byte_access center;
	union byte_access min;
	union byte_access max;
	
	//double min_angle;
	//double max_angle;
	
	double steps_per_degree;

};

struct swtch {
	char enabled;
	char state;
	char clicked;
};

struct action {
	char enabled;
	char working;
};

struct time {
	unsigned char year;
	unsigned char month;
	unsigned char day;

	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	unsigned int  msecond;
};
struct coord {
	unsigned int degree;
	unsigned char minute;
	unsigned int fminute;
	char dir;

};
struct gps_state_machine {

	char buffer[12];
	unsigned char data_ck;
	struct coord lat;
	struct coord lon;
	struct time tm;
	int state;
	char valid;
};

#endif