/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2009
 */

#ifndef ALPHA_APPLICATION_HPP
#define ALPHA_APPLICATION_HPP
#include "alpha.hpp"
#include "resource.h"
#include "buffer.hpp"
//#include "search.hpp"	// ui.SearchDialog
#include <ascension/session.hpp>
#include <ascension/searcher.hpp>
#include <manah/win32/module.hpp>
#include <manah/win32/ui/common-controls.hpp>
#include <manah/win32/gdi-object.hpp>
#include <map>
#include <sstream>


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

	// fwd
	namespace ui {
		class SearchDialog;
//		class BookmarkDialog;
	}
	namespace ambient {
		class ScriptSystem;
	};

	/// The status bar for the application main window.
	class StatusBar : protected manah::win32::ui::StatusBar {
	public:
		StatusBar();
		void adjustPaneWidths();
		bool create(HWND parent);
		using manah::win32::ui::StatusBar::get;
		void hide();
		using manah::win32::ui::StatusBar::isVisible;
		using manah::win32::ui::StatusBar::setSimple;
		void setText(const WCHAR* text, HFONT font = 0);
		void show();
		void updateAll();
		void updateCaretPosition();
		void updateNarrowingStatus();
		void updateOvertypeMode();
		void updateTemporaryMacroRecordingStatus();
		using manah::win32::ui::StatusBar::use;
	private:
		ascension::length_t columnStartValue_;
		manah::win32::gdi::Font font_;
		manah::win32::Handle<HICON, ::DestroyIcon> narrowingIcon_;
	};

	/// The application class of Alpha.
	class Alpha : public manah::win32::ProfilableApplication<> {
	public:
		// constructors
		Alpha();
		~Alpha() throw();
		// ���ʃI�u�W�F�N�g
//		command::KeyboardMap& keyboardMap() throw();
//		const command::KeyboardMap& keyboardMap() const throw();
		StatusBar& statusBar() /*throw()*/;
		// attributes
		static Alpha& instance();
		void textEditorFont(LOGFONTW& font) const /*throw()*/;
		void setFont(const LOGFONTW& font);
		// searchs
		ui::SearchDialog& searchDialog() /*throw()*/;
		const ui::SearchDialog& searchDialog() const /*throw()*/;
		// operations
//		void	loadKeyBinds(const std::wstring& schemeName);
		int messageBox(DWORD id, UINT type, manah::win32::Module::MessageArguments& args = MARGS);
		void parseCommandLine(const WCHAR* currentDirectory, const WCHAR* commandLine);

	private:
		void changeFont();
		LRESULT dispatchEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
//		bool	handleKeyDown(command::VirtualKey key, command::KeyModifier modifiers);
		bool initInstance(int showCommand);
		void loadINISettings();
		bool preTranslateMessage(const ::MSG& msg);
		void readProfileList(const wchar_t* section, const wchar_t* key, std::list<std::wstring>& items, const wchar_t* defaultValue = 0);
		void readProfileSet(const wchar_t* section, const wchar_t* key, std::set<std::wstring>& items, const wchar_t* defaultValue = 0);
		void saveINISettings();
//		void	setupMenus();
//		void	setupToolbar();
		void updateTitleBar();

		// message handlers
	protected:
		void onToolExecuteCommand();
	protected:
		bool onClose();																// WM_CLOSE
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

	private:
		static Alpha* instance_;	// ���� 1 �̃C���X�^���X
		// child windows
		manah::win32::ui::Rebar rebar_;				// ���o�[
		manah::win32::ui::Toolbar toolbar_;			// �W���c�[���o�[
		StatusBar statusBar_;		// �X�e�[�^�X�o�[
		std::auto_ptr<ui::SearchDialog> searchDialog_;
//		std::auto_ptr<ui::BookmarkDialog> bookmarkDialog_;	// [�u�b�N�}�[�N] �_�C�A���O
		// GDI objects
		HFONT editorFont_;	// �G�f�B�^�̃t�H���g
		HFONT statusFont_;	// �X�e�[�^�X�o�[�̃t�H���g
		// features and commands
//		std::auto_ptr<command::CommandManager> commandManager_;
//		command::KeyboardMap keyboardMap_;			// �g�p���̃L�[�{�[�h�}�b�v
//		MRUManager* mruManager_;
//		std::auto_ptr<ScriptMacroManager> scriptMacroManager_;	// �X�N���v�g�}�N���̊Ǘ�
//		command::VirtualKey twoStroke1stKey_;			// ���͒��� 2 �X�g���[�N�V�[�P���X�� 1 �X�g���[�N�ڂ̃L�[
//		command::KeyModifier twoStroke1stModifiers_;	// ���͒��� 2 �X�g���[�N�V�[�P���X�� 1 �X�g���[�N�ڂ̏C���L�[
//		friend class command::CommandManager;
//		friend class command::BuiltInCommand;
	};


	/// Returns the command manager.
//	inline command::CommandManager& Alpha::commandManager() throw() {return *commandManager_;}

	/// Returns the command manager.
//	inline const command::CommandManager& Alpha::commandManager() const throw() {return *commandManager_;}

	/// Returns the singleton application object.
	inline Alpha& Alpha::instance() {assert(instance_ != 0); return *instance_;}

	/// Returns the keyboard map.
//	inline command::KeyboardMap& Alpha::keyboardMap() throw() {return keyboardMap_;}

	/// Returns the keyboard map.
//	inline const command::KeyboardMap& Alpha::keyboardMap() const throw() {return keyboardMap_;}

	/// Returns the MRU manager.
//	inline MRUManager& Alpha::mruManager() throw() {return *mruManager_;}

	/// Returns the MRU manager.
//	inline const MRUManager& Alpha::mruManager() const throw() {return *mruManager_;}

	/// Returns the search dialog box.
	inline ui::SearchDialog& Alpha::searchDialog() throw() {return *searchDialog_;}

	/// Returns the search dialog box.
	inline const ui::SearchDialog& Alpha::searchDialog() const throw() {return *searchDialog_;}

	/// Returns the status bar.
	inline StatusBar& Alpha::statusBar() /*throw()*/ {return statusBar_;}

	/// Returns the font for text editors.
	inline void Alpha::textEditorFont(LOGFONTW& font) const throw() {::GetObjectW(editorFont_, sizeof(LOGFONTW), &font);}

} // namespace alpha

#endif // ALPHA_APPLICATION_HPP
