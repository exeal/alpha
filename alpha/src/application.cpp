/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2009, 2014
 */

#include "application.hpp"
#include "ambient.hpp"
#include "input.hpp"
#include "ui.hpp"
#include "editor-window.hpp"
#include "search.hpp"
#include <ascension/text-editor.hpp>
#include <ascension/regex.hpp>
#include "../resource/messages.h"
#include <manah/win32/dc.hpp>
#include <manah/win32/gdi-object.hpp>
#include <manah/win32/ui/wait-cursor.hpp>
#include <algorithm>
#include <fstream>
#include <commdlg.h>	// ChooseFont
#include <shlwapi.h>	// PathXxxx
#include <comcat.h>		// ICatInformation
#include <dlgs.h>


// グローバル関数//////////////////////////////////////////////////////////

void callOleUninitialize() {
	::OleUninitialize();
}

/// The entry point.
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	// Shift キーを押しながら起動すると英語モードになるようにしてみた
	if(toBoolean(::GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
		::MessageBeep(MB_OK);
		::SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
		::InitMUILanguage(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}

#ifdef _DEBUG
	::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF
//		| _CRTDBG_CHECK_CRT_DF
//		| _CRTDBG_DELAY_FREE_MEM_DF
		| _CRTDBG_CHECK_EVERY_1024_DF);
//		| _CRTDBG_CHECK_ALWAYS_DF);
	long cccc = -1;
	if(cccc != -1)
		::_CrtSetBreakAlloc(cccc);
#endif /* _DEBUG */

	// NT 系か調べる
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	::GetVersionExA(&osvi);
	if(!toBoolean(osvi.dwPlatformId & VER_PLATFORM_WIN32_NT)) {
		char prompt[100];
		::LoadStringA(hInstance, MSG_ERROR__UNSUPPORTED_OS_VERSION, prompt, MANAH_COUNTOF(prompt));
		::MessageBoxA(0, prompt, "Alpha", MB_ICONHAND);	// title is obtained from IDS_APPNAME
		return -1;
	}
	HANDLE mutex = ::CreateMutexW(0, false, IDS_APPFULLVERSION);

	int	exitCode = 0/*EXIT_SUCCESS*/;

	// 簡単な多重起動抑止 (Ctrl キーを押しながら起動すると多重起動するようにしてみた)
	if(::GetLastError() != ERROR_ALREADY_EXISTS || toBoolean(::GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
		::OleInitialize(0);	// STA に入る + 高水準サービスの初期化
		atexit(&callOleUninitialize);
		win32::ui::initCommonControls(ICC_COOL_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_WIN95_CLASSES);
		ambient::Interpreter::instance().install();
		ambient::Interpreter::instance().toplevelPackage();
		Alpha* application = new Alpha();
		exitCode = application->run(nCmdShow);
		delete application;
	} else {	// 既存のプロセスにコマンドライン引数を渡す
		HWND existWnd = ::FindWindowW(IDS_APPNAME, 0);
		while(!toBoolean(::IsWindow(existWnd))) {
			::Sleep(1000);
			existWnd = ::FindWindowW(IDS_APPNAME, 0);
		}
		const WCHAR* const commandLine = ::GetCommandLineW();
		const size_t commandLineLength = wcslen(commandLine);
		WCHAR* data = new WCHAR[commandLineLength + 1 + MAX_PATH];
		::GetCurrentDirectoryW(MAX_PATH, data);
		wcscpy(data + MAX_PATH, commandLine);
		win32::AutoZero<COPYDATASTRUCT> cd;
		cd.lpData = data;
		cd.cbData = static_cast<DWORD>(sizeof(WCHAR) * (commandLineLength + 1 + MAX_PATH));
		::SendMessageW(existWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cd));
		::Sleep(300);
		::SetForegroundWindow(existWnd);
		delete[] data;
	}
	::CloseHandle(mutex);

	return exitCode;
}

AutoBuffer<WCHAR> alpha::a2u(const char* first, const char* last, DWORD flags /* = MB_PRECOMPOSED */) {
	const int len = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, first, static_cast<int>(last - first), 0, 0);
	manah::AutoBuffer<WCHAR> buffer(new WCHAR[len]);
	::MultiByteToWideChar(CP_ACP, flags, first, static_cast<int>(last - first), buffer.get(), len);
	return buffer;
}

AutoBuffer<char> alpha::u2a(const WCHAR* first, const WCHAR* last, DWORD flags /* = 0 */) {
	const int len = ::WideCharToMultiByte(CP_ACP, 0, first, static_cast<int>(last - first), 0, 0, 0, 0);
	manah::AutoBuffer<char> buffer(new char[len]);
	::WideCharToMultiByte(CP_ACP, flags, first, static_cast<int>(last - first), buffer.get(), len, 0, 0);
	return buffer;
}

namespace {

	/// ChooseFontW のためのフックプロシジャ
	UINT_PTR CALLBACK chooseFontHookProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
		if(message == WM_COMMAND && LOWORD(wParam) == psh3) {	// [適用] ボタン
			LOGFONTW lf;
			::SendMessageW(dialog, WM_CHOOSEFONT_GETLOGFONT, 0, reinterpret_cast<LPARAM>(&lf));
			Alpha::instance().setFont(lf);
			return true;
		} else if(message == WM_INITDIALOG) {
			::EnableWindow(::GetDlgItem(dialog, stc2), false);	// [スタイル] を無効に
			::EnableWindow(::GetDlgItem(dialog, cmb2), false);
		}
		return 0;
	}
} // namespace @0

namespace alpha {

	// Alpha //////////////////////////////////////////////////////////////////////////////////////////////////////////

