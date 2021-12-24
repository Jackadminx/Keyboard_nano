#include "RGB.H"

#define LOOP_NUMS 2 //循环计数

// UINT32 LED_BRG[3] = {0x0000ffff, 0x00ff0000, 0x000000ff};
xdata volatile RGBColor colors[3] = {0};
xdata volatile RGBColor colors_tmp[3] = {0};
UINT8 LED_effect_mode = 0;                               //当前灯效模式
UINT16X loop_num[3] = {LOOP_NUMS, LOOP_NUMS, LOOP_NUMS}; //循环计数
UINT8 keynum[3] = {0, 0, 0};                             //记录灯效按键

// sbit RGB = P2 ^ 4; // LED_data Pin

// RGB数据FLASH操作
// rw读写 HID_OUT_report写入数组 mode灯光组读模式有效
void led_flash(UINT8 rw, UINT8X HID_OUT_report[61], UINT8 mode) {
  UINT16 add = 0xf000 + 0x40;
  UINT8 tmp, i, j = 0;
  UINT32 tmpL = 0;

  if (rw == 0) // r
  {
    //遍历FLASH中的RGB数据
    //        for(i = 0; i < 54; i++)
    //        {
    //            tmp = FlashReadByte(add + i);
    ////            printf("[FLASH R]%02x %02x \r\n", (UINT16)add + i,
    ///(UINT16)tmp);
    //        }

    if (mode == 0) //灯光组为0时RGB关闭
    {
      //   LED_BRG[0] = 0;
      //   LED_BRG[1] = 0;
      //   LED_BRG[2] = 0;

      colors[0].B = 0;
      colors[0].R = 0;
      colors[0].G = 0;

      colors[1].B = 0;
      colors[1].R = 0;
      colors[1].G = 0;

      colors[2].B = 0;
      colors[2].R = 0;
      colors[2].G = 0;
    } else {
      //将Flash中的RGB数据数据读入内存 只读取使用的那组
      for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
          tmpL = tmpL << 8; // 8位
          tmpL |= FlashReadByte(add + i * 3 + j + (mode - 1) * 9);
          //                    printf("[Debug]add=%hx data=%lx \r\n",
          //                    (UINT16)add + i * 3 + j + mode * 9,
          //                    (UINT32)tmpL); //打印FLASH中的地址和数据
          //                    使用的RGB组
        }

        // LED_BRG[i] = tmpL;
        tmpL = 0;
        // printf("[RGB RAM] %d 0x%lx \n", i + 1, LED_BRG[i]); //打印变量LED_BRG

        colors[i].B = FlashReadByte(add + i * 3 + 0 + (mode - 1) * 9);
        colors[i].R = FlashReadByte(add + i * 3 + 1 + (mode - 1) * 9);
        colors[i].G = FlashReadByte(add + i * 3 + 2 + (mode - 1) * 9);
        // printf("[DEBUG]B R G %02x %02x %02x\n", (UINT8)colors[i].B,
        // (UINT8)colors[i].R, (UINT8)colors[i].G);
      }
    }

  } else if (rw == 1) // w
  {
    //擦除页
    EA = 0;
    printf("[FLASH W]DataFlash Erase add=%02x...\n", (UINT16)add);
    tmp = FlashErasePage(add); //擦除页

    if (tmp != 0x00) {
      printf("[FLASH W]DataFlash Erase error code=%02x \n", (UINT16)tmp);
    }

    printf("[FLASH W]RGB DataFlash Write add=%02x-%02x \r\n", (UINT16)add,
           (UINT16)add + 53);

    for (i = 0; i < 54; i++) //循环写入
    {
      tmp = FlashProgByte(add + i,
                          HID_OUT_report[i + 4]); //写入 0xF001 i+4去掉头4位指令
      //            printf("[FLASH W]DataFlash Write add=%02x data=%02x \n",
      //            (UINT16)add + i, (UINT16)HID_OUT_report[i + 4]);

      if (tmp != 0x00) {
        printf("[FLASH W]DataFlash error add=%02x code=%02x \n",
               (UINT16)add + i, (UINT16)tmp);
      }
    }

    EA = 1;
  }
}

// 驱动wd2812b
void wd2812b_driver(void) {
  EA = 0;
  LED_SendRGBData(colors, 3);
  EA = 1;
  //   printf("[DEBUG]LED Driver \n");
}

