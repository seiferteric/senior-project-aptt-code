#include "GenericTypeDefs.h"
#include "Compiler.h"
//#include "usb_config.h"
//#include "./USB/usb_device.h"
#include "./USB/usb.h"
#include "./USB/usb_function_cdc.h"
#include "HardwareProfile.h"
#include "USB\usb_device.h"
#include "USB\usb.h"
#include "defs.h"
#include <delays.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "spi_funcs.h"
#include "menu.h"
#pragma udata


//char USB_In_Buffer[64];
char USB_Out_Buffer[64];


volatile struct servo servA, servB;
union byte_access serv_cnt;
union byte_access magval;
unsigned int mag_max = 0;
unsigned int mag_min = 0xFFFF;
unsigned int serv_north = 1000;
unsigned int serv_south = 1000;
unsigned char CUR_LED = 0;
double tst_alt = 0.0;
double tst_az = 0.0;
char cmd_md = 0;
char update = 1;
char done = 0;
const rom char fmt[] = "%X\r\n";
ram struct m_item cmd_mode_item = {"CMD MODE", 0, 0, "0.0"};
struct {
	double latitude;
	double longitude;
	unsigned int heading;
	struct time tm;
	double magnetic_dec;
	rom struct m_item* object;
	ram struct m_item* cmd_object;
	
}position;

ram struct gps_state_machine gps_state;

static void InitializeSystem(void);
void ProcessIO(void);

//void USBDeviceTasks(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void UserInit(void);
void Process_GPS(void);
unsigned int read_compass(unsigned char chan);
void Set_LED(unsigned char LED);
void calibrate(void);
char set_alt_azimuth(double alt, double azimuth);
double sidereal_time(void);
void convert_eq_to_altaz(double ra, double dec, double* alt, double* az);
void set_position(void);
void check_test_mode(void);
void hw_test(char tst_num);
void draw_time(void);
void check_cmd_mode(void);
/** VECTOR REMAPPING ***********************************************/	
	#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x1000
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1008
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1018
	#elif defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)	
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x800
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x808
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x818
	#else	
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x00
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x08
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x18
	#endif
	
	#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)
	extern void _startup (void);        // See c018i.c in your C18 compiler dir
	#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS
	void _reset (void)
	{
	    _asm goto _startup _endasm
	}
	#endif
	#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS
	void Remapped_High_ISR (void)
	{
	     _asm goto YourHighPriorityISRCode _endasm
	}
	#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
	void Remapped_Low_ISR (void)
	{
	     _asm goto YourLowPriorityISRCode _endasm
	}
	
	#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)
	#pragma code HIGH_INTERRUPT_VECTOR = 0x08
	void High_ISR (void)
	{
	     _asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
	}
	#pragma code LOW_INTERRUPT_VECTOR = 0x18
	void Low_ISR (void)
	{
	     _asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
	}
	#endif	//end of "#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_LEGACY_CUSTOM_CLASS_BOOTLOADER)"

	#pragma code
	
	
	//These are your actual interrupt handling routines.
	#pragma interrupt YourHighPriorityISRCode
	void YourHighPriorityISRCode()
	{
		
		//Check which interrupt flag caused the interrupt.
		//Service the interrupt
		//Clear the interrupt flag
		//Etc.
		
		if(INTCONbits.TMR0IF)
		{
		
			if(SERV_A)
				serv_cnt.val = 0xFFFF - servA.PER.val + servA.DS.val;
			else
				serv_cnt.val = 0xFFFF - servA.DS.val;
			
			TMR0H = serv_cnt.v.val_H;
			TMR0L = serv_cnt.v.val_L;
			
			SERV_A ^= 1;
			
			//servA.state = SERV_A;
			
			INTCONbits.TMR0IF = 0;
		}
		if(PIR1bits.TMR1IF)
		{
		
			if(SERV_B)
				serv_cnt.val = 0xFFFF - servB.PER.val + servB.DS.val;
			else
				serv_cnt.val = 0xFFFF - servB.DS.val;
			
			TMR1H = serv_cnt.v.val_H;
			TMR1L = serv_cnt.v.val_L;
			
			SERV_B ^= 1;
			//servB.state = SERV_B;
			
			PIR1bits.TMR1IF = 0;
		}
		
        #if defined(USB_INTERRUPT)
	        USBDeviceTasks();
        #endif
	
	}	//This return will be a "retfie fast", since this is in a #pragma interrupt section 
	#pragma interruptlow YourLowPriorityISRCode
	void YourLowPriorityISRCode()
	{
		
		if(PIR2bits.TMR3IF) {
		
			position.tm.msecond += 100;
			if(position.tm.msecond >= 10000) {
				position.tm.msecond -= 10000;
				position.tm.second++;
			
				if(position.tm.second >= 60) {
					position.tm.second = 0;
					position.tm.minute++;
					
					if(position.tm.minute >= 60) {
						position.tm.minute = 0;
						position.tm.hour++;
				
						if(position.tm.hour >= 24) {
							position.tm.hour = 0;
							position.tm.day++;
						}
					}
				}
				update = 1;
			}
			
			TMR3H = 0xC5;
			TMR3L = 0x68;
			
			PIR2bits.TMR3IF = 0;
		} 
		
		
	}	//This return will be a "retfie", since this is in a #pragma interruptlow section 


    





