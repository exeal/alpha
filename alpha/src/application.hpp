/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2009, 2013-2014
 */

#ifndef ALPHA_APPLICATION_HPP
#define ALPHA_APPLICATION_HPP
#include "resource.h"
#include "buffer.hpp"
//#include "search.hpp"	// ui.SearchDialog
#include "main-window.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "win32/module.hpp"
#endif
#include <ascension/graphics/font/font-description.hpp>
#include <gtkmm/application.h>


// タイトルバーとかに使うアプリケーション名
#define IDS_APPNAME		L"Alpha"
#define IDS_APPVERSION	L"0.7.94.0"
#define IDS_APPFULLVERSION	IDS_APPNAME L" " IDS_APPVERSION

#define IDS_DEFAULTSTATUSTEXT	L""
#define IDS_EVENTSCRIPTFILENAME	L"events.*"
#define IDS_MACRO_DIRECTORY_NAME			L"macros\\"
#define IDS_KEYBOARDSCHEME_DIRECTORY_NAME	L"keyboard-schemes\\"
#define IDS_ICON_DIRECTORY_NAME				L"icons\\"
#define IDS_BREAK_CR	L"CR (Macintosh)"
#define IDS_BREAK_LF	L"LF (Unix)"
#define IDS_BREAK_CRLF	L"CR+LF (Windows)"
#define IDS_BREAK_NEL	L"NEL (EBCDIC)"
#define IDS_BREAK_LS	L"LS (U+2028)"
#define IDS_BREAK_PS	L"PS (U+2029)"

// Timer ID
#define ID_TIMER_QUERYCOMMAND	1	// ツールバーアイテムの有効化
#define ID_TIMER_MOUSEMOVE		2	// カーソル停止後1秒後に発生 (ヒント表示などに使用)

// message ID (シングルスレッドに戻したので使っていないものもある)
#define MYWM_EVENTHANDLER	WM_APP + 1	// メインスレッドがイベントハンドラスクリプトを呼び出す
										// wParam => unused
										// lParam => std::pair<const OLECHAR*, DISPPARAMS*>*
										// lParam->first => イベントハンドラ名
										// lParam->second => 引数
#define MYWM_ENDSCRIPTMACRO	WM_APP + 2	// マクロスクリプト終了時に送られてくる
										// wParam, lParam => unused
#define MYWM_CALLOVERTHREAD	WM_APP + 3	// 任意の関数を呼び出す
										// wParam => unused
										// lParam => 戻り値なしの無引数関数


namespace alpha {
	namespace ambient {
		class ScriptSystem;
	};

	/// The application class of Alpha.
	class Application : public Gtk::Application {
	public:
		/// @name Instance
		/// @{
		static Glib::RefPtr<Application> create();
		static Glib::RefPtr<Application> instance();
		/// @}

		void setFont(const ascension::graphics::font::FontDescription& font);
		ui::MainWindow& window() BOOST_NOEXCEPT;
		const ui::MainWindow& window() const BOOST_NOEXCEPT;
		// operations
		bool teardown(bool callHook = true);

	private:
		Application();
		void changeFont();
//		bool	handleKeyDown(command::VirtualKey key, command::KeyModifier modifiers);
		bool initInstance(int showCommand);
		void loadSettings();
		template<typename Section, typename Key, typename Container>
		void readProfileList(Section section, Key key, Container& items, const Glib::ustring& defaultValue = Glib::ustring());
		template<typename Section, typename Key> inline boost::optional<int> readIntegerProfile(Section section, Key key) const {
			return boost::none;	// dummy
		}
		template<typename Section, typename Key> inline boost::optional<Glib::ustring> readStringProfile(Section section, Key key) const {
			return boost::none;	// dummy
		}
		template<typename Section, typename Key, typename T> inline bool readStructureProfile(Section section, Key key, T& data) const {
			return boost::none;	// dummy
		}
		void saveSettings();
//		void	setupToolbar();
		void updateTitleBar();
		template<typename Section, typename Key, typename Value>
		inline void writeIntegerProfile(Section section, Key key, Value value) {}	// dummy
		template<typename Section, typename Key, typename Value>
		inline void writeStringProfile(Section section, Key key, const Value& value) const {}	// dummy
		template<typename Section, typename Key, typename T>
		inline void writeStructureProfile(Section section, Key key, const T& data) const {}	// dummy

		// Gio.Application
		void on_activate() override;
		void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;

		// message handlers
	protected:
		void onToolExecuteCommand();
	protected:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		bool onCommand(WORD id, WORD notifyCode, HWND control);						// WM_COMMAND
		void onCopyData(HWND window, const COPYDATASTRUCT& cds);					// WM_COPYDATA
		void onDestroy();															// WM_DESTROY
		void onDrawItem(UINT id, const DRAWITEMSTRUCT& drawItem);					// WM_DRAWITEM
		void onDropFiles(HDROP drop);												// WM_DROPFILES
		void onEnterMenuLoop(bool isTrackPopup);									// WM_ENTERMENULOOP
		void onExitMenuLoop(bool isTrackPopup);										// WM_EXITMENULOOP
		void onMeasureItem(UINT id, MEASUREITEMSTRUCT& mi);							// WM_MEASUREITEM
		LRESULT onMenuChar(wchar_t ch, UINT flags, manah::win32::ui::Menu& menu);	// WM_MENUCHAR
		void onMenuSelect(UINT itemID, UINT flags, HMENU sysMenu);					// WM_MENUSELECT
		bool onNotify(int id, NMHDR& nmhdr);										// WM_NOTIFY
		bool onSetCursor(HWND window, UINT hitTest, UINT message);					// WM_SETCURSOR
		void onSettingChange(UINT flags, const wchar_t* section);					// WM_SETTINGCHANGE
		void onSize(UINT type, int cx, int cy);										// WM_SIZE
		void onTimer(UINT timerID);													// WM_TIMER

	protected:
		void onRebarChevronPushed(const NMREBARCHEVRON& nmRebarChevron);	// RBN_CHEVRONPUSHED

	protected:
		/* ウィンドウプロシジャ */
		static LRESULT CALLBACK appWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
#endif

	private:
		static Glib::RefPtr<Application> instance_;
		std::unique_ptr<ui::MainWindow> window_;
	};


	/// Returns the singleton application object.
	inline Glib::RefPtr<Application> Application::instance() {
		if(!instance_)
			throw ascension::NullPointerException("There is no singleton instance.");
		return instance_;
	}

	/// Returns the main window.
	inline ui::MainWindow& Application::window() BOOST_NOEXCEPT {
		assert(window_.get() != nullptr);
		return *window_;
	}

	/// Returns the main window.
	inline const ui::MainWindow& Application::window() const BOOST_NOEXCEPT {
		assert(window_.get() != nullptr);
		return *window_;
	}
} // namespace alpha

#endif // ALPHA_APPLICATION_HPP
