// timer.hpp
// (c) 2003-2008 exeal

#ifndef MANAH_TIMER_HPP
#define MANAH_TIMER_HPP
#include "windows.hpp"

namespace manah {
namespace win32 {

class Timer {
public:
	Timer(const WCHAR* name = 0) : name_((name != 0) ? new WCHAR[std::wcslen(name) + 1] : 0) {
		if(name_ != 0)
			std::wcscpy(name_, name);
		reset();
	}
	~Timer() throw() {
		DumpContext dout;
		if(name_ != 0)
			dout << name_;
		else
			dout << L"(anonymous)";
		dout << L" : " << read() << L"ms\n";
		delete[] name_;
	}

public:
	DWORD read() const throw() {return ::GetTickCount() - count_;}
	void reset() throw() {count_ = ::GetTickCount();}

private:
	WCHAR* name_;	// timer's name
	DWORD count_;
};

}} // namespace manah.win32

#endif	// !MANAH_TIMER_HPP
