#ifndef _BYTE_ORDER_H_
#define _BYTE_ORDER_H_

/*
��˺�С��

Big Endian:  ��λ�ֽڷŵ��ڴ�ĵ�λ��ַ����֮��Ȼ����̫�����紫���ֽ���, PowerPC, UltraSparcһ��Ĵ��������ô�ˡ�

Little Endian: Intel��IA-32�ܹ����á���λ�ֽڷŵ��ڴ��λ��ַ���ǵ�ѧX86�ṹ����һ������High high, low low

����λ˳��һ����ֽ���Ķ�ģʽ��ͬ���������漰Ӳ�����߷�ʽ��һ�������Ʋ���Ҫ�ܡ�
ע�⣬C�����е�λ��ṹҲҪ��ѭ��ģʽ��
���磺
struct  edtest
{
uchar a : 2;
uchar b : 6;
}
��λ��ṹռ1���ֽڣ����踳ֵ a = 0x01; b=0x02;
��˻����ϸ��ֽ�Ϊ�� (01)(000010)
С�˻����ϣ�                 (000010)(01)
����ڱ�д����ֲ����ʱ����Ҫ���������롣

���緢����ʹ�õ��Ǵ�λ��,�����ҵ�˳����
bit0�����Ҷ˵ģ�bit7������˵�
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

//�õ���ǰ�ֽ�˳��
static __inline NEINT32 ne_byte_order() 
{
	NEINT32 a = 1 ;
	NEINT8 *p = (NEINT8*)&a ;
	return (NEINT32)p[0] ;
}

//��β��ת��Сβ��
#define ne_btols(a)    ((NEINT16)( \
        (((NEINT16)(x) & (NEINT16)0x00ff) << 8) | \
        (((NEINT16)(x) & (NEINT16)0xff00) >> 8) ))
        
#define ne_btoll(a) 	((NEINT32)( \
        (((NEINT32)(x) & (NEINT32)0x000000ff) << 24) | \
        (((NEINT32)(x) & (NEINT32)0x0000ff00) << 8) | \
        (((NEINT32)(x) & (NEINT32)0x00ff0000) >> 8) | \
        (((NEINT32)(x) & (NEINT32)0xff000000) >> 24) ))

//Сβ�� ת����β��
#define ne_ltobs(a) 	((NEINT16)( \
        (((NEINT16)(x) & (NEINT16)0x00ff) << 8) | \
        (((NEINT16)(x) & (NEINT16)0xff00) >> 8) ))
        
#define ne_ltobl(a) 	((NEINT32)( \
        (((NEINT32)(x) & (NEINT32)0x000000ff) << 24) | \
        (((NEINT32)(x) & (NEINT32)0x0000ff00) << 8) | \
        (((NEINT32)(x) & (NEINT32)0x00ff0000) >> 8) | \
        (((NEINT32)(x) & (NEINT32)0xff000000) >> 24) ))


#endif 
