/**
 * @file print.hpp
 * @author exeal
 * @date 2007
 */

#ifndef ALPHA_PRINT_HPP
#define ALPHA_PRINT_HPP
#include "../manah/win32/dc.hpp"
#include <windows.h>

namespace alpha {

	class Buffer;

	class Printing : private manah::Noncopyable {
	public:
		static Printing&	instance() throw();
		bool				print(const Buffer& buffer);
		bool				setupPages();
	private:
		Printing();
		~Printing() throw();
		bool	doSetupPages(bool returnDefault);
		void	prepareDeviceContext();
		DWORD	showDialog(bool returnDefault);
	private:
		::HGLOBAL devmode_;
		::HGLOBAL devnames_;
		::SIZE paperSize_;
		::RECT margins_;
	};

}

#endif /* ALPHA_PRINT_HPP */
