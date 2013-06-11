#ifndef __KERNELJDBCLASSES_H__
#define __KERNELJDBCLASSES_H__

#include "ExTypes.h"
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>

namespace JDB{
//---------------------------------прототипы-----------------------------------//
	class CDB;
	class CField;
	class CSysField;
	class CTable;
	class CDBObject;

//-----------------------------------CPage--------------------------------//
#pragma pack(push,1)
	struct HeadPage{
		public:
			unsigned short int GetEmpty() const {return empty;};
			void SetEmpty(unsigned short int _empty){empty = _empty; NeedUpdate(true);};
			void NeedUpdate(bool _value){ _value ? needupdate |= 0x01 : needupdate &= 0xFE;};
			bool NeedUpdate() const{return (needupdate & 0x01) == 0x01;};
			int GetPageNum()const{return pageNum;};
			void SetPageNum(int _pageNum){pageNum = _pageNum;};
			int GetOverPageNum(){return overPageNum;};
			void SetOverPageNum(int _pageNum){overPageNum = _pageNum;};
			PageTypes GetPageType()const{return pageType;};
			void SetPageType(PageTypes _pageType){pageType = _pageType;};
			char GetIxLevel()const{return ixLevel;};
			void SetIxLevel(char _ixLevel){ixLevel = _ixLevel;};
			void SetOffset(int64_t o){offset = o;};
			int64_t GetOffset()const{return offset;};
		private:
			char needupdate;
			//char reserved2;
			//char reserved3;
			char ixLevel;
			int overPageNum;
			int pageNum;
			PageTypes pageType;
			unsigned short int empty;
			//short int reserved4;
			int64_t offset;
	};
#pragma pack(pop)
	class CPage{
		public:
			CPage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024);
			CPage(char* buff, CDB* _db);
			virtual ~CPage();
			virtual void in(void* dst, unsigned int pos, unsigned int count)const;
			virtual void out(unsigned int pos, void* src, unsigned int count);
			// add возвращает смещение в сквозной нумерации в байтах
			virtual int64_t add(void* src, unsigned int count );
			void flush();// сброс данных на диск или куда-то в др. место
			int64_t GetOffset();
			unsigned short int GetEmpty() const {return head->GetEmpty();};
			void SetEmpty(unsigned short int _empty){head->SetEmpty(_empty);};
			void NeedUpdate(bool _value){ head->NeedUpdate(_value);};
			bool NeedUpdate() const{return head->NeedUpdate();};
			int GetPageNum()const{return head->GetPageNum();};
			void SetPageNum(int number);
			//int GetPageNumUpLevel(){return head->GetPageNumUpLevel();};
			int GetPageNumUpLevel()const;
			int GetOverPageNum()const{return head->GetOverPageNum();};
			void SetOverPageNumber(int _pageNum){head->SetOverPageNum(_pageNum);};
			void SetPageType(PageTypes _pageType){head->SetPageType(_pageType);};
			PageTypes GetPageType()const{return head->GetPageType();};
			char GetIxLevel()const{return head->GetIxLevel();};
			void SetIxLevel(char _ixLevel){head->SetIxLevel(_ixLevel);};
			char* GetContent()const{return content;};
		protected:
			virtual int GetCountTables(int tableSize, int& overload) const{throw -1;};
			CDB* db;
		private:
			HeadPage* head;
			char* content;
	};

	class CStatePage : public CPage{
		public:
			CStatePage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024);
			CStatePage(char* buffer, CDB* _db);
			virtual ~CStatePage(){};
			int GetTotalPages();
			virtual int GetFreePage();
			void SetState(int);
			void UnsetState(int);
		private:
			unsigned int lastNoFF;
	};
	
	class CHeaderPage : CPage{
		public:
			CHeaderPage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024);
			CHeaderPage(char* buffer, CDB* _db) : CPage(buffer, _db){};
			virtual ~CHeaderPage(){};
			unsigned int GetPageSize();		//40/4
			int GetMasterDataPN()const;		//44/4
			int GetMasterStringsPN()const;	//52/4
			int GetMasterBinPN()const;		//60/4
			int GetFieldsDataPN()const;		//68/4
			int GetFieldsStringsPN()const;	//76/4
			int GetFieldsBinPN()const;		//84/4
			int GetStatePN()const;			//92/4
			void SetMasterBinPN(int p);
			void SetMasterStringsPN(int p);
			void SetMasterDataPN(int p);
			void SetFieldsDataPN(int p);
			void SetFieldsStringsPN(int p);
			void SetFieldsBinPN(int p);
			void SetStatePN(int p);
		private:
			void SetPageSize(unsigned int pageSize);
	};
	class CDataPage : public CPage{
		public:
			CDataPage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024);
			CDataPage(char* buffer, CDB* _db) : CPage(buffer, _db){};
			virtual ~CDataPage(){};
			virtual int GetCountTables(int tableSize, int& overload) const override;
	};
	class CStringsPage : public CPage{
		public:
			CStringsPage(int _pageNum, CDB* db, unsigned int _pageSize = 1024);
			CStringsPage(char* buffer, CDB* db);
			virtual ~CStringsPage(){};
			virtual int64_t add(void* src, unsigned int count) override;
			//int64_t SelfToIndex(void* src, unsigned int count){return CPage::add(src, count);};
			//---int64_t addIndex(void* src, unsigned int count){return CPage::add(src, count);};
			virtual void out(unsigned int pos, void* src, unsigned int count);
		private:
			//CDB* _db;
	};
	class CBinPage : public CPage{
		public:
			CBinPage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024);
			CBinPage(char* buffer, CDB* _db) : CPage(buffer, _db){};
			virtual ~CBinPage(){};
	};
	class CIndexPage : public CPage{
		public:
			CIndexPage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024) : CPage(_pageNum, _db, _pageSize){};
			CIndexPage(char* buffer, CDB* _db) : CPage(buffer, _db){};
			virtual ~CIndexPage() = 0 {};
			//int GetFirstPageFromIndex();
			int FillPagesPiramid(std::map<int,std::vector<int>*>*){return 0;};
			CPage* GetLastIndexedPage();
			int GetLastRecordValue();
			int GetRecordValue(int recNumber)const;
			int GetRecordCounts() const;
			int GetMaxRecordCount() const;
			int GetIndexedPages();
			int64_t InsertIndex( int pageNumber, int& indexPageNumber);
			virtual void in(void* dst, unsigned int pos, unsigned int count) const override;
			virtual void out(unsigned int pos, void* src, unsigned int count) override;
			virtual int64_t add(void* src, unsigned int count) override;
		protected:
			virtual int GetCountTables(int tableSize, int& overload) const{throw -1;};
	};
	class CStringIndexPage : public CIndexPage{
		public:
			//CStringIndexPage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024) : CIndexPage(_pageNum, _db, _pageSize){SetPageType(ptSTRINGIDEX);};
			CStringIndexPage(char* buffer, CDB* _db) : CIndexPage(buffer, _db){};
			virtual ~CStringIndexPage(){};
			int NextPageNumber(int cur);
			int FindRecordNumber(int cur);
			int64_t InsertIndex( int pageNumber, int& indexPageNumber ){return CIndexPage::InsertIndex(pageNumber,indexPageNumber);};
	};
	class CBinIndexPage : public CIndexPage{
	public:
		CBinIndexPage(char* buffer, CDB* _db) : CIndexPage(buffer, _db){};
		virtual ~CBinIndexPage(){};
	};
	class CDataIndexPage : public CIndexPage{
	public:
		CDataIndexPage(char* buffer, CDB* _db) : CIndexPage(buffer, _db){};
		virtual ~CDataIndexPage(){};
		virtual int GetCountTables(int tableSize, int& overload) const override;
	};
	//--------------------------   CStateIndexPage   ---------------------------//
	class CStateIndexPage : public CIndexPage{
		public:
			CStateIndexPage(int _pageNum, CDB* _db, unsigned int _pageSize = 1024) : CIndexPage(_pageNum, _db, _pageSize){SetPageType(ptSTATEINDEX);};
			CStateIndexPage(char* buffer, CDB* _db) : CIndexPage(buffer, _db){};
			virtual ~CStateIndexPage(){};
			int GetTotalPages();
			virtual int GetFreePage();
			void SetState(int);
			void UnsetState(int);
			CStatePage* GetLastStatePage();
			int64_t InsertIndex( int pageNumber, int& indexPageNumber ){return CIndexPage::InsertIndex(pageNumber,indexPageNumber);};
	};
	
	
