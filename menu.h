#ifndef MENU
#define MENU


struct m_item {
	char descriptor[16];
	double right_ascension;
	double declination;
	char magnitude[8];
};

void m_display_item(rom struct m_item* mi);
void m_up(void);
void m_down(void);
void m_initialize(void);
rom struct m_item* m_get_cur_item(void);








#endif