/** DECLARATIONS ***************************************************/
#pragma code

void main(void)
{   
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	
    InitializeSystem();
	
	Set_LED(LED_GPSC);
    #if defined(USB_INTERRUPT)
       USBDeviceAttach();
    #endif
	
	LCD_snd_cmd(LCD_CLEAR);
	LCD_w_stringROM("Calibration in progress...");
	calibrate();
	LCD_snd_cmd(LCD_CLEAR);
	LCD_w_stringROM("Calibration complete, don't move");
	
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(176);
	
	
	
	LCD_snd_cmd(LCD_CLEAR);
	LCD_w_stringROM("WAITING FOR GPS LOCK");
	
	
    while(1)
    {
        #if defined(USB_POLLING)
        USBDeviceTasks(); 
        #endif
		CDCTxService();
		check_test_mode();
		check_cmd_mode();
		if(!gps_state.valid)
			Process_GPS();
		else if(!done) {
			done = 1;
			set_position();
			Set_LED(LED_GPSA);
			m_initialize();
			
		} else {
		
			if(update) {
				switch(cmd_md) {
					case 0:
						convert_eq_to_altaz(position.object->right_ascension, position.object->declination, &tst_alt, &tst_az);
						break;
					case 1:
						convert_eq_to_altaz(position.cmd_object->right_ascension, position.cmd_object->declination, &tst_alt, &tst_az);
						break;
					case 2:
						Nop(); //tst_az & tst_alt already set
						break;
					default:
						cmd_md = 0;
						break;
				}
				if(!set_alt_azimuth(tst_alt, tst_az))
					Set_LED(LED_ERR);
				else if(CUR_LED == LED_ERR)
					Set_LED(LED_GPSA);
					
				
				
				draw_time();
				
				update = 0;	
			}
			ProcessIO(); //Check buttons
			
			
		}
		
    }
}
void draw_time(void)
{
	LCD_w(254);	
	LCD_w(200);
	LCD_w(position.tm.hour/10 + '0');
	LCD_w(position.tm.hour - ((position.tm.hour/10)*10) + '0');
	LCD_w(':');
	LCD_w(position.tm.minute/10 + '0');
	LCD_w(position.tm.minute - ((position.tm.minute/10)*10) + '0');
	LCD_w(':');
	LCD_w(position.tm.second/10 + '0');
	LCD_w(position.tm.second - ((position.tm.second/10)*10) + '0');
}

