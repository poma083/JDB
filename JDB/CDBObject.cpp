#include "ExTypes.h"
#include "KernelJDBClasses.h"
//#include "CPage.h"

namespace JDB{
	CDBObject::CDBObject(CTable* so, int64_t _recordNumber){
		sysObject = so;
		recNum = _recordNumber;
		recBaseNum = -1;
		baseObject = NULL;
		if(sysObject->GetBaseObject() != NULL){
			baseObject = new CDBObject(sysObject->GetBaseObject(), -1);
		}
		/*if(sysObject->IsDerived()){
			baseObject = new CDBObject(sysObject->GetBaseObject(),_recordBaseNumber);
		}*/
		/*std::vector<char*>* fldNames = NULL;
		fldNames = sysObject->GetFieldNames(fldNames);
		std::vector<char*>::iterator it;
		for(it = fldNames->begin(); it != fldNames->end(); it++){
			CField* fld = new CField(so->GetField(*it));
			fields.push_back(fld);
			delete[] *it;
		}
		delete fldNames;*/
		int fldCount = sysObject->GetThisFieldsCount();
		for(int i = 0; i < fldCount; i++){
			CField* fld = new CField(so->GetField(i));
			fields.push_back(fld);
		}
	}
	CDBObject::~CDBObject(){
		std::vector<CField*>::iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			//printf("CDBObject delete \"%s\" field\n", (*it)->GetName());
			delete *it;
		}
		if(baseObject != NULL){
			delete baseObject;
		}
	}
	bool CDBObject::SetValue(const char* name, const void* value, const int length){
		std::vector<CField*>::iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			if (strcmp( name, (*it)->GetName()) == 0){
				(*it)->SetValue(value, length);
				return true;
			}
		}
		if(baseObject != NULL){
			return baseObject->SetValue(name, value, length);
		}
		return false;
	}
	CField* CDBObject::GetField(const char* name){
		std::vector<CField*>::iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			if (strcmp( name, (*it)->GetName()) == 0){
				return *it;
			}
		}
		if(baseObject != NULL){
			return baseObject->GetField(name);
		}
		return NULL;
	}
	const char* const CDBObject::GetName() const{
		return sysObject->GetName();
	};
	int CDBObject::Write( CDB* db ){
		if(baseObject != NULL){
			recBaseNum = baseObject->Write(db);
		}

		int recordSize = sysObject->GetSize();
		int pos = 0;

		int pageNum = sysObject->GetDataPageNumber();
		CPage* dataPage = (CPage*)db->GetPage(pageNum,ptEMPTY);
		if(sysObject->GetDataPageNumber() < 0){
			sysObject->SetDataPageNumber(pageNum);
		}
		//recNum = 0x20;
		bool fNewObject = recNum < 0;
		if(fNewObject){
			recNum = dataPage->add(&recBaseNum,sizeof(recBaseNum))/recordSize;
		}
		else{
			dataPage->out(recNum * recordSize, &recBaseNum,sizeof(recBaseNum));
		}
		pos += sizeof(recBaseNum);
		std::vector<CField*>::const_iterator it;
		int upPageNumber = -1;
		for(it = fields.begin(); it != fields.end(); it++){
			switch((*it)->GetType()){
			case OBJECT:
				//if(fNewObject){
				//	dataPage->add((*it)->value,(*it)->GetSize());
				//}
				//else{
					upPageNumber = dataPage->GetPageNumUpLevel();
					dataPage->out(recNum * recordSize + pos, (*it)->value, (*it)->GetSize());
					if(upPageNumber != dataPage->GetPageNumUpLevel()){
						upPageNumber = dataPage->GetPageNumUpLevel();
						while(upPageNumber > 0){
							pageNum = upPageNumber;
							dataPage = (CPage*)db->GetPage(pageNum,ptEMPTY);
							upPageNumber = dataPage->GetPageNumUpLevel();
						}
						sysObject->SetDataPageNumber(pageNum);
					}
				//}
				pos += (*it)->GetSize();
				break;
			case OBJECTLIST:
				break;
			case FIXED:
				//if(fNewObject){
				//	dataPage->add((*it)->value,(*it)->GetSize());
				//}
				//else
					upPageNumber = dataPage->GetPageNumUpLevel();
					dataPage->out(recNum * recordSize + pos, (*it)->value, (*it)->GetSize());
					if(upPageNumber != dataPage->GetPageNumUpLevel()){
						upPageNumber = dataPage->GetPageNumUpLevel();
						while(upPageNumber > 0){
							pageNum = upPageNumber;
							dataPage = (CPage*)db->GetPage(pageNum,ptEMPTY);
							upPageNumber = dataPage->GetPageNumUpLevel();
						}
						sysObject->SetDataPageNumber(pageNum);
						sysObject->Write(db);
					}
				//}
				pos += (*it)->GetSize();
				break;
			case VARIABLE:
				//записать значение поля (строку) на страницу строк
				//номер страницы строк взять из поля sysObject.GetStringsPageNumber()
				int pageNum = sysObject->GetStringsPageNumber();
				CPage* strPage = db->GetPage(pageNum,ptSTRINGDATA);
				//strPage->SetEmpty(10);
				if(sysObject->GetStringsPageNumber() < 0){
					sysObject->SetStringsPageNumber(pageNum);
				}
				int al = (*it)->GetAllocateLength();
				char* buffValue = new char[al+sizeof(al)];
				memcpy(buffValue, &al, sizeof(al));
				(*it)->CopyValue(buffValue+sizeof(al));
				int64_t offset = (*it)->GetOffset();
				if(offset < 0){
					offset = strPage->add(buffValue,al+sizeof(al));
					upPageNumber = strPage->GetPageNumUpLevel();
					if(upPageNumber > 0){
						sysObject->SetStringsPageNumber(upPageNumber);///???
						sysObject->Write(db);
					}
					(*it)->SetOffset(offset);
				}
				else{///???
					upPageNumber = strPage->GetPageNumUpLevel();
					strPage->out(offset, buffValue, al+sizeof(al));
					if(upPageNumber != strPage->GetPageNumUpLevel()){
						upPageNumber = strPage->GetPageNumUpLevel();
						while(upPageNumber > 0){
							pageNum = upPageNumber;
							strPage = (CPage*)db->GetPage(pageNum,ptEMPTY);
							upPageNumber = strPage->GetPageNumUpLevel();
						}
						sysObject->SetStringsPageNumber(pageNum);
						sysObject->Write(db);
					}
				}
				delete[] buffValue;
				//if(fNewObject){
				//	dataPage->add(&offset,sizeof(offset));
				//}
				//else{
					upPageNumber = dataPage->GetPageNumUpLevel();
					dataPage->out(recNum * recordSize + pos, &offset, sizeof(offset));
					if(upPageNumber != dataPage->GetPageNumUpLevel()){
						upPageNumber = dataPage->GetPageNumUpLevel();
						while(upPageNumber > 0){
							pageNum = upPageNumber;
							dataPage = (CPage*)db->GetPage(pageNum,ptEMPTY);
							upPageNumber = dataPage->GetPageNumUpLevel();
						}
						sysObject->SetDataPageNumber(pageNum);
						sysObject->Write(db);
					}
				//}
				//dataPage->out(recNum * recordSize + pos, &offset, sizeof(offset));
				pos += sizeof(offset);
				break;
			}
		}
		return recNum;
	}
	int CDBObject::Read(CDB* db){
		if(recNum < 0){
			return -1;
		}
		int pos = 0;
		int recordSize = sysObject->GetSize();
		
		int pageNum = sysObject->GetDataPageNumber();
		CDataPage* dataPage = (CDataPage*)db->GetPage(pageNum,ptEMPTYDATA);
		
		dataPage->in(&recBaseNum, recNum * recordSize, sizeof(recBaseNum));
		pos += sizeof(recBaseNum);
		if(recBaseNum >= 0){
			baseObject->recNum = recBaseNum;
			baseObject->Read(db);
		}

		std::vector<CField*>::const_iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			switch((*it)->GetType()){
			case OBJECT:
				dataPage->in((*it)->value, recNum * recordSize + pos, (*it)->GetSize());
				pos += (*it)->GetSize();
				(*it)->ReadObject(db);
				break;
			case OBJECTLIST:
				break;
			case FIXED:
				dataPage->in((*it)->value, recNum * recordSize + pos, (*it)->GetSize());
				pos += (*it)->GetSize();
				break;
			case VARIABLE:
				int pageNum = sysObject->GetStringsPageNumber();
				CStringsPage* strPage = (CStringsPage*)db->GetPage(pageNum,ptSTRINGDATA);
				
				int al = 0;
				int64_t offset = 0;//(*it)->GetOffset();
				
				dataPage->in(&offset, recNum * recordSize + pos, (*it)->GetSize());
				pos += (*it)->GetSize();
				(*it)->SetOffset(offset);

				strPage->in(&al, offset,sizeof(al));

				char* buffValue = new char[al];
				strPage->in(buffValue,offset+sizeof(al),al);
				(*it)->SetValue(buffValue,al);
				delete[] buffValue;
				
				break;
			}
		}
		return pos;	
	}
}