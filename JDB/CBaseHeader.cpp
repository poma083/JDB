#include <Windows.h>
#include "KernelJDBClasses.h"

namespace JDB{
	CBaseHeader::CBaseHeader( CHeaderPage* headerPage, unsigned int _pageSize){
		if(headerPage != NULL){
			page = headerPage;
			pageSize = page->GetPageSize();

		}
		else{
			page = new CHeaderPage(0, NULL, _pageSize);
			pageSize = _pageSize;
		}
	}

	CBaseHeader::~CBaseHeader(){
	}
}