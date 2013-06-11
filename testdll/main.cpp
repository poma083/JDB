#include "../JDB/KernelJDB.h"
#include <Windows.h>
#include <stdio.h>

#pragma comment (lib, "../Debug/KernelJDB.lib")
//#pragma comment (lib, "../Release/KernelJDB.lib")
void GetInfoFromBases();

int main(int argc, char** argv){

	//_init_test(true);
	//return 0;
	printf("start\n");
	LoadBases();

	//CreateBase("base2", 256);
	//CreateTable("base2.jdb", "MyFirstTableInBase2", NULL);
	//CreateField("base2.jdb", "MyFirstTableInBase2", "MyId", 2, 0, NULL);
	//DeleteField("base2.jdb","MyFirstTableInBase2","MyId");

	GetInfoFromBases();

	printf("stop\n");
}

void GetInfoFromBases(){
	char* buff = NULL;
	int cnt = 0;
	int len = 0;
	int pos = 0;
	buff = GetBasesInfo(buff, cnt);
	while(pos < cnt){
		len = strlen(buff+pos);
		printf("basename:%s\n", buff+pos);

		char* buff_tables = NULL;
		int cnt_tables = 0;
		int pos_tables = 0;
		int len_tables = 0;
		buff_tables = GetTablesInfo(buff+pos, buff_tables, cnt_tables);
		while(pos_tables < cnt_tables){
			len_tables = strlen(buff_tables+pos_tables);
			printf("table:%s\n", buff_tables+pos_tables);

			char* buff_fields = NULL;
			int cnt_fields = 0;
			int pos_fields = 0;
			int len_fields = 0;
			buff_fields = GetFieldsInfo(buff+pos, buff_tables+pos_tables, buff_fields, cnt_fields);
			while(pos_fields < cnt_fields){
				len_fields = strlen(buff_fields+pos_fields);
				printf("fld:%s\n", buff_fields+pos_fields);
				pos_fields += len_fields+2;
			}
			delete[] buff_fields;

			pos_tables += len_tables+1;
		}
		delete[] buff_tables;

		pos += len + 1;
	}
	delete[] buff;
}