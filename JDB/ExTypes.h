#ifndef __JDB_ExTypes_H__
#define __JDB_ExTypes_H__
#include <Windows.h>
#include <vector>

#define uint64_t unsigned long long int
#define int64_t long long int
namespace JDB{
	typedef int* ptrInt;
	//typedef std::vector<std::vector<int> > TPageNumbersPhyramid;
	enum FieldType{FIXED = 0x00, VARIABLE = 0x01, OBJECT = 0x02, OBJECTLIST = 0x03};
	enum PageTypes{
		ptINDEXMASK = 0x10,		//(00010000)
		//ptSUPERINDEXMASK = 0x20,//(00100000)
		
		ptEMPTY = 0x00,			//(00000000)
		ptHEADER = 0x80,		//(10000000)
		ptSTATE = 0x40,			//(01000000)
		ptEMPTYDATA = 0x08,		//(00001000)
		ptSTRINGDATA = 0x04,	//(00000100)
		ptBINDATA = 0x02,		//(00000010)
		ptDATAINDEX = ptINDEXMASK | ptEMPTYDATA,
		ptBININDEX = ptINDEXMASK | ptBINDATA,
		ptSTRINGINDEX = ptINDEXMASK | ptSTRINGDATA,
		ptSTATEINDEX = ptINDEXMASK | ptSTATE
	};
}

#endif