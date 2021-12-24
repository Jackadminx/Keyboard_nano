/********************************** (C) COPYRIGHT *******************************
* File Name          : main.C
* Author             :
* Version            : V1.0
* Date               : 2021/09/15
* Description        : CH549模拟HID兼容设备，支持中断上下传，支持控制端点上下传，支持设置全速，低速
                       注意包含DEBUG.C

                                             3键USB键盘
                                             RGB支持
                                             自定义HID功能，支持上位机修改键位
*******************************************************************************/
#include "CH549.H"
#include "DEBUG.H"
#include "GPIO.H"
#include "FlashRom.H"
#include "MATH.H"
#include "RGB.H"

#define Fullspeed
#ifdef Fullspeed
#define THIS_ENDP0_SIZE 64
#else
#define THIS_ENDP0_SIZE 8 //低速USB，中断传输、控制传输最大包长度为8
#endif
UINT8X Ep0Buffer[THIS_ENDP0_SIZE + 2] _at_ 0x0000;                  //端点0 OUT&IN缓冲区，必须是偶地址
UINT8X Ep2Buffer[2 * MAX_PACKET_SIZE + 4] _at_ THIS_ENDP0_SIZE + 2; //端点2 IN&OUT缓冲区,必须是偶地址
UINT8 SetupReq, Ready, UsbConfig;
UINT16 SetupLen;
PUINT8 pDescr;             // USB配置标志
USB_SETUP_REQ SetupReqBuf; //暂存Setup包
#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

sbit KEY1 = P1 ^ 4; // KEY1
sbit KEY2 = P1 ^ 5; // KEY2
sbit KEY3 = P1 ^ 6; // KEY2
#pragma NOAREGS

/*字符串描述符 略*/
/*HID类报表描述符*/
UINT8C HIDRepDesc[] =
    {

        /* keyboared */
        0x05, 0x01, // USAGE_PAGE (Generic Desktop)
        0x09, 0x06, // USAGE (Keyboard)
        0xa1, 0x01, // COLLECTION (Application)
        0x85, 0x01, // Report ID (1)
        0x05, 0x07, // USAGE_PAGE (Keyboard)
        0x19, 0xe0, // USAGE_MINIMUM (Keyboard LeftControl)
        0x29, 0xe7, // USAGE_MAXIMUM (Keyboard Right GUI)
        0x15, 0x00, // LOGICAL_MINIMUM (0)
        0x25, 0x01, // LOGICAL_MAXIMUM (1)
        0x75, 0x01, // REPORT_SIZE (1)
        0x95, 0x08, // REPORT_COUNT (8)
        0x81, 0x02, // INPUT (Data,Var,Abs)
        0x95, 0x01, // REPORT_COUNT (1)
        0x75, 0x08, // REPORT_SIZE (8)
        0x81, 0x03, // INPUT (Cnst,Var,Abs)
        0x95, 0x05, // REPORT_COUNT (5)
        0x75, 0x01, // REPORT_SIZE (1)
        0x05, 0x08, // USAGE_PAGE (LEDs)
        0x19, 0x01, // USAGE_MINIMUM (Num Lock)
        0x29, 0x05, // USAGE_MAXIMUM (Kana)
        0x91, 0x02, // OUTPUT (Data,Var,Abs)
        0x95, 0x01, // REPORT_COUNT (1)
        0x75, 0x03, // REPORT_SIZE (3)
        0x91, 0x03, // OUTPUT (Cnst,Var,Abs)
        0x95, 0x06, // REPORT_COUNT (6)
        0x75, 0x08, // REPORT_SIZE (8)
        0x15, 0x00, // LOGICAL_MINIMUM (0)
        0x25, 0xFF, // LOGICAL_MAXIMUM (255)
        0x05, 0x07, // USAGE_PAGE (Keyboard)
        0x19, 0x00, // USAGE_MINIMUM (Reserved (no event indicated))
        0x29, 0x65, // USAGE_MAXIMUM (Keyboard Application)
        0x81, 0x00, // INPUT (Data,Ary,Abs)
        0xc0,       // END_COLLECTION    /* 65 */

        /* consumer */
        0x05, 0x0c,       // USAGE_PAGE (Consumer Devices)
        0x09, 0x01,       // USAGE (Consumer Control)
        0xa1, 0x01,       // COLLECTION (Application)
        0x85, 0x02,       //   REPORT_ID (2)
        0x19, 0x00,       //   USAGE_MINIMUM (Unassigned)
        0x2a, 0x3c, 0x03, //   USAGE_MAXIMUM
        0x15, 0x00,       //   LOGICAL_MINIMUM (0)
        0x26, 0x3c, 0x03, //   LOGICAL_MAXIMUM (828)
        0x95, 0x01,       //   REPORT_COUNT (1)
        0x75, 0x10,       //   REPORT_SIZE (16)
        0x81, 0x00,       //   INPUT (Data,Ary,Abs)
        0xc0,             // END_COLLECTION  /* 25 */

        /* mouse */
        0x05, 0x01, // USAGE_PAGE (Generic Desktop)
        0x09, 0x02, // USAGE (Mouse)
        0xa1, 0x01, // COLLECTION (Application)
        0x85, 0x03, // REPORT_ID (3)
        0x09, 0x01, //   USAGE (Pointer)
        0xa1, 0x00, //   COLLECTION (Physical)
        0x05, 0x09, //     USAGE_PAGE (Button)
        0x19, 0x01, //     USAGE_MINIMUM (Button 1)
        0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
        0x15, 0x00, //     LOGICAL_MINIMUM (0)
        0x25, 0x01, //     LOGICAL_MAXIMUM (1)
        0x95, 0x03, //     REPORT_COUNT (3)
        0x75, 0x01, //     REPORT_SIZE (1)
        0x81, 0x02, //     INPUT (Data,Var,Abs)
        0x95, 0x01, //     REPORT_COUNT (1)
        0x75, 0x05, //     REPORT_SIZE (5)
        0x81, 0x03, //     INPUT (Cnst,Var,Abs)
        0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
        0x09, 0x30, //     USAGE (X)
        0x09, 0x31, //     USAGE (Y)
        0x09, 0x38, //		 Usage (Wheel)
        0x15, 0x81, //     LOGICAL_MINIMUM (-127)
        0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
        0x75, 0x08, //     REPORT_SIZE (8)
        0x95, 0x03, //     REPORT_COUNT (3)
        0x81, 0x06, //     INPUT (Data,Var,Rel)
        0xc0,       //   END_COLLECTION
        0xc0,       // END_COLLECTION 	/* 52 + 2 */
        /*
    BYTE1字节：
    |—bit7～3：补充的常数，无意义，这里为0即可
    |—bit2: 1表示中键按下
    |—bit1: 1表示右键按下 0表示右键抬起
    |—bit0: 1表示左键按下 0表示左键抬起
    BYTE2 — X坐标变化量，与byte的bit4组成9位符号数,负数表示向左移，正数表右移。用补码表示变化量
    BYTE3 — Y坐标变化量，与byte的bit5组成9位符号数，负数表示向下移，正数表上移。用补码表示变化量
    BYTE4 — 滚轮变化。0x01表示滚轮向前滚动一格；0xFF表示滚轮向后滚动一格；0x80是个中间值，不滚动。
    */

        //    /* Vendor-out */
        //    0x06, 0xB0, 0xFF, //Usage Page (Vendor-Defined 177)
        //    0x09, 0x01,       //Usage (Vendor-Defined 1)
        //    0xA1, 0x01,       //Collection (Application)
        //    0x85, 0x04,       //REPORT_ID (4)
        //    0x09, 0x04,       //Usage (Vendor-Defined 4)
        //    0x15, 0x00,       //Logical Minimum (0)
        //    0x26, 0xFF, 0x00, //Logical Maximum (255)
        //    0x75, 0x08,       //Report Size (8)
        //    0x95, 0x40,       //Report Count (64)
        //    0x91, 0x02,       //Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
        //    0xC0,             //End Collection   /* 23      4-report=165 */

        /* Vendor-in-out */
        0x06, 0xB1, 0xFF, // Usage Page (Vendor-Defined 178)
        0x09, 0x01,       // Usage (Vendor-Defined 1)
        0xA1, 0x01,       // Collection (Application)

        0x85, 0x04,       // REPORT_ID (4)
        0x09, 0x04,       // Usage (Vendor-Defined 4)
        0x15, 0x00,       // Logical Minimum (0)
        0x26, 0xFF, 0x00, // Logical Maximum (255)
        0x75, 0x08,       // Report Size (8)
        0x95, 0x3c,       // Report Count (60)
        0x91, 0x02,       // Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)

        0x09, 0x01,       //   Usage (0x01)
        0x25, 0x00,       //   Logical Maximum (0)
        0x26, 0xFF, 0x00, //	 Logical Maximum (255)
        0x75, 0x08,       //   Report Size (8)
        0x95, 0x3c,       //   Report Count (60)
        0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

        0xC0, // End Collection   /* 37 */

        /* Touch Screen */
        0x05, 0x0d,       // USAGE_PAGE (Digitizers)
        0x09, 0x04,       // USAGE (Touch Screen)
        0xa1, 0x01,       // COLLECTION (Application)
        0x85, 0x05,       //		REPORT_ID (5)
        0x09, 0x22,       //   USAGE (Finger)
        0xa1, 0x00,       //   COLLECTION (Physical)
        0x09, 0x42,       //     USAGE (Tip Switch)
        0x15, 0x00,       //     LOGICAL_MINIMUM (0)
        0x25, 0x01,       //     LOGICAL_MAXIMUM (1)
        0x75, 0x01,       //     REPORT_SIZE (1)
        0x95, 0x01,       //     REPORT_COUNT (1)
        0x81, 0x02,       //     INPUT (Data,Var,Abs)
        0x09, 0x32,       //     Usage (In Range)
        0x15, 0x00,       //     Logical Minimum (0)
        0x25, 0x01,       //     Logical Maximum (1)
        0x81, 0x02,       //     Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
        0x09, 0x51,       //     Usage (Contact Identifier)
        0x75, 0x05,       //     Report Size (5)
        0x95, 0x01,       //     Report Count (1)
        0x16, 0x00, 0x00, //     Logical Minimum (0)
        0x26, 0x10, 0x00, //     Logical Maximum (16)
        0x81, 0x02,       //     Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
        0x09, 0x47,       //     Usage (Confidence)
        0x75, 0x01,       //     Report Size (1)
        0x95, 0x01,       //     Report Count (1)
        0x15, 0x00,       //     Logical Minimum (0)
        0x25, 0x01,       //     Logical Maximum (1)
        0x81, 0x02,       //     Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
        0x05, 0x01,       //     Usage Page (Generic Desktop)
        0x09, 0x30,       //     Usage (X)
        0x75, 0x10,       //     Report Size (16)
        0x95, 0x01,       //     Report Count (1)
        0x55, 0x0D,       //     Unit Exponent (-3)
        0x65, 0x33,       //     Unit (Eng Lin: in^3)
        0x35, 0x00,       //     Physical Minimum (0)
        0x46, 0x60, 0x17, //     Physical Maximum (5984)
        0x26, 0xFF, 0x0F, //     Logical Maximum (4095)
        0x81, 0x02,       //     Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
        0x09, 0x31,       //     Usage (Y)
        0x75, 0x10,       //     Report Size (16)
        0x95, 0x01,       //     Report Count (1)
        0x55, 0x0D,       //     Unit Exponent (-3)
        0x65, 0x33,       //     Unit (Eng Lin: in^3)
        0x35, 0x00,       //     Physical Minimum (0)
        0x46, 0x26, 0x0E, //     Physical Maximum (3622)
        0x26, 0xFF, 0x0F, //     Logical Maximum (4095)
        0x81, 0x02,       //     Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
        0xC0,             //   End Collection
        0xc0,             // END_COLLECTION                /* 102 */

        /* dial */
        0x05, 0x01,
        0x09, 0x0e,
        0xa1, 0x01,
        0x85, 0x06, //		REPORT_ID (6)
        0x05, 0x0d,
        0x09, 0x21,
        0xa1, 0x00,
        0x05, 0x09,
        0x09, 0x01,
        0x95, 0x01,
        0x75, 0x01,
        0x15, 0x00,
        0x25, 0x01,
        0x81, 0x02,
        0x05, 0x01,
        0x09, 0x37,
        0x95, 0x01,
        0x75, 0x0f,
        0x55, 0x0f,
        0x65, 0x14,
        0x36, 0xf0, 0xf1,
        0x46, 0x10, 0x0e,
        0x16, 0xf0, 0xf1,
        0x26, 0x10, 0x0e,
        0x81, 0x06,
        0xc0,
        0xC0, /* 56 */

};