//------------------------------------CDBObject-------------------------------//
	class CDBObject{
		public:
			CDBObject(CTable* so, int64_t _recordNumber);
			~CDBObject();
			bool SetValue(const char* name, const void* value, const int lenght);
			int Read(CDB* db);
			int Write(CDB* db);
			int64_t GetRecNum() const{return recNum;};
			CField* GetField(const char* name);
			const char* const GetName() const;
			//void SetRecNum(int64_t rn){recNum = rn;};
			friend int CDBObject::Read(CDB* db);

		private:
			int64_t recNum;//номер записи
			int64_t recBaseNum;//номер записи базового типа
			CDBObject* baseObject;//базовый объект загруженый по recBaseNum
			char isDeleted;
			
			//CSysObject* derivedFrom;
			CTable* sysObject;
			//CDBObject* owner;
			//std::vector<CDBObject*>* childObjects;// if this is ObjrctList field
			std::vector<CField*> fields;
	};
//------------------------------------CBaseHeader-------------------------------//
	class CBaseHeader{
		public:
			CBaseHeader( CHeaderPage* headerPage, unsigned int _pageSize = 1024);
			~CBaseHeader();

			unsigned int PageSize() const {return pageSize;};
			int GetMasterDataPN() const{return page->GetMasterDataPN();};
			int GetMasterStringsPN() const{return page->GetMasterStringsPN();};
			int GetMasterBinPN() const{return page->GetMasterBinPN();};
			int GetFieldsDataPN() const{return page->GetFieldsDataPN();};
			int GetFieldsStringsPN() const{return page->GetFieldsStringsPN();};
			int GetStatePN() const{return page->GetStatePN();};
			void SetStatePN(int pn) {page->SetStatePN(pn);};
		private:
			unsigned int pageSize;
			CHeaderPage* page;
	};