void check_cmd_mode(void)
{
	unsigned char numBytesRead;
	static unsigned char state = 0;
	static char cmd = 0;
	static unsigned char cnt = 0;
	static char data_buf[10];
	if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1)) return;
	if(mUSBUSARTIsTxTrfReady())
    {
		numBytesRead = getsUSBUSART(USB_Out_Buffer,64);
		if(numBytesRead != 0)
		{
			unsigned char i;
	        
			for(i=0;i<numBytesRead;i++)
			{
				switch(state) {
					case 0:	//Wait for command
						cmd = USB_Out_Buffer[i];
						memset(data_buf, '\0', 10);
						cnt = 0;
						state = 1;
						break;
					case 1:
						switch(cmd) {
							case 'A':	//Set alt az
								if(USB_Out_Buffer[i] != ' ') {
									strncat(data_buf, &USB_Out_Buffer[i], 1);
								} else {
									cnt++;
									if(cnt == 1) {
										cmd_mode_item.right_ascension = atof(data_buf);
										
										memset(data_buf, '\0', 10);
									} else if(cnt == 2) {
										cmd_mode_item.declination = atof(data_buf);
										position.cmd_object = &cmd_mode_item;
										cmd_md = 1;
										update = 1;
										state = 0;
									}	
								}
								break;
							case 'B':
								if(USB_Out_Buffer[i] != ' ') {
									strncat(data_buf, &USB_Out_Buffer[i], 1);
								} else {
									cnt++;
									if(cnt == 1) {
										tst_alt = atof(data_buf);
										
										memset(data_buf, '\0', 10);
									} else if(cnt == 2) {
										tst_az = atof(data_buf);
										
										cmd_md = 2;
										update = 1;
										state = 0;
									}	
								}
								break;
							default:
								state = 0;
						}
						
						break;
					default:
						state = 0;
				}
			}
		}
	}


}

void check_test_mode(void)
{
	char a;
	char mtst[6] = "TEST";
	static char* ctst;
	static char state = 0;
	if(UART_IRQ == 0)
	{
		while(a = UART_r())
		{
			
			switch(state) {
				case 0:
					ctst = mtst;
					state = 1;
				case 1:
					if(a == *ctst) {
						ctst++;
						if(ctst == &mtst[4])
							state = 2;
						else
							break;
					} else {
						break;
						state = 0;
					}
				case 2:
					UART_w_stringROM("\r\n~~~A.P.P.T TEST MENU~~~\r\n");
					UART_w_stringROM("1.GPS Test\r\n2.LCD Test\r\n3.Servo Test\r\n4.Reset\r\nPlease Select a Test:");
					state = 3;
					break;
				case 3:
					a -= '0';
					hw_test(a);
					state = 0;
					break;
			
				default:
					state = 0;
					break;
			}
		
		}
	
	}
}

void hw_test(char tst_num)
{
	char a;
	UART_w(tst_num + '0');
	switch(tst_num)
	{
		case 1:
			UART_w_stringROM("\r\nGPS TEST\r\n");
			LCD_snd_cmd(LCD_CLEAR);
			LCD_w_stringROM("GPS TEST - RESET TO EXIT");
			while(1) {
				a = GPS_r();
				if(a)
					UART_w(a);
			}
			break;
		case 2:
			UART_w_stringROM("\r\nLCD TEST\r\nENTER TEXT TO DISPLAY ON LCD\r\nRESET TO EXIT\r\n");
			LCD_snd_cmd(LCD_CLEAR);
			while(1) {
				a = UART_r();
				if(a) {
					LCD_w(a);
					UART_w(a);
				}
			}
			break;
		case 3:
			UART_w_stringROM("\r\nSERVO TEST\r\n");
			LCD_snd_cmd(LCD_CLEAR);
			LCD_w_stringROM("SERVO TEST - RESET TO EXIT");
			while(1) {
				set_alt_azimuth(0.0, 0.0);
				set_alt_azimuth(0.0, 90.0);
				set_alt_azimuth(0.0,180.0);
				set_alt_azimuth(0.0, 270.0);
			}
			break;
		case 4:
				UART_w_stringROM("\r\nRESETTING!\r\n");
				_reset();
				break;
		default:
			UART_w_stringROM("\r\nInvalid Test Number!\r\n");
	}

}
void set_position(void)
{
	position.tm = gps_state.tm;
	position.longitude = gps_state.lon.degree + gps_state.lon.minute/60.0 + gps_state.lon.fminute/600000.0;
	if(gps_state.lon.dir == 'W')
		position.longitude = -position.longitude;
	position.latitude = gps_state.lat.degree + gps_state.lat.minute/60.0 + gps_state.lat.fminute/600000.0;
	if(gps_state.lat.dir == 'S')
		position.latitude = -position.latitude;

}


