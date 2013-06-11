#include "KernelJDBClasses.h"
#include <vector>
#include <math.h>

namespace JDB{
	CPage::CPage(int _pageNum, CDB* _db, unsigned int _pageSize){
		//printf("%d\n", sizeof(HeadPage));
		content = new char[_pageSize];
		memset(content, 0,_pageSize);
		head = (HeadPage*)content;
		head->SetPageNum(_pageNum);
		head->SetOverPageNum(-1);
		head->SetIxLevel(0);
		head->SetPageType(ptEMPTY);
		head->SetEmpty(0);
		head->SetOffset(-1);
		head->NeedUpdate(false);
		db = _db;

		if(head->GetPageNum() >= 0){
			//необходимо загрузить данные со страницы pageNum
		}
	}
	CPage::CPage(char* buffer, CDB* _db){
		content = buffer;
		head = (HeadPage*)content;
		head->NeedUpdate(false);
		head->SetOffset(-1);
		db = _db;
	}
	CPage::~CPage(){
		flush();
		if(content != NULL){
			delete[] content;
			content = NULL;
		};
	}

	void CPage::in(void* dst, unsigned int pos, unsigned int count)const{
		//предпологаем что данные синхронизированы с диском если нужно
		memcpy(dst, content + sizeof(HeadPage) + pos, count);
	}
	
	void CPage::out(unsigned int pos, void* src, unsigned int count){
		memcpy(content + sizeof(HeadPage) + pos, src, count);
		if(head->GetEmpty() < pos + count){
			head->SetEmpty(pos + count);
		}
		head->NeedUpdate(true);
	}
	int64_t CPage::add(void* src, unsigned int count){
		int64_t result = head->GetEmpty();
		unsigned int size = db->GetPageSize(true);
		unsigned int cnt = size - result;
		if( cnt > count){
			cnt = count;
		}
		memcpy(content + sizeof(HeadPage) + result, src, cnt);
		head->SetEmpty(result + cnt);
		head->NeedUpdate(true);
		return result;
	}
	void CPage::flush(){
		if(head->NeedUpdate()){
			//тут делаем сброс данных на диск (или куда-нибудь в др. место)
			head->NeedUpdate(false);//данные синхронизированы с диском
		}
	}
	void CPage::SetPageNum(int number){
		if(number == 0){
			head->SetPageNum(number);
		}
		head->SetPageNum(number);
	}
	int CPage::GetPageNumUpLevel()const{
		int m_result = GetOverPageNum();
		int result = m_result;
		while(m_result > 0){
			CIndexPage* ixPage = NULL;
			ixPage = (CIndexPage*)db->GetPage(result, ptEMPTY);
			m_result = ixPage->GetOverPageNum();
			if(m_result >= 0){
				result = m_result;
			}
		}
		return result;
	}
	int64_t CPage::GetOffset(){
		int64_t result = head->GetOffset();
		if(result >= 0){
			return result;
		}
		else{
			result = 0;
		}
		int64_t power = 0;
		int contentSize = db->GetPageSize(true);
		int maxRecords = contentSize / sizeof(int);
		int thisPageNumber = GetPageNum();
		int overPageNumber = GetOverPageNum();
		int i = 0;
		while(overPageNumber > 0){
			CIndexPage* overPage = (CIndexPage*)db->GetPage(overPageNumber,ptEMPTY);
			power = pow(maxRecords, (double)(overPage->GetIxLevel()-1));
			int recordCount = overPage->GetRecordCounts();
			int record = -1;
			for(i = 0; i < recordCount; i++){
				if(overPage->GetRecordValue(i) == thisPageNumber){
					record = i;
					break;
				}
			}
			if(record < 0){
				throw -1;
			}
			result += power*contentSize*record;
			overPageNumber = overPage->GetOverPageNum();
			thisPageNumber = overPage->GetPageNum();
		}
		head->SetOffset(result);
		return result;
	}
	//--------------------------   CHeaderPage   ---------------------------//
	CHeaderPage::CHeaderPage(int _pageNum, CDB* _db, unsigned int _pageSize) : CPage(_pageNum, _db, _pageSize){
		char* cryptMethod = new char[32];
		memset(cryptMethod,0,32);
		strcpy(cryptMethod,"None");
		int isCrypted = 0;
		unsigned int id = 0x30313233;
		int soDataPageNumber = 3;
		int soStringsPageNumber = 5;
		int soBinPageNumber = 7;
		int fldDataPageNumber = 4;
		int fldStringsPageNumber = 6;
		int fldBinPageNumber = 8;
		
		int pos = 0;
		out(pos,&id,sizeof(id));
		pos += sizeof(id);
		out(pos,&isCrypted,sizeof(isCrypted));
		pos += sizeof(isCrypted);
		out(pos,cryptMethod,32*sizeof(char));
		pos += 32*sizeof(char);
		out(pos,&_pageSize,sizeof(_pageSize));
		pos += sizeof(_pageSize);

		SetPageType(ptHEADER);
	}
	unsigned int CHeaderPage::GetPageSize(){
		unsigned int ps = 0;
		this->in(&ps, 40, 4);
		return ps;
	}
	void CHeaderPage::SetPageSize(unsigned int pageSize){
		this->out(40,&pageSize, sizeof(int));
	}
	int CHeaderPage::GetMasterBinPN()const{
		int p = 0;
		this->in(&p, 60, sizeof(int));
		return p;
	}
	int CHeaderPage::GetMasterStringsPN()const{
		int p = 0;
		this->in(&p, 52, sizeof(int));
		return p;
	}
	int CHeaderPage::GetMasterDataPN()const{
		int p = 0;
		this->in(&p, 44, sizeof(int));
		return p;
	}
	int CHeaderPage::GetFieldsDataPN()const{
		int p = 0;
		this->in(&p, 68, sizeof(int));
		return p;
	}
	int CHeaderPage::GetFieldsStringsPN()const{
		int p = 0;
		this->in(&p, 76, sizeof(int));
		return p;
	}
	int CHeaderPage::GetFieldsBinPN()const{
		int p = 0;
		this->in(&p, 84, sizeof(int));
		return p;
	}
	int CHeaderPage::GetStatePN()const{
		int p = 0;
		this->in(&p, 92, sizeof(int));
		return p;
	}

