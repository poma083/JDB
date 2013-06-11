#include <fstream>
#include <vector>
#include <list>
#include "ExTypes.h"
//#include "CPage.h"
//#include "CBaseHeader.h"
#include "KernelJDBClasses.h"
//#include "CDBObject.h"

namespace JDB{
	CDB::CDB(){
		masterTable.clear();
	}
	CDB::~CDB(){
		ClearMasterTable();
		std::map<int, CPage*>::iterator pageIterator;
		int i = 0;
		for(pageIterator = _pages.begin(); pageIterator != _pages.end(); pageIterator++ ){
			CPage* p = pageIterator->second;
			i++;
			if(p != NULL){
				if(p->NeedUpdate()){
					dbFile.seekp(p->GetPageNum() * header->PageSize());
					dbFile.write(p->GetContent(),header->PageSize());
					//dbFile.flush();
				}
				delete p;
			}
		}
		delete header;
		dbFile.close();
		if(fName != NULL){
			delete[] fName;
		}
		
	}
	void CDB::Flush(){
		std::map<int, CPage*>::iterator pageIterator;
		for(pageIterator = _pages.begin(); pageIterator != _pages.end(); pageIterator++ ){
			CPage* p = pageIterator->second;
			if(p != NULL){
				if(p->NeedUpdate()){
					dbFile.seekp(p->GetPageNum() * header->PageSize());
					dbFile.write(p->GetContent(),header->PageSize());
				}
			}
		}
		dbFile.flush();
	}
	void CDB::Create(char* fname, unsigned int PageSize){
		fName = new char[strlen(fname)+1];
		strcpy(fName, fname);
		dbFile.open(fname, std::ios::out | std::ios::in | std::ios::_Noreplace | std::ios::trunc | std::ios::binary );
		if(!dbFile){
			printf("fopen error\n");
			return;
		}

		CHeaderPage* pageHeader = new CHeaderPage(0, this, PageSize);
		SetPage((CPage*)pageHeader);
		//pages[0] = (CPage*)pageHeader;
		header = new CBaseHeader(pageHeader,PageSize);

		int ipsNumber = 2;
		CStatePage* ps = new CStatePage(1, this, header->PageSize());
		CStateIndexPage* ips = new CStateIndexPage(ipsNumber, this, header->PageSize());
		SetPage(ips);
		SetPage(ps);
		ips->SetIxLevel(1);
		ips->InsertIndex(1,ipsNumber);
		ps->SetState(0);
		ps->SetState(1);
		ps->SetState(ipsNumber);
		ps->SetEmpty(1);
		//ps->SetOverPageNumber(ipsNumber);
		pageHeader->SetStatePN(ipsNumber);

		for(int i = 2; i < 8; i++){
			dbFile.seekp(header->PageSize() * i);
			CPage* p = new CPage(i, this, header->PageSize());
			p->NeedUpdate(true);
			dbFile.write(p->GetContent(),header->PageSize());
			delete[] p;
		}

		pageHeader->SetMasterBinPN(GetNumberEmptyPage(true));
		pageHeader->SetMasterStringsPN(GetNumberEmptyPage(true));
		pageHeader->SetMasterDataPN(GetNumberEmptyPage(true));
		pageHeader->SetFieldsDataPN(GetNumberEmptyPage(true));
		pageHeader->SetFieldsStringsPN(GetNumberEmptyPage(true));

		int indexPageNumber = GetMasterBinPN();
		CPage* indexPage = GetPage(indexPageNumber,ptBININDEX);
		int pageNumber = GetNumberEmptyPage(true);
		CPage* page = GetPage(pageNumber,ptBINDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);

		indexPageNumber = GetMasterStringsPN();
		indexPage = GetPage(indexPageNumber,ptSTRINGINDEX);
		pageNumber = GetNumberEmptyPage(true);
		page = GetPage(pageNumber,ptSTRINGDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);

		indexPageNumber = GetMasterDataPN();
		indexPage = GetPage(indexPageNumber,ptDATAINDEX);
		pageNumber = GetNumberEmptyPage(true);
		page = GetPage(pageNumber,ptEMPTYDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);

		indexPageNumber = GetFieldsDataPN();
		indexPage = GetPage(indexPageNumber,ptDATAINDEX);
		pageNumber = GetNumberEmptyPage(true);
		page = GetPage(pageNumber,ptEMPTYDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);

		indexPageNumber = GetFieldsStringsPN();
		indexPage = GetPage(indexPageNumber,ptSTRINGINDEX);
		pageNumber = GetNumberEmptyPage(true);
		page = GetPage(pageNumber,ptSTRINGDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);
	}
	void CDB::Open(char* fname){
		fName = new char[strlen(fname)+1];
		strcpy(fName, fname);
		dbFile.open(fname, std::ios::out | std::ios::in | std::ios::binary );
		if(!dbFile){
			printf("fopen error\n");
			return;
		}
		char* buff = new char[sizeof(HeadPage)+44];
		dbFile.seekg(0);
		dbFile.read(buff, sizeof(HeadPage)+44);
		CHeaderPage* p = new CHeaderPage(buff, this);
		int PageSize = p->GetPageSize();
		delete p;
		//delete[] buff;

		buff = new char[PageSize];
		dbFile.seekg(0);
		dbFile.read(buff, PageSize);
		p = new CHeaderPage(buff, this);
		SetPage((CPage*)p);
		//pages[0] = (CPage*)p;
		header = new CBaseHeader(p,PageSize);

		LoadTables();
	}
	void CDB::LoadTables(){
		ClearMasterTable();
		int tblCount = GetCountTables();
		for(int i = 0; i < tblCount; i++){
			CTable* t = new CTable(i,NULL,"");
			t->Read(this);
			masterTable[t->GetRecNumber()] = t;
		}
		std::map<int,CTable*>::iterator tableIterator;
		for(tableIterator = masterTable.begin(); tableIterator != masterTable.end(); tableIterator++){
			CTable* t = tableIterator->second;
			t->SetBaseTable(masterTable);
		}
	}
	CPage* CDB::GetPage(int i){
		return _pages[i];
	}
	void CDB::SetPage(CPage* page){
		_pages[page->GetPageNum()] = page;
	}
	void CDB::ClearMasterTable(){
		std::map<int,CTable*>::iterator tableIterator;
		for(tableIterator = masterTable.begin(); tableIterator != masterTable.end(); tableIterator++){
			CTable* t = tableIterator->second;
			int recordNumber = tableIterator->first;
			delete t;
			t = NULL;
			masterTable[recordNumber] = NULL;
		}
	}
	CPage* CDB::GetPage(int& _pageNum,PageTypes pt){
		CPage* result = GetPage(_pageNum);//pages[_pageNum];
		if(result == NULL){
			if(_pageNum < 0){
				//вернуть номер свободной страницы
				//в статусной сранице установить флаг если страница свободная
				_pageNum = GetNumberEmptyPage(true);
				if(_pageNum == 0){
					_pageNum = -1;
				}
			};
			dbFile.seekg(_pageNum * header->PageSize());
				
			char* buff = new char[header->PageSize()];
			dbFile.read(buff,header->PageSize());
			HeadPage* ph = (HeadPage*)buff;
			switch(ph->GetPageType()){
			case ptEMPTY:
				switch(pt){
				case ptSTATE:
					result = (CPage*) new CStatePage(buff, this);
					break;
				case ptSTRINGDATA:
					result = (CPage*) new CStringsPage(buff, this);
					break;
				case ptSTRINGINDEX:
					result = new CStringIndexPage(buff, this);
					break;
				case ptBINDATA:
					result = (CPage*) new CBinPage(buff, this);
					break;
				case ptBININDEX:
					result = new CBinIndexPage(buff, this);
					break;
				case ptEMPTYDATA:
					result = (CPage*) new CDataPage(buff, this);
					break;
				case ptDATAINDEX:
					result = new CDataIndexPage(buff, this);
					break;
				default:
					result = new CPage(buff, this);
					break;
				}
				result->SetPageType(pt);
				break;
			case ptSTATE:
				if((pt != ptSTATE)&&(pt != ptEMPTY)){
					throw -1;
				}
				result = (CPage*) new CStatePage(buff,this);
				break;
			case ptEMPTYDATA:
				if((pt != ptEMPTYDATA)&&(pt != ptEMPTY)){
					throw -1;
				}
				result = (CPage*) new CDataPage(buff,this);
				break;
			case ptBINDATA:
				if((pt != ptBINDATA)&&(pt != ptEMPTY)){
					throw -1;
				}
				result = (CPage*) new CBinPage(buff,this);
				break;
			case ptSTRINGDATA:
				if((pt != ptSTRINGDATA)&&(pt != ptEMPTY)){
					throw -1;
				}
				result = (CPage*) new CStringsPage(buff,this);
				break;
			case ptDATAINDEX:
				if((pt != ptEMPTYDATA)&&(pt != ptDATAINDEX)){
					throw -1;
				}
				result = (CPage*) new CDataIndexPage(buff,this);
				break;
			case ptBININDEX:
				if((pt != ptBINDATA)&&(pt != ptBININDEX)){
					throw -1;
				}
				result = (CPage*) new CBinIndexPage(buff,this);
				break;
			case ptSTRINGINDEX:
				if((pt != ptSTRINGDATA)&&(pt != ptSTRINGINDEX)&&(pt != ptEMPTY)){
					throw -1;
				}
				result = (CPage*) new CStringIndexPage(buff,this);
				break;
			case ptSTATEINDEX:
				if(pt != ptSTATEINDEX){
					throw -1;
				}
				result = (CPage*) new CStateIndexPage(buff,this);
				break;
			default:
				throw -1;
				result = new CPage(buff, this);
				break;
			}
			SetPage(result);
			//pages[_pageNum] = result;
		}
		return result;
	}
	int CDB::GetNumberEmptyPage(bool reserved){
		int contentSize = GetPageSize(true);
		int numberStatePage = header->GetStatePN();
		//это статусная страница самого верхнего уровня!!!
		CStateIndexPage* page = (CStateIndexPage*)GetPage(numberStatePage,ptSTATEINDEX);
			
		/*
		PageTypes type = page->GetPageType();
		switch(type){
		case ptSTATE:
			int empty = page->GetEmpty();
			if(empty >= contentSize){
				//пора выделить новую статусную страницу
				throw -1;
			}
			break;
		case ptINDEXSTATE:
			break;
		default:
			throw -1;
			break;
		}
		//*/
		
		
		int result = page->GetFreePage();
		
		if(result < 0){
			int totalPages = page->GetTotalPages();
			int addcnt = 1;
			//throw -1;
			CStatePage* lastPage = page->GetLastStatePage();
			int empty = lastPage->GetEmpty();
			if(empty >= contentSize){
				CStatePage* p = new CStatePage(totalPages, this, header->PageSize());
				p->NeedUpdate(true);
				SetPage(p);
				//pages[p->GetPageNum()] = p;
				totalPages++;
				int overPageNumber = lastPage->GetOverPageNum();
				CStateIndexPage* overPage = (CStateIndexPage*)GetPage(overPageNumber,ptSTATEINDEX);
				overPage->InsertIndex(p->GetPageNum(),overPageNumber);
				//p->SetOverPageNum(overPageNumber);
				dbFile.seekp(header->PageSize() * p->GetPageNum());
				dbFile.write(p->GetContent(),header->PageSize());
				p->SetState(0);
				lastPage = p;
				empty = lastPage->GetEmpty();
			}
			empty += addcnt;
			lastPage->SetEmpty(empty);
			for(int i = 0; i < addcnt*8; i++){
				dbFile.seekp(header->PageSize() * (totalPages + i));
				CPage* p = new CPage(totalPages + i, this, header->PageSize());
				p->NeedUpdate(true);
				dbFile.write(p->GetContent(),header->PageSize());
				delete[] p;
			}
			
			result = totalPages;
		}
		if(reserved){
			page->SetState(result);
		}
		return result;
	}
	CTable* CDB::CreateTable(const char* tableName, CTable* baseTable){
		JDB::CTable* table = new JDB::CTable(-1,baseTable,tableName);
		table->SetDataPageNumber(this->GetNumberEmptyPage(true));
		table->SetStringsPageNumber(this->GetNumberEmptyPage(true));
		table->SetBinPageNumber(this->GetNumberEmptyPage(true));
	
		int indexPageNumber = table->GetBinPageNumber();
		JDB::CPage* indexPage = this->GetPage(indexPageNumber,JDB::ptBININDEX);
		int pageNumber = this->GetNumberEmptyPage(true);
		JDB::CPage* page = this->GetPage(pageNumber,JDB::ptBINDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((JDB::CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);

		indexPageNumber = table->GetStringsPageNumber();
		indexPage = this->GetPage(indexPageNumber,JDB::ptSTRINGINDEX);
		pageNumber = this->GetNumberEmptyPage(true);
		page = this->GetPage(pageNumber,JDB::ptSTRINGDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((JDB::CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);

		indexPageNumber = table->GetDataPageNumber();
		indexPage = this->GetPage(indexPageNumber,JDB::ptDATAINDEX);
		pageNumber = this->GetNumberEmptyPage(true);
		page = this->GetPage(pageNumber,JDB::ptEMPTYDATA);
		indexPage->SetIxLevel(page->GetIxLevel()+1);
		((JDB::CIndexPage*)indexPage)->InsertIndex(pageNumber,indexPageNumber);

		table->Write(this);
		this->masterTable[table->GetRecNumber()] = table;

		return table;
	}
	char* CDB::GetFieldsInfo(char* buff, const char* tableName, int& cnt) const{
		std::map<int,CTable*>::const_iterator tableInfo;
		for(tableInfo = masterTable.begin(); tableInfo != masterTable.end(); tableInfo++){
			if(strcmp(tableName,(*tableInfo).second->GetName()) == 0){
				return (*tableInfo).second->GetFieldsInfo(buff,cnt);
			}
		}
	}
	char* CDB::GetTablesInfo(char* buff, int& cnt) const{
		cnt = 0;
		std::map<int,CTable*>::const_iterator tableInfo;
		for(tableInfo = masterTable.begin(); tableInfo != masterTable.end(); tableInfo++){
			cnt += strlen((*tableInfo).second->GetName()) + 1;
		}
		if(buff == NULL){
			buff = new char[cnt];
		}
		cnt = 0;
		for(tableInfo = masterTable.begin(); tableInfo != masterTable.end(); tableInfo++){
			strcpy(buff + cnt, (*tableInfo).second->GetName());
			cnt += strlen((*tableInfo).second->GetName()) + 1;
		}
		return buff;
	}
	int CDB::GetCountTables(){
		//получим количествр записей со всех страниц данных мастер-таблицы
		int result = 0;
		int pNum = header->GetMasterDataPN();
		CDataPage* p = (CDataPage*)GetPage(pNum,ptEMPTYDATA);
		CTable t(-1,NULL,"");
		int overload = 0;
		int tableSize = t.GetMasterRecordSize();
		result = p->GetCountTables(tableSize,overload);
		result += overload/tableSize;
		/*throw -1;
		PageTypes type = p->GetPageType();
		if(type == ptDATAINDEX){
			int records = ((CIndexPage*)p)->GetRecordCounts();
			for(int i = 0; i < records; i++){
				int pageNum = ((CIndexPage*)p)->GetRecordValue(i);
				CDataPage* page = (CDataPage*)GetPage(pageNum,ptEMPTYDATA);
			}
		}*/
		return result;
	}
	void CDB::ReservePage(int pageNumber){
		throw -1;
		int numberStatePage = 1;
		CStatePage* StatePage = (CStatePage*)GetPage(numberStatePage, ptSTATE);
		StatePage->SetState(pageNumber);
	}
	unsigned int CDB::GetPageSize( bool contentOnly){
		if(contentOnly){
			return header->PageSize() - sizeof(HeadPage);
		}
		else{
			throw -1;
		}
	}
}