char set_alt_azimuth(double alt, double azimuth)
{
	unsigned int svB_goal;
	unsigned int svA_goal;
	double tmin;
	double theta;
	double minval = (double)servA.min.val;
	double northval = (double)serv_north;
	
	if(alt < 0.0 || alt > 90.0)
		return 0;
	
	azimuth += position.magnetic_dec;	//Correct for magnetic variation
	if(azimuth > 360.0)
		azimuth -= 360.0;
	if(azimuth < 0.0)
		azimuth += 360.0;
	tmin = (minval - northval)/servA.steps_per_degree;
	theta = azimuth - tmin;
	
	if(theta > 360.0)
		theta -= 360.0;
		
		
	if(theta < 180.0) {	//Region 1
		//servB.DS.val = servB.center.val - (int)((90.0 - alt)*servB.steps_per_degree);
		//servA.DS.val = servA.min.val + (int)(theta*servA.steps_per_degree);
		svB_goal = servB.center.val - (int)((90.0 - alt)*servB.steps_per_degree);	//Do this to prevent the servo from moving too fast!
		svA_goal = servA.min.val + (int)(theta*servA.steps_per_degree);
		
	} else				//Region 2
	{
		//servB.DS.val = servB.center.val + (90.0 - alt)*servB.steps_per_degree;
		//servA.DS.val = servA.min.val + (theta - 180.0)*servA.steps_per_degree;
		svB_goal = servB.center.val + (90.0 - alt)*servB.steps_per_degree;
		svA_goal = servA.min.val + (theta - 180.0)*servA.steps_per_degree;
	}
	
	while(servB.DS.val != svB_goal || servA.DS.val != svA_goal) {
		if(servB.DS.val < svB_goal)
			servB.DS.val++;
		if(servB.DS.val > svB_goal)
			servB.DS.val--;
		
		if(servA.DS.val < svA_goal)
			servA.DS.val++;
		if(servA.DS.val > svA_goal)
			servA.DS.val--;
		Delay1KTCYx(41);	//About five seonds total for 1/2 maximum rotation
	
	}
	
	return 1;
	
}



double sidereal_time(void)
{
	double y; //Year
	double m; //Month
	double day; //Day
	
	double h; //UTC Hour
	double mins; //UTC Min
	double seconds; // UTC Seconds

	double dwhole;
	double dfrac;

	double d;
	double LMST;
	
	y		= (double)position.tm.year + 2000.0;
	m		= (double)position.tm.month;
	day		= (double)position.tm.day;
	
	h		= (double)position.tm.hour;
	mins	= (double)position.tm.minute;
	seconds	= (double)position.tm.second;
	
	
	dwhole	= 367.0*y-(int)(7.0*(y+(int)((m+9.0)/12.0))/4.0)+(int)(275.0*m/9.0)+day-730531.5;
	dfrac	= (h + mins/60.0 + seconds/3600.0)/24.0;

	d = dwhole + dfrac;
	LMST = 280.46061837 + (360.98564736629 * d) + position.longitude;

	
	if(LMST > 360.0 || LMST < -360.0)
		LMST -= ((int)(LMST/360.0)) * 360.0;
	if(LMST < 0.0)
		LMST += 360.0;
	
	return LMST;

}

void convert_eq_to_altaz(double ra, double dec, double* alt, double* az)
{
	double lat;
	double hour_angle;
	double A;
	double a;
	
	dec *= PI/180.0;
	lat = position.latitude * PI/180.0;
	hour_angle = sidereal_time() - ra;
	hour_angle *= PI/180.0;
	
	A = atan2(-cos(dec)*sin(hour_angle),(cos(lat)*sin(dec)-sin(lat)*cos(dec)*cos(hour_angle)));
	a = asin(sin(lat)*sin(dec) + cos(lat)*cos(dec)*cos(hour_angle));

	a *= (180.0/PI);
	A *= (180.0/PI);

	if(A < 0.0)
		A += 360.0;
	*alt	= a;
	*az		= A;
}

