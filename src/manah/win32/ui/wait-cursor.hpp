// wait-cursor.hpp
// (C) 2003-2007 exeal

#ifndef MANAH_WAIT_CURSOR_HPP
#define MANAH_WAIT_CURSOR_HPP
#include "../windows.hpp"

namespace manah {
	namespace win32 {
		namespace ui {
			class WaitCursor {
			public:
				WaitCursor() throw() : originalCursor_(::GetCursor()) {::SetCursor(::LoadCursorW(0, IDC_WAIT));}
				virtual ~WaitCursor() throw() {::SetCursor(originalCursor_);}
			private:
				::HCURSOR	originalCursor_;
			};
		}
	}
} // namespace manah.win32.ui

#endif /* !MANAH_WAIT_CURSOR_HPP */
