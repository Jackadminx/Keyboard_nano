//#include "GPIO.H"
#include "FlashRom.H"
// #include "Timer.H"
#include "WS2812B.h"
#include <stdlib.h>
//#include <intrins.h>



extern void wd2812b_driver(void);
extern void led_flash(UINT8 rw, UINT8X HID_OUT_report[65],UINT8 mode);
extern void change_LED_BRG(UINT8X HID_OUT_report[65]);
extern void LED_effects_fades(void);
extern void LED_effect_release(UINT8X i);
extern void LED_effect_press(UINT8X i);
extern void LED_effect_init(UINT8 LED_effect_mode_1);

