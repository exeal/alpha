/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2009, 2014 (was application.cpp)
 * @date 2014-03-29 Separated from application.cpp
 */

#include "status-bar.hpp"

namespace alpha {
	namespace ui {
		// StatusBar //////////////////////////////////////////////////////////////////////////////////////////////////

		StatusBar::StatusBar() : columnStartValue_(1) {
		}

#if 0
		void StatusBar::adjustPaneWidths() {
			if(!isWindow())
				return;

			static const int ICON_WIDTH = 16;
			int parts[5], borders[3];
			getBorders(borders);
			const int padding = (borders[0] + borders[2]) * 2 + 5;
			RECT rc;
			getRect(rc);

			const Alpha& app = Alpha::instance();
			win32::gdi::ClientDC dc = getDC();
			win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
			HFONT font = ::CreateFontIndirectW(&ncm.lfStatusFont);
			HFONT oldFont = static_cast<HFONT>(dc.selectObject(font_.get()));

			parts[4] = rc.right - rc.left;
			if(!app.getMainWindow().isZoomed())
				parts[4] -= 20;
			// ナローイング
			parts[3] = parts[4] - ICON_WIDTH - padding;
			// 上書き/挿入モード
			const wstring overtypeMode = app.loadMessage(MSG_STATUS__OVERTYPE_MODE);
			const wstring insertMode = app.loadMessage(MSG_STATUS__INSERT_MODE);
			parts[2] = parts[3] - max(
				dc.getTextExtent(overtypeMode.data(), static_cast<int>(overtypeMode.length())).cx,
				dc.getTextExtent(insertMode.data(), static_cast<int>(insertMode.length())).cx) - padding;
			// キーボードマクロ
			parts[1] = parts[2] - ICON_WIDTH - padding;
			// キャレット位置
			wchar_t text[256];
			static const wstring format(app.loadMessage(MSG_STATUS__CARET_POSITION));
#if(_MSC_VER < 1400)
			swprintf(text, format.c_str(), 88888888, 88888888, 88888888);
#else
			swprintf(text, MANAH_COUNTOF(text), format.c_str(), 88888888, 88888888, 88888888);
#endif // _MSC_VER < 1400
			parts[0] = parts[1] - dc.getTextExtent(text, static_cast<int>(wcslen(text))).cx - padding;

			dc.selectObject(oldFont);
			setParts(MANAH_COUNTOF(parts), parts);
		}

		void StatusBar::updateAll() {
			updateCaretPosition();
			updateNarrowingStatus();
			updateOvertypeMode();
			updateTemporaryMacroRecordingStatus();
		}

		void StatusBar::updateCaretPosition() {
			if(isWindow()) {
				const EditorView& viewer = EditorWindows::instance().activePane().visibleView();
				// build the current position indication string
				static AutoBuffer<WCHAR> message, messageFormat;
				static size_t formatLength = 0;
				if(messageFormat.get() == 0) {
					void* messageBuffer;
					if(0 != ::FormatMessageW(
							FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
							::GetModuleHandle(0), MSG_STATUS__CARET_POSITION, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							reinterpret_cast<wchar_t*>(&messageBuffer), 0, 0)) {
						formatLength = wcslen(static_cast<WCHAR*>(messageBuffer));
						messageFormat.reset(new WCHAR[formatLength + 1]);
						wcscpy(messageFormat.get(), static_cast<WCHAR*>(messageBuffer));
						::LocalFree(messageBuffer);
					} else {
						messageFormat.reset(new WCHAR[1]);
						messageFormat[0] = 0;
					}
					message.reset(new WCHAR[formatLength + 100]);
				}
				if(formatLength != 0) {
					length_t messageArguments[3];
					win32::AutoZero<SCROLLINFO> si;
					viewer.getScrollInformation(SB_VERT, si, SIF_POS | SIF_RANGE);
					messageArguments[0] = viewer.caret().line() + viewer.verticalRulerConfiguration().lineNumbers.startValue;
					messageArguments[1] = viewer.caret().visualColumn() + columnStartValue_;
					messageArguments[2] = viewer.caret().column() + columnStartValue_;
					::FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING, messageFormat.get(),
						0, 0, message.get(), static_cast<DWORD>(formatLength) + 100, reinterpret_cast<va_list*>(messageArguments));
					// show in the status bar
					win32::ui::StatusBar::setText(1, message.get());
				}
			}
		}

		void StatusBar::updateDefaultFont() {
			ascension::win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
			::DeleteObject(defaultFont_);
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
			defaultFont_ = ::CreateFontIndirectW(&ncm.lfStatusFont);
			adjustPaneWidths();
		}

		void StatusBar::updateNarrowingStatus() {
			if(isWindow() && hasFocus()) {
				const bool narrow = EditorWindows::instance().activePane().visibleBuffer().isNarrowed();
				Alpha& app = Alpha::instance();
				if(narrowingIcon_.get() == 0)
					narrowingIcon_.reset(win32::managed(static_cast<HICON>(app.loadImage(IDR_ICON_NARROWING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR))));
		//		setText(4, narrow ? app.loadMessage(MSG_STATUS__NARROWING).c_str() : L"");
				setTipText(4, narrow ? app.loadMessage(MSG_STATUS__NARROWING).c_str() : L"");
				setIcon(4, narrow ? narrowingIcon_.use() : 0);
			}
		}

		void StatusBar::updateOvertypeMode() {
			if(isWindow())
				win32::ui::StatusBar::setText(3, Alpha::instance().loadMessage(
					EditorWindows::instance().activePane().visibleView().caret().isOvertypeMode() ?
						MSG_STATUS__OVERTYPE_MODE : MSG_STATUS__INSERT_MODE).c_str());
		}

		void StatusBar::updateTemporaryMacroRecordingStatus() {
			// TODO: not implemented.
		}
#endif
	}
}