static void InitializeSystem(void)
{
    
	ADCON1 |= 0x0F;                 // Default all pins to digital

    #if defined(USE_USB_BUS_SENSE_IO)
    tris_usb_bus_sense = INPUT_PIN; // See HardwareProfile.h
    #endif
    
    #if defined(USE_SELF_POWER_SENSE_IO)
    tris_self_power = INPUT_PIN;	// See HardwareProfile.h
    #endif
    
    UserInit();

    USBDeviceInit();	//usb_device.c.  Initializes USB module SFRs and firmware
    					//variables to known states.
}



void calibrate(void)
{
	
	unsigned int magav = 0;
	unsigned char cnt = 0;
	servA.DS = servA.min;
	Delay10KTCYx(0);			//Make sure the servo is where we want it
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(0);
	Delay10KTCYx(176);
	servA.PER.val = servB.PER.val = 30000;
	while(servA.DS.val < servA.max.val) {
		magav += read_compass(0);
		cnt++;
		if(cnt >= 63) {
			magav /= 63;
			if(magav > mag_max) {
				mag_max = magav;
				serv_south = servA.DS.val;
			}
			if(magav < mag_min) {
				mag_min = magav;
				serv_north = servA.DS.val;
			}
			servA.DS.val++;
			//Delay10KTCYx(1);
			magav	= 0;
			cnt		= 0;
		
		}
	}
	
	
}



