//改了EP6的缓冲由1024/4倍改为512/2倍，EP6只有2/4倍
#pragma NOIV                    // Do not generate interrupt vectors
//-----------------------------------------------------------------------------
//   File:      tcxmaster.c
//   Contents:  Hooks required to implement USB peripheral function.
//              Code written for FX2 56-pin REVD...
//              This firmware is used to test the FX ext. master CY3682 DK
//   Copyright (c) 2001 Cypress Semiconductor All rights reserved
//加上了SLCS=pa.7
//-----------------------------------------------------------------------------
#include "fx2.h"
#include "fx2regs.h"
#include "fx2sdly.h"            // SYNCDELAY macro
extern BOOL GotSUD;             // Received setup data flag
extern BOOL Sleep;
extern BOOL Rwuen;
extern BOOL Selfpwr;

BYTE Configuration;             // Current configuration
BYTE AlternateSetting;          // Alternate settings

// EZUSB FX2 PORTA = slave fifo enable(s), when IFCFG[1:0]=11
sbit PA0 = IOA ^ 0;             // alt. func., INT0#
//OEA=(OEA|0x03);
//sbit PA1 = IOA ^ 1;             // alt. func., INT1#
// sbit PA2 = IOA ^ 2;          // is SLOE
//sbit PA3 = IOA ^ 3;             // alt. func., WU2
// sbit PA4 = IOA ^ 4;          // is FIFOADR0
// sbit PA5 = IOA ^ 5;          // is FIFOADR1
// sbit PA6 = IOA ^ 6;          // is PKTEND
// sbit PA7 = IOA ^ 7;          // is FLAGD

// EZUSB FX2 PORTC i/o...       port NA for 56-pin FX2
// sbit PC0 = IOC ^ 0;
// sbit PC1 = IOC ^ 1;
// sbit PC2 = IOC ^ 2;
// sbit PC3 = IOC ^ 3;
// sbit PC4 = IOC ^ 4;
// sbit PC5 = IOC ^ 5;
// sbit PC6 = IOC ^ 6;
// sbit PC7 = IOC ^ 7;

// EZUSB FX2 PORTB = FD[7:0], when IFCFG[1:0]=11
// sbit PB0 = IOB ^ 0;
// sbit PB1 = IOB ^ 1;
// sbit PB2 = IOB ^ 2;
// sbit PB3 = IOB ^ 3;
// sbit PB4 = IOB ^ 4;
// sbit PB5 = IOB ^ 5;
// sbit PB6 = IOB ^ 6;
// sbit PB7 = IOB ^ 7;

// EZUSB FX2 PORTD = FD[15:8], when IFCFG[1:0]=11 and WORDWIDE=1
//sbit PD0 = IOD ^ 0;
//sbit PD1 = IOD ^ 1;
//sbit PD2 = IOD ^ 2;
//sbit PD3 = IOD ^ 3;
//sbit PD4 = IOD ^ 4;
//sbit PD5 = IOD ^ 5;
//sbit PD6 = IOD ^ 6;
//sbit PD7 = IOD ^ 7;

