#include <stdio.h>
#include "ExTypes.h"
#include "KernelJDBClasses.h"

namespace JDB{
	CSysField::CSysField(int64_t recordNumber, CDB* db){
		name = NULL;
		size = 0;
		type = FIXED;
		isDeleted = 0;
		nameOffset = -1;
		recNum = recordNumber;
		tableReferenceRecord = -1;
		tableReference = NULL;
		Read(db);
	}
	CSysField::CSysField(char* _name, char _sizeFromPower, FieldType _type, CTable* objectTable, char _isDeleted){
		name = NULL;
		SetName(_name);
		size = _sizeFromPower;
		type = _type;
		isDeleted = _isDeleted;
		nameOffset = -1;
		recNum = -1;
		tableReference = objectTable;
		if(tableReference != NULL){
			tableReferenceRecord = tableReference->GetRecNumber();
		}
		else{
			tableReferenceRecord = -1;
		}
	}
	CSysField::~CSysField(){
		if(name != NULL){
			//printf("delete field %s\n", name);
			delete[] name;
		}
	}
	int64_t CSysField::Write(CDB* db){
		//запишем имя поля на страницу strings и получим смещение
		int pageNum = db->GetFieldsStringsPN();
		CStringsPage* strPage = (CStringsPage*)db->GetPage(pageNum,ptSTRINGDATA);

		int allocatedLength = strlen(name)+1;
		int alsize = sizeof(allocatedLength);
		char* buffValue = new char[allocatedLength+alsize];
		memcpy(buffValue, &allocatedLength, alsize);
		memcpy(buffValue + alsize, name, allocatedLength);
				
		if(nameOffset < 0){
			nameOffset = strPage->add(buffValue,allocatedLength+alsize);
		}
		else{
			int alTmp = 0;
			strPage->in(&alTmp, nameOffset, alsize);
			if(alTmp >= allocatedLength){
				strPage->out(nameOffset + alsize, name,allocatedLength);
			}
			else{
				nameOffset = strPage->add(buffValue,allocatedLength+alsize);
			}
		}
		delete[] buffValue;
		//теперь запишем сам объект
		int recordSize = sizeof(isDeleted) + sizeof(size) +
						sizeof(type) + sizeof(nameOffset) + 
						sizeof(tableReferenceRecord);
		char* buff = new char[recordSize];
		int pos = 0;			
		memcpy(buff+pos,&isDeleted, sizeof(isDeleted));
		pos += sizeof(isDeleted);
		memcpy(buff+pos,&size, sizeof(size));
		pos += sizeof(size);
		memcpy(buff+pos,&type, sizeof(type));
		pos += sizeof(type);
		memcpy(buff+pos,&nameOffset, sizeof(nameOffset));
		pos += sizeof(nameOffset);
		memcpy(buff+pos,&tableReferenceRecord, sizeof(tableReferenceRecord));
		pos += sizeof(tableReferenceRecord);		

		pageNum = db->GetFieldsDataPN();
		CDataPage* dataPage = (CDataPage*)db->GetPage(pageNum,ptEMPTYDATA);
		if(recNum < 0){
			recNum = dataPage->add(buff,pos)/pos;
		}
		else{
			dataPage->out(recNum * pos, buff, pos);
		}
		delete[] buff;
		return recNum;
	}
	void CSysField::Read(CDB* db){
		if(recNum < 0){
			return;
		}
		int recordSize = sizeof(isDeleted) + sizeof(size) +
						sizeof(type) + sizeof(nameOffset) +
						sizeof(tableReferenceRecord);

		int pageNum = db->GetFieldsDataPN();
		CDataPage* dp = (CDataPage*)db->GetPage(pageNum,ptEMPTYDATA);
		int pos = 0;
		dp->in(&isDeleted, recordSize*recNum+pos, sizeof(isDeleted));
		pos += sizeof(isDeleted);
		dp->in(&size, recordSize*recNum+pos, sizeof(size));
		pos += sizeof(size);
		dp->in(&type, recordSize*recNum+pos, sizeof(type));
		pos += sizeof(type);
		dp->in(&nameOffset, recordSize*recNum+pos, sizeof(nameOffset));
		pos += sizeof(nameOffset);
		dp->in(&tableReferenceRecord, recordSize*recNum+pos, sizeof(tableReferenceRecord));
		pos += sizeof(tableReferenceRecord);

		pageNum = db->GetFieldsStringsPN();
		CStringsPage* sp = (CStringsPage*)db->GetPage(pageNum,ptSTRINGDATA);
		int alloc = 0;
		sp->in(&alloc, nameOffset, 4);
		name = new char[alloc];
		sp->in(name, nameOffset+4, alloc);

		if(tableReferenceRecord >= 0){
			tableReference = db->masterTable[tableReferenceRecord];
		}
	}
	char CSysField::GetSize() const {
		switch(type){
		case FIXED:
			return 0x01 << size;
			break;
		/*case VARIABLE:
			return 8;
			break;
		case OBJECT:
			return 8;
			break;
		case OBJECTLIST:
			return 8;
			break;*/
		default:
			return 8;
			break;
		}
	}
	void CSysField::SetName(const char* newName){
		int newlen = 0;
		if(newName != NULL){
			newlen = strlen(newName);
		};
		int len = 0;
		if(name == NULL){
			name = new char[newlen + 1];
			len = newlen;
		}
		else{
			len = strlen(name);
		};
		if(len < newlen){
			delete[] name;
			name = new char[newlen + 1];
		};
		strcpy(name,newName);
		name[newlen] = 0;
	}

	const CTable* CSysField::GetTableField() const{
		if((type == FIXED)||(type == VARIABLE)){
			throw -1;
		}
		return tableReference;
	}
}