/*设备描述符*/
UINT8C DevDesc[] = {0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, THIS_ENDP0_SIZE,
                    //                    0x86, 0x1A, 0xE0, 0xE6, 0x00, 0x00, 0x01, 0x02, //0x86, 0x1A,VID    0xE0, 0xE6,PID
                    0x86, 0x2B, 0xE0, 0xE6, 0x00, 0x00, 0x01, 0x02,
                    0x00, 0x01};
UINT8C CfgDesc[] =
    {
        0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x04, 0xA0, 0x32, //配置描述符
        0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x05, //接口描述符
        //    0x09, 0x21, 0x00, 0x01, 0x00, 0x01, 0x22, 0x22, 0x00,       //HID类描述符 wDescriptorLength = 0x22
        0x09, 0x21, 0x01, 0x01, 0x21, 0x01, 0x22, sizeof(HIDRepDesc) & 0xFF, sizeof(HIDRepDesc) >> 8, // HID类描述符 修改wDescriptorLength 长度为0x10B

#ifdef Fullspeed
        0x07, 0x05, 0x82, 0x03, THIS_ENDP0_SIZE, 0x00, 0x01, //端点描述符(全速间隔时间改成1ms)
        0x07, 0x05, 0x02, 0x03, THIS_ENDP0_SIZE, 0x00, 0x01, //端点描述符
#else
        0x07, 0x05, 0x82, 0x03, THIS_ENDP0_SIZE, 0x00, 0x0A, //端点描述符(低速间隔时间最小10ms)
        0x07, 0x05, 0x02, 0x03, THIS_ENDP0_SIZE, 0x00, 0x0A, //端点描述符
#endif
};

