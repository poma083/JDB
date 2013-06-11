#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include "KernelJDB.h"
#include "KernelJDBClasses.h"
#include "ExTypes.h"

static JDB::CDB masterdb;
static std::vector<JDB::CDB*> bases;

JDB::CTable* GetTable(JDB::CDB* base, const char* baseName);
void createBaseMaster(JDB::CDB* masterdb);


//int _main(int argc, char** argv){
int _stdcall _init_test(bool create){
	if(create){
		createBaseMaster(&masterdb);

		CreateBase("base1", 256);
		JDB::CDB* db = NULL;//new JDB::CDB();
		////create base table
		std::vector<JDB::CDB*>::iterator base;
		for(base = bases.begin(); base != bases.end(); base++){
			strcmp("base1.jdb", (*base)->GetFName());
			db = *base;
		}
		JDB::CTable* soBased = db->CreateTable("BaseTable", NULL);//new JDB::CTable(-1, NULL, "BaseTable");
		JDB::CSysField* fldBased = new JDB::CSysField("Id",2,JDB::FIXED);
		soBased->CreateField(fldBased);

		//create table derived from baseTeabe
		JDB::CTable* so = db->CreateTable("MyTable", soBased);//new JDB::CTable(-1, soBased, "MyTable");

		JDB::CSysField* fld = new JDB::CSysField("MyName", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName1", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName2", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName3", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName4", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName5", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName6", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName7", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyName8", 0, JDB::VARIABLE);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyIsDeleted", 0, JDB::FIXED);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyDataPage", 3, JDB::FIXED);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyStringsPage", 3, JDB::FIXED);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyBinPage", 3, JDB::FIXED);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyIsCrypted", 0, JDB::FIXED);
		so->CreateField(fld);
		fld = new JDB::CSysField("MyBaseSysObjectPtr", 2, JDB::FIXED);
		so->CreateField(fld);
		
		//create object
		int objCnt = 1024;
		int stringSize = 8;
		for(int i = 0; i < objCnt; i++){
			int pos = 0;
			
			char* buff = new char[stringSize];
			char* buff1 = new char[stringSize];
			char* buff2 = new char[stringSize];
			char* buff3 = new char[stringSize];
			for(pos = 0; pos < stringSize; pos += 8){
				memcpy(buff + pos, "01234567", (stringSize - pos) < 8 ? (stringSize - pos) : 8);
				memcpy(buff1 + pos, "abcdefjh", (stringSize - pos) < 8 ? (stringSize - pos) : 8);
				memcpy(buff2 + pos, "iklmnopr", (stringSize - pos) < 8 ? (stringSize - pos) : 8);
				memcpy(buff3 + pos, "stuwvxyz", (stringSize - pos) < 8 ? (stringSize - pos) : 8);
			}
	
			JDB::CDBObject* dbObject = new JDB::CDBObject(so,-1);
			// Set base fields
			unsigned int id = i;
			dbObject->SetValue("Id", &id, 0);
			// set self fields
			dbObject->SetValue("MyName", "MyObject", 9);
			dbObject->SetValue("MyName1", buff, stringSize);
			dbObject->SetValue("MyName2", buff1, stringSize);
			dbObject->SetValue("MyName3", buff2, stringSize);
			dbObject->SetValue("MyName4", buff3, stringSize);
			dbObject->SetValue("MyName5", buff, stringSize);
			dbObject->SetValue("MyName6", buff1, stringSize);
			dbObject->SetValue("MyName7", buff2, stringSize);
			dbObject->SetValue("MyName8", buff3, stringSize);
			unsigned char deleted = 0x33;
			dbObject->SetValue("MyIsDeleted", &deleted, 0);
			int64_t datapage = 0x3131313131313131;
			dbObject->SetValue("MyDataPage", &datapage, 0);
			int64_t stringsPage = 0x3030303030303030;//00000000;
			dbObject->SetValue("MyStringsPage",&stringsPage, 0);
			int64_t binPage = 0x3232323232323232;
			dbObject->SetValue("MyBinPage", &binPage, 0);
			unsigned char isCrypted = 0x34;
			dbObject->SetValue("MyIsCrypted", &isCrypted, 0);
			int baseSysObjectPtr = 0x35;
			dbObject->SetValue("MyBaseSysObjectPtr", &baseSysObjectPtr, 0);

			dbObject->Write(db);

			delete buff;
			delete buff1;
			delete buff2;
			delete buff3;

			delete dbObject;
		}
	}
	else{
		LoadBases();

		char* bi = NULL;
		int cnt = 0;
		bi = GetBasesInfo(bi,cnt);
		delete[] bi;


		std::vector<JDB::CDB*>::iterator base;
		for(base = bases.begin(); base != bases.end(); base++){
			std::vector<JDB::CDBObject*> * records;
			std::map<int,JDB::CTable*>::iterator table;// masterTable
			for(table = (*base)->masterTable.begin(); table != (*base)->masterTable.end(); table++){
				printf("TableName:%s\n", (*table).second->GetName());
				std::vector<const char*> * fldNames = NULL;
				fldNames = (*table).second->GetFieldNames(fldNames);
				std::vector<const char*>::iterator name;
				for(name = (*fldNames).begin(); name != (*fldNames).end(); name++){
					printf("fldName:%s\n",*name);
				}
				delete fldNames;
				printf("\n\n");

				std::vector<int64_t> recNums;
				recNums.push_back(0x00);
				records = (*table).second->ReadRecords(recNums,(*base));
			}
		}
	}
	printf("%s\n","werr");
	{
		JDB::CTable so(5, NULL, "MyObject");
	}
	printf("end\n");
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		delete *base;
	}
	return 0;
};

