/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2007
 */

#ifndef ALPHA_APPLICATION_HPP
#define ALPHA_APPLICATION_HPP
#include "resource.h"
#include "buffer.hpp"
#include "keyboard-map.hpp"
#include "temporary-macro.hpp"
#include "ascension/session.hpp"
#include "ascension/searcher.hpp"
#include "../manah/win32/module.hpp"
#include "../manah/win32/ui/common-controls.hpp"
#include <map>
#include <sstream>
#include <boost/regex.hpp>


// タイトルバーとかに使うアプリケーション名
#define __WTIMESTAMP__	_T(__TIMESTAMP__)
#define IDS_APPNAME		L"Alpha"
#define IDS_APPVERSION	L"0.7.94.0"
#ifdef _DEBUG
#	define IDS_APPVERSIONINFO	L"debug version [" __WTIMESTAMP__ L"]"
#else
#	define IDS_APPVERSIONINFO	L"[" __WTIMESTAMP__ L"]"
#endif /* _DEBUG */
#define IDS_APPFULLVERSION	IDS_APPNAME L" " IDS_APPVERSION L" " IDS_APPVERSIONINFO

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

// INI sections
#define INI_SECTION_SCRIPTENGINES	L"ScriptEngines"

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
/*
class ICallable {
public:
	virtual ~ICallable() {}
	virtual void call() = 0;
};
*/

	// 前方宣言
	class MRUManager;
	namespace command {
		class CommandManager;
		class BuiltInCommand;
	}
	namespace ui {
		class SearchDialog;
		class BookmarkDialog;
	}
	namespace ambient {
		class Application;
	}
	namespace ankh {
		class ScriptSystem;
	}

	/// Alpha のアプリケーションクラス
	class Alpha : public manah::win32::ProfilableApplication<> {
	public:
		// constructors
		Alpha();
		~Alpha() throw();
		// 下位オブジェクト
		BufferList&						getBufferList() throw();
		const BufferList&				getBufferList() const throw();
		command::CommandManager&		getCommandManager() throw();
		const command::CommandManager&	getCommandManager() const throw();
		command::KeyboardMap&			getKeyboardMap() throw();
		const command::KeyboardMap&		getKeyboardMap() const throw();
		MRUManager&						getMRUManager() throw();
		const MRUManager&				getMRUManager() const throw();
		void							getScriptSystem(ankh::ScriptSystem*& scriptSystem) throw();
		void							getScriptSystem(const ankh::ScriptSystem*& scriptSystem) const throw();
		manah::win32::ui::StatusBar&	getStatusBar() throw();
		// attributes
		const std::wstring*	getCodePageName(ascension::encodings::CodePage cp) const;
		static Alpha&		getInstance();
		void				getTextEditorFont(::LOGFONTW& font) const throw();
		void				setFont(const ::LOGFONTW& font);
		void				setStatusText(const wchar_t* text, HFONT font = 0);
		// searchs
		ui::SearchDialog&		getSearchDialog() throw();
		const ui::SearchDialog&	getSearchDialog() const throw();
		void					showSearchDialog();
		// operations
		void	loadKeyBinds(const std::wstring& schemeName);
		int		messageBox(DWORD id, UINT type, manah::win32::Module::MessageArguments& args = MARGS);
		void	parseCommandLine(const WCHAR* currentDirectory, const WCHAR* commandLine);

	private:
		void	changeFont();
		LRESULT	dispatchEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
		bool	handleKeyDown(command::VirtualKey key, command::KeyModifier modifiers);
		bool	initInstance(int showCommand);
		void	loadINISettings();
		bool	preTranslateMessage(const ::MSG& msg);
		void	readProfileList(const wchar_t* section, const wchar_t* key, std::list<std::wstring>& items, const wchar_t* defaultValue = 0);
		void	readProfileSet(const wchar_t* section, const wchar_t* key, std::set<std::wstring>& items, const wchar_t* defaultValue = 0);
		void	registerScriptEngineAssociations();
		void	saveINISettings();
		void	setupMenus();
		void	setupToolbar();
		void	updateStatusBarPaneSize();
		void	updateTitleBar();

		// message handlers
	protected:
		void	onToolExecuteCommand();
	protected:
		bool	onClose();															// WM_CLOSE
		bool	onCommand(WORD id, WORD notifyCode, HWND control);					// WM_COMMAND
		void	onCopyData(HWND window, const COPYDATASTRUCT& cds);					// WM_COPYDATA
		void	onDestroy();														// WM_DESTROY
		void	onDrawItem(UINT id, const DRAWITEMSTRUCT& drawItem);				// WM_DRAWITEM
		void	onDropFiles(HDROP drop);											// WM_DROPFILES
		void	onEnterMenuLoop(bool isTrackPopup);									// WM_ENTERMENULOOP
		void	onExitMenuLoop(bool isTrackPopup);									// WM_EXITMENULOOP
		void	onInitMenuPopup(HMENU menu, UINT index, bool sysMenu);				// WM_INITMENUPOPUP
		void	onMeasureItem(UINT id, MEASUREITEMSTRUCT& mi);						// WM_MEASUREITEM
		LRESULT	onMenuChar(wchar_t ch, UINT flags, manah::win32::ui::Menu& menu);	// WM_MENUCHAR
		void	onMenuSelect(UINT itemID, UINT flags, HMENU sysMenu);				// WM_MENUSELECT
		bool	onNotify(int id, NMHDR& nmhdr);										// WM_NOTIFY
		bool	onSetCursor(HWND window, UINT hitTest, UINT message);				// WM_SETCURSOR
		void	onSettingChange(UINT flags, const wchar_t* section);				// WM_SETTINGCHANGE
		void	onSize(UINT type, int cx, int cy);									// WM_SIZE
		void	onTimer(UINT timerID);												// WM_TIMER

	protected:
		void	onRebarChevronPushed(const NMREBARCHEVRON& nmRebarChevron);	// RBN_CHEVRONPUSHED

	protected:
		/* ウィンドウプロシジャ */
		static LRESULT CALLBACK appWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		static Alpha* instance_;	// ただ 1 つのインスタンス
		// child windows
		manah::win32::ui::Rebar rebar_;				// レバー
		manah::win32::ui::Toolbar toolbar_;			// 標準ツールバー
		manah::win32::ui::StatusBar statusBar_;		// ステータスバー
		std::auto_ptr<ui::SearchDialog> searchDialog_;		// [検索と置換] ダイアログ
		std::auto_ptr<ui::BookmarkDialog> bookmarkDialog_;	// [ブックマーク] ダイアログ
		// GDI objects
		HFONT editorFont_;	// エディタのフォント
		HFONT statusFont_;	// ステータスバーのフォント
		// features and commands
		ankh::ScriptSystem* scriptSystem_;
		std::auto_ptr<command::CommandManager> commandManager_;
		command::KeyboardMap keyboardMap_;			// 使用中のキーボードマップ
		MRUManager* mruManager_;
		std::auto_ptr<BufferList> buffers_;
//		std::auto_ptr<ScriptMacroManager> scriptMacroManager_;	// スクリプトマクロの管理
		command::VirtualKey twoStroke1stKey_;			// 入力中の 2 ストロークシーケンスの 1 ストローク目のキー
		command::KeyModifier twoStroke1stModifiers_;	// 入力中の 2 ストロークシーケンスの 1 ストローク目の修飾キー
		std::map<ascension::encodings::CodePage, std::wstring>	codePageNameTable_;
		// オートメーション用インターフェイス
//		alpha::ambient::Application* automation_;
		// オプション
		bool showMessageBoxOnFind_;			// 検索処理でメッセージボックスを表示する
		bool initializeFindTextFromEditor_;	// [検索と置換] ダイアログを開いたときに
											// 検索テキストをエディタから初期化する
		friend class command::CommandManager;
		friend class command::BuiltInCommand;
	};


	/// Returns the buffer list.
	inline BufferList& Alpha::getBufferList() throw() {return *buffers_;}

	/// Returns the buffer list.
	inline const BufferList& Alpha::getBufferList() const throw() {return *buffers_;}

	/// Returns the command manager.
	inline command::CommandManager& Alpha::getCommandManager() throw() {return *commandManager_;}

	/// Returns the command manager.
	inline const command::CommandManager& Alpha::getCommandManager() const throw() {return *commandManager_;}

	/// Returns the singleton application object.
	inline Alpha& Alpha::getInstance() {assert(instance_ != 0); return *instance_;}

	/// Returns the keyboard map.
	inline command::KeyboardMap& Alpha::getKeyboardMap() throw() {return keyboardMap_;}

	/// Returns the keyboard map.
	inline const command::KeyboardMap& Alpha::getKeyboardMap() const throw() {return keyboardMap_;}

	/// Returns the MRU manager.
	inline MRUManager& Alpha::getMRUManager() throw() {return *mruManager_;}

	/// Returns the MRU manager.
	inline const MRUManager& Alpha::getMRUManager() const throw() {return *mruManager_;}

	/// Returns the search dialog box.
	inline ui::SearchDialog& Alpha::getSearchDialog() throw() {return *searchDialog_;}

	/// Returns the search dialog box.
	inline const ui::SearchDialog& Alpha::getSearchDialog() const throw() {return *searchDialog_;}

	/// Returns the status bar.
	inline manah::win32::ui::StatusBar& Alpha::getStatusBar() throw() {return statusBar_;}

	/// Returns the font for text editors.
	inline void Alpha::getTextEditorFont(::LOGFONTW& font) const throw() {::GetObject(editorFont_, sizeof(::LOGFONTW), &font);}

} // namespace alpha

#endif /* ALPHA_APPLICATION_HPP */
