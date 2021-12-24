#include "WS2812B.h"
#include <intrins.h>

sbit RGB = P2 ^ 4; // LED_data Pin

void LED__SendZero_(void)
{
    RGB = 1;
    Wait10nop;
    Wait10nop;
    RGB = 0;
    Wait4nop;
    Wait10nop;
    Wait10nop;
}
void LED__SendOne_(void)
{
    RGB = 1;
    Wait10nop;
    Wait10nop;
    Wait10nop;
    Wait10nop;
    RGB = 0;
    Wait4nop;

}

void LED_Latch(void)
{
    short a = 40000;
    RGB = 0;

    //Each loop should produce 3 instructions: decrement, comparison, and jmp.
    //At least 600 instructions are needed for 50us.
    while(a--);
}

void LED__SendByte_(unsigned char dat)
{
    if(dat & 0x80) LED__SendOne_();
    else LED__SendZero_();

    if(dat & 0x40) LED__SendOne_();
    else LED__SendZero_();

    if(dat & 0x20) LED__SendOne_();
    else LED__SendZero_();

    if(dat & 0x10) LED__SendOne_();
    else LED__SendZero_();

    if(dat & 0x08) LED__SendOne_();
    else LED__SendZero_();

    if(dat & 0x04) LED__SendOne_();
    else LED__SendZero_();

    if(dat & 0x02) LED__SendOne_();
    else LED__SendZero_();

    if(dat & 0x01) LED__SendOne_();
    else LED__SendZero_();
}
void LED_SendRGBColor(RGBColor *color)
{
    LED__SendByte_((*color).G);
    LED__SendByte_((*color).R);
    LED__SendByte_((*color).B);
}
void LED_SendColor(unsigned char R, unsigned char G, unsigned char B)
{
    LED__SendByte_(G);
    LED__SendByte_(R);
    LED__SendByte_(B);
}
void LED_SendRGBData(RGBColor *colors, unsigned short count)
{
    while(count--)
    {
        LED_SendRGBColor(colors++);
    }

    LED_Latch();
}



