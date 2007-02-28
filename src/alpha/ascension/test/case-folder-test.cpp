// case-folder-test.cpp

#include "stdafx.h"
#include "../unicode.hpp"

using namespace ascension;
using namespace ascension::unicode;

namespace {
} // namespace @0

void testCaseFolder() {
	BOOST_CHECK_EQUAL(CaseFolder::compare(L"ABC", L"abc"), 0);
	BOOST_CHECK_EQUAL(CaseFolder::compare(L"\x00DF", L"SS"), 0);	// ouch
}
