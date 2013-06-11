#include "ExTypes.h"
#include "KernelJDBClasses.h"
#include <stack>
//#include "CSysField.h"

namespace JDB{
	CTable::CTable(int64_t recordNumber, CTable* base, const char* _name){ //CDB* _db){
		name = NULL;
		dataPageNumber = -1;
		stringsPageNumber = -1;
		binPageNumber = -1;

		baseObject = base;
		if(base == NULL){
			ptrBaseObject = -1;
		}
		else{
			ptrBaseObject = base->GetRecNumber();
		}

		recNum = recordNumber;
		fieldsOffset = -1;
		nameOffset = -1;
		masterRecSize = -1;
		SetName(_name);

		//db = _db;
	}
	CTable::~CTable(){
		std::vector<CSysField*>::iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			printf("CSysObject delete \"%s\" sysField\n", (*it)->GetName());
			delete *it;
		}
	}
	void CTable::SetDataPageNumber( const int& _dataPageNumber){ 
		dataPageNumber = _dataPageNumber;
	}
	void CTable::SetName(const char* newName){
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
			nameOffset = -1;
		};
		strcpy(name,newName);
		name[newlen] = 0;
	}
	int CTable::GetFieldsCount() const {
		int result = fields.size();
		if(baseObject != NULL){
			result += baseObject->GetFieldsCount();
		}
		return result;
	}
	CSysField* CTable::GetField(char* name) const{
		std::vector<CSysField*>::const_iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			if(strcmp((*it)->GetName(), name) == 0){
				return *it;
			}
		}
		if(baseObject != NULL){
			return baseObject->GetField(name);
		}
		return NULL;
	}
	std::vector<const char*>* CTable::GetFieldNames(std::vector<const char*>* buff) const{
		if(buff == NULL){
			buff = new std::vector<const char*>;
		}
		std::vector<CSysField*>::const_iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			const char* name =(*it)->GetName();
			//int lenName = strlen(name);
			//char* tmp = new char[lenName+1];
			//strcpy(tmp, name);
			//buff->push_back(tmp);
			buff->push_back(name);
		}
		if(baseObject != NULL){
			return baseObject->GetFieldNames(buff);
		}
		return buff;
	}
	int CTable::GetLengthForFieldsInfoFunction() const{
		int result = 0;
		std::vector<CSysField*>::const_iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			if(!(*it)->IsDeleted()){
				result += strlen((*it)->GetName());
				result += 2;
			}
		}
		if(baseObject != NULL){
			result += baseObject->GetLengthForFieldsInfoFunction();
		}
		return result;
	}
	char* CTable::GetFieldsInfo(char* buff, int& offset) const{
		int len = GetLengthForFieldsInfoFunction();
		if(buff == NULL){
			buff = new char[len];
		}
		len = offset;
		std::vector<CSysField*>::const_iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			if(!(*it)->IsDeleted()){
				const char* name =(*it)->GetName();
				strcpy(buff + len, name);
				len += strlen(name);
				len++;
				buff[len] = 0;
				buff[len] = buff[len] | ((*it)->GetType() << 4) | (*it)->GetSize();
				len++;
			}
		}
		if(baseObject != NULL){
			baseObject->GetFieldsInfo(buff,len);
		}
		offset = len;
		return buff;
	}
	char* CTable::Select(char* buff, int& offset){

		return NULL;
	}
	int CTable::CreateField(CSysField* fld){
		if(fld->GetType() == OBJECT){
			if(fld->GetTableField() == NULL){
				throw -1;
			}
		}
		fields.push_back(fld);
		return fields.size();
	}
	int CTable::AddField(CSysField* fld, CDB* db){
		int content_size = db->GetPageSize(true);
		//старый размер объекта
		int oldSize = GetSize();
		//новый размер объекта
		int newSize = oldSize + fld->GetSize();
		
		//выделим новую индексную страницу
		int iNewPageNum = -1;
		CDataIndexPage* iNewDataPage = (CDataIndexPage*)db->GetPage(iNewPageNum,ptDATAINDEX);
		//выделим новую страницу данных
		int newPageNum = -1;
		CDataPage* newDataPage = (CDataPage*)db->GetPage(newPageNum,ptEMPTYDATA);
		//добавим эту страницу данных на индексную
		iNewDataPage->InsertIndex(newPageNum,iNewPageNum);
		

		//находим количество страниц необходимое дл€ записи
		//всех элементов с новым набором полей
		//int64_t rec_counts = GetRecordsCount(db);
		//int64_t allSize = rec_counts * newSize;
		//int new_page_counts = (allSize+content_size-1)/content_size;
		
		//теперь нужно бежать по всем старым страницам
		//перебира€ их записи, и добавл€ть эти записи с новым набором
		//полей на новые страницы

		//создадим стек индексных страниц
		std::stack<std::pair<int,CPage*>* > pagesThread;

		int pageNum = this->GetDataPageNumber();
		CPage* dataPage = db->GetPage(pageNum,ptEMPTYDATA);
		char lvl = dataPage->GetIxLevel();
		//если страница не индексна€ то перебираем еЄ записи
		//и к каждой добавл€ем байты дл€ нового пол€
		int prevOverSize = 0;
		char* newRecordData = new char[newSize];
				
		if(lvl = 0){
			int readed = 0;
			while(readed < dataPage->GetEmpty()){
				int cnt = (dataPage->GetEmpty()-readed) >= oldSize ? oldSize : (dataPage->GetEmpty()-readed);
				dataPage->in( newRecordData, readed, cnt);
				readed += cnt;
				if (cnt < oldSize){
					//если попали сюда то данные были только на 
					//одной странице а это значит что не может быть
					//в конце сттаницы только части записи об объекте
					throw -1;
				}
				else{
					iNewDataPage->add(newRecordData, newSize);
				}
			}
		}
		else{
			std::pair<int,CPage*>* p = new std::pair<int,CPage*>();
			p->first = -1;
			p->second = dataPage;
			pagesThread.push(p);
		}
		
		while(!pagesThread.empty()){
			std::pair<int,CPage*>* p = pagesThread.top();
			p->first++;
			CDataIndexPage* dPage = (CDataIndexPage*)(p->second);
			if(dPage->GetRecordCounts() <= p->first){
				pagesThread.pop();
				delete p;
				continue;
			}
			int pageNum = dPage->GetRecordValue(p->first);
			CPage* dataPage = db->GetPage(pageNum,ptEMPTYDATA);
			lvl = dataPage->GetIxLevel();
			//если страница не индексна€ то перебираем еЄ записи
			//и к каждой добавл€ем байты дл€ нового пол€
			if(lvl = 0){
				int readed = 0;
				if(prevOverSize > 0){
					dataPage->in( newRecordData + oldSize - prevOverSize, readed, prevOverSize);
					readed += prevOverSize;
					iNewDataPage->add(newRecordData, newSize);
				}
				prevOverSize = 0;
				while(readed < dataPage->GetEmpty()){
					int cnt = (dataPage->GetEmpty()-readed) >= oldSize ? oldSize : (dataPage->GetEmpty()-readed);
					dataPage->in( newRecordData, readed, cnt);
					readed += cnt;
					if (cnt < oldSize){
						prevOverSize = oldSize - cnt;
					}
					else{
						iNewDataPage->add(newRecordData, newSize);
					}
				}
			}
			else{
				std::pair<int,CPage*>* p = new std::pair<int,CPage*>();
				p->first = -1;
				p->second = dataPage;
				pagesThread.push(p);
			}
		}
		delete[] newRecordData;

		CreateField(fld);
		return 0;
	}
	int CTable::DeleteField(const char* fldName, CDB* db){
		std::vector<CSysField*>::const_iterator it;
		int i;
		for(it = fields.begin(), i = 0; it != fields.end(); it++, i++){
			if(strcmp(fldName, (*it)->GetName()) == 0){
				(*it)->Delete();
				Write(db);
				return i;
			}
		}
		return -1;
	}
	int CTable::GetSize() const{
		int result = sizeof(int64_t);// в этут область пам€ти запишим указатель на базовый объект
		std::vector<CSysField*>::const_iterator it;
		for(it = fields.begin(); it != fields.end(); it++){
			result += (*it)->GetSize();
		}
		return result;
	}
	int CTable::GetMasterRecordSize(){
		if(masterRecSize < 0){
			masterRecSize = sizeof(dataPageNumber) + sizeof(stringsPageNumber) +
				sizeof(binPageNumber) + sizeof(fieldsOffset) + 
				sizeof(isCrypted) + sizeof(ptrBaseObject) + sizeof(nameOffset);
		}
		return masterRecSize;
	}
	int64_t CTable::Write(CDB* db){
		//запишем базовый объект, если он есть, и получим его поинтер
		if(baseObject != NULL){
			ptrBaseObject = baseObject->Write(db);
		}
		
		//дл€ начала запишем каждое из полей объекта в объект FieldTable
		//и получим дл€ каждого поинтер записи в список fields_ptrs
		int fieldsCount = fields.size();
		int64_t* fields_ptrs = new int64_t[fieldsCount];
		std::vector<CSysField*>::const_iterator it;
		int i;
		for(it = fields.begin(), i = 0; it != fields.end(); it++, i++){
			fields_ptrs[i] = (*it)->Write(db);
		}
		//затем запишем этот список на страницу bin и смещение 
		//с этой страницы положим в fieldsOffset
		char* buff = new char[8*fieldsCount + 4];
		int allocated = 8*fieldsCount;
		int pos = 0;
		memcpy(buff + pos, &allocated, 4);
		pos += 4;
		for(i = 0; i < fieldsCount; i++){
			memcpy(buff + pos, &fields_ptrs[i], 8);
			pos += 8;
		}
		delete[] fields_ptrs;
		int pageNum = db->GetMasterBinPN();
		CBinPage* binPage = (CBinPage*)db->GetPage(pageNum,ptBINDATA);
		if(fieldsOffset < 0){
			fieldsOffset = binPage->add(buff,8*fieldsCount+4);
		}
		else{
			int alloc = 0;
			binPage->in(&alloc, fieldsOffset, 4);
			if(alloc >= allocated){
				binPage->out(fieldsOffset, buff, 8*fieldsCount+4);
			}
			else{
				fieldsOffset = binPage->add(buff, 8*fieldsCount + 4);
			}
		}
		delete[] buff;
		//теперь запишем им€ таблицы на строковую таблицу
		//и полученое смещение положим в nameOffset;
		pageNum = db->GetMasterStringsPN();
		CStringsPage* strPage = (CStringsPage*)db->GetPage(pageNum,ptSTRINGDATA);

		int allocatedLength = strlen(name)+1;
		buff = new char[allocatedLength+4];
		memcpy(buff, &allocatedLength, 4);
		memcpy(buff + 4, name, allocatedLength);	
		if(nameOffset < 0){
			nameOffset = strPage->add(buff,allocatedLength+4);
		}
		else{
			int alTmp = 0;
			strPage->in(&alTmp, nameOffset, 4);
			if(alTmp >= allocatedLength){
				strPage->out(nameOffset + 4, name,allocatedLength);
			}
			else{
				nameOffset = strPage->add(buff,allocatedLength+4);
			}
		}
		delete[] buff;
		//теперь запишем объект
		buff = new char[GetMasterRecordSize()];
		pos = 0;
		memcpy(buff+pos,&ptrBaseObject, sizeof(ptrBaseObject));
		pos += sizeof(ptrBaseObject);
		memcpy(buff+pos, &isCrypted, sizeof(isCrypted));
		pos += sizeof(isCrypted);
		memcpy(buff+pos, &dataPageNumber, sizeof(dataPageNumber));
		pos += sizeof(dataPageNumber);
		memcpy(buff+pos, &stringsPageNumber, sizeof(stringsPageNumber));
		pos += sizeof(stringsPageNumber);
		memcpy(buff+pos, &binPageNumber, sizeof(binPageNumber));
		pos += sizeof(binPageNumber);
		memcpy(buff+pos, &fieldsOffset, sizeof(fieldsOffset));
		pos += sizeof(fieldsOffset);
		memcpy(buff+pos, &nameOffset, sizeof(nameOffset));
		pos += sizeof(nameOffset);
		
		pageNum = db->GetMasterDataPN();
		CDataPage* dataPage = (CDataPage*)db->GetPage(pageNum,ptEMPTYDATA);
		if(recNum < 0){
			recNum = dataPage->add(buff,pos)/pos;
		}
		else{
			dataPage->out(recNum * pos, buff, pos);
		}
		return recNum;
	}
	int CTable::Read(CDB* db){
		int result = -1;
		if(recNum < 0){
			return result;
		}
		int pageNum = db->GetMasterDataPN();
		CDataPage* dp = (CDataPage*)db->GetPage(pageNum, ptEMPTYDATA);
		int size = GetMasterRecordSize();
		int pos = 0;
		dp->in(&ptrBaseObject,size*recNum + pos, sizeof(ptrBaseObject));
		pos += sizeof(ptrBaseObject);
		dp->in( &isCrypted,size*recNum+pos, sizeof(isCrypted));
		pos += sizeof(isCrypted);
		dp->in( &dataPageNumber,size*recNum+pos, sizeof(dataPageNumber));
		pos += sizeof(dataPageNumber);
		dp->in(&stringsPageNumber, size*recNum+pos, sizeof(stringsPageNumber));
		pos += sizeof(stringsPageNumber);
		dp->in(&binPageNumber, size*recNum+pos, sizeof(binPageNumber));
		pos += sizeof(binPageNumber);
		dp->in(&fieldsOffset, size*recNum+pos, sizeof(fieldsOffset));
		pos += sizeof(fieldsOffset);
		dp->in(&nameOffset, size*recNum+pos, sizeof(nameOffset));
		pos += sizeof(nameOffset);
		result = pos;

		pageNum = db->GetMasterBinPN();
		CBinPage* bp = (CBinPage*)db->GetPage(pageNum, ptBINDATA);
		int fldAlloc = 0;
		bp->in(&fldAlloc, fieldsOffset,4);
		pos = 4;
		int fldCount = fldAlloc / 8;
		int64_t fields_ptr = -1;
		for(int i = 0; i < fldCount; i++){
			bp->in(&fields_ptr, fieldsOffset + pos, 8);
			pos += 8;
			CSysField* fld = new CSysField(fields_ptr, db);
			fields.push_back(fld);
		}

		pageNum = db->GetMasterStringsPN();
		CStringsPage* sp = (CStringsPage*)db->GetPage(pageNum,ptSTRINGDATA);
		int alloc = 0;
		sp->in(&alloc, nameOffset, 4);
		name = new char[alloc];
		sp->in(name, nameOffset+4, alloc);
		return result;
	}
	const int64_t CTable::GetRecordsCount(CDB* db) const{
		int pageNum = GetDataPageNumber();
		if(pageNum < 0){
			throw -1;
		}
		CIndexPage* dataPage = (CIndexPage*)db->GetPage(pageNum,ptDATAINDEX);
		CPage* p = dataPage->GetLastIndexedPage();
		PageTypes type = p->GetPageType();
		if((((~ptINDEXMASK)&type) & (ptSTRINGDATA | ptBINDATA | ptEMPTYDATA)) == 0){
			throw -1;
		}
		int64_t result = p->GetEmpty() + p->GetOffset();
		return result / GetSize();
	}
	std::vector<CDBObject*>* CTable::ReadRecords(const std::vector<int64_t> records, CDB* db){
		std::vector<CDBObject*>* result = new std::vector<CDBObject*>;
		std::vector<int64_t>::const_iterator it;
		for(it = records.begin(); it != records.end(); it++){
			CDBObject* obj = new CDBObject(this, *it);
			obj->Read(db);
			result->push_back(obj);
		}

		return result;
	}
	void CTable::SetBaseTable(std::map<int,CTable*> masterTable){
		if(ptrBaseObject >=0){
			baseObject = masterTable[ptrBaseObject];
		}
	}
}