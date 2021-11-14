#ifndef __PWM_H__
#define __PWM_H__
#include "CH549.H"
#include "DEBUG.H"
//#define PWM_INTERRUPT
/* PWM通道定义（可以组合） */
#define   CH0        0x01
#define   CH1        0x02
#define   CH2        0x04
#define   CH3        0x08
#define   CH4        0x10
#define   CH5        0x20
#define   CH6        0x40
#define   CH7        0x80
/* 输出使能状态 */
#define  Disable     0x00                                                 //关闭输出
#define  Enable      0x01                                                 //使能输出
/* 外部调用恒定义 */
#define SetPWMClkDiv(n)          PWM_CK_SE = n                            //PWM时钟配置,FREQ_SYS/n  
#define SetPWMCycle64Clk()       PWM_CTRL |= bPWM_MOD_6BIT                //PWM周期为 FREQ_SYS/n/64
#define SetPWMCycle256Clk()      PWM_CTRL &= ~bPWM_MOD_6BIT               //PWM周期为 FREQ_SYS/n/256
#define PWM0OutPolarHighAct()    PWM_CTRL &= ~bPWM0_POLAR                 //PWM0输出默认低，高有效(默认)
#define PWM0OutPolarLowAct()     PWM_CTRL |= bPWM0_POLAR                  //PWM0输出默认高，低有效
#define PWM1OutPolarHighAct()    PWM_CTRL &= ~bPWM1_POLAR                 //PWM1输出默认低，高有效(默认)          
#define PWM1OutPolarLowAct()     PWM_CTRL |= bPWM1_POLAR                  //PWM1输出默认高，低有效
#define SetPWM0Dat(dat)          PWM_DATA0 = dat                          //设置PWM0输出占空比
#define SetPWM1Dat(dat)          PWM_DATA1 = dat                          //设置PWM1输出占空比
#define SetPWM2Dat(dat)          PWM_DATA2 = dat                          //设置PWM2输出占空比
#define SetPWM3Dat(dat)          PWM_DATA3 = dat                          //设置PWM3输出占空比
#define SetPWM4Dat(dat)          PWM_DATA4 = dat                          //设置PWM4输出占空比
#define SetPWM5Dat(dat)          PWM_DATA5 = dat                          //设置PWM5输出占空比
#define SetPWM6Dat(dat)          PWM_DATA6 = dat                          //设置PWM6输出占空比
#define SetPWM7Dat(dat)          PWM_DATA7 = dat                          //设置PWM7输出占空比

extern void LEDtoBits();

/* 外部调用子函数 */
extern void PWM_SEL_CHANNEL(UINT8 Channel,UINT8 NewState);                //PWM通道输出使能,包含IO端口配置
#ifdef PWM_INTERRUPT
extern void PWMInterruptEnable();                                         //使能PWM周期溢出中断
#endif
#endif
