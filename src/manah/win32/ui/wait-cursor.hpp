// wait-cursor.hpp
// (C) 2003-2007 exeal

#ifndef MANAH_WAIT_CURSOR_HPP
#define MANAH_WAIT_CURSOR_HPP
#include "../windows.hpp"

namespace manah {
	namespace windows {
		namespace ui {

			class WaitCursor {
			public:
				WaitCursor() throw() : originalCursor_(::GetCursor()) {::SetCursor(::LoadCursor(0, IDC_WAIT));}
				virtual ~WaitCursor() throw() {::SetCursor(originalCursor_);}
			private:
				HCURSOR	originalCursor_;
			};

		}
	}
}

#endif /* !MANAH_WAIT_CURSOR_HPP */