void __stdcall LoadBases(){
	masterdb.Open("master.jdb");
	std::map<int,JDB::CTable*>::iterator masterTables;
	for(masterTables = masterdb.masterTable.begin(); masterTables != masterdb.masterTable.end(); masterTables++){
		//const char* const name = (*masterTables).second->GetName();
		if(strcmp((*masterTables).second->GetName(),"Bases") == 0){
			
			int64_t rec_count = (*masterTables).second->GetRecordsCount(&masterdb);
			std::vector<JDB::CDBObject*> * records;
			std::vector<int64_t> pointers;
			for(int64_t i = 0; i < rec_count; i++){
				pointers.push_back(i);
			}
				
			records = (*masterTables).second->ReadRecords(pointers,&masterdb);
			
			std::vector<JDB::CDBObject*>::iterator base;
			for(base = records->begin(); base != records->end(); base++){
				JDB::CField* fld = (*base)->GetField("basePath");
				char* buff = new char[fld->GetLength()];
				int cnt = fld->CopyValue(buff);
				JDB::CDB* db = new JDB::CDB();
				db->Open(buff);
				bases.push_back(db);
			}
			break;
		}
	}
};

void __stdcall CreateBase(const char* baseName, int PageSize){
	JDB::CDB* db = new JDB::CDB();
	bases.push_back(db);
	char* fName = new char[strlen(baseName) + 5];
	strcpy(fName, baseName);
	strcat(fName, ".jdb");
	db->Create(fName,PageSize);

	JDB::CTable* soBased = db->CreateTable("SOUserBaseTable", NULL);
	JDB::CSysField* fldBased = new JDB::CSysField("Id",2,JDB::FIXED);
	soBased->CreateField(fldBased);
	fldBased = new JDB::CSysField("Name",0,JDB::VARIABLE);
	soBased->CreateField(fldBased);
	fldBased = new JDB::CSysField("Password",0,JDB::VARIABLE);
	soBased->CreateField(fldBased);
	soBased->Write(db);
	
	JDB::CTable* so_ur = db->CreateTable("SOUserRight", soBased);
	JDB::CSysField* fld = new JDB::CSysField("TableName", 0, JDB::VARIABLE);
	so_ur->CreateField(fld);
	fld = new JDB::CSysField("Right", 3, JDB::FIXED);
	so_ur->CreateField(fld);
	so_ur->Write(db);
	
	JDB::CTable* so_u = db->CreateTable("SOUser", soBased);
	fld = new JDB::CSysField("TableRight", 0, JDB::OBJECT, so_ur);
	so_u->CreateField(fld);
	so_u->Write(db);

	db->Flush();

	JDB::CTable* soTableBases = GetTable(&masterdb, "Bases");
	JDB::CDBObject* dbObjectInMaster = new JDB::CDBObject(soTableBases,-1);
	int id = 0x01;
	dbObjectInMaster->SetValue("Id", &id, 0);
	dbObjectInMaster->SetValue("baseName", baseName, strlen(baseName) + 1);
	dbObjectInMaster->SetValue("basePath", fName, strlen(fName) + 1);
	dbObjectInMaster->SetValue("baseUserDefault", "semin", 6);
	dbObjectInMaster->SetValue("baseUserPass", "123456", 7);

	dbObjectInMaster->Write(&masterdb);
	masterdb.Flush();
};