void Process_GPS(void)
{
	
	char rmc[] = "GPRMC";
	static char* rmc_pnt = rmc;
	static unsigned char cur_sum = 0;
	static unsigned char fin_sum = 0;
	char gin;
	
	if(GPS_IRQ == 0) {				//Checks if GPS IRQ fired
		gin = GPS_r();
		if(gin) {
			
			cur_sum ^= gin;
			if(gin == ',' && gps_state.state > GPS_ST_WAIT)
				gps_state.state++;
			else
			switch(gps_state.state) {
				case GPS_ST_RESET:
					memset(&gps_state, 0, sizeof(struct gps_state_machine));
					rmc_pnt = rmc;
					cur_sum = fin_sum = 0;
					gps_state.state = GPS_ST_WAIT;
					gps_state.data_ck = 0;
					break;
				case GPS_ST_WAIT:
					if(gin == '$') {
						cur_sum = fin_sum = 0;
						gps_state.state = GPS_ST_MSGID;
					}
					break;
				case GPS_ST_MSGID:
					if(gin != *rmc_pnt) 
						gps_state.state = GPS_ST_RESET;
					rmc_pnt++;
					break;
				case GPS_ST_TIME:
					strncat(gps_state.buffer, &gin, 1);
					
					break;
				case GPS_ST_STATUS:
					if(gin != 'A') {
						gps_state.state = GPS_ST_RESET;
						
					} 
					gps_state.tm.hour = (gps_state.buffer[0]-'0') * 10;
					gps_state.tm.hour += (gps_state.buffer[1]-'0');
					gps_state.tm.minute = (gps_state.buffer[2]-'0') * 10;
					gps_state.tm.minute += (gps_state.buffer[3]-'0');
					gps_state.tm.second = (gps_state.buffer[4]-'0') * 10;
					gps_state.tm.second += (gps_state.buffer[5]-'0');
					gps_state.tm.msecond = (gps_state.buffer[7]-'0') * 1000;
					gps_state.tm.msecond += (gps_state.buffer[8]-'0') * 100;
					gps_state.tm.msecond += (gps_state.buffer[9]-'0') * 10;
					
					memset(gps_state.buffer, '\0', 12);
					gps_state.data_ck++;
					break;
				case GPS_ST_LAT:
					strncat(gps_state.buffer, &gin, 1);
					break;
				case GPS_ST_NS:
					gps_state.lat.degree = (gps_state.buffer[0]-'0') * 10;
					gps_state.lat.degree += (gps_state.buffer[1]-'0');
					gps_state.lat.minute = (gps_state.buffer[2]-'0') * 10;
					gps_state.lat.minute += (gps_state.buffer[3]-'0');
					gps_state.lat.fminute = (gps_state.buffer[5]-'0') * 1000;
					gps_state.lat.fminute += (gps_state.buffer[6]-'0') * 100;
					gps_state.lat.fminute += (gps_state.buffer[7]-'0') * 10;
					gps_state.lat.fminute += (gps_state.buffer[8]-'0');
					
					memset(gps_state.buffer, '\0', 12);
					gps_state.lat.dir = gin;
					gps_state.data_ck++;
					break;
				case GPS_ST_LON:
					strncat(gps_state.buffer, &gin, 1);
					break;
				case GPS_ST_EW:
					gps_state.lon.degree = (gps_state.buffer[0]-'0') * 100;
					gps_state.lon.degree += (gps_state.buffer[1]-'0') * 10;
					gps_state.lon.degree += (gps_state.buffer[2]-'0');
					gps_state.lon.minute = (gps_state.buffer[3]-'0') * 10;
					gps_state.lon.minute += (gps_state.buffer[4]-'0');
					gps_state.lon.fminute = (gps_state.buffer[6]-'0') * 1000;
					gps_state.lon.fminute += (gps_state.buffer[7]-'0') * 100;
					gps_state.lon.fminute += (gps_state.buffer[8]-'0') * 10;
					gps_state.lon.fminute += (gps_state.buffer[9]-'0');
					
					memset(gps_state.buffer, '\0', 12);
					gps_state.lon.dir = gin;
					gps_state.data_ck++;
					break;
				case GPS_ST_SPD:
					break;
				case GPS_ST_CRS:
					break;
				case GPS_ST_DATE:
					strncat(gps_state.buffer, &gin, 1);
					
					break;
				case GPS_ST_MVAR:
										//Not getting run, since string is ,, and ,'s don't count
					break;
				case GPS_ST_CKSUM:
					if(gin == '*') {
				
						
						gps_state.tm.day	= (gps_state.buffer[0]-'0') * 10;
						gps_state.tm.day	+= (gps_state.buffer[1]-'0');
						gps_state.tm.month	= (gps_state.buffer[2]-'0') * 10;
						gps_state.tm.month	+= (gps_state.buffer[3]-'0');
						gps_state.tm.year	= (gps_state.buffer[4]-'0') * 10;
						gps_state.tm.year	+= (gps_state.buffer[5]-'0');
						memset(gps_state.buffer, '\0', 12);
					
						fin_sum = cur_sum ^ '*';
					}
					else if(gin == '\r')
						gps_state.state++;
					else
						strncat(gps_state.buffer, &gin, 1);
					break;
				case GPS_ST_VALIDATE:
					sprintf(&gps_state.buffer[2],fmt,fin_sum);
					if(gps_state.buffer[0] == gps_state.buffer[2] && gps_state.buffer[1] == gps_state.buffer[3] && gps_state.data_ck >= 3)
						gps_state.valid = 1;	
					gps_state.state = GPS_ST_RESET;
					break;
				default:
					gps_state.state = GPS_ST_RESET;
					break;
			}	
		}		
	}
}
void Set_LED(unsigned char LED)
{
	
	switch(LED) {
		case LED_GPSA:
			LED_S_A = LED_S_B = 0;
			break;
		case LED_GPSB:
			LED_S_A = 1;
			LED_S_B = 0;
			break;
		case LED_GPSC:
			LED_S_A = 0;
			LED_S_B = 1;
			break;
		case LED_ERR:
			LED_S_A = LED_S_B = 1;
			break;
		default:
			Nop();
			break;
			
		}
		CUR_LED = LED;

}