// 语言描述符
UINT8C MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
// 厂家信息
UINT8C MyManuInfo[] = {0x30, 0x03, 'M', 0, 'o', 0, 'y', 0, 'u', 0, ' ', 0, 'a', 0, 't', 0, ' ', 0, 'w', 0, 'o', 0, 'r', 0, 'k', 0, ' ', 0, 'T', 0, 'e', 0, 'c', 0, 'h', 0, 'n', 0, 'o', 0, 'l', 0, 'o', 0, 'g', 0, 'y', 0}; // MoYu at work Technology
// 产品信息
UINT8C MyProdInfo[] =
    {
        0x1C,
        0x03,
        'K',
        0,
        'e',
        0,
        'y',
        0,
        'b',
        0,
        'o',
        0,
        'a',
        0,
        'r',
        0,
        'd',
        0,
        ' ',
        0,
        'N',
        0,
        'a',
        0,
        'n',
        0,
        'o',
        0,
}; //字符串描述符 Keyboard Nano 13*2bit + 2bit

// UINT8 SendHIDKey[9] = {0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x53};   //每次发送按键BUFF
// UINT8 ReleaseHIDKey[9] = {0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}; //释放按键
// UINT8 SendHIDKeyMUL[5] = {0x02, 0x00, 0x00, 0x00, 0x00};    //多媒体按键数据
// UINT8 ReleaseHIDKeyMUL[5] = {0x02, 0x00, 0x00, 0x00, 0x00}; //释放多媒体按键数据
// UINT8 SendHIDMouse[5] = {0x03, 0x00, 0x00, 0x00, 0x00};    //鼠标数据
// UINT8 ReleaseHIDMouse[5] = {0x03, 0x00, 0x00, 0x00, 0x00}; //释放鼠标点击数据
// UINT8 SendHIDTouch[6] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00}; //触摸屏数据 用于输入绝对坐标
// UINT8 SendHIDKey1[9] = {0};   //每次发送按键BUFF
// UINT8 ReleaseHIDKey1[9] = {0}; //释放按键
// UINT8 SendHIDKey2[9] = {0};   //每次发送按键BUFF
// UINT8 ReleaseHIDKey2[9] = {0}; //释放按键
// UINT8 SendHIDKey3[9] = {0};   //每次发送按键BUFF
// UINT8 ReleaseHIDKey3[9] = {0}; //释放按键

UINT8 SendHIDKey[3][9] = {{0}, {0}, {0}};
UINT8 ReleaseHIDKey[3][9] = {{0}, {0}, {0}};

UINT8 SendHID[9] = {0}; //每次发送按键BUFF

UINT8 KeyState[3] = {1, 1, 1};       //按键状态 0按下 1未按下 2长按
UINT8 BackState[3] = {1, 1, 1};      //按键上一次的状态
UINT16 KeyState_time[3] = {0, 0, 0}; //按键状态 记录长按时间

UINT8X HID_OUT_report[61] = {0}; //接收到的上位机数据

// UINT8 Get_key_1[3] = {0, 0, 0}; //按键状态 记录长按时间
UINT8 Get_mode[12] = {0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 1};
//# buffer[0] = 0  # 预设模式
//# buffer[1] = 0  # 保留
//# buffer[2] = 4  # 保留
//# buffer[3] = 0  # led模式组 0x00-0x06 0为关闭
//# buffer[4] = 2  # 多媒体键
//# buffer[5] = 0xff   # 按键扫描速度 单位1ms
//# buffer[6] = 0xff   # 长按识别间隔 单位100ms
//# buffer[7] = 0xff   # 屏幕分辨率 X 低位
//# buffer[8] = 0xff   # 屏幕分辨率 X 高位
//# buffer[9] = 0xff   # 屏幕分辨率 Y 低位
//# buffer[10] = 0xff   # 屏幕分辨率 Y 高位
//# buffer[10] = 0xff   # 屏幕分辨率 Y 高位
//# buffer[11] = 0   # led灯效组 0x00-0x01 0为关闭

UINT8 TH0_T, TL0_T; //定时器0数据
UINT8 Scan_ms = 10; //定时器扫描间隔 默认10ms

UINT8X UserEp2Buf[64]; //用户数据定义
UINT8 Endp2Busy = 0;
/*******************************************************************************
 * Function Name  : USBDeviceInit()
 * Description    : USB设备模式配置,设备模式启动，收发端点配置，中断开启
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USBDeviceInit()
{
    IE_USB = 0;
    USB_CTRL = 0x00;        // 先设定USB设备模式
    UDEV_CTRL = bUD_PD_DIS; // 禁止DP/DM下拉电阻
#ifndef Fullspeed
    UDEV_CTRL |= bUD_LOW_SPEED; //选择低速1.5M模式
    USB_CTRL |= bUC_LOW_SPEED;
#else
    UDEV_CTRL &= ~bUD_LOW_SPEED;                             //选择全速12M模式，默认方式
    USB_CTRL &= ~bUC_LOW_SPEED;
#endif
    UEP2_T_LEN = 0;                                            //预使用发送长度一定要清空
    UEP2_DMA = Ep2Buffer;                                      //端点2数据传输地址
    UEP2_3_MOD |= bUEP2_TX_EN | bUEP2_RX_EN;                   //端点2发送接收使能
    UEP2_3_MOD &= ~bUEP2_BUF_MOD;                              //端点2收发各64字节缓冲区
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK; //端点2自动翻转同步标志位，IN事务返回NAK，OUT返回ACK
    UEP0_DMA = Ep0Buffer;                                      //端点0数据传输地址
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);                //端点0单64字节收发缓冲区
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                 // OUT事务返回ACK，IN事务返回NAK
    USB_DEV_AD = 0x00;
    USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
    UDEV_CTRL |= bUD_PORT_EN;                              // 允许USB端口
    USB_INT_FG = 0xFF;                                     // 清中断标志
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;
}
/*******************************************************************************
 * Function Name  : Enp2BlukIn()
 * Description    : USB设备模式端点2的批量上传
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void Enp2BlukIn(UINT8 *buf, UINT8 len)
{
    memcpy(Ep2Buffer + MAX_PACKET_SIZE, buf, len);           //加载上传数据
    UEP2_T_LEN = len;                                        //上传最大包长度
    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; //有数据时上传数据并应答ACK
}
/*******************************************************************************
 * Function Name  : DeviceInterrupt()
 * Description    : CH559USB中断处理函数
 *******************************************************************************/