// EZUSB FX2 PORTE is not bit-addressable...

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//void TD_Init( void )用于配置端点及大小、数据总线位数、复位FIFO、FIFO相关输出信号(Empty, Full, Programmable)。
//-----------------------------------------------------------------------------
void TD_Init( void )
{ // Called once at startup

  CPUCS = 0x10;                 // CLKSPD[1:0]=10, for 48MHz operation  cpu时钟速率为48MHZ

//有些寄存器中间必须用SYNCDELAY来延时

  IFCONFIG = 0xCB;				//1100，1011  .7=1：内部时钟；.6=1：48MHZ时钟；.5=0内部时钟不输出；.4=0：正常极性 .3=1：异步方式；【.1，.0】=11：从属FIFO模式
  SYNCDELAY;
  // IFCLKSRC=1   , FIFOs executes on internal clk source
  // xMHz=1       , 48MHz internal clk rate
  // IFCLKOE=0    , Don't drive IFCLK pin signal at 48MHz
  // IFCLKPOL=0   , Don't invert IFCLK pin signal from internal clk
  // ASYNC=1      , master samples asynchronous
  // GSTATE=0     , Don't drive GPIF states out on PORTE[2:0], debug WF
  // IFCFG[1:0]=11, FX2 in slave FIFO mode


  // Registers which require a synchronization delay, see section 15.14
  // FIFORESET        FIFOPINPOLAR
  // INPKTEND         OUTPKTEND
  // EPxBCH:L         REVCTL
  // GPIFTCB3         GPIFTCB2
  // GPIFTCB1         GPIFTCB0
  // EPxFIFOPFH:L     EPxAUTOINLENH:L
  // EPxFIFOCFG       EPxGPIFFLGSEL
  // PINFLAGSxx       EPxFIFOIRQ
  // EPxFIFOIE        GPIFIRQ
  // GPIFIE           GPIFADRH:L
  // UDMACRCH:L       EPxGPIFTRIG
  // GPIFTRIG
  
  // Note: The pre-REVE EPxGPIFTCH/L register are affected, as well...
  //      ...these have been replaced by GPIFTC[B3:B0] registers

  EP6CFG = 0xE2;                   ////激活、IN,blk;b3=size:1=1024,0=512;b1b0=00:4倍缓冲,10=2倍
  SYNCDELAY;



  FIFORESET = 0x80;             // activate NAK-ALL to avoid race conditions不应答所有从主机来的传输
  SYNCDELAY;                    // see TRM section 15.14
  FIFORESET = 0x02;             // reset, FIFO 2复位FIFO2：恢复端点FIFO标志和字节计数为它们的默认状态
  SYNCDELAY;                    // 
  FIFORESET = 0x04;             // reset, FIFO 4
  SYNCDELAY;                    // 
  FIFORESET = 0x06;             // reset, FIFO 6
  SYNCDELAY;                    // 
  FIFORESET = 0x08;             // reset, FIFO 8
  SYNCDELAY;                    // 
  FIFORESET = 0x00;             // deactivate NAK-ALL  恢复正常操作

  SYNCDELAY;


  PINFLAGSAB = 0x6B;            // FLAGA - fixed EP6EF端点6空标志, FLAGB - fixed EP6PF端点6可编程标志
  SYNCDELAY;
  PINFLAGSCD = 0x4E;            // FLAGC - fixed EP6FF端点6满标志, FLAGD - fixed EP6EF端点6空标志
  SYNCDELAY;
  
  PORTACFG= 0x40;              //PORTACFG.6=1（端点复用）且IFCFG1:0=11选择pa.7为slcs
  SYNCDELAY;
  FIFOPINPOLAR = 0x01;          // all signals active low 控制引脚信号都是低电平有效：PKEND、sloe、slrd、slwr、ef.；ff高有效
  SYNCDELAY;
 

//当autoout＝1时，主机和外部设备直接连接，68013的8051失效，此时外部设备是master，
//也就是说读写时序要有外部设备产 生，不能向68013写数据是不可能的，
//原则上说至少可以写两个512的数据进去，然后就写不下去了，你可以验证一下是不是这样 


  // handle the case where we were already in AUTO mode...
//  EP2FIFOCFG = 0x00;            // .4=AUTOOUT=0只要缓冲区填入USB数据,端点中断就被激活。缓冲区和端点FIFO的连接由固件程序控制。
								// .0=WORDWIDE=0：8位字长 
								//********.2=0：零长度包将不能在PKTEND上发送*****---->所以只能EPxAUTOINLENH：L中定义的长度一致 
//								
 // SYNCDELAY;
  
 // EP2FIFOCFG = 0x10;            // AUTOOUT=1：只要缓冲区填入USB数据，缓冲区就自动绕过CPU直接向端点FIFO提交数据。, WORDWIDE=0
 // SYNCDELAY;
  
  // handle the case where we were already in AUTO mode...
 // EP4FIFOCFG = 0x00;            // AUTOOUT=0, WORDWIDE=0
 // SYNCDELAY;
  
 // EP4FIFOCFG = 0x10;            // AUTOOUT=1, WORDWIDE=0
 // SYNCDELAY;

  EP6FIFOCFG = 0x09;            // AUTOIN=1, ZEROLENIN=0 ：bu允许0长度的包->只要PKTEND引脚被激活且当前包中为空时，提交0长度包, WORDWIDE=1
  SYNCDELAY;
 // EP8FIFOCFG = 0x09;            // AUTOIN=1, ZEROLENIN=1, WORDWIDE=0
}

