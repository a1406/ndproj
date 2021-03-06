#ifndef _BYTE_ORDER_H_
#define _BYTE_ORDER_H_

/*
大端和小端

Big Endian:  高位字节放到内存的低位地址，反之亦然。以太网网络传输字节序, PowerPC, UltraSparc一类的处理器采用大端。

Little Endian: Intel的IA-32架构采用。高位字节放到内存高位地址。记得学X86结构是有一记忆口令：High high, low low

比特位顺序一般和字节序的端模式相同，但是这涉及硬件连线方式，一般软件设计不需要管。
注意，C语言中的位域结构也要遵循端模式。
例如：
struct  edtest
{
uchar a : 2;
uchar b : 6;
}
该位域结构占1个字节，假设赋值 a = 0x01; b=0x02;
大端机器上该字节为： (01)(000010)
小端机器上：                 (000010)(01)
因此在编写可移植代码时，需要加条件编译。

网络发送是使用的是大位数,从左到右的顺序发送
bit0是最右端的，bit7是最左端的
*/

#define NE_L_ENDIAN  1
#define NE_B_ENDIAN  0

#define NE_BYTE_ORDER 1 

#ifndef u_8 
typedef NEUINT8 u_8;
#endif 

#ifndef u_16 
typedef NEUINT16 u_16;
#endif 

#ifndef u_32
typedef NEUINT32 u_32;
#endif 

//得到当前字节顺序
static __inline NEINT32 ne_byte_order() 
{
	NEINT32 a = 1 ;
	NEINT8 *p = (NEINT8*)&a ;
	return (NEINT32)p[0] ;
}

//大尾数转换小尾数
#define ne_btols(a)    ((NEINT16)( \
        (((NEINT16)(x) & (NEINT16)0x00ff) << 8) | \
        (((NEINT16)(x) & (NEINT16)0xff00) >> 8) ))
        
#define ne_btoll(a) 	((NEINT32)( \
        (((NEINT32)(x) & (NEINT32)0x000000ff) << 24) | \
        (((NEINT32)(x) & (NEINT32)0x0000ff00) << 8) | \
        (((NEINT32)(x) & (NEINT32)0x00ff0000) >> 8) | \
        (((NEINT32)(x) & (NEINT32)0xff000000) >> 24) ))

//小尾数 转换大尾数
#define ne_ltobs(a) 	((NEINT16)( \
        (((NEINT16)(x) & (NEINT16)0x00ff) << 8) | \
        (((NEINT16)(x) & (NEINT16)0xff00) >> 8) ))
        
#define ne_ltobl(a) 	((NEINT32)( \
        (((NEINT32)(x) & (NEINT32)0x000000ff) << 24) | \
        (((NEINT32)(x) & (NEINT32)0x0000ff00) << 8) | \
        (((NEINT32)(x) & (NEINT32)0x00ff0000) >> 8) | \
        (((NEINT32)(x) & (NEINT32)0xff000000) >> 24) ))


#endif 
