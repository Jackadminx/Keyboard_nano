#include "RGB.H"

UINT32 LED_BRG[3] = {0x0000ffff, 0x00ff0000, 0x000000ff};

sbit RGB = P2 ^ 4; //LED_data Pin

void led_flash(UINT8 rw, UINT8X HID_OUT_report[61], UINT8 mode) //rw读写 HID_OUT_report写入数组 mode灯光组读模式有效
{
    UINT16 add = 0xf000 + 0x40;
    UINT8 tmp, i, j = 0;
    UINT32 tmpL = 0;

    if(rw == 0) //r
    {
        //遍历FLASH中的RGB数据
//        for(i = 0; i < 54; i++)
//        {
//            tmp = FlashReadByte(add + i);
////            printf("[FLASH R]%02x %02x \r\n", (UINT16)add + i, (UINT16)tmp);
//        }

        if(mode == 0) //灯光组为0时RGB关闭
        {
            LED_BRG[0] = 0;
            LED_BRG[1] = 0;
            LED_BRG[2] = 0;
        }
        else
        {
            //将Flash中的RGB数据数据读入内存 只读取使用的那组
            for(i = 0; i < 3; i++)
            {
                for(j = 0; j < 3; j++)
                {
                    tmpL =  tmpL << 8; //8位
                    tmpL |= FlashReadByte(add + i * 3 + j + (mode - 1) * 9);
//                    printf("[Debug]add=%hx data=%lx \r\n", (UINT16)add + i * 3 + j + mode * 9, (UINT32)tmpL); //打印FLASH中的地址和数据 使用的RGB组
                }

                LED_BRG[i] = tmpL;
                tmpL = 0;
                printf("[RGB RAM] %d 0x%lx \r\n", (UINT8)i + 1, (UINT32)LED_BRG[i]); //打印变量LED_BRG
            }

        }


//        //遍历内存中的RGB数据
//        for(i = 0; i < 3; i++)
//        {
//            for(j = 0; j < 4; j++)
//            {
//                tmpL = LED_BRG[i];
//                tmp = (tmpL >> (j * 8)) & 0xff;
//                printf("[RGB RAM]%02x %02x \r\n", (UINT16)i + 1, (UINT16)tmp);
//            }
//        }

    }
    else if(rw == 1) //w
    {
        //擦除页
        EA = 0;
        printf("[FLASH W]DataFlash Erase add=%02x...\n", (UINT16)add);
        tmp = FlashErasePage( add ) ;                                             //擦除页

        if(tmp != 0x00)
        {
            printf("[FLASH W]DataFlash Erase error code=%02x \n", (UINT16)tmp);
        }

        printf("[FLASH W]RGB DataFlash Write add=%02x-%02x \r\n", (UINT16)add, (UINT16)add + 53);

        for(i = 0; i < 54; i++)//循环写入
        {
            tmp = FlashProgByte( add + i, HID_OUT_report[i + 4] );																		//写入 0xF001 i+4去掉头4位指令
//            printf("[FLASH W]DataFlash Write add=%02x data=%02x \n", (UINT16)add + i, (UINT16)HID_OUT_report[i + 4]);

            if(tmp != 0x00)
            {
                printf("[FLASH W]DataFlash error add=%02x code=%02x \n", (UINT16)add + i, (UINT16)tmp);
            }
        }

        EA = 1;
    }

}
void send_0(void)
{
    RGB = 1;
    Wait400ns;
    RGB = 0;
    Wait800ns;
}

void send_1(void)
{
    RGB = 1;
    Wait800ns;
    RGB = 0;
    Wait400ns;
}

void send_reset(void)
{
    RGB = 0;
    Wait800ns;
    Wait800ns;
}



void wd2812b_driver(void)
{
    UINT8 *a; //声明指针变量
    UINT8 i = 0;
    UINT8 LED_hex[73];
    UINT32 tmp = 1;

    printf(" ......\r\n[LED]Color Bin B R G  \r\nLED1: ");

    // hex to bin
    // 将3个LED的BRG值转换为二进制填充至数组中
    for(i = 24; i > 0 ; i--)
    {
        if((LED_BRG[0] & (tmp << i - 1)) == 0)
        {
            LED_hex[i - 1] = 0;
            printf("0");
        }
        else
        {
            LED_hex[i - 1] = 1;
            printf("1");
        }

        tmp = 1;
    }

    printf("\r\nLED2: ");

    for(i = 24; i > 0 ; i--)
    {
        if((LED_BRG[1] & (tmp << i - 1)) == 0)
        {
            LED_hex[i + 24 - 1] = 0;
            printf("0");
        }
        else
        {
            LED_hex[i + 24 - 1] = 1;
            printf("1");
        }

        tmp = 1;

    }

    printf("\r\nLED3: ");

    for(i = 24; i > 0 ; i--)
    {
        if((LED_BRG[2] & (tmp << i - 1)) == 0)
        {
            LED_hex[i + 48 - 1] = 0;
            printf("0");
        }
        else
        {
            LED_hex[i + 48 - 1] = 1;
            printf("1");
        }

        tmp = 1;
    }

    printf("\r\n ......\r\n");



    a = LED_hex; //将指针赋予首元素地址

    // run
    // 将数组转换为电平
    EA = 0;//关闭中断
    send_reset();

    for(i = 0; i < 72; i++)
    {
        if(i == 24 || i == 48)
        {
            send_reset();
        }

//    printf("%02x %02x \n", (UINT16) *(a+i), (UINT16)LED_hex[i]); //使用指针读取数组
//        if(LED_hex[i])
        if(*(a + i)) //使用指针读取数组
        {
            send_1();
        }
        else
        {
            send_0();
        }
    }

    send_reset();
    EA = 1;

}

void change_LED_BRG(UINT8X HID_OUT_report[61])
{
    UINT8 i;
    UINT32 tmpL = 0;
    {
        //将Flash中的RGB数据数据读入内存 只读取使用的那组
        for(i = 4; i < 13; i++)
        {
            tmpL =  tmpL << 8; //8位
            tmpL |= HID_OUT_report[i]; //+4 去掉开头4位指令
            printf("[Debug]i=%x data=%02x \r\n", (UINT16)i, (UINT16)HID_OUT_report[i]); //打印FLASH中的地址和数据 使用的RGB组


            if(i % 3 == 0) //i从4开始 当等于6 9 12时运行下面代码
            {
                LED_BRG[i / 3 - 2] = tmpL; //将i转换为 0 1 2
                tmpL = 0;

            }
        }
    }
}