//----------------------------------CDB---------------------------------//
	class CDB{
	public:
		CDB();
		~CDB();
		void Create(char* fname, unsigned int PageSize = 1024);
		void Open(char* fname);
		void Flush();
		CPage* GetPage(int& _pageNum, PageTypes pt);
		int GetNumberEmptyPage(bool reserved);
		void ReservePage(int pageNumber);
		unsigned int GetPageSize() const{return header->PageSize();};
		unsigned int GetPageSize( bool contentOnly);

		int GetFieldsDataPN()const{return header->GetFieldsDataPN();};
		int GetFieldsStringsPN()const{return header->GetFieldsStringsPN();};
		int GetMasterDataPN()const{return header->GetMasterDataPN();};
		int GetMasterStringsPN()const{return header->GetMasterStringsPN();};
		int GetMasterBinPN()const{return header->GetMasterBinPN();};

		CTable* CreateTable(const char* tableName, CTable* baseTable);
		int GetCountTables();
		char* GetTablesInfo(char* buff, int& cnt) const;
		char* GetFieldsInfo(char* buff, const char* tableName, int& cnt) const;
		const char* GetFName() const{return fName;};
		
		std::map<int,CTable*> masterTable;
	private:
		void LoadTables();
		void ClearMasterTable();
		CPage* GetPage(int i);
		void SetPage(CPage* page);

		std::fstream dbFile;
		char* fName;
		int pagesTotal;
		std::map<int, CPage*> _pages;

		CBaseHeader* header;
	};
//----------------------------------CField---------------------------------//
	class CField{
		public:
			CField(CSysField* fld);
			~CField();
			const char* GetName() const;
			void SetValue(const void* newValue, int length = 0);
			int CopyValue(char* dst) const;
			FieldType GetType() const;
			int GetSize() const;
			int GetLength() const;
			int GetAllocateLength()const{return allocateLength;};
			int64_t GetOffset()const{return offset;};
			void SetOffset(int64_t _offset){offset = _offset;};
			//во избежание лишних действий, и во имя ускорения, немножко
			//нарушим изоляцию класса, позволив доступ к его приватным 
			//полям, функциям класса CDBObject CDBObject::Write и CDBObject::Read,
			//записав их дружественными в следующих инструкциях
			friend int CDBObject::Write( CDB* db );
			friend int CDBObject::Read( CDB* db );

			void ReadObject(CDB* db);
		private:
			CSysField* sysField;
			char* value;
			CDBObject* objectValue;
			int64_t offset;
			int valueLength;
			int allocateLength;
	};