void TD_Poll( void )
{ // Called repeatedly while the device is idle

  // ...nothing to do... slave fifo's are in AUTO mode...

}

BOOL TD_Suspend( void )          
{ // Called before the device goes into suspend mode
   return( TRUE );
}

BOOL TD_Resume( void )          
{ // Called after the device resumes
   return( TRUE );
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------
BOOL DR_GetDescriptor( void )
{
   return( TRUE );
}

BOOL DR_SetConfiguration( void )   
{ // Called when a Set Configuration command is received
  
  if( EZUSB_HIGHSPEED( ) )
  { // ...FX2 in high speed mode高速
    EP6AUTOINLENH = 0x00;
    SYNCDELAY;
   // EP8AUTOINLENH = 0x01;   // AUTOIN长度= 512 bytes********************************
   // SYNCDELAY;
    EP6AUTOINLENL = 0x08;  
    SYNCDELAY;
   // EP8AUTOINLENL = 0x00;
  }
  else
  { // ...FX2 in full speed mode全速
    EP6AUTOINLENH = 0x00;
    SYNCDELAY;
  //  EP8AUTOINLENH = 0x00;   // set core AUTO commit len = 64 bytes
  //  SYNCDELAY;
    EP6AUTOINLENL = 0x08;   
    SYNCDELAY;
  //  EP8AUTOINLENL = 0x40;
  }
      
  Configuration = SETUPDAT[ 2 ];
  return( TRUE );        // Handled by user code
}

BOOL DR_GetConfiguration( void )   
{ // Called when a Get Configuration command is received
   EP0BUF[ 0 ] = Configuration;
   EP0BCH = 0;
   EP0BCL = 1;
   return(TRUE);          // Handled by user code
}

BOOL DR_SetInterface( void )       
{ // Called when a Set Interface command is received
   AlternateSetting = SETUPDAT[ 2 ];
   return( TRUE );        // Handled by user code
}

BOOL DR_GetInterface( void )       
{ // Called when a Set Interface command is received
   EP0BUF[ 0 ] = AlternateSetting;
   EP0BCH = 0;
   EP0BCL = 1;
   return( TRUE );        // Handled by user code
}

BOOL DR_GetStatus( void )
{
   return( TRUE );
}

BOOL DR_ClearFeature( void )
{
   return( TRUE );
}

BOOL DR_SetFeature( void )
{
   return( TRUE );
}

BOOL DR_VendorCmnd( void )
{

  // Registers which require a synchronization delay, see section 15.14
  // FIFORESET        FIFOPINPOLAR
  // INPKTEND         OUTPKTEND
  // EPxBCH:L         REVCTL
  // GPIFTCB3         GPIFTCB2
  // GPIFTCB1         GPIFTCB0
  // EPxFIFOPFH:L     EPxAUTOINLENH:L
  // EPxFIFOCFG       EPxGPIFFLGSEL
  // PINFLAGSxx       EPxFIFOIRQ
  // EPxFIFOIE        GPIFIRQ
  // GPIFIE           GPIFADRH:L
  // UDMACRCH:L       EPxGPIFTRIG
  // GPIFTRIG
  
  // Note: The pre-REVE EPxGPIFTCH/L register are affected, as well...
  //      ...these have been replaced by GPIFTC[B3:B0] registers


    

	switch( SETUPDAT[ 1 ] )
	{ 
 
    case 0xC3:
    { // turn OFF debug LEDs...

      
      *EP0BUF = 0xC3;
  		EP0BCH = 0;
  		EP0BCL = 1; 
		EZUSB_Delay(1);
	    OEE=0xFF; //OED定义为输出端口
		IOE=0x00;
		EZUSB_Delay(10);
		IOE=0xFF;
  		EP0CS |= bmHSNAK;         // Acknowledge handshake phase of device request
  







      break;
    }

  
  /*  default:
    {
      ledX_rdvar = LED3_ON;     // debug visual, stuck "ON" to warn developer...
	    return( FALSE );          // no error; command handled OK
    } */
	}
  

	return( FALSE );              // no error; command handled OK
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav( void ) interrupt 0
{
   GotSUD = TRUE;         // Set flag
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUDAV;      // Clear SUDAV IRQ
}
// Setup Token Interrupt Handler
void ISR_Sutok( void ) interrupt 0
{
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUTOK;      // Clear SUTOK IRQ
}

void ISR_Sof( void ) interrupt 0
{
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSOF;        // Clear SOF IRQ
}

void ISR_Ures( void ) interrupt 0
{
   if ( EZUSB_HIGHSPEED( ) )
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }
   
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmURES;       // Clear URES IRQ
}

void ISR_Susp( void ) interrupt 0
{
   Sleep = TRUE;
   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmSUSP;
}

void ISR_Highspeed( void ) interrupt 0
{
   if ( EZUSB_HIGHSPEED( ) )
   {
      pConfigDscr = pHighSpeedConfigDscr;
      pOtherConfigDscr = pFullSpeedConfigDscr;
   }
   else
   {
      pConfigDscr = pFullSpeedConfigDscr;
      pOtherConfigDscr = pHighSpeedConfigDscr;
   }

   EZUSB_IRQ_CLEAR( );
   USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack( void ) interrupt 0
{
}
void ISR_Stub( void ) interrupt 0
{
}
void ISR_Ep0in( void ) interrupt 0
{
}
void ISR_Ep0out( void ) interrupt 0
{
}
void ISR_Ep1in( void ) interrupt 0
{
}
void ISR_Ep1out( void ) interrupt 0
{
}
void ISR_Ep2inout( void ) interrupt 0
{
}
void ISR_Ep4inout( void ) interrupt 0
{
}
void ISR_Ep6inout( void ) interrupt 0
{
}
void ISR_Ep8inout( void ) interrupt 0
{
}
void ISR_Ibn( void ) interrupt 0
{
}
void ISR_Ep0pingnak( void ) interrupt 0
{
}
void ISR_Ep1pingnak( void ) interrupt 0
{
}
void ISR_Ep2pingnak( void ) interrupt 0
{
}
void ISR_Ep4pingnak( void ) interrupt 0
{
}
void ISR_Ep6pingnak( void ) interrupt 0
{
}
void ISR_Ep8pingnak( void ) interrupt 0
{
}
void ISR_Errorlimit( void ) interrupt 0
{
}
void ISR_Ep2piderror( void ) interrupt 0
{
}
void ISR_Ep4piderror( void ) interrupt 0
{
}
void ISR_Ep6piderror( void ) interrupt 0
{
}
void ISR_Ep8piderror( void ) interrupt 0
{
}
void ISR_Ep2pflag( void ) interrupt 0
{
}
void ISR_Ep4pflag( void ) interrupt 0
{
}
void ISR_Ep6pflag( void ) interrupt 0
{
}
void ISR_Ep8pflag( void ) interrupt 0
{
}
void ISR_Ep2eflag( void ) interrupt 0
{
}
void ISR_Ep4eflag( void ) interrupt 0
{
}
void ISR_Ep6eflag( void ) interrupt 0
{
}
void ISR_Ep8eflag( void ) interrupt 0
{
}
void ISR_Ep2fflag( void ) interrupt 0
{
}
void ISR_Ep4fflag( void ) interrupt 0
{
}
void ISR_Ep6fflag( void ) interrupt 0
{
}
void ISR_Ep8fflag( void ) interrupt 0
{
}
void ISR_GpifComplete( void ) interrupt 0
{
}
void ISR_GpifWaveform( void ) interrupt 0
{
}