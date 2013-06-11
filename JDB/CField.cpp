#include "KernelJDBClasses.h"

namespace JDB{
	CField::CField(CSysField* fld){
		sysField = fld;
		objectValue = NULL;
		switch(sysField->GetType()){
		case FIXED:
			valueLength = sysField->GetSize();
			allocateLength = valueLength;
			value = new char[valueLength];
			break;
		default:
			offset = -1;
			value = NULL;
			valueLength = 0;
			allocateLength = 0;
			break;
		}
	}
	CField::~CField(){
		if(value != NULL){
			delete[] value;
			value = NULL;
			valueLength = 0;
			allocateLength = 0;
		}
		if(objectValue != NULL){
			delete objectValue;
		}
	}
	void CField::SetValue(const void* newValue, int length){
		switch(sysField->GetType()){
		case VARIABLE:
			if(allocateLength < length){
				if(value != NULL){
					delete[] value;
				}
				value = new char[length];
				allocateLength = length * 2;
				offset = -1;
			}
			valueLength = length;
			break;
		default:
			break;
		}
		memcpy(value, newValue, valueLength);
	}
	void CField::ReadObject(CDB* db){
		if(GetType() != OBJECT){
			throw -1;
		}
		std::vector<int64_t> recNumbers;
		int64_t rec = 0;
		memcpy(&rec, value, 8);
		recNumbers.push_back(rec);
		CTable* tbl = const_cast<CTable*> (sysField->GetTableField());
		std::vector<JDB::CDBObject*> * records = tbl->ReadRecords(recNumbers,db);
		objectValue = (*records)[0];
		delete records;
	}
	int CField::CopyValue(char* dst) const{
		memcpy(dst, value, valueLength);
		return sysField->GetSize();
	}
	const char* CField::GetName() const {
		return sysField->GetName();
	}
	FieldType CField::GetType() const { 
		return sysField->GetType();
	}
	int CField::GetSize() const{
		return sysField->GetSize();
	}
	int CField::GetLength() const{
		return valueLength;
	}
}