	void CHeaderPage::SetMasterBinPN(int p){
		this->out(60, &p, sizeof(int));
	}
	void CHeaderPage::SetMasterStringsPN(int p){
		this->out(52,&p, sizeof(int));
	}
	void CHeaderPage::SetMasterDataPN(int p){
		this->out(44,&p, sizeof(int));
	}
	void CHeaderPage::SetFieldsDataPN(int p){
		this->out(68,&p, sizeof(int));
	}
	void CHeaderPage::SetFieldsStringsPN(int p){
		this->out(76,&p, sizeof(int));
	}
	void CHeaderPage::SetFieldsBinPN(int p){
		this->out(84,&p, sizeof(int));
	}
	void CHeaderPage::SetStatePN(int p){
		this->out(92,&p, sizeof(int));
	}
	//--------------------------   CStatePage   ---------------------------//
	CStatePage::CStatePage(int _pageNum, CDB* _db, unsigned int _pageSize) : CPage(_pageNum, _db, _pageSize){
		SetPageType(ptSTATE);
		lastNoFF = 0;
	}
	CStatePage::CStatePage( char* buffer, CDB* _db) : CPage(buffer, _db){
		lastNoFF = 0;
	}
	int CStatePage::GetFreePage(){
		int empty = this->GetEmpty();
		char mask = 0x01;
		for(int i = lastNoFF; i < empty; i++){
			if((this->GetContent()[sizeof(HeadPage)+i] & 0xff)!=0xff){
				for(int j = 0; j < 8; j++){
					if(!(mask & this->GetContent()[sizeof(HeadPage)+i])){
						lastNoFF = i;
						return (i * 8) + j;
					};
					mask <<= 1;
				};
			};
		};
		return -1;
	}
	void CStatePage::SetState(int pNum){
		unsigned char mask = 0x01;
		int numByte = pNum/8;
		//int hsize = sizeof(HeadPage);
		this->GetContent()[sizeof(HeadPage) + numByte] |= (mask << pNum % 8);
		this->NeedUpdate(true);
	}
	void CStatePage::UnsetState(int pNum){
		unsigned char mask = 0x01;
		int numByte = pNum/8;
		mask <<= pNum%8;
		this->GetContent()[sizeof(HeadPage) + numByte] &= ~mask;
		this->NeedUpdate(true);
		lastNoFF = numByte;
	}
	int CStatePage::GetTotalPages(){
		int result = 0;
		int overPageNum = GetOverPageNum();
		if(overPageNum > 0){
			CStateIndexPage* overPage = (CStateIndexPage*)db->GetPage(overPageNum, ptSTATEINDEX);
			overPage->GetTotalPages();
		}
			
		return this->GetEmpty()*8;
	}
	//--------------------------   CDataPage   ---------------------------//
	CDataPage::CDataPage(int _pageNum, CDB* _db, unsigned int _pageSize) : CPage(_pageNum, _db, _pageSize){
		SetPageType(ptEMPTYDATA);
	}
	int CDataPage::GetCountTables(int tableSize, int& overload)const{
		//получим количествр записей со страницы данных
		int result = 0;
		result = GetEmpty()/tableSize;
		overload += GetEmpty()%tableSize;
		return result;
	}
	//--------------------------   CDataIndexPage ------------------------//
	int CDataIndexPage::GetCountTables(int tableSize, int& overload)const{
		//получим количествр записей со всех страниц данных мастер-таблицы
		int result = 0;
		int records = GetRecordCounts();
		for(int i = 0; i < records; i++){
			int pageNum = GetRecordValue(i);
			CDataPage* page = (CDataPage*)db->GetPage(pageNum,ptEMPTYDATA);
			result += page->GetCountTables(tableSize, overload);
		}
		return result;
	}
	//--------------------------   CStringsPage   ---------------------------//
	CStringsPage::CStringsPage(int _pageNum, CDB* _db, unsigned int _pageSize) : CPage(_pageNum, _db, _pageSize){
		SetPageType(ptSTRINGDATA);
	}
	CStringsPage::CStringsPage(char* buffer, CDB* _db) : CPage(buffer, _db){
		//_db = db;
	}
	int64_t CStringsPage::add(void* src, unsigned int count){
		int64_t result = CPage::add(src, count);
		int64_t resultEx = 0;
		unsigned int diferent = db->GetPageSize(true) - result;
		/*if(diferent <= count){
			//возмём страницу более верхнего уровня, если она есть.
			int overPageNum = GetOverPageNum();
			CStringIndexPage* overPage = (CStringIndexPage*)db->GetPage(overPageNum, ptSTRINGINDEX);
			if(GetOverPageNum() < 0){
				//если такой нет то создадим её и запишем на неё текущюю страницу
				SetOverPageNum(overPageNum);
				overPage->SetIxLevel(GetIxLevel() + 1);
				int thisPN = GetPageNum();
				overPage->InsertIndex(thisPN, overPageNum);
			}
			//здесь нужно взять последнюю строковую страницу и 
			//только если она полная то создать новую
			//сейчас новая создаётся всегда
			if(overPageNum >= 2360){
				int tmp = db->GetNumberEmptyPage(true);
			}
			int string_pn = db->GetNumberEmptyPage(true);
			CStringsPage* exPage = (CStringsPage*)db->GetPage(string_pn, ptSTRINGDATA);
			resultEx = overPage->InsertIndex(string_pn,overPageNum);
			exPage->SetOverPageNum(overPageNum);
			resultEx += exPage->add((char*)src+diferent, count-diferent);
			if(diferent == 0){
				result = resultEx;
			}
		}*/
		return result;
	}
	void CStringsPage::out(unsigned int pos, void* src, unsigned int count){
		CPage::out(pos, src, count);
		unsigned int diferent = db->GetPageSize() - pos;
		if(diferent <= count){
			throw -1;
			//возмём страницу более верхнего уровня, если она есть.
			int overPageNum = GetOverPageNum();
			CStringIndexPage* overPage = (CStringIndexPage*)db->GetPage(overPageNum, ptSTRINGINDEX);
			if(GetOverPageNum() < 0){
				//если такой нет то создадим её и запишем на неё текущюю страницу
				//такого быть не должно, поэтому выбросим исключение
				throw -1;
				//SetOverPageNumber(overPageNum);
				overPage->SetIxLevel(GetIxLevel() + 1);
				int thisPN = GetPageNum();
				overPage->InsertIndex(thisPN, overPageNum);
			}
			
			int string_pn = overPage->NextPageNumber(this->GetPageNum());
			CStringsPage* exPage = (CStringsPage*)db->GetPage(string_pn, ptSTRINGDATA);
			exPage->out(0, (char*)src+count-diferent, count-diferent);
		}
	}
	//--------------------------   CBinPage   ---------------------------//
	CBinPage::CBinPage(int _pageNum, CDB* _db, unsigned int _pageSize) : CPage(_pageNum, _db, _pageSize){
		SetPageType(ptBINDATA);
	}

