/**
 * @file ui.hpp
 */

#ifndef ALPHA_UI_HPP
#define ALPHA_UI_HPP
#include <manah/win32/windows.hpp>

namespace alpha {
	namespace ambient {
		namespace ui {
			void handleINITMENUPOPUP(WPARAM wp, LPARAM lp);
			void handleMENUCOMMAND(WPARAM wp, LPARAM lp);
		}
	}
}

#endif // !ALPHA_UI_HPP