	/// Constructor.
	Application::Application() : editorFont_(0) {
		searchDialog_.reset(new ui::SearchDialog());	// ctor of SearchDialog calls Alpha
		onSettingChange(0, 0);	// statusFont_ の初期化
	}

/// [フォント] ダイアログを表示してエディタのフォントを変更する
void Alpha::changeFont() {
	EditorView& activeView = EditorWindows::instance().activePane().visibleView();
	LOGFONTW font;
	win32::AutoZeroSize<CHOOSEFONTW> cf;

	textEditorFont(font);
	cf.hwndOwner = getMainWindow().use();
	cf.lpLogFont = &font;
	cf.lpfnHook = chooseFontHookProc;
	cf.Flags = CF_APPLY | CF_ENABLEHOOK | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_SCREENFONTS;
	cf.hInstance = use();

	if(toBoolean(::ChooseFontW(&cf))) {
		font.lfItalic = false;
		font.lfWeight = FW_REGULAR;
		setFont(font);
	}
}

/// @see manah#windows#Alpha#initInstance
bool Alpha::initInstance(int showCommand) {
	// setup the script engine
//	ambient::ScriptEngine::instance().installModules();

	// register the top level window class
	win32::AutoZeroSize<WNDCLASSEXW> wc;
	wc.style = CS_DBLCLKS/* | CS_DROPSHADOW*/;
	wc.lpfnWndProc = Alpha::appWndProc;
//	wc.cbClsExtra = 0;
//	wc.cbWndExtra = 0;
	wc.hInstance = use();
	wc.hIcon = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	wc.hIconSm = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	wc.hCursor = loadStandardCursor(MAKEINTRESOURCEW(32512));	// IDC_ARROW
	wc.hbrBackground = manah::win32::ui::BrushHandleOrColor(COLOR_3DFACE).get();
//	wc.lpszMenuName = 0;
	wc.lpszClassName = IDS_APPNAME;
	if(!toBoolean(::RegisterClassExW(&wc)))
		return false;

	static win32::ui::Window applicationWindow;

/*	// コードページ名の読み込み
	if(HRSRC resource = findResource(IDR_CODEPAGE_NAME_TABLE, RT_RCDATA)) {
		if(HGLOBAL buffer = loadResource(resource)) {
			if(const wchar_t* p = static_cast<const wchar_t*>(::LockResource(buffer))) {
				const wchar_t* const end = p + sizeofResource(resource) / sizeof(wchar_t);
				if(*p == 0xFEFF)	// UTF-16 BOM
					++p;
				while(true) {
					const wchar_t* const tab = find(p, end, L'\t');
					if(tab == end)
						break;
					const wchar_t* const lf = find(tab + 1, end, L'\n');
					if(lf == end)
						break;
					codePageNameTable_.insert(make_pair(wcstoul(p, 0, 10), wstring(tab + 1, lf)));
					p = lf + 1;
				}
//				::UnlockResource(buffer);
			}
			::FreeResource(buffer);
		}
	}
*/
	// 既定の書式を読み込む
	try {
		kernel::Newline newline =
			static_cast<kernel::Newline>(readIntegerProfile(L"File", L"defaultNewline", kernel::NLF_CR_LF));
		if(newline == kernel::NLF_RAW_VALUE)
			newline = kernel::NLF_CR_LF;
//		kernel::Document::setDefaultCode(readIntegerProfile(L"File", L"defaultCodePage", ::GetACP()), newline);
	} catch(std::invalid_argument&) {
		// TODO: 設定が間違っていることをユーザに通知
	}

	// トップレベルウィンドウ
	if(!applicationWindow.create(IDS_APPNAME, reinterpret_cast<HWND>(use()),
			win32::ui::DefaultWindowRect(), 0, /*WS_VISIBLE |*/ WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW))
		return false;
	setMainWindow(applicationWindow);

	// レバーの作成
	REBARINFO rbi = {sizeof(REBARINFO), 0, 0};
	rebar_.create(applicationWindow.use(), win32::ui::DefaultWindowRect(), 0, 0,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_BANDBORDERS | RBS_VARHEIGHT | CCS_NODIVIDER,
		WS_EX_TOOLWINDOW);
	rebar_.setBarInfo(rbi);

	// ツールバーの作成
//	setupToolbar();
	BufferList::instance().createBar(rebar_);

	// エディタウィンドウの作成
	// (WS_CLIPCHILDREN を付けると分割ウィンドウのサイズ変更枠が不可視になる...)
	EditorWindows::instance().create(getMainWindow().use(),
		win32::ui::DefaultWindowRect(), WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, *(new EditorWindow));
	assert(EditorWindows::instance().isWindow());

	// 一般設定の読み込み
	loadSettings();

	// スクリプトによる設定
	wchar_t dotAlpha[MAX_PATH];
	::GetModuleFileNameW(0, dotAlpha, MAX_PATH);
	wchar_t* fileName = ::PathFindFileNameW(dotAlpha);
	wcscpy(fileName, L".alpha");
	try {
		ambient::Interpreter::instance().executeFile(dotAlpha);
	} catch(const invalid_argument&) {
		// TODO: warn that ".alpha" is not found
	} catch(const boost::python::error_already_set&) {
		ambient::Interpreter::instance().handleException();
	}

	// ステータスバーの作成
	statusBar_.create(applicationWindow.use());
	statusBar_.adjustPaneWidths();

	// その他の初期化
	applicationWindow.dragAcceptFiles(true);
	applicationWindow.setTimer(ID_TIMER_QUERYCOMMAND, 200, 0);
	applicationWindow.setPosition(0, 0, 0, 760, 560, SWP_NOMOVE | SWP_NOZORDER);
	applicationWindow.center();

	// TODO: invoke the initialize script

	// 初期のビューの作成
	BufferList::instance().addNew();

//	setupMenus();
	if(!toBoolean(readIntegerProfile(L"View", L"visibleToolbar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_TOOLBAR), false);
	if(!toBoolean(readIntegerProfile(L"View", L"visibleStatusBar", true)))
		statusBar_.hide();
	if(!toBoolean(readIntegerProfile(L"View", L"visibleBufferBar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_BUFFERBAR), false);
	applicationWindow.show(showCommand);

	// コマンドラインから与えられたファイルを開く
	WCHAR cd[MAX_PATH];
	::GetCurrentDirectoryW(MAX_PATH, cd);
	parseCommandLine(cd, ::GetCommandLineW());

	// ...
	statusBar_.setText(0);

	// アウトプットウィンドウの作成
//	outputWindow.create(getMainWindow());
//	outputWindow.writeLine(OTT_GENERAL, IDS_APPFULLVERSION);

	// ツールダイアログの作成
	searchDialog_->doModeless(applicationWindow.get(), false);
	pushModelessDialog(searchDialog_->get());
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
//		searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
	}

	applicationWindow.setFocus();
	return true;
}

	/// Loads settings from the file.
	void Application::loadSettings() {
		// 表示に関する設定
		win32::AutoZero<LOGFONTW> lf;
		if(!readStructureProfile(L"View", L"Font.default", lf)) {
			lf.lfCharSet = ANSI_CHARSET;
			lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
			lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			lf.lfQuality = DEFAULT_QUALITY;
			lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
			wcscpy(lf.lfFaceName, L"Terminal");
		}
		setFont(lf);

		// Migemo DLL & 辞書パス
		const Glib::ustring migemoRuntimePath(boost::get_optional_value_or(readStringProfile("Find", "migemoRuntimePath")), Glib::ustring());
		const Glib::ustring migemoDictionaryPath(boost::get_optional_value_or(readStringProfile("Find", "migemoDictionaryPath")), Glib::ustring());
		if(migemoRuntimePath != boost::none && !migemoRuntimePath->empty() && migemoDictionaryPath != boost::none && !migemoDictionaryPath->empty()) {
	//		s2b(migemoRuntimePath);
	//		s2b(migemoDictionaryPath);
			ascension::regex::MigemoPattern::initialize(
				u2a(migemoRuntimePath->c_str(), migemoRuntimePath->c_str() + migemoRuntimePath->length() + 1).get(),
				u2a(migemoDictionaryPath->c_str(), migemoDictionaryPath->c_str() + migemoDictionaryPath->length() + 1).get());
		}

		// 検索文字列の履歴
		char keyName[30];
		std::list<ascension::String> findWhats, replacesWiths;
		for(unsigned short i = 0; i < 16; ++i) {
#if(_MSC_VER < 1400)
			std::sprintf(keyName, "findWhat(%u)", i);
#else
			std::snprintf(keyName, std::extent<decltype(keyName)>::value, "findWhat(%u)", i);
#endif // _MSC_VER < 1400
			const boost::optional<Glib::ustring>> v(readStringProfile("Find", keyName));
			if(v == boost::none || v->empty())
				break;
			findWhats.push_back(*v);
		}
		for(unsigned short i = 0; i < 16; ++i) {
#if(_MSC_VER < 1400)
			std::sprintf(keyName, "replaceWith(%u)", i);
#else
			std::snprintf(keyName, std::extent<decltype(keyName)>::value, "replaceWith(%u)", i);
#endif // _MSC_VER < 1400
			const boost::optional<Glib::ustring>> v(readStringProfile("Find", keyName));
			if(v == boost::none || v->empty())
				break;
			replacesWiths.push_back(*v);
		}
		ascension::searcher::TextSearcher& s = BufferList::instance().editorSession().textSearcher();
		s.setMaximumNumberOfStoredStrings(16);
		s.setStoredStrings(std::begin(findWhats), std::end(findWhats), false);
		s.setStoredStrings(std::begin(replacesWiths), std::end(replacesWiths), true);
	}

#if 0
/// スクリプトエンジンとファイルパターンの関連付けをする
void Alpha::registerScriptEngineAssociations() {
	// コンポーネントカテゴリからスクリプトエンジンを列挙し、INI から拡張子を拾う
	const CATID CATID_ActiveScript = {0xf0b7a1a1, 0x9847, 0x11cf, {0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64}};
	const CATID CATID_ActiveScriptParse = {0xf0b7a1a2, 0x9847, 0x11cf, {0x8f, 0x20, 0x00, 0x80, 0x5f, 0x2c, 0xd0, 0x64}};

	ComPtr<ICatInformation> category;
	if(FAILED(category.createInstance(CLSID_StdComponentCategoriesMgr)))
		return;

	ComPtr<IEnumCLSID> enumerator;
	if(FAILED(category->EnumClassesOfCategories(
			1, const_cast<CATID*>(&CATID_ActiveScript), 1, const_cast<CATID*>(&CATID_ActiveScriptParse), &enumerator)))
		return;

	CLSID clsid;
	OLECHAR* progID;
	for(enumerator->Reset(); enumerator->Next(1, &clsid, 0) == S_OK; ) {
		if(SUCCEEDED(::ProgIDFromCLSID(clsid, &progID))) {
			if(wchar_t* firstPeriod = wcschr(progID, L'.'))	// ProgID 中のバージョン番号を捨てる
				*firstPeriod = 0;
			const wstring pattern = readStringProfile(INI_SECTION_SCRIPTENGINES, progID);
			if(!pattern.empty())
				scriptSystem_->addEngineScriptNameAssociation(pattern.c_str(), clsid);
			::CoTaskMemFree(progID);
		}
	}
}
#endif // 0

	/// Saves the settings into the file.
	void Application::saveSettings() {
		char keyName[30];

		// バーの可視性の保存
		win32::AutoZero<REBARBANDINFOW> rbbi;
		rbbi.fMask = RBBIM_STYLE;
		rebar_.getBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
		writeIntegerProfile(L"View", L"visibleToolbar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
		rebar_.getBandInfo(rebar_.idToIndex(IDC_BUFFERBAR), rbbi);
		writeIntegerProfile(L"View", L"visibleBufferBar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
		writeIntegerProfile(L"View", L"visibleStatusBar", statusBar_.isVisible() ? 1 : 0);

		// 検索文字列履歴の保存
		const ascension::searcher::TextSearcher& s = BufferList::instance().editorSession().textSearcher();
		for(std::size_t i = 0; i < s.numberOfStoredPatterns(); ++i) {
#if(_MSC_VER < 1400)
			std::sprintf(keyName, L"findWhat(%u)", i);
#else
			std::snprintf(keyName, std::extent<decltype(keyName)>::value, "findWhat(%u)", i);
#endif // _MSC_VER < 1400
			writeStringProfile("Find", keyName, s.pattern(i).c_str());
		}
#if(_MSC_VER < 1400)
		std::sprintf(keyName, "findWhat(%u)", s.numberOfStoredPatterns());
#else
		std::snprintf(keyName, std::extent<decltype(keyName)>::value, "findWhat(%u)", s.numberOfStoredPatterns());
#endif // _MSC_VER < 1400
		writeStringProfile("Find", keyName, "");
		for(std::size_t i = 0; i < s.numberOfStoredReplacements(); ++i) {
#if(_MSC_VER < 1400)
			std::sprintf(keyName, "replaceWith(%u)", i);
#else
			std::snprintf(keyName, std::extent<decltype(keyName)>::value, "replaceWith(%u)", i);
#endif // _MSC_VER < 1400
			writeStringProfile("Find", keyName, s.replacement(i).c_str());
		}
#if(_MSC_VER < 1400)
		std::sprintf(keyName, "replaceWith(%u)", s.numberOfStoredReplacements());
#else
		std::snprintf(keyName, std::extent<decltype(keyName)>::value, "replaceWith(%u)", s.numberOfStoredReplacements());
#endif // _MSC_VER < 1400
		writeStringProfile("Find", keyName, "");
	}

/// 全てのエディタと一部のコントロールに新しいフォントを設定
void Alpha::setFont(const LOGFONTW& font) {
	LOGFONTW lf = font;

	lf.lfWeight = FW_NORMAL;
	::DeleteObject(editorFont_);
	editorFont_ = ::CreateFontIndirectW(&lf);

	// update the all presentations
	const int ydpi = win32::gdi::ScreenDC().getDeviceCaps(LOGPIXELSY); 
	BufferList& buffers = BufferList::instance();
	for(size_t i = 0; i < buffers.numberOfBuffers(); ++i) {
		ascension::presentation::Presentation& p = buffers.at(i).presentation();
		std::shared_ptr<const ascension::presentation::RunStyle> defaultStyle(p.defaultTextRunStyle());
		std::unique_ptr<ascension::presentation::RunStyle> newDefaultStyle(
			(defaultStyle.get() != 0) ? new ascension::presentation::RunStyle(*defaultStyle) : new ascension::presentation::RunStyle);
		newDefaultStyle->fontFamily = lf.lfFaceName;
		newDefaultStyle->fontProperties.weight = static_cast<ascension::presentation::FontProperties::Weight>(lf.lfWeight);
		newDefaultStyle->fontProperties.style = (lf.lfItalic != 0) ?
			ascension::presentation::FontProperties::ITALIC : presentation::FontProperties::NORMAL_STYLE;
		newDefaultStyle->fontProperties.size = lf.lfHeight * 96.0 / ydpi;
		newDefaultStyle->fontSizeAdjust = 0.0;
		p.setDefaultTextRunStyle(std::shared_ptr<const ascension::presentation::RunStyle>(newDefaultStyle.release()));
	}

	// 一部のコントロールにも設定
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
//		if(bookmarkDialog_.get() != 0 && bookmarkDialog_->isWindow())
//			bookmarkDialog_->sendItemMessage(IDC_LIST_BOOKMARKS, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		if(searchDialog_.get() != 0 && searchDialog_->isWindow()) {
//			searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//			searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		}
	}

	// INI ファイルに保存
	writeStructureProfile("View", "Font.default", lf);

	// 等幅 <-> 可変幅で表記を変える必要がある
	statusBar_.adjustPaneWidths();
}
#if 0
/// ツールバーの初期化 (1 回しか呼び出してはならない)
void Alpha::setupToolbar() {
	// 標準ツールバー
	CommandID* commands = 0;	// ツールバーに乗せるボタンに対応するコマンド
	size_t buttonCount;			// ボタン数
	list<wstring> buttonIDs;

	// 設定を読み込む
	readProfileList(L"ToolbarButtons", L"standard", buttonIDs, L"");

	if(!buttonIDs.empty()) {	// エントリがあった場合
		list<wstring>::const_iterator it;
		size_t i = 0;

		buttonCount = buttonIDs.size();
		commands = new CommandID[buttonCount];
		for(i = 0, it = buttonIDs.begin(); it != buttonIDs.end(); ++i, ++it)
			commands[i] = static_cast<CommandID>(wcstoul(it->c_str(), 0, 10));
	} else {	// デフォルトの設定を使う
		commands = new CommandID[buttonCount = 17];
		commands[0] = CMD_FILE_NEW;			commands[1] = CMD_FILE_OPEN;
		commands[2] = CMD_FILE_SAVE;		commands[3] = 0;
		commands[4] = CMD_EDIT_CUT;			commands[5] = CMD_EDIT_COPY;
		commands[6] = CMD_EDIT_PASTE;		commands[7] = 0;
		commands[8] = CMD_EDIT_UNDO;		commands[9] = CMD_EDIT_REDO;
		commands[10] = 0;					commands[11] = CMD_SEARCH_FIND;
		commands[12] = CMD_SEARCH_FINDNEXT;	commands[13] = CMD_SEARCH_FINDPREV;
		commands[14] = 0;					commands[15] = CMD_VIEW_WRAPNO;
		commands[16] = CMD_VIEW_WRAPBYWINDOWWIDTH;
	}
	for(size_t i = 0; i < buttonCount; ++i) {
		if(commands[i] != 0)
			commands[i] -= CMD_SPECIAL_START;
	}

	// イメージリストを作成する
	WCHAR iconDir[MAX_PATH];
	wcscpy(iconDir, getModuleFileName());
	::PathFindFileNameW(iconDir)[0] = 0;
	wcscat(iconDir, IDS_ICON_DIRECTORY_NAME);
	commandManager_->createImageList(iconDir);

	// ボタンを作る
	::TBBUTTON* buttons = new ::TBBUTTON[buttonCount];
	bool hasDropArrow;
	for(size_t i = 0, j = 0; i < buttonCount; ++i, ++j) {
		hasDropArrow = commands[j] + CMD_SPECIAL_START == CMD_FILE_NEW || commands[i] + CMD_SPECIAL_START == CMD_FILE_OPEN;
		ZeroMemory(buttons + i, sizeof(::TBBUTTON));
		buttons[i].fsState = TBSTATE_ENABLED;
		if(commands[j] == 0)
			buttons[i].fsStyle = BTNS_SEP;
		else {
			size_t icon = commandManager_->iconIndex(commands[j] + CMD_SPECIAL_START);
			if(icon == -1) {
				--i;
				--buttonCount;
				continue;
			}

			buttons[i].fsStyle = hasDropArrow ? BTNS_BUTTON | BTNS_DROPDOWN : BTNS_BUTTON;
//			if(/*!hasDropArrow*/)
				buttons[i].iBitmap = static_cast<int>(icon);
//			else {
//				const wstring caption = commandManager_->getCaption(commands[j]);
//				wchar_t* p = new wchar_t[caption.length() + 1];
//				wcscpy(p, caption.c_str());
//				buttons[i].iString = reinterpret_cast<INT_PTR>(p);
//				buttons[i].iBitmap = hasDropArrow ? static_cast<int>(icon) : I_IMAGENONE;
//			}
		}
		buttons[i].idCommand = commands[j];
	}

	if(!toolbar_.isWindow()) {
		toolbar_.create(rebar_.getHandle(), DefaultWindowRect(), L"", IDC_TOOLBAR,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE
			| CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP
			| TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT, WS_EX_TOOLWINDOW);
		HWND toolTips = toolbar_.getToolTips();
		toolbar_.setButtonStructSize();
		toolbar_.setExtendedStyle(TBSTYLE_EX_DRAWDDARROWS /*| TBSTYLE_EX_HIDECLIPPEDBUTTONS*/);
		::SetWindowLongPtrW(toolTips, GWL_STYLE, ::GetWindowLongPtrW(toolTips, GWL_STYLE) | TTS_NOPREFIX);
	} else {
		toolbar_.setImageList(0);
		toolbar_.setDisabledImageList(0);
		while(toolbar_.getButtonCount() != 0)
			toolbar_.deleteButton(0);
	}
	toolbar_.setBitmapSize(16, 16);
	toolbar_.setButtonSize(22, 22);
	toolbar_.addButtons(static_cast<int>(buttonCount), buttons);
	toolbar_.setImageList(commandManager_->imageList(CommandManager::ICONSTATE_NORMAL).getHandle());
	toolbar_.setDisabledImageList(commandManager_->imageList(CommandManager::ICONSTATE_DISABLED).getHandle());
	toolbar_.setHotImageList(commandManager_->imageList(CommandManager::ICONSTATE_HOT).getHandle());
//	toolbar_.setPadding(6, 6);

	for(size_t i = 0; i < buttonCount; ++i) {
		delete[] reinterpret_cast<wchar_t*>(buttons[i].iString);
//		if(buttons[i].fsStyle != BTNS_SEP
//				&& buttons[i].iBitmap != I_IMAGENONE) {	// アイコン付きボタンの幅をここで固定する
//			AutoZeroCB<TBBUTTONINFOW> tbi;
//			tbi.dwMask = TBIF_SIZE;
//			tbi.cx = (buttons[i].idCommand != CMD_FILE_NEW && buttons[i].idCommand != CMD_FILE_OPEN) ? 22 : 38;
//			toolbar_.setButtonInfo(buttons[i].idCommand, tbi);
//		}
	}
	delete[] commands;
	delete[] buttons;

	// レバーに乗せる
	MANAH_AUTO_STRUCT_SIZE(::REBARBANDINFOW, rbbi);
	const wstring caption = loadMessage(MSG_DIALOG__BUFFERBAR_CAPTION);
	rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;
	rbbi.fStyle = RBBS_GRIPPERALWAYS | RBBS_USECHEVRON;
	rbbi.wID = IDC_TOOLBAR;
	rbbi.hwndChild = toolbar_.getHandle();
	rbbi.cxMinChild = 0;
	rbbi.cyMinChild = 22;
	rebar_.insertBand(0, rbbi);

	// ツールバーのシェブロンが現れる幅の設定
	::RECT rect;
	toolbar_.getItemRect(toolbar_.getButtonCount() - 1, rect);
	rbbi.fMask = RBBIM_IDEALSIZE;
	rbbi.cxIdeal = rect.right;
	rebar_.setBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
}
#endif

/// @see Window::onCommand
bool Alpha::onCommand(WORD id, WORD notifyCode, HWND control) {
	EditorWindows::instance().activePane().showBuffer(BufferList::instance().at(id));
/*	const CommandID command = id + CMD_SPECIAL_START;
//	if(::GetCurrentThreadId() != getMainWindow().getWindowThreadID())
//		getMainWindow().sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
	if(command == CMD_SPECIAL_ILLEGALKEYSTROKES) {
		::MessageBeep(MB_OK);
		return true;
	} else if(command >= CMD_SPECIAL_BUFFERSSTART && command <= CMD_SPECIAL_BUFFERSEND) {
		if(commandManager_->isEnabled(command, true))
			buffers_->setActive(buffers_->at(command - CMD_SPECIAL_BUFFERSSTART));
	} else
		commandManager_->executeCommand(command, true);
*/	return true;
}

/// @see WM_COPYDATA
void Alpha::onCopyData(HWND window, const COPYDATASTRUCT& cds) {
	const WCHAR* const data = static_cast<WCHAR*>(cds.lpData);
	parseCommandLine(data, data + MAX_PATH);
}

/// @see Window::onDestroy
void Alpha::onDestroy() {
	// see also 'teardown'
	getMainWindow().killTimer(ID_TIMER_QUERYCOMMAND);
	getMainWindow().setMenu(0);

//	delete mruManager_;
//	toolbar_.destroyWindow();
//	statusBar_.destroyWindow();

	::PostQuitMessage(0);	// terminate STA thread
}

/// @see WM_DRAWITEM
void Alpha::onDrawItem(UINT, const DRAWITEMSTRUCT& drawItem) {
	if(drawItem.CtlType != ODT_MENU)	// 現時点ではメニューの描画のみ
		return;
/*	if(drawItem.itemID != 0) {
		const wstring text(commandManager_->menuName(drawItem.itemID + CMD_SPECIAL_START));
		manah::AutoBuffer<wchar_t> caption(new wchar_t[text.length() + 1]);
		wcscpy(caption.get(), text.c_str());
		wchar_t* accel = wcschr(caption.get(), L'\t');
		if(accel == caption.get())
			accel = 0;
		else if(accel != 0)
			*(accel++) = 0;
		Menu::drawItem(drawItem, caption.get(), accel, 0, 0,
			bufferList().bufferIcon(drawItem.itemID - (CMD_SPECIAL_BUFFERSSTART - CMD_SPECIAL_START)));
	} else
*/		win32::ui::Menu::drawItem(drawItem, 0);
}

/// @see WM_DROPFILES
void Alpha::onDropFiles(HDROP drop) {
	const UINT c = ::DragQueryFileW(drop, 0xffffffffu, 0, 0);
	AutoBuffer<WCHAR> fileName(new WCHAR[MAX_PATH]);
	UINT fileNameLength = MAX_PATH;
//	py::list files;

	// TODO: detect the window under the cursor by using DragQueryPoint.

//	if(<activates-on-drop-files>) {
//		if(mainWindow().isIconic())
//			mainWindow().show(SW_RESTORE);
//		mainWindow().forceSetForeground();
//	}

	for(uint i = 0; i < c; ++i) {
		const UINT len = ::DragQueryFileW(drop, i, 0, 0);
		if(len > fileNameLength)
			fileName.reset(new WCHAR[(fileNameLength = len) + 1]);
		::DragQueryFileW(drop, i, fileName.get(), fileNameLength);
#if 1
//		files.extend(ambient.convertWideStringToUnicodeObject(fileName));
#else
		if(!toBoolean(::PathIsDirectoryW(filePath)))
			BufferList::instance().open(filePath);
		else
			BufferList::instance().openDialog(filePath);
#endif
	}
	::DragFinish(drop);

	EditorView& activeView(EditorWindows::instance().activePane().visibleView());
	if(activeView.isWindow())
		activeView.setFocus();
}

/// @see WM_ENTERMENULOOP
void Alpha::onEnterMenuLoop(bool isTrackPopup) {
	statusBar_.setSimple(true);
}

/// @see WM_EXITMENULOOP
void Alpha::onExitMenuLoop(bool isTrackPopup) {
	statusBar_.setSimple(false);
}

/// @see WM_MEASUREITEM
void Alpha::onMeasureItem(UINT id, ::MEASUREITEMSTRUCT& mi) {
	if(mi.CtlType == ODT_MENU) {
		if(mi.itemID == 0)
			win32::ui::Menu::measureItem(mi, 0);
/*		else {
			const wstring s(commandManager_->menuName(mi.itemID + CMD_SPECIAL_START));
			manah::AutoBuffer<wchar_t> caption(new wchar_t[s.length() + 1]);
			wcscpy(caption.get(), s.c_str());
			wchar_t* accel = wcschr(caption.get(), L'\t');
			if(accel == caption.get())
				accel = 0;
			else if(accel != 0)
				*(accel++) = 0;
			Menu::measureItem(mi, caption.get(), accel);
		}
*/	}
}

/// @see WM_MENUCHAR
LRESULT Alpha::onMenuChar(wchar_t ch, UINT flags, win32::ui::Menu& menu) {
	return menu.handleMenuChar(ch, flags);
}

/// @see Window#onNotify
bool Alpha::onNotify(int id, NMHDR& nmhdr) {
	if(id == IDC_BUFFERBAR)
		return toBoolean(BufferList::instance().handleBufferBarNotification(*reinterpret_cast<NMTOOLBARW*>(&nmhdr)));
	else if(id == IDC_BUFFERBARPAGER)
		return toBoolean(BufferList::instance().handleBufferBarPagerNotification(nmhdr));

	switch(nmhdr.code) {
	case RBN_HEIGHTCHANGE:	// レバーの高さが変わった
		onSize(0, -1, -1);
		return true;
	case RBN_CHEVRONPUSHED:	// ツールバーのシェブロンが押された
		onRebarChevronPushed(*reinterpret_cast<LPNMREBARCHEVRON>(&nmhdr));
		return true;
#if 0
	case TBN_DROPDOWN: {	// ツールバーのドロップダウン
		RECT rect;
		POINT pt;
		const bool ctrlPressed = toBoolean(::GetKeyState(VK_CONTROL) & 0x8000);

		toolbar_.getRect(reinterpret_cast<LPNMTOOLBAR>(&nmhdr)->iItem, rect);
		pt.x = rect.left;
		pt.y = rect.bottom;
		getMainWindow().clientToScreen(pt);
		switch(reinterpret_cast<LPNMTOOLBAR>(&nmhdr)->iItem + CMD_SPECIAL_START) {
		case CMD_FILE_NEW:
/*			while(newDocTypeMenu_->getItemCount() > 0)	// すべて削除
				newDocTypeMenu_->deleteMenuItem<Menu::BY_POSITION>(0);
			for(size_t i = 0; i < buffers_->getDocumentTypeManager().getCount(); ++i) {
				const DocumentType& type = buffers_->getDocumentTypeManager().getAt(i);
				if(type.hidden && !ctrlPressed)
					continue;
				newDocTypeMenu_->appendMenuItem(CMD_FILE_DOCTYPELIST_START + static_cast<UINT>(i), type.name.c_str(), MFT_OWNERDRAW);
			}
			newDocTypeMenu_->trackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow());
*/			return true;
		case CMD_FILE_OPEN:
//			mruManager_->popupMenu().trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow().getHandle());
			return true;
		}
		break;
	}
#endif // 0
	case TBN_GETOBJECT:
		onCommand(reinterpret_cast<::LPNMOBJECTNOTIFY>(&nmhdr)->iItem, 0, 0);
		return 0;
/*	case TTN_GETDISPINFOW: {	// ツールバーのツールチップ
		const CommandID id = static_cast<CommandID>(nmhdr.idFrom + CMD_SPECIAL_START);
		if(id >= CMD_SPECIAL_BUFFERSSTART && id <= CMD_SPECIAL_BUFFERSEND)
			return toBoolean(buffers_->handleBufferBarNotification(*reinterpret_cast<::NMTOOLBARW*>(&nmhdr)));
		else {
			static wchar_t tipText[1024];
			::NMTTDISPINFOW& nmttdi = *reinterpret_cast<::NMTTDISPINFOW*>(&nmhdr);
			nmttdi.hinst = getHandle();
			wstring name(commandManager_->name(id));
			const wstring key(keyboardMap_.keyString(id));
			if(!key.empty()) {
				name += L" (";
				name += key;
				name += L")";
			}
			wcscpy(nmttdi.lpszText = tipText, name.c_str());
		}
		return true;
*/	}
/*	case TBN_HOTITEMCHANGE:
		if(nmhdr.idFrom = IDC_TOOLBAR) {
			NMTBHOTITEM& nmtbhi = *reinterpret_cast<NMTBHOTITEM*>(&nmhdr);
			dout << L"flags=" << nmtbhi.dwFlags << L":old=" << nmtbhi.idOld << L":new=" << nmtbhi.idNew << L"\n";
			if(toBoolean(nmtbhi.dwFlags & HICF_LEAVING))
				setStatusText(L"");
			else if(nmtbhi.idNew == nmtbhi.idOld)
				break;
			else {
				static wchar_t caption[500];
				if(0 != loadMessage(nmtbhi.idNew, caption, countof(caption))) {
					if(const wchar_t* const lf = wcschr(caption, L'\n'))
						setStatusText(lf + 1);
				} else
					setStatusText(L"");
			}
		}
		break;
	}*/
	return false;
}

/// RBN_CHEVRONPUSHED の処理
void Alpha::onRebarChevronPushed(const ::NMREBARCHEVRON& chevron) {
	win32::AutoZeroSize<REBARBANDINFOW> rbi;
	RECT bandRect;

	// ツールバーを得る (バッファバーでも共通のコードが使える)
	rebar_.getRect(chevron.uBand, bandRect);
	rbi.fMask = RBBIM_CHILD | RBBIM_IDEALSIZE;
	rebar_.getBandInfo(chevron.uBand, rbi);
	const long buttonCount = static_cast<long>(::SendMessageW(rbi.hwndChild, TB_BUTTONCOUNT, 0, 0L));

	// 非表示のボタンまで進む
	long i;
	RECT buttonRect;
	for(i = 0; i < buttonCount; ++i) {
		::SendMessageW(rbi.hwndChild, TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&buttonRect));
		if(buttonRect.right + bandRect.left > chevron.rc.left)
			break;
	}

	// 非表示のボタンをメニュー項目に変換
	win32::ui::PopupMenu popup;
	POINT pt = {chevron.rc.left, chevron.rc.bottom};
	win32::AutoZeroSize<TBBUTTONINFOW> tbbi;
    tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE;
	for(; i < buttonCount; ++i) {
		::SendMessageW(rbi.hwndChild, TB_GETBUTTONINFOW, i, reinterpret_cast<LPARAM>(&tbbi));
		if(toBoolean(tbbi.fsStyle & TBSTYLE_SEP))
			popup << win32::ui::Menu::SeparatorItem();
/*		else
			popup << Menu::StringItem(tbbi.idCommand, commandManager_->menuName(tbbi.idCommand + CMD_SPECIAL_START).c_str(),
				(commandManager_->isEnabled(tbbi.idCommand + CMD_SPECIAL_START, true) ? MFS_ENABLED : MFS_DISABLED)
				| ((commandManager_->isChecked(tbbi.idCommand + CMD_SPECIAL_START)) ? MFS_CHECKED : 0));
*/	}
	rebar_.clientToScreen(pt);
	popup.trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow().use());
}

/// @see WM_SETCURSOR
bool Alpha::onSetCursor(HWND hWnd, UINT nHitTest, UINT message) {
	POINT pt;	// カーソル位置
	RECT clientRect, statusBarRect;

	getMainWindow().getClientRect(clientRect);
	if(statusBar_.isVisible())	::GetWindowRect(statusBar_.get(), &statusBarRect);
	else						::SetRect(&statusBarRect, 0, 0, 0, 0);
	::GetCursorPos(&pt);
	getMainWindow().screenToClient(pt);

	if(pt.y >= clientRect.bottom - (statusBarRect.bottom - statusBarRect.top) - 3
			&& pt.y <= clientRect.bottom - (statusBarRect.bottom - statusBarRect.top)) {
		::SetCursor(loadStandardCursor(MAKEINTRESOURCEW(32645)));	// IDC_SIZENS
		return true;
	}

	return false;
}

/// @see WM_SETTINGCHNAGE
void Alpha::onSettingChange(UINT, const wchar_t*) {
	win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
	ascension::updateSystemSettings();
	::DeleteObject(statusFont_);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
	statusFont_ = ::CreateFontIndirectW(&ncm.lfStatusFont);
	statusBar_.adjustPaneWidths();
}

/**
 *	@see			Window::onSize
 *	@param type		Window::onSize 参照
 *	@param cx, cy	-1にするとこのメソッドは現在のウィンドウサイズを使う。
 *					このメソッドが行う幾つかの処理を単純に使用したい場合にも呼び出すことができる
 */
void Alpha::onSize(UINT type, int cx, int cy) {
	RECT rebarRect, statusBarRect, editorRect;

	if(cx == -1 && cy == -1) {
		RECT rect;
		getMainWindow().getClientRect(rect);
		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;
	}

	if(statusBar_.get() != 0 && statusBar_.isVisible()) {
		::SendMessage(statusBar_.get(), WM_SIZE, cx, cy);
		::GetWindowRect(statusBar_.get(), &statusBarRect);
		statusBar_.adjustPaneWidths();
	} else
		::SetRect(&statusBarRect, 0, 0, 0, 0);

	if(rebar_.get() != 0 && rebar_.isVisible()) {
		rebar_.sendMessage(WM_SIZE, cx, cy);
		rebar_.getRect(rebarRect);
//		toolbar_.sendMessage(WM_SIZE, cx, rebarRect.bottom - rebarRect.top - 2);
	} else
		::SetRect(&rebarRect, 0, 0, 0, 0);

	editorRect.left =  0;
	editorRect.top = rebarRect.bottom - rebarRect.top;
	editorRect.right = cx;
	editorRect.bottom = cy
		- ((statusBar_.get() != 0 && statusBar_.isVisible()) ? statusBarRect.bottom - statusBarRect.top : 0);
//	if(outputWindow.isWindow() && outputWindow.isVisible())
//		editorRect.bottom -= outputWndHeight_;
	if(EditorWindows::instance().isWindow())
		EditorWindows::instance().move(editorRect, true);

//	if(outputWindow_.isWindow()) {
//		outputWindow_.move(0, editorRect.bottom + 2, cx, outputWndHeight_);
//		outputWindow_.show(SW_SHOW);
//	}
}

/// @see WM_TIMER
void Alpha::onTimer(UINT timerID) {
	return;
	if(timerID == ID_TIMER_QUERYCOMMAND && BufferList::instance().numberOfBuffers() != 0) {
		// ツールバーアイテムの有効化/無効化
		if(toolbar_.isWindow() && toolbar_.isVisible()) {
			const size_t buttonCount = toolbar_.getButtonCount();
			TBBUTTON button;

//			toolbar_.lockWindowUpdate();	// これがあるとツールバーがスクリーン外にある時に画面がちらつく
			for(size_t i = 0; i < buttonCount; ++i) {
				toolbar_.getButton(static_cast<int>(i), button);
//				toolbar_.checkButton(button.idCommand, commandManager_->isChecked(button.idCommand + CMD_SPECIAL_START));
//				toolbar_.enableButton(button.idCommand, commandManager_->isEnabled(button.idCommand + CMD_SPECIAL_START, true));
			}
//			toolbar_.unlockWindowUpdate();
		}
	} else if(timerID == ID_TIMER_MOUSEMOVE) {	// ヒント表示
/*		if(GetActiveTab()->GetTextEditor() != 0
				&& m_pActiveDebugger != 0
				&& m_pActiveDebugger->IsDebugging()) {
			CAlphaView*			pActiveView = GetActiveTab()->GetTextEditor()->GetActiveView();
			DebugPropertyInfo	dpi;
			POINT				pt, ptView;
			wstring				strExpression;
			wostringstream		ssResult;

			::GetCursorPos(&pt);
			ptView = pt;
			pActiveView->ScreenToClient(&ptView);
			strExpression = (pActiveView->HasSelection() && pActiveView->IsOverSelection(ptView)) ?
				pActiveView->GetSelection() : pActiveView->GetNearestWordFromCursor();
			dpi.m_dwValidFields = DBGPROP_INFO_NAME | DBGPROP_INFO_TYPE | DBGPROP_INFO_VALUE;
			if(!strExpression.empty()
						&& SUCCEEDED(m_pActiveDebugger->EvaluateExpression(strExpression, 10, true, &dpi))) {
				if(dpi.m_bstrType != 0
						&& wcscmp(dpi.m_bstrType, L"Error") != 0	// VC6
						&& wcscmp(dpi.m_bstrType, L"エラー") != 0)	// VC7
					ssResult << dpi.m_bstrName << L" = " << dpi.m_bstrValue;
				if(dpi.m_dwValidFields & DBGPROP_INFO_NAME)		::SysFreeString(dpi.m_bstrName);
				if(dpi.m_dwValidFields & DBGPROP_INFO_TYPE)		::SysFreeString(dpi.m_bstrType);
				if(dpi.m_dwValidFields & DBGPROP_INFO_VALUE)	::SysFreeString(dpi.m_bstrValue);
			}
		}
*/		getMainWindow().killTimer(ID_TIMER_MOUSEMOVE);
	}
}

bool Alpha::teardown(bool callHook /* = true */) {
	if(callHook) {
		py::object toplevel(ambient::Interpreter::instance().toplevelPackage());
		if(toBoolean(::PyObject_HasAttrString(toplevel.ptr(), "about_to_be_killed_hook"))) {
			try {
				if(!toBoolean(::PyObject_IsTrue(toplevel.attr("about_to_be_killed_hook")().ptr())))
					return false;
			} catch(py::error_already_set&) {
				ambient::Interpreter::instance().handleException();
			}
		}
	}
	saveINISettings();
	getMainWindow().destroy();
//	::PostQuitMessage(0);
	return true;
}


namespace {
	bool killAlpha(bool callHook) {return Alpha::instance().teardown(callHook);}
}

ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
	ambient::Interpreter& interpreter = ambient::Interpreter::instance();
	py::scope scope(interpreter.toplevelPackage());

	py::def("kill_alpha", &killAlpha, py::arg("call_hook") = true);
ALPHA_EXPOSE_EPILOGUE()
}
