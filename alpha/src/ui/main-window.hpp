/**
 * @file main-window.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2009, 2013-2014
 * @date 2014-06-04 Separated from application.hpp
 */

#ifndef ALPHA_MAIN_WINDOW_HPP
#define ALPHA_MAIN_WINDOW_HPP
#include "editor-panes.hpp"
#include "status-bar.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/box.h>
#	include <gtkmm/window.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/window/custom-control.hpp>
#	include <CommCtrl.h>
#	include <shellapi.h>
#endif

namespace alpha {
	namespace ui {
		class MainWindow : public
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::/*Application*/Window
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			ascension::win32::CustomControl<MainWindow>
#endif
		{
		public:
			MainWindow();

			void updateTitle();

			/// @name Children
			/// @{
			EditorPanes& editorPanes() const BOOST_NOEXCEPT;
			StatusBar& statusBar() const BOOST_NOEXCEPT;
			/// @}

		private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			bool onCommand(WORD id, WORD notifyCode, HWND control);
			void onCopyData(ascension::win32::Handle<HWND> window, const COPYDATASTRUCT& data);
			void onDestroy();
			void onDrawItem(UINT id, const DRAWITEMSTRUCT& item);
			void onDropFiles(ascension::win32::Handle<HDROP> drop);
			void onEnterMenuLoop(bool isTrackPopup);
			void onExitMenuLoop(bool shortcutMenu);
			void onMeasureItem(UINT id, MEASUREITEMSTRUCT& mi);
			LRESULT onMenuChar(WCHAR c, UINT type, ascension::win32::Handle<HMENU> menu);
			void onMenuSelect(UINT itemID, UINT flags, HMENU sysMenu);
			void onNotify(UINT_PTR id, NMHDR& nmhdr, bool& consumed);
			void onRebarChevronPushed(const NMREBARCHEVRON& nmRebarChevron);
			void onSetCursor(ascension::win32::Handle<HWND> window, UINT hitTest, UINT message, bool& consumed);
			void onSetFocus(ascension::win32::Handle<HWND> oldWindow);
			void onSettingChange(UINT flags, const wchar_t* section);
			void onSize(UINT type, int cx, int cy);
			void onToolExecuteCommand();
			void onTimer(UINT_PTR timerID, TIMERPROC procedure);
			// ascension.win32.CustomControl
			LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) override;
			void realized(const Type& type) override;
			void windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT override;
#endif
		private:
			EditorPanes editorPanes_;
			std::unique_ptr<StatusBar> statusBar_;
			boost::signals2::scoped_connection bufferSelectionChangedConnection_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::Box box_;	// TODO: Replace by Gtk.Grid.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
//			manah::win32::ui::Rebar rebar_;		// rebar
//			manah::win32::ui::Toolbar toolbar_;	// standard toolbar
#endif
		};

		/// Returns the editor panes.
		inline EditorPanes& MainWindow::editorPanes() const BOOST_NOEXCEPT {
			return const_cast<MainWindow*>(this)->editorPanes_;
		}

		/// Returns the status bar.
		inline StatusBar& MainWindow::statusBar() const BOOST_NOEXCEPT {
			return *const_cast<MainWindow*>(this)->statusBar_;
		}
	}
} // namespace alpha

#endif // !ALPHA_MAIN_WINDOW_HPP
