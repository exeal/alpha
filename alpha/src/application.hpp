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
#include "status-bar.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "win32/module.hpp"
#endif
#include <ascension/graphics/font/font-description.hpp>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <memory>	// std.unique_ptr


// �^�C�g���o�[�Ƃ��Ɏg���A�v���P�[�V������
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
#define ID_TIMER_QUERYCOMMAND	1	// �c�[���o�[�A�C�e���̗L����
#define ID_TIMER_MOUSEMOVE		2	// �J�[�\����~��1�b��ɔ��� (�q���g�\���ȂǂɎg�p)

// message ID (�V���O���X���b�h�ɖ߂����̂Ŏg���Ă��Ȃ����̂�����)
#define MYWM_EVENTHANDLER	WM_APP + 1	// ���C���X���b�h���C�x���g�n���h���X�N���v�g���Ăяo��
										// wParam => unused
										// lParam => std::pair<const OLECHAR*, DISPPARAMS*>*
										// lParam->first => �C�x���g�n���h����
										// lParam->second => ����
#define MYWM_ENDSCRIPTMACRO	WM_APP + 2	// �}�N���X�N���v�g�I�����ɑ����Ă���
										// wParam, lParam => unused
#define MYWM_CALLOVERTHREAD	WM_APP + 3	// �C�ӂ̊֐����Ăяo��
										// wParam => unused
										// lParam => �߂�l�Ȃ��̖������֐�


namespace alpha {
	namespace ambient {
		class ScriptSystem;
	};

	// fwd
	class EditorPanes;
	namespace ui {
		class SearchDialog;
//		class BookmarkDialog;

		class MainWindow : public Gtk::ApplicationWindow {
		public:
			/// @name Children
			/// @{
			EditorPanes& editorPanes() const BOOST_NOEXCEPT;
			SearchDialog& searchDialog() const BOOST_NOEXCEPT;
			StatusBar& statusBar() const BOOST_NOEXCEPT;
			/// @}

		private:
			std::unique_ptr<ui::SearchDialog> searchDialog_;
			StatusBar statusBar_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			manah::win32::ui::Rebar rebar_;		// rebar
			manah::win32::ui::Toolbar toolbar_;	// standard toolbar
#endif
		};
	}

	/// The application class of Alpha.
	class Application : public Gtk::Application {
	public:
		// constructors
		Application();
		// attributes
		static Application& instance();
		void setFont(const ascension::graphics::font::FontDescription& font);
		ui::MainWindow& window() const BOOST_NOEXCEPT;
		// operations
		bool teardown(bool callHook = true);

	private:
		void changeFont();
//		bool	handleKeyDown(command::VirtualKey key, command::KeyModifier modifiers);
		bool initInstance(int showCommand);
		void loadSettings();
		template<typename Section, typename Key, typename Container>
		void readProfileList(Section section, Key key, Container& items, const Glib::ustring& defaultValue = Glib::ustring());
		template<typename Section, typename Key>
		boost::optional<int> readIntegerProfile(Section section, Key key) const;	// dummy
		template<typename Section, typename Key>
		boost::optional<Glib::ustring> readStringProfile(Section section, Key key) const;	// dummy
		template<typename Section, typename Key, typename T>
		bool readStructureProfile(Section section, Key key, T& data) const;	// dummy
		void saveSettings();
//		void	setupToolbar();
		void updateTitleBar();
		template<typename Section, typename Key, typename Value>
		void writeIntegerProfile(Section section, Key key, Value value);	// dummy
		template<typename Section, typename Key, typename Value>
		void writeStringProfile(Section section, Key key, const Value& value) const;	// dummy
		template<typename Section, typename Key, typename T>
		void writeStructureProfile(Section section, Key key, const T& data) const;	// dummy

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
		/* �E�B���h�E�v���V�W�� */
		static LRESULT CALLBACK appWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
#endif
	};


	/// Returns the singleton application object.
	inline Application& Application::instance() {
		static Application singleton;
		return singleton;
	}

	namespace ui {
		/// Returns the search dialog box.
		inline SearchDialog& MainWindow::searchDialog() const BOOST_NOEXCEPT {
			return *searchDialog_;
		}

		/// Returns the status bar.
		inline StatusBar& MainWindow::statusBar() const BOOST_NOEXCEPT {
			return const_cast<MainWindow*>(this)->statusBar_;
		}
	}
} // namespace alpha

#endif // ALPHA_APPLICATION_HPP