// 通过上位机修改RGB灯效，不保存到FLASH
void change_LED_BRG(UINT8X HID_OUT_report[61]) {
  colors[0].B = HID_OUT_report[4];
  colors[0].R = HID_OUT_report[5];
  colors[0].G = HID_OUT_report[6];

  colors[1].B = HID_OUT_report[7];
  colors[1].R = HID_OUT_report[8];
  colors[1].G = HID_OUT_report[9];

  colors[2].B = HID_OUT_report[10];
  colors[2].R = HID_OUT_report[11];
  colors[2].G = HID_OUT_report[12];
}

//灯效初始化
void LED_effect_init(UINT8 LED_effect_mode_1) {
  UINT8 i;
  LED_effect_mode = LED_effect_mode_1;

  if (LED_effect_mode == 0 ) //关闭灯效,不改变灯效
  {
    return;
  }

  for (i = 0; i < 3; i++) //初始化，关闭灯光
  {
    colors_tmp[i].B = colors[i].B;
    colors_tmp[i].R = colors[i].R;
    colors_tmp[i].G = colors[i].G;
    if (LED_effect_mode == 1 || LED_effect_mode == 2 || LED_effect_mode == 3) {
      colors[i].B = 0;
      colors[i].R = 0;
      colors[i].G = 0;
    }
  }
  wd2812b_driver();
}

void LED_effect_press(UINT8 i) {

  if (LED_effect_mode == 0) {
    return;
  }

  if (LED_effect_mode == 1) {
    //点亮对应的led mode1
    colors[i].B = colors_tmp[i].B;
    colors[i].R = colors_tmp[i].R;
    colors[i].G = colors_tmp[i].G;
  } else if (LED_effect_mode == 2) {
    //点亮对应的led mode2
    colors[i].B = 0x7f;
    colors[i].R = 0x7f;
    colors[i].G = 0x7f;
    keynum[i] = 0;
  } else if (LED_effect_mode == 3) {
    //点亮对应的led mode3
    colors[i].B = rand() % 0x3f + 0x6f;
    colors[i].R = rand() % 0x3f + 0x6f;
    colors[i].G = rand() % 0x3f + 0x6f;
    keynum[i] = 0;
  } else if (LED_effect_mode == 4) {
    //熄灭对应的LED mode4
    colors[i].B = 0;
    colors[i].R = 0;
    colors[i].G = 0;
  }

  wd2812b_driver();
}

void LED_effect_release(UINT8 i) {

  if (LED_effect_mode == 0) {
    return;
  }

  if (LED_effect_mode == 1) {
    //关闭对应的led mode1
    colors[i].B = 0;
    colors[i].R = 0;
    colors[i].G = 0;
    wd2812b_driver();
  } else if (LED_effect_mode == 2 || LED_effect_mode == 3) {
    keynum[i] = 1;
  } else if (LED_effect_mode == 4) {
    colors[i].B = colors_tmp[i].B;
    colors[i].R = colors_tmp[i].R;
    colors[i].G = colors_tmp[i].G;
    wd2812b_driver();
  }
}

void LED_effects_fades(void) {
  UINT8 i;

  for (i = 0; i < 3; i++) {
    if (keynum[i] == 1) //该按键enable灯效
    {

      if (LED_effect_mode == 2) {
        if (colors[i].B == 0 || colors[i].G == 0 || colors[i].R == 0) {
          keynum[i] = 0;

          colors[i].B = 0;
          colors[i].R = 0;
          colors[i].G = 0;
          wd2812b_driver();
        }

        if (loop_num[i]) {
          loop_num[i]--;
        } else {
          loop_num[i] = LOOP_NUMS;
          colors[i].B--;
          colors[i].R--;
          colors[i].G--;
          wd2812b_driver();
          //   printf("[DEBUG] M2 B R G %02x %02x %02x\n", (UINT8)colors[i].B,
          //   (UINT8)colors[i].R, (UINT8)colors[i].G);
        }
      } else if (LED_effect_mode == 3) {
        if (colors[i].B <= 0x0f || colors[i].G <= 0x0f || colors[i].R <= 0x0f) {
          keynum[i] = 0;

          colors[i].B = 0;
          colors[i].R = 0;
          colors[i].G = 0;
          wd2812b_driver();
        }

        if (loop_num[i]) {
          loop_num[i]--;
        } else {
          loop_num[i] = LOOP_NUMS;
          colors[i].B--;
          colors[i].R--;
          colors[i].G--;
          wd2812b_driver();
          //   printf("[DEBUG] M3 B R G %02x %02x %02x\n", (UINT8)colors[i].B,
          //   (UINT8)colors[i].R, (UINT8)colors[i].G);
        }
      }
    }
  }
}