void DeviceInterrupt(void) interrupt INT_NO_USB using 1 // USB中断服务程序,使用寄存器组1
{
    UINT8 i;
    UINT16 len;
    UINT8 test_report[2] = {0};
    test_report[0] = 4;
    test_report[1] = 3;

    if (UIF_TRANSFER) // USB传输完成标志
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 2: // endpoint 2# 端点批量上传
            UEP2_T_LEN = 0;    //预使用发送长度一定要清空
            //            UEP1_CTRL ^= bUEP_T_TOG;                                          //如果不设置自动翻转则需要手动翻转
            Endp2Busy = 0;
            UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; //默认应答NAK
            break;

        case UIS_TOKEN_OUT | 2: // endpoint 2# 端点批量下传
            if (U_TOG_OK)       // 不同步的数据包将丢弃
            {
                if (Ep2Buffer[0] == 4 && Ep2Buffer[1] == 3) //当reportid==4 测试功能
                {
                    Enp2BlukIn(test_report, 61);
#ifdef DE_PRINTF
                    printf("[HID-handle]quick-test\n");
#endif
                }

                len = USB_RX_LEN; //接收数据长度，数据从Ep2Buffer首地址开始存放
#ifdef DE_PRINTF
                printf("[HID]OUT ");
#endif

                for (i = 0; i < len; i++)
                {
#ifdef DE_PRINTF
                    printf("0x%02x ", (UINT16)Ep2Buffer[i]); //遍历OUT数据
#endif

                    if (Ep2Buffer[0] == 0x04) //当reportid==4
                    {
                        HID_OUT_report[i] = Ep2Buffer[i]; //赋值到HID_OUT_report
                    }

                    //                        Ep2Buffer[MAX_PACKET_SIZE + i] = Ep2Buffer[i] ^ 0xFF; // OUT数据取反到IN由计算机验证
                }

#ifdef DE_PRINTF
                printf("len=%u\n", len);
#endif

                //                    UEP2_T_LEN = len;
                //                    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // 允许上传
            }

            break;

        case UIS_TOKEN_SETUP | 0:                                      // SETUP事务
            UEP0_CTRL = UEP0_CTRL & (~MASK_UEP_T_RES) | UEP_T_RES_NAK; //预置NAK,防止stall之后不及时清除响应方式
            len = USB_RX_LEN;

            if (len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = ((UINT16)UsbSetupBuf->wLengthH << 8) + UsbSetupBuf->wLengthL;
                len = 0; // 默认为成功并且上传0长度
                SetupReq = UsbSetupBuf->bRequest;

                if ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) /*HID类命令*/
                {
                    Ready = 1; // HID类命令一般代表usb枚举完成的标志

                    switch (SetupReq)
                    {
                        //                        Ready = 1; //HID类命令一般代表usb枚举完成的标志

                    case 0x01:               // GetReport
                        pDescr = UserEp2Buf; //控制端点上传输据

                        if (SetupLen >= THIS_ENDP0_SIZE) //大于端点0大小，需要特殊处理
                        {
                            len = THIS_ENDP0_SIZE;
                        }
                        else
                        {
                            len = SetupLen;
                        }

                        break;

                    case 0x02: // GetIdle
                        break;

                    case 0x03: // GetProtocol
                        break;

                    case 0x09: // SetReport
                        break;

                    case 0x0A: // SetIdle
                        break;

                    case 0x0B: // SetProtocol
                        break;

                    default:
                        len = 0xFFFF; /*命令不支持*/
                        break;
                    }

                    if (SetupLen > len)
                    {
                        SetupLen = len; //限制总长度
                    }

                    len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; //本次传输长度
                    memcpy(Ep0Buffer, pDescr, len);                                 //加载上传数据
                    SetupLen -= len;
                    pDescr += len;
                }
                else //标准请求
                {
                    switch (SetupReq) //请求码
                    {
                    case USB_GET_DESCRIPTOR:
                        switch (UsbSetupBuf->wValueH)
                        {
                        case 1:               //设备描述符
                            pDescr = DevDesc; //把设备描述符送到要发送的缓冲区
                            len = sizeof(DevDesc);
                            break;

                        case 2:               //配置描述符
                            pDescr = CfgDesc; //把设备描述符送到要发送的缓冲区
                            len = sizeof(CfgDesc);
                            break;

                        case 3:
                            switch (UsbSetupBuf->wValueL)
                            {
                            case 1:
                                pDescr = (PUINT8)(&MyManuInfo[0]);
                                len = sizeof(MyManuInfo);
                                break;

                            case 2:
                                pDescr = (PUINT8)(&MyProdInfo[0]);
                                len = sizeof(MyProdInfo);
                                break;

                            case 0:
                                pDescr = (PUINT8)(&MyLangDescr[0]);
                                len = sizeof(MyLangDescr);
                                break;

                            default:
                                len = 0xFFFF; // 不支持的字符串描述符
                                break;
                            }

                            break;

                        case 0x22:               //报表描述符
                            pDescr = HIDRepDesc; //数据准备上传
                            len = sizeof(HIDRepDesc);
                            break;

                        default:
                            len = 0xFFFF; //不支持的命令或者出错
                            break;
                        }

                        if (SetupLen > len)
                        {
                            SetupLen = len; //限制总长度
                        }

                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; //本次传输长度
                        memcpy(Ep0Buffer, pDescr, len);                                 //加载上传数据
                        SetupLen -= len;
                        pDescr += len;
                        break;

                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL; //暂存USB设备地址
                        break;

                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;

                        if (SetupLen >= 1)
                        {
                            len = 1;
                        }

                        break;

                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;

                        if (UsbConfig)
                        {
                            Ready = 1; // set config命令一般代表usb枚举完成的标志
                        }

                        break;

                    case 0x0A:
                        break;

                    case USB_CLEAR_FEATURE:                                                         // Clear Feature
                        if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // 端点
                        {
                            switch (UsbSetupBuf->wIndexL)
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;

                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                break;

                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                                break;

                            default:
                                len = 0xFFFF; // 不支持的端点
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFFFF; // 不是端点不支持
                        }

                        break;

                    case USB_SET_FEATURE:                               /* Set Feature */
                        if ((UsbSetupBuf->bRequestType & 0x1F) == 0x00) /* 设置设备 */
                        {
                            if ((((UINT16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01)
                            {
                                if (CfgDesc[7] & 0x20)
                                {
                                    /* 设置唤醒使能标志 */
                                }
                                else
                                {
                                    len = 0xFFFF; /* 操作失败 */
                                }
                            }
                            else
                            {
                                len = 0xFFFF; /* 操作失败 */
                            }
                        }
                        else if ((UsbSetupBuf->bRequestType & 0x1F) == 0x02) /* 设置端点 */
                        {
                            if ((((UINT16)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00)
                            {
                                switch (((UINT16)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL)
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点2 IN STALL */
                                    break;

                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL; /* 设置端点2 OUT Stall */
                                    break;

                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点1 IN STALL */
                                    break;

                                default:
                                    len = 0xFFFF; /* 操作失败 */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFFFF; /* 操作失败 */
                            }
                        }
                        else
                        {
                            len = 0xFFFF; /* 操作失败 */
                        }

                        break;

                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;

                        if (SetupLen >= 2)
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }

                        break;

                    default:
                        len = 0xFFFF; //操作失败
                        break;
                    }
                }
            }
            else
            {
                len = 0xFFFF; //包长度错误
            }

            if (len == 0xFFFF)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
            }
            else if (len <= THIS_ENDP0_SIZE) //上传数据或者状态阶段返回0长度包
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; //默认数据包是DATA1，返回应答ACK
            }
            else
            {
                UEP0_T_LEN = 0;                                                      //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; //默认数据包是DATA1,返回应答ACK
            }

            break;

        case UIS_TOKEN_IN | 0: // endpoint0 IN
            switch (SetupReq)
            {
            case USB_GET_DESCRIPTOR:
            case HID_GET_REPORT:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen; //本次传输长度
                memcpy(Ep0Buffer, pDescr, len);                                 //加载上传数据
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG; //同步标志位翻转
                break;

            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;

            default:
                UEP0_T_LEN = 0; //状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }

            break;

        case UIS_TOKEN_OUT | 0: // endpoint0 OUT
            len = USB_RX_LEN;
            UEP0_CTRL ^= bUEP_R_TOG; //同步标志位翻转
            break;

        default:
            break;
        }

        UIF_TRANSFER = 0; //写0清空中断
    }
    else if (UIF_BUS_RST) //设备模式USB总线复位中断
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        Endp2Busy = 0;
        Ready = 0;
        UIF_BUS_RST = 0; //清中断标志
    }
    else if (UIF_SUSPEND) // USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;

        if (USB_MIS_ST & bUMS_SUSPEND) //挂起
        {
#ifdef DE_PRINTF
            printf("[USB]sleep...\r\n"); //睡眠状态
#endif
            //             while ( XBUS_AUX & bUART0_TX )
            //             {
            //                 ;    //等待发送完成
            //             }
            //             SAFE_MOD = 0x55;
            //             SAFE_MOD = 0xAA;
            //             WAKE_CTRL = bWA  K_BY_USB | bWAK_RXD0_LO;                              //USB或者RXD0有信号时可被唤醒
            //             PCON |= PD;                                                          //睡眠
            //             SAFE_MOD = 0x55;
            //             SAFE_MOD = 0xAA;
            //             WAKE_CTRL = 0x00;
        }
        else // 唤醒
        {
#ifdef DE_PRINTF
            printf("[USB]Wake up!\r\n");
#endif
        }
    }
    else //意外的中断,不可能发生的情况
    {
        USB_INT_FG = 0xFF; //清中断标志
#ifdef DE_PRINTF
        printf("[USB]UnknownInt  \r\n");
#endif
    }
}