void UserInit(void)
{
 
	
	//GPS IRQ
	TRIS_GPS_IRQ = 1;
	gps_state.state = 0;
	gps_state.valid = 0;
	//UART IRQ
	TRIS_UART_IRQ = 1;
	//switches
	TRIS_SW_A = TRIS_SW_B = TRIS_SW_C = 1;
	
	//Set up servos
	TRIS_SERV_A	= TRIS_SERV_B = 0;
	SERV_A = SERV_B = 0;
	
	servA.center.val = 2250;
	servB.center.val = 2180;
	servA.min.val = servB.min.val = 784;
	servA.max.val = servB.max.val = 3715;
	servA.DS = servA.min;
	servB.DS = servB.center;
	servA.PER.val = servB.PER.val = 0xFFFF; //Until the the servos catch up, then 30000
	//servA.steps_per_degree = servB.steps_per_degree = 15.422;
	//14.7
	servA.steps_per_degree = servB.steps_per_degree = 14.33276207; //Verified
	
	
	serv_cnt.val = 0;
	//Set up the A2D
	ADCON1 = 0b00111011;
	ADCON2 = 0b10111110;
	//Set up magnetic sensor
	TRIS_MAG_SET = 0;
	TRIS_MAG_IN_A = TRIS_MAG_IN_B = 1;
	MAG_SET = 0;
	Delay1KTCYx(1);
	MAG_SET = 1;
	Delay10KTCYx(1);
	MAG_SET = 0;
	Delay1KTCYx(1);
	MAG_SET = 1;
	Delay10KTCYx(1);
	MAG_SET = 0;
	//LEDs
	TRIS_LED_S_A = TRIS_LED_S_B = 0;
	LED_S_A = LED_S_B = 0;
	//Set up SPI
	
	TRIS_SPI_SCK = TRIS_SPI_DOUT = 0;
	TRIS_SPI_DIN = 1;
	TRIS_SPI_CS_A = TRIS_SPI_CS_B = 0;
	SSPSTAT = 0b01000000;
	SSPCON1 = 0b00100010;
	config_SPI_devs();
	
	//Set up interrupts
	INTCON = 0x20;				//disable global and enable TMR0 interrupt
	INTCON2 = 0x84;             //TMR0 high priority
	RCONbits.IPEN = 1;          //enable priority levels
	
	PIE1 = 0b00000001;
	IPR1 = 0b00000001;
	PIE2bits.TMR3IE = 1;
	IPR2bits.TMR3IP = 0;
		
	memset(&position.tm, 0, sizeof(struct time));	//Initialize time
	TMR3H = 0xC5;
	TMR3L = 0x68;
	T3CON = 0b10110001;
	
	TMR1H = 0;
	TMR1L = 0;
	T1CON = 0b10110001;
	
	TMR0H = 0;					//Set Timer
	TMR0L = 0;
	T0CON = 0b10000010;
	INTCONbits.GIEH = 1;
	INTCONbits.GIEL = 1;
	
	//position.magnetic_dec = 14.81667;	//Positive Indicates East (actual value)
	position.magnetic_dec = -20.0;
	position.cmd_object = NULL;
	position.object = m_get_cur_item();

	
}
unsigned int read_compass(unsigned char chan)
{
	union byte_access mval;
	
	if(chan)
		ADCON0 = 0b00000111;
	else
		ADCON0 = 0b00000011;
	
	while(ADCON0bits.GO);
	mval.v.val_H = ADRESH;
	mval.v.val_L = ADRESL;

	return mval.val;

}
void ProcessIO(void)
{   
	
	static char swA_st = 0;
	static char swB_st = 0;
	static char swC_st = 0;

	if(SW_A) {
		if(!swA_st) {
			swA_st = 1;
			m_up();
		}
	} else
		swA_st = 0;
	if(SW_B) {
		if(!swB_st) {
			swB_st = 1;
			m_down();
		}
	} else
		swB_st = 0;
	if(SW_C) {
		if(!swC_st) {
			swC_st = 1;
			cmd_md = 0;
			position.object = m_get_cur_item();
			update = 1;
		}
	} else
		swC_st = 0;
}		

