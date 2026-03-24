/*
*********************************************************************************************************
*数码管驱动IC TM1637 驱动文件
*
*版本：V1.0
*
*
*********************************************************************************************************
*/
#include "main.h"

uint8_t Icon = 0;
uint8_t Bai = 3;
uint8_t Shi = 2;
uint8_t Ge = 3;

icon_type_t      icon_set_t;
icon_type_t      icon_test_t;

// TM1637 生成数组，LED数码管生成器要选共阴极。
const uint8_t SmgTab[] =
{
    0xFC,/*0*/
    0x60,/*1*/
    0xDA,/*2*/
    0xF2,/*3*/
    0x66,/*4*/
    0xB6,/*5*/
    0xBE,/*6*/
    0xE0,/*7*/
    0xFE,/*8*/
    0xF6,/*9*/
    0xEE,/*A*/
    0x3E,/*b*/
    0x9C,/*C*/
    0x7A,/*d*/
    0x9E,/*E*/
    0x8E,/*F*/
    0x6E,/*H*/
    0x1C,/*L*/
    0x3A,/*c*/
    0xCE,/*P*/
    0x0A,/*r*/
    0x1E,/*t*/
    0x7C,/*U*/
    0x02,/*-*/
    0x00,/*空*/
};
/*
*********************************************************************************************************
*                                              TM1637_Start()
* Description: 初始化TM1637
* Arguments :
* Returns   :  NO
*********************************************************************************************************
*/
void TM1637_DalayUs( uint8_t i )
{
    for( ; i > 0; i-- )
    {
        nop();
    }
}
/*
*********************************************************************************************************
*                                              TM1637_Start()
* Description: 初始化TM1637
* Arguments :
* Returns   :  NO
*********************************************************************************************************
*/
void TM1637_Start( void )
{
    TM1637_CLK_HIGH;
    TM1637_DIO_HIGH;
    TM1637_DalayUs( 2 );
    TM1637_DIO_LOW;
}
/*
*********************************************************************************************************
*                                              TM1637_Stop()
* Description: 初始化TM1637
* Arguments :
* Returns   :  NO
*********************************************************************************************************
*/
void TM1637_Stop( void )
{
    TM1637_CLK_LOW;
    TM1637_DalayUs( 2 );
    TM1637_DIO_LOW;
    TM1637_DalayUs( 2 );
    TM1637_CLK_HIGH;
    TM1637_DalayUs( 2 );
    TM1637_DIO_HIGH;
}
/*
*********************************************************************************************************
*                                              TM1637_Ask()
* Description: 初始化TM1637
* Arguments :
* Returns   :  NO
*********************************************************************************************************
*/
void TM1637_Ask( void )
{
    TM1637_CLK_LOW;
    TM1637_DalayUs( 5 );
    //while(GPIO_ReadInputPin(TM1637_PORT,TM1637_DIO_PIN));
    TM1637_CLK_HIGH;
    TM1637_DalayUs( 2 );
    TM1637_CLK_LOW;
}
/*
*********************************************************************************************************
*                                              TM1637_Write_Byte()
* Description: 初始化TM1637
* Arguments :
* Returns   :  NO
*********************************************************************************************************
*/
void TM1637_Write_Byte( unsigned char dat )
{
    uint8_t i;
    for( i = 0; i < 8; i++ )
    {
        TM1637_CLK_LOW;
        if( dat & 0x01 )
        {
            TM1637_DIO_HIGH;
        }
        else
        {
            TM1637_DIO_LOW;
        }
        TM1637_DalayUs( 3 );
        dat = dat >> 1;
        TM1637_CLK_HIGH;
        TM1637_DalayUs( 3 );
    }
}
// 读按键-----------------------------------------
unsigned char TM1637_key_scan( void )
{
    uint8_t rekey, i;
    TM1637_Start();
    TM1637_Write_Byte( 0x42 );
    TM1637_Ask();
    TM1637_DIO_HIGH;

    for( i = 0; i < 8; i++ )
    {
        TM1637_CLK_LOW;
        rekey = rekey >> 1;
        TM1637_DalayUs( 10 );
        TM1637_CLK_HIGH;

        if( TM1637_DIO_IN() )
        {
            rekey = rekey | 0x80;
        }
        else
        {
            rekey = rekey | 0x00;
        }
        TM1637_DalayUs( 20 );
    }
    TM1637_Ask();
    TM1637_Stop();
    return( rekey );
}
/*
*********************************************************************************************************
*                                              init_tm1640()
* Description: 初始化TM1637
* Arguments :
* Returns   :  NO
*********************************************************************************************************
*/
void TM1637_init( void )
{
    GPIO_Init( TM1637_PORT, ( GPIO_Pin_TypeDef )( TM1637_CLK_PIN | TM1637_DIO_PIN ), GPIO_MODE_OUT_PP_LOW_FAST );
    icon_set_t.byte = 0x00;
}

/*
*********************************************************************************************************
*                                              init_tm1640()
* Description: 初始化TM1637
* Arguments :
* Returns   :  NO
*********************************************************************************************************
*/

void TM1637_display( void )
{
    Icon = icon_set_t.byte;
  //Icon = icon_test_t.byte;
    TM1637_Start();
    TM1637_Write_Byte( DATA_COMMAND_Z );
    TM1637_Ask();
    TM1637_Stop();
    TM1637_Start();
    TM1637_Write_Byte( ADDR_START );
    TM1637_Ask();
//------------------------------------------
    TM1637_Write_Byte( Icon );
    TM1637_Ask();

    TM1637_Write_Byte( SmgTab[Bai] );
    TM1637_Ask();

    if( sys_flag_t.idot == ON )
        TM1637_Write_Byte( SmgTab[Shi] | 0x01 );
    else
        TM1637_Write_Byte( SmgTab[Shi] );
    TM1637_Ask();

    if( sys_flag_t.FuHao == ON )
        TM1637_Write_Byte( SmgTab[Ge] | 0x01 );
    else
        TM1637_Write_Byte( SmgTab[Ge] );
    TM1637_Ask();
//------------------------------------------

    TM1637_Stop();
    TM1637_Start();
    TM1637_Write_Byte( DISP_OPEN );
    TM1637_Ask();
    TM1637_Stop();
}











/************************************************* END *************************************************/


