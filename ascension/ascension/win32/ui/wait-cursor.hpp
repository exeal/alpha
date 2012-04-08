/**
 * @file wait-cursor.hpp
 * @date 2003-2008 (manah/win32/ui/wait-cursor.hpp)
 * @date 2010
 */

#ifndef ASCENSION_WAIT_CURSOR_WINDOWS_HPP
#define ASCENSION_WAIT_CURSOR_WINDOWS_HPP
#include "../windows.hpp"

namespace ascension {
	namespace win32 {
		class WaitCursor {
		public:
			WaitCursor() /*throw()*/ : originalCursor_(::GetCursor()) {
				::SetCursor(::LoadCursorW(0, MAKEINTRESOURCEW(32514)));
			}
			~WaitCursor() /*throw()*/ {
				::SetCursor(originalCursor_);
			}
		private:
			HCURSOR originalCursor_;
		};
	}
} // namespace ascension.win32

#endif // !ASCENSION_WAIT_CURSOR_WINDOWS_HPP
