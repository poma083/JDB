#ifndef __JDB_KERNELJDB_H__
#define __JDB_KERNELJDB_H__

//enum FieldType{FIXED = 0x00, VARIABLE = 0x01, OBJECT = 0x02, OBJECTLIST = 0x03};
	
int _stdcall _init_test(bool create);
void __stdcall LoadBases();
void __stdcall CreateBase(const char* baseName, int PageSize);
int __stdcall CreateTable(const char* baseFileName, const char* tableName, const char* baseTableName);
int __stdcall CreateField(const char* baseFileName, const char* tableName, char* fieldName, const int powerSize, const int type, const char* objectName);
int __stdcall DeleteField(const char* baseFileName, const char* tableName, const char* fieldName);
//void __stdcall FlushBase(const char* baseName);
char* __stdcall GetBasesInfo(char* buff, int& cnt);
char* __stdcall GetTablesInfo(const char* baseName, char* buff, int& cnt);
char* __stdcall GetFieldsInfo(const char* baseName, const char* tableName, char* buff, int& cnt);

#endif