int __stdcall CreateTable(const char* baseFileName, const char* tableName, const char* baseTableName){
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		if(strcmp(baseFileName, (*base)->GetFName()) == 0){
			JDB::CTable* soBased = NULL;
			if(baseTableName != NULL){
				soBased = GetTable((*base), baseTableName);
				if(soBased == NULL){
					return -2;
				}
			}
			JDB::CTable* soTable = (*base)->CreateTable(tableName, soBased);
			soTable->Write((*base));
			(*base)->Flush();
			return 0;
		}
	}
	return -1;
}
int __stdcall CreateField(const char* baseFileName, const char* tableName, char* fieldName, const int powerSize, const int type, const char* objectName){
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		if(strcmp(baseFileName, (*base)->GetFName()) == 0){			
			JDB::CTable* DstTable = NULL;
			JDB::CTable* FldTable = NULL;
			std::map<int,JDB::CTable*>::iterator masterTables;
			for(masterTables = (*base)->masterTable.begin(); masterTables != (*base)->masterTable.end(); masterTables++){
				if(DstTable == NULL){
					if(strcmp(masterTables->second->GetName(),tableName) == 0){
						DstTable = masterTables->second;
						if(objectName == NULL){
							break;
						}
					}
				}
				if(objectName != NULL){
					if(strcmp(masterTables->second->GetName(),objectName) == 0){
						FldTable = masterTables->second;
						if(DstTable != NULL){
							break;
						}
					}
				}
			}
			if(DstTable == NULL){
				return -2;
			}
			if(objectName != NULL){
				if(FldTable == NULL){
					return -3;
				}
			}
			JDB::CSysField* fld = new JDB::CSysField(fieldName, powerSize, (JDB::FieldType)type, FldTable);
			DstTable->AddField(fld,*base);
			DstTable->Write(*base);
			(*base)->Flush();
	
			return 0;
		}
	}
	return -1;
}
int __stdcall DeleteField(const char* baseFileName, const char* tableName, const char* fieldName){
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		if(strcmp(baseFileName, (*base)->GetFName()) == 0){			
			JDB::CTable* DstTable = NULL;
			JDB::CTable* FldTable = NULL;
			std::map<int,JDB::CTable*>::iterator masterTables;
			for(masterTables = (*base)->masterTable.begin(); masterTables != (*base)->masterTable.end(); masterTables++){
				if(strcmp(masterTables->second->GetName(),tableName) == 0){
					DstTable = masterTables->second;
					break;
				}
			}
			if(DstTable == NULL){
				return -2;
			}
			int result = DstTable->DeleteField(fieldName,(*base));
			if(result >= 0){
				DstTable->Write(*base);
				(*base)->Flush();
			}
			return result;
		}
	}
	return -1;
}
char* __stdcall GetBasesInfo(char* buff, int& cnt){
	cnt = 0;
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		cnt += strlen((*base)->GetFName()) + 1;
	}
	if(buff == NULL){
		buff = new char[cnt];
	}
	cnt = 0;
	for(base = bases.begin(); base != bases.end(); base++){
		strcpy(buff + cnt, (*base)->GetFName());
		cnt += strlen((*base)->GetFName()) + 1;
	}
	return buff;	
};

char* __stdcall GetTablesInfo(const char* baseName, char* buff, int& cnt){
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		if(strcmp((*base)->GetFName(), baseName) == 0){
			return (*base)->GetTablesInfo(buff, cnt);
		}
	}
	return NULL;	
};

char* __stdcall GetFieldsInfo(const char* baseName, const char* tableName, char* buff, int& cnt){
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		if(strcmp((*base)->GetFName(), baseName) == 0){
			std::map<int,JDB::CTable*>::iterator table;
			for(table = (*base)->masterTable.begin(); table != (*base)->masterTable.end(); table++){
				if(strcmp((*table).second->GetName(),tableName) == 0){
					return (*table).second->GetFieldsInfo(buff, cnt);
				}
			}
		}
	}
	return NULL;	
};

char* __stdcall Select(const char* baseName, const char* tableName, int& cnt){
	std::vector<JDB::CDB*>::iterator base;
	for(base = bases.begin(); base != bases.end(); base++){
		if(strcmp((*base)->GetFName(), baseName) == 0){
			std::map<int,JDB::CTable*>::iterator table;
			for(table = (*base)->masterTable.begin(); table != (*base)->masterTable.end(); table++){
				if(strcmp((*table).second->GetName(),tableName) == 0){
					return (*table).second->Select(cnt);
				}
			}
		}
	}
	return NULL;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
			//MessageBox(NULL,L"привет",L"привет",MB_OK);
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
};



JDB::CTable* GetTable(JDB::CDB* base, const char* baseName){
	std::map<int,JDB::CTable*>::iterator tableInfo;
	for(tableInfo = base->masterTable.begin(); tableInfo != base->masterTable.end(); tableInfo++){
		if(strcmp(baseName,(*tableInfo).second->GetName()) == 0){
			return (*tableInfo).second;
		}
	}
	return NULL;
};
void createBaseMaster(JDB::CDB* masterdb){
	masterdb->Create("master.jdb",256);
	JDB::CTable* soBase =  masterdb->CreateTable("Bases", NULL);
	
	JDB::CSysField* fldId = new JDB::CSysField("Id",0,JDB::FIXED);
	soBase->CreateField(fldId);
	JDB::CSysField* fldName = new JDB::CSysField("baseName",0,JDB::VARIABLE);
	soBase->CreateField(fldName);
	JDB::CSysField* fldPath = new JDB::CSysField("basePath",0,JDB::VARIABLE);
	soBase->CreateField(fldPath);
	JDB::CSysField* fldUser = new JDB::CSysField("baseUserDefault",0,JDB::VARIABLE);
	soBase->CreateField(fldUser);
	JDB::CSysField* fldPass = new JDB::CSysField("baseUserPass",0,JDB::VARIABLE);
	soBase->CreateField(fldPass);
	
	soBase->Write(masterdb);
//masterdb->Flush();
};