/*******************************************************************************
 * Function Name  : button_read()
 * Description    : 按键检测
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void button_read(void)
{
    if (KEY1 == 0) //按键按下
    {
        KeyState[0] = 0;
        KeyState_time[0] += Scan_ms;
    }
    else if (KEY1 == 1) //按键松开
    {
        KeyState[0] = 1;
    }

    if (KEY2 == 0)
    {
        KeyState[1] = 0;
        KeyState_time[1] += Scan_ms;
    }
    else if (KEY2 == 1)
    {
        KeyState[1] = 1;
    }

    if (KEY3 == 0)
    {
        KeyState[2] = 0;
        KeyState_time[2] += Scan_ms;
    }
    else if (KEY3 == 1)
    {
        KeyState[2] = 1;
    }
}

/*******************************************************************************
* Function Name  : touch_drive()
* Description    : 实现屏幕触摸效果
* Input          : UINT8 flag, UINT16 x, UINT16 y
    UINT8 flag： 标志字节 0x83按下 0x82松开
    UINT16 x： x坐标
    UINT16 y： y坐标
* Output         : None
* Return         : None
*******************************************************************************/
void touch_drive(UINT8 flag, UINT16 x, UINT16 y)
{

    UINT8 SendHIDTouch[6] = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00}; //触摸屏数据 用于输入绝对坐标

    UINT8 i, len;
    UINT32 dispX = 4096 * 100000 / (Get_mode[8] * 255 + Get_mode[7]);
    UINT32 dispY = 4096 * 100000 / (Get_mode[10] * 255 + Get_mode[9]);
    //    UINT32 dispX = 4096 * 100000 / 1920;
    //    UINT32 dispY = 4096 * 100000 / 1200;
    UINT32 TX = x, TY = y;

    //    printf("[DEBUG]dispX dispY 0x%lx 0x%lx \n", (UINT32)dispX, (UINT32)dispY);

    /*

    第1字节：触摸1 标志字节（详见后面描述）0x83按下 0x82松开
    第2字节：触摸1 x坐标的低八位
    第3字节：触摸1 x坐标的高八位
    第4字节：触摸1 y坐标的低八位
    第5字节：触摸1 y坐标的高八位

    标志字节组成：
    bit7: confidence触摸是否可信 1可信 0不可信（对于安卓不起作用 电脑起作用）
    bit6-bit2: 触控identifice身份指明是哪一个手的触控 注意上报多点触控的时候要不一样！！！
    bit1: Range数据范围 一般为1
    bit0: 按键状态 1按下 0松开

        触摸屏分辨率是4096*4096

        将分辨率除2得到屏幕中心点坐标
        x = 4096*1/2-1;
        y = 4096*1/2-1;
        printf("[DEBUG]XY 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", (UINT16)x, (UINT16)y, (UINT16) x%256, (UINT16)x/256,(UINT16)y%256, (UINT16)y/256);

        显示器分辨率为1920*1200，转换为得到对应触摸屏的XY
        x = 4096/1920*y-1;
        y = 4096/1200*x-1;

                //不使用小数进行运算
            UINT32 dispX = 4096 * 100000 / 1920 ; //乘以100000用于保留精度
            UINT32 dispY = 4096 * 100000 / 1200 ;
            UINT32 TX = x, TY = y;	//x y为UINT16需要临时放在UINT32中
                x = dispX * TX / 100000;
                y = dispY * TY / 100000;

    */

    x = dispX * TX / 100000;
    y = dispY * TY / 100000;
    //    printf("[DEBUG]eXeY 0x%02x 0x%02x \n", (UINT16)x, (UINT16)y);

    //赋值
    SendHIDTouch[1] = flag;
    SendHIDTouch[2] = x % 256;
    SendHIDTouch[3] = x / 256;
    SendHIDTouch[4] = y % 256;
    SendHIDTouch[5] = y / 256;
    //遍历HID数据
    printf("[HID]IN ");

    for (i = 0; i < sizeof(SendHIDTouch); i++)
    {
        printf("0X%02x ", (UINT16)SendHIDTouch[i]);
    }

    len = sizeof(SendHIDTouch);
    printf("len= %02u \n", (UINT8)len);
    //向USB发送数据
    Enp2BlukIn(SendHIDTouch, sizeof(SendHIDTouch));

    //清空SendHIDTouch中的数据
    for (i = 1; i < sizeof(SendHIDTouch) - 1; i++)
    {
        SendHIDTouch[i] = 0;
    }
}

/*******************************************************************************
* Function Name  : draw_circle()
* Description    : 屏幕中心绘制圆形(多边形)
* Input          : UINT8 radius, UINT8 sides, UINT8 time
     UINT8 radius： 半径
     UINT8 sides： 边长
         UINT8 time： 绘制一个圆所需时间 毫秒
* Output         : None
* Return         : None
*******************************************************************************/
void draw_circle(UINT8 time, UINT8 n, UINT8 r)
{
    UINT8 i;
    //    UINT8 n = 8; //多边体边数
    //    UINT8 r = 40; //多边体边长
    UINT16 X, Y;
    UINT16 dispResX = (Get_mode[8] * 255 + Get_mode[7]) / 2;  //屏幕中心点x
    UINT16 dispResY = (Get_mode[10] * 255 + Get_mode[9]) / 2; //屏幕中心点y
    time = time / n;                                          //画单条边完成后延时

    for (i = 0; i < n; i++)
    {
        X = r * cos(2 * 3.14 * i / n) + dispResX;
        Y = r * sin(2 * 3.14 * i / n) + dispResY;
        touch_drive(0x83, X, Y);
        mDelaymS(time);
        //        printf("X=%d Y=%d\n", X, Y );
    }
}

/*******************************************************************************
 * Function Name  : KeyAction()
 * Description    : 向上位机发送HID消息
 * Input          : keyCode
 * Output         : None
 * Return         : None
 *******************************************************************************/