//-------------------------------------CTable------------------------------//
	class CTable{
		public:
			CTable(int64_t recordNumber, CTable* base, const char* _name);
			~CTable();
			bool IsDerived() const{return baseObject != NULL;};//наследует ли класс
			CTable* GetBaseObject() const{return baseObject;};
			int64_t GetRecNumber()const{return recNum;};
			int64_t GetBaseRecNumber() const{return ptrBaseObject;};
			void SetBaseTable(std::map<int,CTable*> masterTable);
			std::vector<CDBObject*>* ReadRecords(const std::vector<int64_t> records, CDB* db);
			const int64_t GetRecordsCount(CDB* db) const;
			int64_t Write(CDB* db);
			int Read(CDB* db);
			int AddField(CSysField* fld, CDB* db);
			int CreateField(CSysField* fld);
			int DeleteField(const char* fldName,CDB* db);
			int GetDataPageNumber() const{ return dataPageNumber;};
			int GetStringsPageNumber() const{ return stringsPageNumber;};
			int GetBinPageNumber() const{ return binPageNumber;};
			void SetDataPageNumber( const int& _dataPageNumber);
			void SetStringsPageNumber(const int& _stringsPageNumber){ stringsPageNumber = _stringsPageNumber;};
			void SetBinPageNumber(const int& _binPageNumber ){ binPageNumber = _binPageNumber;};
			int GetFieldsCount() const;
			int GetThisFieldsCount() const{return fields.size();};
			int GetSize() const;
			int GetMasterRecordSize();
			std::vector<const char* > * GetFieldNames(std::vector<const char* > * buff) const;
			char* GetFieldsInfo(char* buff, int& offset) const;
			char* Select(char* buff, int& offset);
			void GetThisFieldNames(std::vector<char* > * buff) const{};
			CSysField* GetField(char* name) const;
			CSysField* GetField(const int index) const {return fields[index];};
			const char* const GetName() const{return name;};
		protected:
			int GetLengthForFieldsInfoFunction() const;
		private:
			void SetName(const char* newName);
			
			int dataPageNumber;		//номер стр. простых(конечных типов и поинтеров) данных
			int stringsPageNumber;	//номер стр. строковых данных (типов произвольной длины)
			int binPageNumber;		//номер стр. бинарных данных (типов произвольной длины, списков поинтеров)
			int64_t fieldsOffset;		//смещение на список полей в biinPage
			char isCrypted;				//флаг шифрованности объектов танного типа
			int64_t ptrBaseObject;		//номер записи таблицы SysObject (это базовый тип)
			int64_t nameOffset;
			
			CTable* baseObject;			//указатель на базовый тип, если данный тип наследует
			std::vector<CSysField*> fields; //список полей
			int64_t recNum;
			char* name;
			int masterRecSize;
			int recordSize; // данное поле хронит размер экземпляра типа (не имеет представления в файле)
			//TPagesNumberPhyramid indexPages;
	};
//-------------------------------------CSysField------------------------------//
	class CSysField{
		public:
			CSysField(int64_t recordNumber, CDB* db);
			CSysField(char* _name, char _sizeFromPower, FieldType _type, CTable* objectTable = NULL, char _isDeleted = 0);
			~CSysField();
			int64_t Write(CDB* db);
			bool IsDeleted(){return isDeleted == 1;};
			void Delete(){isDeleted = 1;};
			const char* GetName() const {return name;};
			char GetSize() const;
			FieldType GetType() const{return type;};
			const CTable* GetTableField() const;
		private:
			void Read(CDB* db);

			char isDeleted;
			char size;
			FieldType type;
			char* name;
			int64_t nameOffset;
			int64_t recNum;
			int64_t tableReferenceRecord;
			CTable* tableReference;

			void SetName(const char* newName);
	};
	//-------------------------------------------------------------------//

	//-------------------------------------------------------------------//
	
	//-------------------------------------------------------------------//
}

#endif