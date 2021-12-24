#include "GPIO.H"

typedef struct _RGBColor {
	unsigned char R;
	unsigned char G;
	unsigned char B;

} RGBColor;

//_nop_ 50 ns
#define    Wait1nop 				{_nop_();}          //42ns
#define    Wait2nop 				{_nop_();_nop_();}      //82ns
#define    Wait4nop 				{_nop_();_nop_();_nop_();_nop_();}  //164ns
#define    Wait10nop        {_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();} //420ns
#define    Wait16nop        {_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();} //672ns
#define    Wait18nop        {_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();} //756ns
#define    Wait400ns        {Wait10nop;Wait10nop;}
#define    Wait800ns        {Wait10nop;Wait10nop;Wait10nop;Wait10nop}

void LED__SendZero__(void);
void LED__SendOne__(void);
void LED__SendByte__(unsigned char);
void LED_Latch(void);

void LED_SendRGBColor(RGBColor*);
void LED_SendColor(unsigned char, unsigned char, unsigned char);
void LED_SendRGBData(RGBColor*, unsigned short);


