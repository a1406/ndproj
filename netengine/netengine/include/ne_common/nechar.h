/*
 * nechar 类似于TCHAR
 * 因为我不想分别实现ansi和unicode二进制版本,只是想在编译时方便进行选择!
 */
#ifndef _NECHAR_H_
#define _NECHAR_H_

#include <wchar.h>
#include <stdio.h>

#define NE_ESC		0x1b
#ifdef WIN32 

//#if _MSC_VER < 1300 // 1200 == VC++ 6.0

#define snprintf 		_snprintf
#define snwprintf 		_snwprintf
#define vsnwprintf		_vsnwprintf
#define vsnprintf		_vsnprintf

//#else 
//#endif

#endif 

#ifdef NE_UNICODE

#define nechar_t wchar_t
#define _NET(x)		L ## x
#define neisalnum     iswalnum		//测试字符是否为数字或字母 
#define neisalpha     iswalpha 		// 测试字符是否是字母 
#define neiscntrl     iswcntrl 		//测试字符是否是控制符 
#define neisdigit     iswdigit 		//测试字符是否为数字 
#define neisgraph     iswgraph 		//测试字符是否是可见字符 
#define neislower     iswlower 		//测试字符是否是小写字符 
#define neisprint     iswprint 		//测试字符是否是可打印字符 
#define neispunct     iswpunct 		//测试字符是否是标点符号 
#define neisspace     iswspace 		//测试字符是否是空白符号 
#define neisupper     iswupper 		//测试字符是否是大写字符 
#define neisxdigit    iswxdigit		//测试字符是否是十六进制的数字 

#define netolower     towlower 		//把字符转换为小写 
#define netoupper     towupper 		//把字符转换为大写 
#define necscoll      wcscoll 		//比较字符串 

/*
打印和扫描字符串： 
宽字符函数描述 
*/
#define nefprintf		fwprintf     //使用vararg参量的格式化输出 
#define neprintf		wprintf      //使用vararg参量的格式化输出到标准输出 
#define nesprintf		swprintf     //根据vararg参量表格式化成字符串 
#define nevfprintf		vfwprintf    //使用stdarg参量表格式化输出到文件 
#define nevsprintf		vsnwprintf    //格式化stdarg参量表并写到字符串 

#define nesnprintf 		snwprintf

#define nestrtod 		wcstod    //把宽字符的初始部分转换为双精度浮点数 
#define nestrtol		wcstol     //把宽字符的初始部分转换为长整数 
#define nestrtoul		wcstoul    //宽字符的初始部分转换为无符号长整数 

/*
字符串操作： 
宽字符函数        普通C函数描述 
*/
#define nestrcat	wcscat			//strcat把一个字符串接到另一个字符串的尾部 
#define nestrncat	wcsncat			//strncat
#define nestrchr	wcschr			//strchr查找子字符串的第一个位置 
#define nestrrchr	wcsrchr			// strrchr（）     从尾部开始查找子字符串出现的第一个位置 
#define nestrpbrk	wcspbrk         //strpbrk（）     从一字符字符串中查找另一字符串中任何一个字符第一次出现的位置 
#define nestrstr	wcsstr			//    strstr（）     在一字符串中查找另一字符串第一次出现的位置 
#define nestrcspn	wcscspn			//（）         strcspn（）     返回不包含第二个字符串的的初始数目 
#define nestrspn	wcsspn			//（）         strspn（）     返回包含第二个字符串的初始数目 
#define nestrcpy	wcscpy			//（）         strcpy（）     拷贝字符串 
#define nestrncpy	wcsncpy			//（）         strncpy（）     类似于wcscpy（）， 同时指定拷贝的数目 
#define nestrcmp	wcscmp			//（）         strcmp（）     比较两个宽字符串 
#define nestrncmp	wcsncmp			//（）         strncmp（）     类似于wcscmp（）， 还要指定比较字符字符串的数目 
#define nestrlen	wcslen			//（）         strlen（）     获得宽字符串的数目 
#define nestrtok	wcstok			//（）         strtok（）     根据标示符把宽字符串分解成一系列字符串 

#else		//ansi
#define _NET(x) x
#define nechar_t char

#define neisalnum     isalnum		//测试字符是否为数字或字母 
#define neisalpha     isalpha 		// 测试字符是否是字母 
#define neiscntrl     iscntrl 		//测试字符是否是控制符 
#define neisdigit     isdigit 		//测试字符是否为数字 
#define neisgraph     isgraph 		//测试字符是否是可见字符 
#define neislower     islower 		//测试字符是否是小写字符 
#define neisprint     isprint 		//测试字符是否是可打印字符 
#define neispunct     ispunct 		//测试字符是否是标点符号 
#define neisspace     isspace 		//测试字符是否是空白符号 
#define neisupper     isupper 		//测试字符是否是大写字符 
#define neisxdigit     isxdigit		//测试字符是否是十六进制的数字 

#define netolower     tolower 		//把字符转换为小写 
#define netoupper     toupper 		//把字符转换为大写 
#define necscoll     strcoll 		//比较字符串 

/*
打印和扫描字符串： 
宽字符函数描述 
*/
#define nefprintf		fprintf     //使用vararg参量的格式化输出 
#define neprintf		printf      //使用vararg参量的格式化输出到标准输出 
#define nesprintf		sprintf     //根据vararg参量表格式化成字符串 
#define nevfprintf		vfprintf    //使用stdarg参量表格式化输出到文件 
#define nevsprintf		vsnprintf    //格式化stdarg参量表并写到字符串 
#define nesnprintf 		snprintf

#define nestrtod 		strtod		//把宽字符的初始部分转换为双精度浮点数 
#define nestrtol		strtol		//把宽字符的初始部分转换为长整数 
#define nestrtoul		strtoul		//把宽字符的初始部分转换为无符号长整数 

/*
字符串操作： 
宽字符函数        普通C函数描述 
*/
#define nestrcat	strcat			//把一个字符串接到另一个字符串的尾部 
#define nestrncat	strncat			//而且指定粘接字符串的粘接长度. 
#define nestrchr	strchr			//查找子字符串的第一个位置 
#define nestrrchr	strrchr			//从尾部开始查找子字符串出现的第一个位置 
#define nestrpbrk	strpbrk			//从一字符字符串中查找另一字符串中任何一个字符第一次出现的位置 
#define nestrstr	strstr			//在一字符串中查找另一字符串第一次出现的位置 
#define nestrcspn	strcspn			//返回不包含第二个字符串的的初始数目 
#define nestrspn	strspn			//返回包含第二个字符串的初始数目 
#define nestrcpy	strcpy			//拷贝字符串 
#define nestrncpy	strncpy			//同时指定拷贝的数目 
#define nestrcmp	strcmp			//比较两个宽字符串 
#define nestrncmp	strncmp			//指定比较字符字符串的数目 
#define nestrlen	strlen			// 获得宽字符串的数目 
#define nestrtok	strtok

#endif //NE_UNICODE

#endif