void KeyAction(void)
{
    UINT8 i, a, tmp;
    UINT8 ckey_array[9] = {1, 0, 0, 0, 0, 0, 0, 0, 0};
    UINT16 x, y;

    for (i = 0; i < 3; i++)
    {
        if (KeyState[i] == 0) //按下
        {
            if (SendHIDKey[i][0] == 1) //标准按键
            {
                SendHID[0] = 1; // report-id

                if (SendHIDKey[i][1] != 0) //该键使用了组合键才使用此位
                {
                    SendHID[1] |= SendHIDKey[i][1];
                }

                SendHID[3 + i] = SendHIDKey[i][3]; //标准按键
            }
            else if (SendHIDKey[i][0] == 2) //媒体键
            {
                Enp2BlukIn(SendHIDKey[i], 3);
                return;
            }
            else if (SendHIDKey[i][0] == 3) //鼠标模式
            {
                while (1)
                {
                    Enp2BlukIn(SendHIDKey[i], 5);
                    printf("[DEBUG]mouse\n");
                    mDelaymS(200);

                    if (KeyState[i] == 1)
                    {
                        Enp2BlukIn(ReleaseHIDKey[i], 5);
                        printf("[DEBUG]mouse Release\n");
                        break;
                    }
                }

                return;
            }
            else if (SendHIDKey[i][0] == 5) //触摸模式
            {

                if (SendHIDKey[i][7] == 4) // osu轮盘模式
                {
                    while (1)
                    {
                        draw_circle(SendHIDKey[i][6], SendHIDKey[i][1], SendHIDKey[i][2]);
                        printf("[DEBUG]Touch draw_circle\n");

                        if (KeyState[i] == 1)
                        {
                            break;
                        }
                    }
                    return;
                }

                printf("[DEBUG]Touch \n");

                x = SendHIDKey[i][3] * 255 + SendHIDKey[i][2];
                y = SendHIDKey[i][5] * 255 + SendHIDKey[i][4];
                printf("[DEBUG]XY 0x%02x 0x%02x \n", (UINT16)x, (UINT16)y);
                touch_drive(SendHIDKey[i][1], x, y);

                mDelaymS(SendHIDKey[i][6] * 10);

                x = ReleaseHIDKey[i][3] * 255 + ReleaseHIDKey[i][2];
                y = (ReleaseHIDKey[i][5] * 255) + ReleaseHIDKey[i][4];
                printf("[DEBUG]-> XY 0x%02x 0x%02x \n", (UINT16)x, (UINT16)y);
                touch_drive(ReleaseHIDKey[i][1], x, y);

                return;
            }
            else if (SendHIDKey[i][0] == 6) //模拟dial
            {
                if (SendHIDKey[i][1] == 1) //当为按钮按下
                {
                    Enp2BlukIn(SendHIDKey[i], 3);
                    return;
                }
                else
                {
                    tmp = SendHIDKey[i][3] / 10;

                    for (a = 0; a < tmp; a++)
                    {
                        Enp2BlukIn(SendHIDKey[i], 3);
                        mDelaymS(SendHIDKey[i][4]);
                    }

                    return;
                }
            }
        }
        else if (KeyState[i] == 1) //松开
        {
            if (SendHIDKey[i][0] == 1) //标准键盘
            {
                SendHID[0] = 1;

                if (SendHIDKey[i][1] != 0 && BackState[i] == 0) //该键使用了组合键才使用此位
                {
                    SendHID[1] |= SendHIDKey[i][1];
                }
                else
                {
                    SendHID[1] |= 0;
                }

                SendHID[3 + i] = 0; //标准按键
            }
            else if (SendHIDKey[i][0] == 2 && BackState[i] == 0) //媒体键松开
            {
                Enp2BlukIn(ReleaseHIDKey[i], 3);
                return;
            }
            else if (SendHIDKey[i][0] == 5 && BackState[i] == 0) //触摸模式
            {
                return;
            }
            else if (SendHIDKey[i][0] == 6 && BackState[i] == 0) //模拟dial松开
            {
                Enp2BlukIn(ReleaseHIDKey[i], 3);
                return;
            }
        }
    }

    if ((KeyState[0] && KeyState[1] && KeyState[2] == 1) && SendHID[0] == 1) //当为标准按键，且都是松开状态
    {
        SendHID[1] = 0;
    }

    //遍历标准键盘数组
    printf("[HID]IN 104K ");

    for (i = 0; i < sizeof(SendHID); i++)
    {
        printf("0X%02x ", (UINT16)SendHID[i]);
    }

    printf("len %02u \n", sizeof(SendHID));

    //发送标准键盘
    if (SendHID[1] != 0) //当为组合键
    {
        ckey_array[1] = SendHID[1];
        Enp2BlukIn(ckey_array, sizeof(ckey_array));
        mDelaymS(1); //不能连续发送，中间需要间隔时间

        for (i = 0; i < 6; i++)
        {
            if (SendHID[3 + i] != 0)
            {
                ckey_array[3 + i] = SendHID[3 + i];
                Enp2BlukIn(ckey_array, sizeof(ckey_array));
                mDelaymS(1); //不能连续发送，中间需要间隔时间
            }
        }

        for (i = 1; i < 9; i++) //清空数组
        {
            ckey_array[i] = 0;
        }
    }
    else //当为单个按键
    {
        Enp2BlukIn(SendHID, sizeof(SendHID));
    }
}