	//--------------------------   CStringIndexPage   ---------------------------//
	int CStringIndexPage::FindRecordNumber(int cur){
		throw -1;
		int* recPtr = NULL;
		
		int begin = 0;
		int end = GetEmpty()/sizeof(int);
		int interval = end-begin;
		int avg = (end+begin)/2;
		while(interval > 0){
			recPtr = (int*)(GetContent() + sizeof(HeadPage)) + avg;
			if(*recPtr == cur){
				break;
			}
			else{
				if(*recPtr < cur){
					begin = avg + 1;
				}
				else{
					end = avg;
				}
			}
			//result = GetContent() + avg;
			avg = (end+begin)/2;
			interval = end - begin;
		}
		return avg;
	}
	int CStringIndexPage::NextPageNumber(int cur){
		throw -1;
		int* result = (int*)GetContent() + sizeof(HeadPage);
		int rec = FindRecordNumber(cur);
		int recCount = GetEmpty()/sizeof(int);
		if(rec < recCount-1){
			result = result + rec;
			if(*result = cur){
				result += 1;
			}
			return *result;
		}
		return -1;
	}
	//--------------------------   CIndexPage   ---------------------------//
	/*int CIndexPage::GetFirstPageFromIndex(){
		int* result = (int*)(GetContent() + sizeof(HeadPage));
		return *result;
	}*/
	CPage* CIndexPage::GetLastIndexedPage(){
		int lastPageNum = GetLastRecordValue();
		CPage* lastPage = db->GetPage(lastPageNum, ptEMPTY);
		PageTypes type = lastPage->GetPageType();
		while((type & ptINDEXMASK) == ptINDEXMASK){
			lastPageNum = ((CIndexPage*)lastPage)->GetLastRecordValue();
			lastPage = db->GetPage(lastPageNum, ptEMPTY);
			type = lastPage->GetPageType();
		}
		return (CPage*)lastPage;
	}
	int CIndexPage::GetLastRecordValue(){
		int* result = (int*)(GetContent() + sizeof(HeadPage) + GetEmpty()) - 1;
		return *result;
	}
	int CIndexPage::GetRecordValue(int recNumber)const{
		int result = 0;
		memcpy(&result,GetContent() + sizeof(HeadPage) + recNumber*sizeof(int), sizeof(int));
		//int* result = (int*)(GetContent() + sizeof(HeadPage)) + recNumber;
		//return *result;
		return result;
	}
	int CIndexPage::GetRecordCounts() const{
		return GetEmpty()/sizeof(int);
	}
	int CIndexPage::GetMaxRecordCount() const{
		return (db->GetPageSize()-sizeof(HeadPage))/sizeof(int);
	}
	//функция для записи номера страницы на индексную
	//если места на индексной странице больше нет, то 
	//автоматически выделяется новая страница и информация
	//пишется уже на неё.
	//номер индексной страницы, на которую была
	//записана индексированая страница на самом деле,
	//возвращается в параметре indexPageNumber.
	//при необходимости функция выделяет индексные
	//страницы более высоких уровней
	int64_t CIndexPage::InsertIndex(int pageNumber, int& indexPageNumber){			
		int64_t result = 0;
		int64_t prevRes = 0;

		int usePN = this->GetPageNum();
		CIndexPage* use = this;
		int recCount = use->GetEmpty()/sizeof(int);
		int maxRecCount = (db->GetPageSize(true))/sizeof(int);
		
		if(recCount == maxRecCount){
			PageTypes PageType = use->GetPageType();
			//берём страницу более высокого уровня чтоб записать индекс на текущюю
			//если такой нет то создадим её
			int overPageNum = GetOverPageNum();
			CIndexPage* overPage = (CIndexPage*)db->GetPage(overPageNum, PageType);
			if(GetOverPageNum() < 0){
				//если такой небыло то запишем информацию ра неё о текущей
				overPage->SetIxLevel(this->GetIxLevel() + 1);
				overPage->InsertIndex(this->GetPageNum(), overPageNum);
				//SetOverPageNum(overPageNum);
			}
			//на странице нет места нужно выделить новую
			usePN = -1;
			use = (CIndexPage*)db->GetPage(usePN, PageType);
			use->SetIxLevel(this->GetIxLevel());
			prevRes += overPage->InsertIndex(usePN, overPageNum);
			//use->SetOverPageNum(overPageNum);
			indexPageNumber = usePN;
		}
		int* recPtr = (int*)(use->GetContent() + sizeof(HeadPage) + use->GetEmpty());
		use->SetEmpty(use->GetEmpty() + sizeof(int));
		*recPtr = pageNumber;

		CPage* indexedPage = db->GetPage(pageNumber,ptEMPTY);
		indexedPage->SetOverPageNumber(usePN);

		int contentSize = db->GetPageSize(true);
		int power = pow(maxRecCount, (double)use->GetIxLevel() - 1);
		result = power * contentSize;
		recCount = use->GetEmpty()/sizeof(int);
		return result*(recCount-1) + prevRes;
	}
	void CIndexPage::in(void* dst, unsigned int pos, unsigned int count)const{
		int contentSize = db->GetPageSize(true);
		int maxRecords = contentSize / sizeof(int);
		int power = pow(maxRecords, (double)(GetIxLevel()-1));
		power = power*contentSize;
		int record = pos/power;
		int recordsForCount = (count + power-1)/power;
		if((record + recordsForCount) > maxRecords){
			throw -1;
		}
		int readed = 0;
		int cnt = power - pos%power;
		int i;
		int j;
		for(i = record, j = 0; i < (record + recordsForCount); i++, j++){
			int pageNuber = GetRecordValue(i);
			CPage* page = db->GetPage(pageNuber, ptEMPTY);
			//page->in(dst, pos%power, count);
			if((count - readed) < cnt){
				cnt = count - readed;
			}
			page->in((char*)dst + readed, j == 0 ? pos%power : 0, cnt);
			readed = readed + cnt;
			cnt = power;
		}
	}
	void CIndexPage::out(unsigned int pos, void* src, unsigned int count){
		CIndexPage* self = this;
		//найдём количество информации в байтах 
		//способное поместится на сраницах проиндексированых
		//одной записью данной индексной страницы
		//(например если данная страница индексная страница 1го уровня
		//то одна её запись спрособна указывать на contentSize информации)
		int contentSize = db->GetPageSize(true);
		int maxRecords = contentSize / sizeof(int);
		int power = pow(maxRecords, (double)(self->GetIxLevel()-1));
		power = power*contentSize;
		//теперь помотрим сколько записей индексной страницы текущего уровня
		//нам потребуется (с округлением до 1 в большую сторону)
		//с какой индексной записи начнётся запись данных
		//и на какой закончится
		int beginrecord = pos/power;
		int endrecord = (pos + count - 1)/power;//округляем в меньшуюсторону
		//if(endrecord >= maxRecords){
		//	if(self->GetOverPageNum() > 0){
		//		throw -1;
		//	}
		//	//создаём страницу более высокого уровня и перебрасываем
		//	//вызов функции на неё
		//	int newPageNuber = -1;
		//	//создаём индексную страницу того же типа
		//	CIndexPage* newPage = (CIndexPage*)db->GetPage(newPageNuber, self->GetPageType());
		//	int iPageNumber = self->GetPageNum();
		//	newPage->InsertIndex(iPageNumber,newPageNuber);
		//	self = newPage;
		//	//throw -1;//??????????????????
		//}
		int writed = 0;
		int cnt = power - pos%power;
		int i = beginrecord;
		int j = 0;
		//в цикле пробегаем эти записи, загружаем по ним сраницы
		//и передаём в их out'ы данные на запись
		int pageNuber = -1;
		while(i <= endrecord){
			if(i >= maxRecords){
				int iPageNumber = self->GetOverPageNum();
				if(iPageNumber > 0){
					throw -1;
					char needLevelPage = self->GetIxLevel();
					pageNuber = self->GetPageNum();
					while(iPageNumber > 0){
						CIndexPage* iPage = (CIndexPage*)db->GetPage(iPageNumber, ptEMPTY);
						int rec;
						for(rec = 0; rec < iPage->GetRecordCounts(); rec++){
							if(iPage->GetRecordValue(rec) == pageNuber){
								break;
							}
						}
						rec = rec + 1;
						if(rec < maxRecords){
							throw -1;
						}
						else{
							//здесь нужно добавить новую страницу если rec < GetRecordCount
							pageNuber = iPage->GetRecordValue(rec);
						}
						iPage = (CIndexPage*)db->GetPage(pageNuber, ptEMPTY);
						while(iPage->GetIxLevel() != needLevelPage){
							pageNuber = iPage->GetRecordValue(0);
							iPage = (CIndexPage*)db->GetPage(pageNuber, ptEMPTY);
						}
						//if(page->GetIxLevel() == needLevelPage){
						self = iPage;
						endrecord = endrecord - i;
						beginrecord = 0;
						i = beginrecord;
						break;
					}
					//throw -1;
				}
				//создаём страницу более высокого уровня и перебрасываем
				//вызов функции на неё
				int newPageNuber = -1;
				//создаём индексную страницу того же типа
				CIndexPage* newPage = (CIndexPage*)db->GetPage(newPageNuber, self->GetPageType());
				iPageNumber = self->GetPageNum();
				newPage->InsertIndex(iPageNumber,newPageNuber);
				newPage->SetIxLevel(self->GetIxLevel()+1);
				self = newPage;
				endrecord = endrecord - i + 1;
				beginrecord = 1;
				i = beginrecord;
				//throw -1;//??????????????????
			}
		//for(i = beginrecord, j = 0; i <= endrecord; i++, j++){
			if(i >= self->GetRecordCounts()){
				//в этом случае индексные записи, а значит и
				//страница данных на которые они должны ссылаться
				//отсутствуют физически. их нужно создать.
//				throw -1;
				//на этой индексной странице индексных записей меньше чем
				//чем требуется, поэтому создадим страницу более низкого
				//уровня и добавим её индекс на текущюю страницу
				
				//узнаем уровень требуемой страницы
				char needLevelPage = self->GetIxLevel() - 1;
				int newPageNuber = -1;
				if(needLevelPage > 0){//нужно создать индексную страницу
					//создаём индексную страницу того же типа
					CPage* newPage = db->GetPage(newPageNuber, self->GetPageType());
					newPage->SetIxLevel(needLevelPage);
				}
				else{//нужно создать страницу данных
					PageTypes type = (PageTypes)((int)(self->GetPageType()) & (~(int)ptINDEXMASK));
					CPage* newPage = db->GetPage(newPageNuber, type);
				}
				int iPageNumber = self->GetPageNum();
				self->InsertIndex(newPageNuber,iPageNumber);
			}
			pageNuber = self->GetRecordValue(i);
			CPage* page = db->GetPage(pageNuber, ptEMPTY);
			if((count - writed) < cnt){
				cnt = count - writed;
			}
			page->out(j == 0 ? pos%power : 0, (char*)src + writed, cnt);
			writed = writed + cnt;
			cnt = power;
			i++;
			j++;
		}
		/*int contentSize = db->GetPageSize(true);
		int maxRecords = contentSize / sizeof(int);
		int power = pow(maxRecords, (double)(GetIxLevel()-1));
		power = power*contentSize;
		int record = pos/power;
		int pageNuber = GetRecordValue(record);
		CPage* page = db->GetPage(pageNuber, ptEMPTY);
		page->out(pos%power, src, count);*/
	}			
	int64_t CIndexPage::add(void* src, unsigned int count){
		//может стоит выделить ВСЕ страницы сразу?!
		CPage* p = GetLastIndexedPage();
		PageTypes type = p->GetPageType();
		if((((~ptINDEXMASK)&type) & (ptSTRINGDATA | ptBINDATA | ptEMPTYDATA)) == 0){
			throw -1;
		}
		int64_t result = p->add(src, count);
		int64_t resultEx = 0;
		unsigned int cnt = count - (p->GetEmpty() - result);//осталось записать...
		result += p->GetOffset();
		while(cnt > 0){
			//возмём страницу более верхнего уровня, если она есть.
			int overPageNum = p->GetOverPageNum();
			if(overPageNum <= 0){
				throw -1;
			}
			CIndexPage* overPage = (CIndexPage*)db->GetPage(overPageNum, ptSTRINGINDEX);
			
			int pn = db->GetNumberEmptyPage(true);
			p = (CPage*)db->GetPage(pn, type);
			overPage->InsertIndex(pn,overPageNum);
			//exPage->SetOverPageNum(overPageNum);
			resultEx = p->add((char*)src+count-cnt, cnt);
			cnt = cnt - (p->GetEmpty() - resultEx);
		}
		return result;
	}
	int CIndexPage::GetIndexedPages() {
		int result = 0;
		
		int contentSize = db->GetPageSize(true);
		int maxRecCount = contentSize/sizeof(int);
		int recCount = GetRecordCounts();
		char ix_level = GetIxLevel();
		if(ix_level > 1){
			int power = pow(maxRecCount, (double)ix_level - 1);
			result = power*(recCount-1);
  			int lastPageNum = GetLastRecordValue();
			CPage* lastPage = db->GetPage(lastPageNum, ptEMPTY);
			PageTypes type = lastPage->GetPageType();
			if((ptINDEXMASK & type) == ptINDEXMASK){
				result = result + ((CIndexPage*)lastPage)->GetIndexedPages();
			}
			else{
				throw -1;
			}
		}
		else{
			result = recCount;
		}
		return result;
	}

