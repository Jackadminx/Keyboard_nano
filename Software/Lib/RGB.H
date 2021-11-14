#include "GPIO.H"
#include "FlashRom.H"
#include <intrins.h>

//_nop_ 50 ns
#define    Wait1nop 				{_nop_();}
#define    Wait2nop 				{_nop_();_nop_();}
#define    Wait4nop 				{_nop_();_nop_();_nop_();_nop_();}
#define    Wait10nop        {_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();} //250ns
#define    Wait400ns        {Wait10nop;Wait10nop;}
#define    Wait800ns        {Wait10nop;Wait10nop;Wait10nop;Wait10nop}



extern void wd2812b_driver(void);
extern void led_flash(UINT8 rw, UINT8X HID_OUT_report[65],UINT8 mode);
extern void change_LED_BRG(UINT8X HID_OUT_report[65]);