void KeyAction_long(void)
{
    UINT8 i, a, tmp;

    for (i = 0; i < 3; i++)
    {
        if (KeyState[i] == 0) //按下
        {
            if (SendHIDKey[i][0] == 6 && SendHIDKey[i][1] != 1) //当为模拟dial模式，且功能不是button
            {
                if (SendHIDKey[i][3] % 10 == 1) //允许长按
                {
                    while (1)
                    {
                        printf("[DEBUG]dial report loop\n");

                        for (a = 0; a < tmp; a++)
                        {
                            Enp2BlukIn(SendHIDKey[i], 3);
                            mDelaymS(SendHIDKey[i][4]);
                        }

                        if (KeyState[i] == 1)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
}
/*******************************************************************************
 * Function Name  : button_drive()
 * Description    : 按键检测
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void button_driver(void)
{
    UINT8 j;

    for (j = 0; j < 3; j++)
    {

        if (KeyState_time[j] > (Get_mode[6] * 100)) //长按检测
        {
            // printf("[KEY]KEY%x long press\n", j);
            KeyAction_long();
        }

        if (KeyState[j] != BackState[j]) //按键按下
        {
            KeyAction();
            LED_effect_press(j);

            if (KeyState[j] > BackState[j]) //按键松开
            {
                printf("[KEY]KEY%x", j);
                printf(" %02u ms\n", (UINT16)KeyState_time[j]);
                KeyState_time[j] = 0;
                LED_effect_release(j);
            }

            BackState[j] = KeyState[j];
        }
    }
}

void InitT0()
{
    TMOD |= 0x01; //设置定时器工作模式为模式1

    Scan_ms = Get_mode[5]; //更新扫描时间

    TH0_T = (65536 - 4000 * Scan_ms) / 256;
    TL0_T = (65536 - 4000 * Scan_ms) % 256;

    TH0 = TH0_T;
    TL0 = TL0_T;

    printf("[TIMER0]TH0 LT0 %02x %02x \n", (UINT16)TH0_T, (UINT16)TH0_T);

    TR0 = 1; // T0定时器启动
    ET0 = 1; // T0定时器中断开启
    EA = 1;
}

void key_flash(UINT8 rw)
{
    UINT16 add = 0xf000 + 0x40 * 2;
    UINT8 tmp, i, j, len, len1, len2 = 0;
    UINT8 report_data[60] = {0};
    report_data[0] = 4; // report-id

    if (rw == 0) //将flash中数据读取到数组
    {
        for (i = 0; i < 3; i++)
        {
            add = 0xf000 + 0x40 * (2 + i); //键位的report所在地址
            len = FlashReadByte(add + 1);  //获取键位的report长度

            for (j = 0; j < len * 2; j++)
            {
                tmp = FlashReadByte(add + 2 + j);

                if (j >= len)
                {
                    ReleaseHIDKey[i][j - len] = tmp;
                }
                else
                {
                    SendHIDKey[i][j] = tmp;
                }
            }
        }

        //        add = 0xf000 + 0x40 * 2;//键位1的report所在地址
        //        len = FlashReadByte(add + 1); //获取键位1的report长度

        //        for(i = 0; i < len * 2; i++)
        //        {
        //            tmp = FlashReadByte(add + 2 + i );
        //            printf("add %02x data %02x ", (UINT16)add + 2 + i,(UINT16)tmp);
        //            if(i  >= len)
        //            {
        ////                printf("ReleaseHIDKey1\n");
        //                ReleaseHIDKey[0][i - len] = tmp;
        //            }
        //            else
        //            {
        //                printf("SendHIDKey1\n");
        //                SendHIDKey[0][i] = tmp;
        //            }
        //        }
        //        add = 0xf000 + 0x40 * 3;//键位2的report所在地址
        //        len = FlashReadByte(add + 1); //获取键位2的report长度

        //        for(i = 0; i < len * 2; i++)
        //        {
        //            tmp = FlashReadByte(add + 2 + i );
        //            if(i >= len)
        //            {
        //                ReleaseHIDKey[1][i - len] = tmp;
        //            }
        //            else
        //            {
        //                SendHIDKey[1][i] = tmp;
        //            }
        //        }
        //        add = 0xf000 + 0x40 * 4;//键位3的report所在地址
        //        len = FlashReadByte(add + 1); //获取键位3的report长度
        //        for(i = 0; i < len * 2; i++)
        //        {
        //            tmp = FlashReadByte(add + 2 + i );
        //            if(i >= len)
        //            {
        //                ReleaseHIDKey[2][i - len] = tmp;
        //            }
        //            else
        //            {
        //                SendHIDKey[2][i] = tmp;
        //            }
        //        }

#ifdef DE_PRINTF

        //遍历SendHIDKey1-3 SendHIDKey1-3
        for (i = 0; i < 3; i++)
        {
            printf("[KEY]SendHIDKey[%x] =    ", (UINT8)i);

            for (j = 0; j < sizeof(SendHIDKey[i]); j++)
            {
                printf("%02x ", (UINT16)SendHIDKey[i][j]);
            }

            printf("\r\n");
            printf("[KEY]ReleaseHIDKey[%x] = ", (UINT8)i);

            for (j = 0; j < sizeof(ReleaseHIDKey[i]); j++)
            {
                printf("%02x ", (UINT16)ReleaseHIDKey[i][j]);
            }

            printf("\r\n");
        }

#endif
    }
    else if (rw == 1) //向flash写入数据
    {
        //判断是哪个按键
        if (HID_OUT_report[2] == 2)
        {
            add = 0xf000 + 0x40 * 2;
        }
        else if (HID_OUT_report[2] == 3)
        {
            add = 0xf000 + 0x40 * 3;
        }
        else if (HID_OUT_report[2] == 4)
        {
            add = 0xf000 + 0x40 * 4;
        }

        //擦除页
        printf("[FLASH W]DataFlash Erase add=%02x...\n", (UINT16)add);
        tmp = FlashErasePage(add);

        if (tmp != 0x00)
        {
            printf("[FLASH W]DataFlash Erase error code=%02x \n", (UINT16)tmp);
        }

        len = HID_OUT_report[5] * 2 + 2; //获取报文长度，决定写入长度

        //循环写入
        for (i = 0; i < len; i++)
        {
            tmp = FlashProgByte(add + i, HID_OUT_report[i + 4]); //从report的第三位开始写入
            printf("[FLASH W]DataFlash Write add=%02x data=%02x \n", (UINT16)add + i, (UINT16)HID_OUT_report[i + 4]);

            if (tmp != 0x00)
            {
                printf("[FLASH W]DataFlash error add=%02x code=%02x \n", (UINT16)add + i, (UINT16)tmp);
            }
        }
    }
    else if (rw == 2) //发送flash中数据到上位机
    {
        report_data[1] = HID_OUT_report[1];
        report_data[2] = HID_OUT_report[2];

        //判断是哪个按键
        if (HID_OUT_report[2] == 2)
        {
            add = 0xf000 + 0x40 * 2;
        }
        else if (HID_OUT_report[2] == 3)
        {
            add = 0xf000 + 0x40 * 3;
        }
        else if (HID_OUT_report[2] == 4)
        {
            add = 0xf000 + 0x40 * 4;
        }
        else if (HID_OUT_report[2] == 0xff) //读取全部键位
        {
            add = 0xf000 + 0x40 * 2;
            len = FlashReadByte(add + 1); //获取该键位的report长度
            len = len * 2 + 2;            //获取总长度

            report_data[4] = FlashReadByte(0xf000 + 0x40 * 2);
            report_data[5] = FlashReadByte(0xf000 + 0x40 * 3);
            report_data[6] = FlashReadByte(0xf000 + 0x40 * 4);

            //将Flash中的数据数据读入内存
            for (i = 0; i < len; i++)
            {
                tmp = FlashReadByte(add + i);
                report_data[i + 4 + 3] = tmp;
            }

            len2 = len;

            add = 0xf000 + 0x40 * 3;
            len = FlashReadByte(add + 1); //获取该键位的report长度
            len = len * 2 + 2;            //获取总长度

            //将Flash中的数据数据读入内存
            for (i = 0; i < len; i++)
            {
                tmp = FlashReadByte(add + i);
                report_data[i + 4 + 3 + len2] = tmp;
            }

            len1 = len;

            add = 0xf000 + 0x40 * 4;
            len = FlashReadByte(add + 1); //获取该键位的report长度
            len = len * 2 + 2;            //获取总长度

            //将Flash中的数据数据读入内存
            for (i = 0; i < len; i++)
            {
                tmp = FlashReadByte(add + i);
                report_data[i + 4 + 3 + len2 + len1] = tmp;
            }

            printf("[DEBUG] len2 len1 len %02x %02x %02x \r\n", (UINT16)len2, (UINT16)len1, (UINT16)len);

            //            printf("[HID]OUT ");
            //            for (i = 0; i < sizeof(report_data); i++)
            //            {
            //                printf("0x%02x ", report_data[i]);              //遍历OUT数据
            //            }
            //            printf("len=%02u\n", (UINT8)sizeof(report_data));

            Enp2BlukIn(report_data, sizeof(report_data) + 1); //发送到上位机
            printf("[HID]KEY reply\r\n");

            return;
        }

        len = FlashReadByte(add + 1); //获取该键位的report长度

        //将Flash中的数据数据读入内存
        for (i = 0; i < len * 2; i++)
        {
            tmp = FlashReadByte(add + i);
            report_data[i + 4] = tmp;
        }

        Enp2BlukIn(report_data, sizeof(report_data) + 1); //发送到上位机
        printf("[HID]KEY reply\r\n");
    }
}

void mode_flash(UINT8 rw)
{
    UINT16 add = 0xf000;
    UINT8 tmp, i = 0;

    UINT8 report_data[60] = {0};
    report_data[0] = 4;

    if (rw == 0) //将flash中数据读取到数组
    {
        //将Flash中的RGB数据数据读入内存
        for (i = 0; i < sizeof(Get_mode); i++)
        {
            tmp = FlashReadByte(add + i);
            //            printf("[FLASH R]add=%02x data=%02x \r\n", (UINT16)add + i, (UINT16)tmp);
            Get_mode[i] = tmp;
        }

        printf("[MODE RAM] ");

        for (i = 0; i < sizeof(Get_mode); i++)
        {
            printf("%02x, ", (UINT16)Get_mode[i]);
        }

        printf("\r\n");
    }
    else if (rw == 1) //向flash写入数据
    {
        //擦除页
        printf("[FLASH W]DataFlash Erase add=%02x...\n", (UINT16)add);
        tmp = FlashErasePage(add); //擦除页

        if (tmp != 0x00)
        {
            printf("[FLASH W]DataFlash Erase error code=%02x \n", (UINT16)tmp);
        }

        //循环写入
        for (i = 0; i < 12; i++)
        {
            tmp = FlashProgByte(add + i, HID_OUT_report[i + 4]); //写入FLASH HID_OUT_report[i + 4] ，跳过前4位
            printf("[FLASH W]DataFlash Write add=%02x data=%02x \n", (UINT16)add + i, (UINT16)HID_OUT_report[i + 4]);

            if (tmp != 0x00)
            {
                printf("[FLASH W]DataFlash error add=%02x code=%02x \n", (UINT16)add + i, (UINT16)tmp);
            }
        }
    }
    else if (rw == 2) //发送flash中数据到上位机
    {
        report_data[1] = HID_OUT_report[1];
        report_data[2] = HID_OUT_report[2];

        //将Flash中的数据数据读入内存
        for (i = 0; i < sizeof(Get_mode); i++)
        {
            tmp = FlashReadByte(add + i);
            report_data[i + 4] = tmp;
        }

        Enp2BlukIn(report_data, sizeof(report_data) + 1); //发送到上位机
        printf("[HID]MODE reply\r\n");
    }
}
void report_handle(void)
{
    UINT8 i;
    UINT16 add = 0xf000;
    UINT8 report_data[60] = {0};
    UINT16 sleep_time = 0;

    //    printf("[HID-handle]OUT ");
    //    for (i = 0; i < sizeof(HID_OUT_report); i++)
    //    {
    //        printf("0x%02x ", (UINT16)HID_OUT_report[i]);             //遍历OUT数据
    //    }
    //    printf("len=%u\n", sizeof(HID_OUT_report));

    if (HID_OUT_report[1] == 1) // w
    {
        printf("[HID-handle]write\r\n");

        if (HID_OUT_report[2] == 0) // mode
        {
            mode_flash(1);
        }
        else if (HID_OUT_report[2] == 1) // led
        {
            led_flash(1, HID_OUT_report, 0);
        }
        else if (HID_OUT_report[2] == 2 || HID_OUT_report[2] == 3 || HID_OUT_report[2] == 4 || HID_OUT_report[2] == 0xff) // key
        {
            key_flash(1);
        }
    }
    else if (HID_OUT_report[1] == 0) // r
    {
        printf("[HID REPLY]Read\r\n");

        if (HID_OUT_report[2] == 0) // mode
        {
            mode_flash(2);
        }
        else if (HID_OUT_report[2] == 1) // led
        {

            add = 0xf000 + 0x40;
            report_data[0] = 4;
            report_data[1] = 1;
            report_data[2] = 1;

            //将Flash中的RGB数据数据读入内存
            for (i = 0; i < 54; i++)
            {
                report_data[i + 4] = FlashReadByte(add + i);
                //                printf("[FLASH R]%02x %02x \r\n", (UINT16)add + i, (UINT16)report_data[i]);
            }

            Enp2BlukIn(report_data, sizeof(report_data) + 1); //发送到上位机
            printf("[FLASH R]RGB DATA\r\n");
        }
        else if (HID_OUT_report[2] == 2 || HID_OUT_report[2] == 3 || HID_OUT_report[2] == 4 || HID_OUT_report[2] == 0xff) // key
        {
            key_flash(2);
        }
    }
    else if (HID_OUT_report[1] == 2) // reset
    {
        printf("[SYS]Reset\r\n");
        CH549SoftReset();
    }
    else if (HID_OUT_report[1] == 4) // reload
    {
        mode_flash(0);                 //读取flash中mode数据
        InitT0();                      //按键扫描间隔
        led_flash(0, 0, Get_mode[3]);  //读取flash中led数据
        LED_effect_init(Get_mode[11]); //    LED_effect_init(Get_mode[11]); //设置灯效组
        key_flash(0);                  //读取flash中key数据
        wd2812b_driver();              // wd2812b加载
        printf("[SYS]CONFIG RELOADED\r\n");
    }
    else if (HID_OUT_report[1] == 5) // command
    {
        printf("[SYS]Command Mode\r\n");

        if (HID_OUT_report[2] == 1) // led控制
        {
            change_LED_BRG(HID_OUT_report);
            wd2812b_driver(); // wd2812b加载
        }
    }
    else if (HID_OUT_report[1] == 6) // copy
    {
        sleep_time = HID_OUT_report[2]; //高8位
        sleep_time = sleep_time << 8;
        sleep_time |= HID_OUT_report[3]; //低8位

        printf("[HID]COPY Mode Sleep: 0x%04x ms\r\n", (UINT16)sleep_time);
        printf("[HID]COPY: ");

        for (i = 0; i < sizeof(report_data); i++)
        {
            report_data[i] = HID_OUT_report[i + 4]; //去除前4位，前4位为描述指令
            printf("%02x ", (UINT16)report_data[i]);
        }

        printf("\r\n");
        mDelaymS(sleep_time); //延迟发送
        printf("SEND");
        Enp2BlukIn(report_data, sizeof(report_data) + 1); //发送到上位机
    }
}

void main()
{
    CfgFsys();    // CH549时钟选择配置
    mDelaymS(10); //修改主频等待内部晶振稳定,必加
    mInitSTDIO(); //串口0初始化
#ifdef DE_PRINTF
    printf("[SYS]CLOCK_CFG=0x%02x\r\n", (UINT16)CLOCK_CFG);
    printf("[SYS]PowerOn ...\r\n");
    printf(".......................\n");
    printf("    Keyboard\n");
    printf("    Nano\n");
    printf("    Ver0.2.3a\n");
    printf(".......................\n");
#endif
    USBDeviceInit(); // USB设备模式初始化
    EA = 1;          //允许单片机中断

    GPIO_Init(PORT1, PIN4, MODE3); // P1.4上拉输入 KEY1
    GPIO_Init(PORT1, PIN5, MODE3); // P1.5上拉输入 KEY2
    GPIO_Init(PORT1, PIN6, MODE3); // P1.6上拉输入 KEY3
    GPIO_Init(PORT2, PIN4, MODE3); // P2.4上拉 RGB

    mDelaymS(600); //等待USB中断完成

    mode_flash(0);                 //读取flash中mode数据
    InitT0();                      //按键扫描间隔
    led_flash(0, 0, Get_mode[3]);  //读取flash中led数据
    LED_effect_init(Get_mode[11]); //    LED_effect_init(Get_mode[11]); //设置灯效组
    key_flash(0);                  //读取flash中key数据
    wd2812b_driver();              // wd2812加载

    while (1)
    {
        button_driver();
        LED_effects_fades();

        if (HID_OUT_report[0] == 0x04)
        {
            report_handle();
            HID_OUT_report[0] = 0x00;
        }
    }
}

/*******************************************************************************
* Function Name  : mTimer0Interrupt()
* Description    : CH549定时计数器0定时计数器中断 处理函数
                    定时器溢出时间由Scan_ms定义
*******************************************************************************/
void mTimer0Interrupt(void) interrupt 1
{
    //重启定时器
    TH0 = TH0_T;
    TL0 = TH0_T;
    button_read();
}