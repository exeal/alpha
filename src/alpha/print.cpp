/**
 * @file print.cpp
 * @author exeal
 * @date 2007
 */

#include "stdafx.h"
#include "print.hpp"
#include "application.hpp"
#include <commdlg.h>
using namespace alpha;
using namespace std;


/**
 * @param buffer the buffer to print
 */
void alpha::printBuffer(const Buffer& buffer) {
	::PRINTDLGEXW pdex;
	::PRINTPAGERANGE pageRanges[10];
	memset(&pdex, 0, sizeof(::PRINTDLGEXW));
	pdex.lStructSize = sizeof(::PRINTDLGEXW);
	pdex.hwndOwner = Alpha::getInstance().getMainWindow().getHandle();
	pdex.Flags = PD_COLLATE | PD_RETURNDC;
	pdex.nMaxPageRanges = countof(pageRanges);
	pdex.lpPageRanges = pageRanges;
	pdex.nMinPage = pdex.nMaxPage = pdex.nCopies = 1;
	pdex.nStartPage = START_PAGE_GENERAL;

	HRESULT hr = ::PrintDlgExW(&pdex);
}
