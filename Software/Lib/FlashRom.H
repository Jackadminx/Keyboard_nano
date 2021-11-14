#ifndef __DATAFLASH_H__
#define __DATAFLASH_H__
#include "CH549.H"
#include "DEBUG.H"
/*********************** Flash空间划分 *****************************
*   0xFFFD  ---
*            |                   Code Flash(BOOT区域3K)
*   0xF400  ---
*   0xF3FF  ---
*            |                   Data Flash（1K）
*   0xF000  ---
*   0xEFFF  ---
*            |                   Code Flash（用户代码区60K）
*   0x0000  ---
******************************************************************/
/* 子函数调用 */
/* Code Flash和Data Flash操作 */
extern UINT8 FlashErasePage( UINT16 Addr ) ;
extern UINT8 FlashProgByte( UINT16 Addr, UINT8 Data );
extern UINT8 FlashProgPage( UINT16 Addr, PUINT8X Buf, UINT8 len );
extern UINT8 FlashReadBuf(UINT16 Addr, PUINT8 buf, UINT16 len);
extern UINT8 FlashReadByte(UINT16 Addr);
/* OTP区与ReadOnly区操作 */
extern UINT8  FlashProgOTPbyte( UINT8 Addr, UINT8 Data );
extern UINT32 FlashReadOTPword( UINT8 Addr );
#endif