	//--------------------------   CStateIndexPage   ---------------------------//
	int CStateIndexPage::GetFreePage(){
		int result = -1;
		int pNum = -1;
		CPage* page = NULL;
		PageTypes type;
		char ix_level = GetIxLevel();
		int maxRecCount = GetMaxRecordCount();
		int power = pow(maxRecCount, (double)ix_level - 1);
		power = power * db->GetPageSize(true) * 8;
		int recCount = GetRecordCounts();
		for(int i = 0; i < recCount; i++){
			pNum = GetRecordValue(i);
			page = db->GetPage(pNum,ptEMPTY);
			type = page->GetPageType();
			switch(type){
			case ptSTATE:
				result = ((CStatePage*)page)->GetFreePage();
				break;
			case ptSTATEINDEX:
				result = ((CStateIndexPage*)page)->GetFreePage();
				break;
			default:
				throw -1;
			}
			if(result > 0){
				return result + i*power;
			}
		}
		
		if(recCount >= maxRecCount){
			throw -1;
		}

		return result;
	}
	void CStateIndexPage::SetState(int pn_toSetState){
		int maxRecCount = GetMaxRecordCount();
		char ix_level = GetIxLevel();
		int power = pow(maxRecCount, (double)ix_level - 1);
		power = power * db->GetPageSize(true) * 8;
		int recordNumber = pn_toSetState / power;
		int recordCount = GetRecordCounts();
		if(recordCount <= recordNumber){
			throw - 1;
		}
		int pageNumber = GetRecordValue(recordNumber);
		CPage* page = db->GetPage(pageNumber, ptEMPTY);	
		PageTypes type = page->GetPageType();
		if(type == ptSTATE){
			((CStatePage*)page)->SetState(pn_toSetState%power);
		}
		else{
			((CStateIndexPage*)page)->SetState(pn_toSetState%power);
		}
	}
	void CStateIndexPage::UnsetState(int pn_toSetState){
		int maxRecCount = GetMaxRecordCount();
		char ix_level = GetIxLevel();
		int power = pow(maxRecCount, (double)ix_level - 1);
		power = power * db->GetPageSize(true) * 8;
		int recordNumber = pn_toSetState / power;
		int recordCount = GetRecordCounts();
		if(recordCount <= recordNumber){
			throw - 1;
		}
		int pageNumber = GetRecordValue(recordNumber);
		CPage* page = db->GetPage(pageNumber, ptEMPTY);	
		PageTypes type = page->GetPageType();
		if(type == ptSTATE){
			((CStatePage*)page)->UnsetState(pn_toSetState%power);
		}
		else{
			((CStateIndexPage*)page)->UnsetState(pn_toSetState%power);
		}
	}
	int CStateIndexPage::GetTotalPages(){
		//throw -1;
		int result = CIndexPage::GetIndexedPages();
		int contentSize = db->GetPageSize(true);
		result = (result-1) * contentSize * 8;
		
		CPage* lastPage = GetLastStatePage();
		result += lastPage->GetEmpty() * 8;
		
		return result;	
	}
	CStatePage* CStateIndexPage::GetLastStatePage(){
		int lastPageNum = GetLastRecordValue();
		CPage* lastPage = db->GetPage(lastPageNum, ptEMPTY);
		PageTypes type = lastPage->GetPageType();
		while(type != ptSTATE){
			lastPageNum = ((CStateIndexPage*)lastPage)->GetLastRecordValue();
			CPage* lastPage = db->GetPage(lastPageNum, ptEMPTY);
			PageTypes type = lastPage->GetPageType();
		}
		return (CStatePage*)lastPage;
	}
}