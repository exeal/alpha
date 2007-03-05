// timer.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_TIMER_HPP
#define MANAH_TIMER_HPP
#include "windows.hpp"

namespace manah {
namespace win32 {

class Timer {
	// コンストラクタ
public:
	Timer(const TCHAR* name = 0) : name_((name != 0) ? new TCHAR[std::_tcslen(name) + 1] : 0) {
		if(name_ != 0)
			std::_tcscpy(name_, name);
		reset();
	}
	~Timer() throw() {
		DumpContext dout;
		if(name_ != 0)
			dout << name_;
		else
			dout << _T("(anonymous)");
		dout << _T(" : ") << read() << _T("ms\n");
		delete[] name_;
	}

	// メソッド
public:
	DWORD read() const throw() {return ::GetTickCount() - count_;}
	void reset() throw() {count_ = ::GetTickCount();}

	// データメンバ
private:
	TCHAR* name_;	// timer's name
	DWORD count_;
};

}} // namespace manah::win32

#endif	/* !MANAH_TIMER_HPP */
