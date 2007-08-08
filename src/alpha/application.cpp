/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "application.hpp"
#include "ankh/core.hpp"
#include "command.hpp"
#include "mru-manager.hpp"
#include "search-dialog.hpp"
#include "bookmark-dialog.hpp"
//#include "DebugDlg.h"
#include "ascension/text-editor.hpp"
#include "ascension/regex.hpp"
#include "ankh/startup-handler.hpp"
#include "../manah/win32/dc.hpp"
#include "../manah/win32/gdi-object.hpp"
#include "../manah/win32/ui/wait-cursor.hpp"
#include <algorithm>
#include <fstream>
#include <commdlg.h>	// ChooseFont
#include <shlwapi.h>	// PathXxxx
#include <comcat.h>		// ICatInformation
#include <dlgs.h>
//#include "Msxml3.tlh"	// MSXML2::IXMLDOMDocument
using namespace alpha;
using namespace alpha::command;
using namespace ascension;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace ascension::encodings;
using namespace ascension::texteditor::commands;
using namespace manah::win32;
using namespace manah::win32::ui;
using namespace manah::win32::gdi;
using namespace manah::com;
using namespace std;


// �O���[�o���֐�//////////////////////////////////////////////////////////

/// �G���g���|�C���g
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	// Shift �L�[�������Ȃ���N������Ɖp�ꃂ�[�h�ɂȂ�悤�ɂ��Ă݂�
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

	// NT �n�����ׂ�
	::OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOA);
	::GetVersionExA(&osvi);
	if(!toBoolean(osvi.dwPlatformId & VER_PLATFORM_WIN32_NT)) {
		char prompt[100];
		::LoadStringA(hInstance, MSG_ERROR__UNSUPPORTED_OS_VERSION, prompt, countof(prompt));
		::MessageBoxA(0, prompt, "Alpha", MB_ICONHAND);	// title is obtained from IDS_APPNAME
		return -1;
	}
	HANDLE mutex = ::CreateMutexW(0, false, IDS_APPFULLVERSION);

	int	exitCode = 0/*EXIT_SUCCESS*/;

	// �ȒP�ȑ��d�N���}�~ (Ctrl �L�[�������Ȃ���N������Ƒ��d�N������悤�ɂ��Ă݂�)
	if(::GetLastError() != ERROR_ALREADY_EXISTS || toBoolean(::GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
		::OleInitialize(0);	// STA �ɓ��� + �������T�[�r�X�̏�����
		initCommonControls(ICC_COOL_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_WIN95_CLASSES);
		Alpha* application = new Alpha();
		exitCode = application->run(nCmdShow);
		delete application;
		::OleUninitialize();
	} else {	// �����̃v���Z�X�ɃR�}���h���C��������n��
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
		AutoZero<::COPYDATASTRUCT> cd;
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

namespace {
	inline void showLastErrorMessage(HWND parent = 0) {
		void* buffer;
		::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<wchar_t*>(&buffer), 0, 0);
		::MessageBoxW(parent, static_cast<wchar_t*>(buffer), IDS_APPNAME, MB_OK);
		::LocalFree(buffer);
	}

	// �X���b�V���ƃo�b�N�X���b�V���̌���
	inline void s2b(WCHAR* first, WCHAR* last) {replace_if(first, last, bind2nd(equal_to<WCHAR>(), L'/'), L'\\');}
	inline void b2s(WCHAR* first, WCHAR* last) {replace_if(first, last, bind2nd(equal_to<WCHAR>(), L'\\'), L'/');}
	inline void s2b(std::basic_string<WCHAR>& s) {replace_if(s.begin(), s.end(), bind2nd(equal_to<WCHAR>(), L'/'), L'\\');}
	inline void b2s(std::basic_string<WCHAR>& s) {replace_if(s.begin(), s.end(), bind2nd(equal_to<WCHAR>(), L'\\'), L'/');}

	// UTF-16 �ƃ��[�U����R�[�h�y�[�W�̕ϊ�
	manah::AutoBuffer<char> u2a(const wchar_t* first, const wchar_t* last) {
		const int len = ::WideCharToMultiByte(CP_ACP, 0, first, static_cast<int>(last - first), 0, 0, 0, 0);
		manah::AutoBuffer<char> buffer(new char[len]);
		::WideCharToMultiByte(CP_ACP, 0, first, static_cast<int>(last - first), buffer.get(), len, 0, 0);
		return buffer;
	}
	manah::AutoBuffer<wchar_t> a2u(const char* first, const char* last) {
		const int len = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, first, static_cast<int>(last - first), 0, 0);
		manah::AutoBuffer<wchar_t> buffer(new wchar_t[len]);
		::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, first, static_cast<int>(last - first), buffer.get(), len);
		return buffer;
	}

	/// ChooseFontW �̂��߂̃t�b�N�v���V�W��
	UINT_PTR CALLBACK chooseFontHookProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
		if(message == WM_COMMAND && LOWORD(wParam) == psh3) {	// [�K�p] �{�^��
			::LOGFONTW lf;
			::SendMessageW(dialog, WM_CHOOSEFONT_GETLOGFONT, 0, reinterpret_cast<LPARAM>(&lf));
			Alpha::getInstance().setFont(lf);
			return true;
		} else if(message == WM_INITDIALOG) {
			::EnableWindow(::GetDlgItem(dialog, stc2), false);	// [�X�^�C��] �𖳌���
			::EnableWindow(::GetDlgItem(dialog, cmb2), false);
		}
		return 0;
	}
} // namespace @0


// Alpha //////////////////////////////////////////////////////////////

Alpha* Alpha::instance_ = 0;

/// �R���X�g���N�^
Alpha::Alpha() : editorFont_(0), scriptSystem_(new ankh::ScriptSystem), mruManager_(0), twoStroke1stKey_(VK_NULL), twoStroke1stModifiers_(0) {
	assert(Alpha::instance_ == 0);
	Alpha::instance_ = this;
	scriptSystem_->AddRef();
	commandManager_.reset(new CommandManager);
	searchDialog_.reset(new ui::SearchDialog);
	bookmarkDialog_.reset(new ui::BookmarkDialog);
	registerScriptEngineAssociations();
	onSettingChange(0, 0);	// statusFont_ �̏�����

	// load startup.xml
	::WCHAR fileName[MAX_PATH];
	wcscpy(fileName, getModuleFileName());
	::WCHAR* const separator = wcsrchr(fileName, _T('\\'));
	assert(separator != 0);
	wcscpy(separator + 1, L"startup.xml");
	ankh::StartupHandler sh(*scriptSystem_, fileName);
}

/// �f�X�g���N�^
Alpha::~Alpha() {
	buffers_.reset(0);	// ��ɉ�̂���
	::DeleteObject(statusFont_);
	scriptSystem_->shutdown();
	scriptSystem_->Release();
	Alpha::instance_ = 0;
}

LRESULT CALLBACK Alpha::appWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	return (instance_ != 0) ?
		instance_->dispatchEvent(window, message, wParam, lParam) : ::DefWindowProc(window, message, wParam, lParam);
}

/// [�t�H���g] �_�C�A���O��\�����ăG�f�B�^�̃t�H���g��ύX����
void Alpha::changeFont() {
	EditorView& activeView = buffers_->getActiveView();
	::LOGFONTW font;
	AutoZeroLS<::CHOOSEFONTW> cf;

	getTextEditorFont(font);
	cf.hwndOwner = getMainWindow().getHandle();
	cf.lpLogFont = &font;
	cf.lpfnHook = chooseFontHookProc;
	cf.Flags = CF_APPLY | CF_ENABLEHOOK | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_SCREENFONTS;
	cf.hInstance = getHandle();

	if(toBoolean(::ChooseFontW(&cf))) {
		font.lfItalic = false;
		font.lfWeight = FW_REGULAR;
		setFont(font);
	}
}

/// ���b�Z�[�W�̐U�蕪��
LRESULT	Alpha::dispatchEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
	case WM_ACTIVATE:
		if(wParam == WA_ACTIVE) {
			for(size_t i = 0; i < buffers_->getCount(); ++i)
				buffers_->getAt(i).checkTimeStamp();
		}
		return 0L;
	case WM_COMMAND:
		return onCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
	case WM_CLOSE:
		onClose();
		return 0;
	case WM_COPYDATA:
		onCopyData(reinterpret_cast<HWND>(wParam), *reinterpret_cast<::COPYDATASTRUCT*>(lParam));
		break;
	case WM_CREATE:
		break;
	case WM_DESTROY:
		onDestroy();
		break;
	case WM_DRAWITEM:
		onDrawItem(static_cast<UINT>(wParam), *reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
		break;
	case WM_DROPFILES:
		onDropFiles(reinterpret_cast<HDROP>(wParam));
		break;
	case WM_ENTERMENULOOP:
		onEnterMenuLoop(toBoolean(wParam));
		break;
	case WM_EXITMENULOOP:
		onExitMenuLoop(toBoolean(wParam));
		break;
	case WM_INITMENUPOPUP:
		onInitMenuPopup(reinterpret_cast<HMENU>(wParam), LOWORD(lParam), toBoolean(HIWORD(lParam)));
		break;
	case WM_MEASUREITEM:
		onMeasureItem(static_cast<UINT>(wParam), *reinterpret_cast<::MEASUREITEMSTRUCT*>(lParam));
		break;
	case WM_MENUCHAR: {
		auto_ptr<Menu> activePopup(new Menu(reinterpret_cast<HMENU>(lParam)));
		return onMenuChar(LOWORD(wParam), HIWORD(wParam), *activePopup);
	}
	case WM_MENUSELECT:
		onMenuSelect(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HMENU>(lParam));
		break;
	case WM_NOTIFY:
		onNotify(static_cast<UINT>(wParam), *reinterpret_cast<NMHDR*>(lParam));
		break;
	case WM_QUERYENDSESSION:
		return onClose();
	case WM_SETCURSOR:
		if(onSetCursor(reinterpret_cast<HWND>(wParam),
				static_cast<UINT>(LOWORD(lParam)), static_cast<UINT>(HIWORD(lParam))))
			return false;
		break;
	case WM_SETFOCUS:
		// m_iActiveView �������悤�Ƃ��Ă���r���[���w���\��������
//		if(m_buffers.GetCount() != 0 && m_buffers.GetActiveDocumentIndex() != -1
//				&& reinterpret_cast<HWND>(wParam) != m_documents.GetActiveDocument()->GetWindow())
//			::SendMessage(m_documents.GetActiveDocument()->GetWindow(), message, wParam, lParam);
		buffers_->getEditorWindow().setFocus();
		return 0L;
	case WM_SETTINGCHANGE:
		onSettingChange(static_cast<UINT>(wParam), reinterpret_cast<wchar_t*>(lParam));
		break;
	case WM_SIZE:
		onSize(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_TIMER:
		onTimer(static_cast<UINT>(wParam));
		break;
	}
	return ::DefWindowProc(window, message, wParam, lParam);
}

/**
 * �R�[�h�y�[�W�̖��O��Ԃ��B�R�[�h�y�[�W���������Ȃ���� null
 * @param cp �R�[�h�y�[�W
 */
const wstring* Alpha::getCodePageName(CodePage cp) const {
	map<CodePage, wstring>::const_iterator it = codePageNameTable_.find(cp);
	return (it != codePageNameTable_.end()) ? &it->second : 0;
}

/// �X�N���v�g�V�X�e����Ԃ�
void Alpha::getScriptSystem(ankh::ScriptSystem*& scriptSystem) throw() {(scriptSystem = scriptSystem_)->AddRef();}

/// �X�N���v�g�V�X�e����Ԃ�
void Alpha::getScriptSystem(const ankh::ScriptSystem*& scriptSystem) const throw() {
	const_cast<ankh::ScriptSystem*>(scriptSystem_)->AddRef(); (scriptSystem = scriptSystem_);}

/**
 * �L�[�g�ݍ��킹���R�}���h�ɕϊ����Ď��s����
 * @param key ���z�L�[
 * @param modifiers �C���L�[
 * @return �R�}���h�ɕϊ��ł����ꍇ true
 */
bool Alpha::handleKeyDown(VirtualKey key, KeyModifier modifiers) {
	if(key == VK_MENU && modifiers == 0) {	// [Alt] �̂� -> ���j���[���A�N�e�B�u�ɂ���
		getMainWindow().sendMessage(WM_INITMENU, reinterpret_cast<WPARAM>(getMainWindow().getMenu().getHandle()));
		return true;
	} else if(key == VK_CONTROL || key == VK_MENU || key == VK_SHIFT)	// �C���L�[���P�Ƃŉ����ꂽ -> ����
		return false;

	if(twoStroke1stKey_ == VK_NULL) {	// 1�X�g���[�N��
		Command* command = keyboardMap_.getCommand(KeyCombination(key, modifiers));
		if(command == 0)
			return false;
		else if(command->isBuiltIn() && command->getID() == CMD_SPECIAL_WAITFOR2NDKEYS) {
			twoStroke1stKey_ = key;
			twoStroke1stModifiers_ = modifiers;
			const wstring s = loadString(MSG_STATUS__WAITING_FOR_2ND_KEYS,
				MARGS % KeyboardMap::getStrokeString(KeyCombination(key, modifiers)));
			setStatusText(s.c_str());
		} else
			command->execute();
	} else {	// 2�X�g���[�N��
		Command* command = keyboardMap_.getCommand(
			KeyCombination(twoStroke1stKey_, twoStroke1stModifiers_), KeyCombination(key, modifiers));
		if(command != 0) {
			setStatusText(0);
			command->execute();
		} else {
			const wstring s = loadString(MSG_STATUS__INVALID_2STROKE_COMBINATION,
				MARGS % KeyboardMap::getStrokeString(
					KeyCombination(twoStroke1stKey_, twoStroke1stModifiers_), KeyCombination(key, modifiers)));
			::MessageBeep(MB_OK);
			setStatusText(s.c_str());
		}
		twoStroke1stKey_ = VK_NULL;
	}
	return true;
}

/// @see manah#windows#Alpha#initInstance
bool Alpha::initInstance(int showCommand) {
	// �E�B���h�E�N���X�̓o�^
	AutoZeroCB<::WNDCLASSEXW> wc;
	wc.style = CS_DBLCLKS/* | CS_DROPSHADOW*/;
	wc.lpfnWndProc = Alpha::appWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = getHandle();
	wc.hIcon = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	wc.hIconSm = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	wc.hCursor = loadStandardCursor(IDC_ARROW);
	wc.hbrBackground = manah::win32::BrushHandleOrColor(COLOR_3DFACE).brush;
	wc.lpszClassName = IDS_APPNAME;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
	if(!toBoolean(::RegisterClassExW(&wc)))
		return false;

	static Window applicationWindow;

	// �R�[�h�y�[�W���̓ǂݍ���
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

	// ����̏�����ǂݍ���
	try {
		text::Newline newline =
			static_cast<text::Newline>(readIntegerProfile(L"File", L"defaultNewline", text::NLF_CRLF));
		if(newline == text::NLF_AUTO)
			newline = text::NLF_CRLF;
		text::Document::setDefaultCode(readIntegerProfile(L"File", L"defaultCodePage", ::GetACP()), newline);
	} catch(invalid_argument&) {
		// TODO: �ݒ肪�Ԉ���Ă��邱�Ƃ����[�U�ɒʒm
	}

	// �g�b�v���x���E�B���h�E
	if(!applicationWindow.create(IDS_APPNAME, reinterpret_cast<HWND>(getHandle()),
			DefaultWindowRect(), 0, /*WS_VISIBLE |*/ WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW))
		return false;
	setMainWindow(applicationWindow);

	// ���o�[�̍쐬
	::REBARINFO rbi = {sizeof(::REBARINFO), 0, 0};
	rebar_.create(applicationWindow.getHandle(), DefaultWindowRect(), 0, 0,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_BANDBORDERS | RBS_VARHEIGHT | CCS_NODIVIDER,
		WS_EX_TOOLWINDOW);
	rebar_.setBarInfo(rbi);

	// �o�b�t�@���X�g
	buffers_.reset(new BufferList(*this));

	// �c�[���o�[�̍쐬
	setupToolbar();
	buffers_->createBar(rebar_);

	// ��ʐݒ�̓ǂݍ���
	loadINISettings();

	// �X�N���v�g�ɂ��ݒ�
	wchar_t scriptName[MAX_PATH];
	wchar_t* fileName = 0;
	wcscpy(scriptName, getModuleFileName());
	fileName = ::PathFindFileNameW(scriptName);
	wcscpy(fileName, IDS_MACRO_DIRECTORY_NAME IDS_EVENTSCRIPTFILENAME);
	// TODO: initialize script

	// MRU ���X�g�̍쐬
	mruManager_ = new MRUManager(readIntegerProfile(L"File", L"mruLimit", 8), CMD_FILE_MRULIST_START);
	mruManager_->load();

	// �X�e�[�^�X�o�[�̍쐬
	statusBar_.create(applicationWindow.getHandle(), DefaultWindowRect(), 0, IDC_STATUSBAR,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_BOTTOM | CCS_NODIVIDER | SBARS_SIZEGRIP | SBT_TOOLTIPS);
	updateStatusBarPaneSize();

	// ���̑��̏�����
	applicationWindow.dragAcceptFiles(true);
	applicationWindow.setTimer(ID_TIMER_QUERYCOMMAND, 200, 0);
	applicationWindow.setPosition(0, 0, 0, 760, 560, SWP_NOMOVE | SWP_NOZORDER);
	applicationWindow.center();

	// TODO: invoke the initialize script

	// �����̃r���[�̍쐬
	buffers_->addNew();

	setupMenus();
	if(!toBoolean(readIntegerProfile(L"View", L"visibleToolbar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_TOOLBAR), false);
	if(!toBoolean(readIntegerProfile(L"View", L"visibleStatusBar", true)))
		statusBar_.show(SW_HIDE);
	if(!toBoolean(readIntegerProfile(L"View", L"visibleBufferBar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_BUFFERBAR), false);
	applicationWindow.show(showCommand);

	// �R�}���h���C������^����ꂽ�t�@�C�����J��
	WCHAR cd[MAX_PATH];
	::GetCurrentDirectoryW(MAX_PATH, cd);
	parseCommandLine(cd, ::GetCommandLineW());

	// ...
	setStatusText(0);

	// �A�E�g�v�b�g�E�B���h�E�̍쐬
//	outputWindow.create(getMainWindow());
//	outputWindow.writeLine(OTT_GENERAL, IDS_APPFULLVERSION);

	// �c�[���_�C�A���O�̍쐬
	searchDialog_->doModeless(applicationWindow.getHandle(), false);
	pushModelessDialog(searchDialog_->getHandle());
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
		searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
		searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
	}

	applicationWindow.setFocus();
	return true;
}

/// INI �t�@�C������ݒ��ǂݍ���
void Alpha::loadINISettings() {
	// �\���Ɋւ���ݒ�
	AutoZero<::LOGFONTW> lf;
	if(!readStructureProfile(L"View", L"Font.default", lf)) {
		lf.lfCharSet = ANSI_CHARSET;
		lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = DEFAULT_QUALITY;
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		wcscpy(lf.lfFaceName, L"Terminal");
	}
	setFont(lf);

	// Migemo DLL & �����p�X
	const basic_string<WCHAR> migemoRuntimePath = readStringProfile(L"Find", L"migemoRuntimePath", L"");
	const basic_string<WCHAR> migemoDictionaryPath = readStringProfile(L"Find", L"migemoDictionaryPath", L"");
	if(!migemoRuntimePath.empty() && !migemoDictionaryPath.empty()) {
//		s2b(migemoRuntimePath);
//		s2b(migemoDictionaryPath);
		regex::MigemoPattern::initialize(
			u2a(migemoRuntimePath.c_str(), migemoRuntimePath.c_str() + migemoRuntimePath.length() + 1).get(),
			u2a(migemoDictionaryPath.c_str(), migemoDictionaryPath.c_str() + migemoDictionaryPath.length() + 1).get());
	}

	// ����������̗���
	wchar_t	keyName[30];
	wstring	value;
	list<wstring> findWhats, replacesWiths;
	for(ushort i = 0; i < 16; ++i) {
		swprintf(keyName, L"findWhat(%u)", i);
		value = readStringProfile(L"Find", keyName);
		if(value.empty())
			break;
		findWhats.push_back(value);
	}
	for(ushort i = 0; i < 16; ++i) {
		swprintf(keyName, L"replaceWith(%u)", i);
		value = readStringProfile(L"Find", keyName);
		if(value.empty())
			break;
		replacesWiths.push_back(value);
	}
	searcher::TextSearcher& s = buffers_->getEditorSession().getTextSearcher();
	s.setMaximumNumberOfStoredStrings(16);
	s.setStoredStrings(findWhats.begin(), findWhats.end(), false);
	s.setStoredStrings(replacesWiths.begin(), replacesWiths.end(), true);

	// ���̑�
	showMessageBoxOnFind_ = toBoolean(readIntegerProfile(L"Find", L"showMessageBox", 1));
	initializeFindTextFromEditor_ = toBoolean(readIntegerProfile(L"Find", L"initializeFromEditor", 1));
}

/**
 * �L�[�� (��) ���蓖��
 * @param schemeName �g�p����L�[�{�[�h�}�b�v�X�L�[���̖��O�B
 * �󕶎�����w�肷��ƃA�v���P�[�V�����Ɍ��ѕt�����Ă���L�[���蓖�ăI�u�W�F�N�g����A�N�Z�����[�^�e�[�u�����č\�z����
 */
void Alpha::loadKeyBinds(const wstring& schemeName) {
	if(!schemeName.empty()) {
		wchar_t pathName[MAX_PATH];
		::GetModuleFileNameW(0, pathName, MAX_PATH);
		wcscpy(::PathFindFileNameW(pathName), IDS_KEYBOARDSCHEME_DIRECTORY_NAME);
		if(wcslen(pathName) + schemeName.length() + 4 >= MAX_PATH)
			return;
		wcscat(pathName, schemeName.c_str());
		wcscat(pathName, L".akm");
		keyboardMap_.load(pathName);
	}

	// ���j���[�̍č\�z
	setupMenus();
}

/**
 * ���b�Z�[�W�e�[�u���̕���������b�Z�[�W�{�b�N�X�ɕ\������
 * @param id ���b�Z�[�W���ʎq
 * @param type �_�C�A���O�̃^�C�v (::MessageBox �Ɠ���)
 * @param args ���b�Z�[�W�̈���
 * @return ���[�U�̕ԓ� (::MessageBox �Ɠ���)
 */
int Alpha::messageBox(DWORD id, UINT type, MessageArguments& args /* = MessageArguments() */) {
	return getMainWindow().messageBox(loadString(id, args).c_str(), IDS_APPNAME, type);
}

/**
 * �R�}���h���C������͂��Ď��s����B�����Ȉ����͖�������
 * @param currentDirectory �J�����g�f�B���N�g��
 * @param commandLine �R�}���h���C��
 * @see Alpha#onCopyData, WinMain
 */
void Alpha::parseCommandLine(const WCHAR* currentDirectory, const WCHAR* commandLine) {
	// CommandLineToArgvW �� argv[0] �ɑ΂����d���p���̈����ɖ�肪����悤��...
	int argc;
	WCHAR** argv = ::CommandLineToArgvW(commandLine, &argc);
	WCHAR canonical[MAX_PATH];
	for(int i = 1; i < argc; ++i) {
		if(toBoolean(::PathIsRelativeW(argv[i])))
			::PathCombineW(canonical, currentDirectory, argv[i]);
		else
			wcscpy(canonical, argv[i]);
		if(toBoolean(::PathIsDirectoryW(canonical)))
			buffers_->openDialog(canonical);
		else
			buffers_->open(canonical);
	}
	::LocalFree(argv);
}

/// @see manah#windows#Alpha#preTranslateMessage
bool Alpha::preTranslateMessage(const MSG& msg) {
	// �R�}���h�Ɋ��蓖�Ă��Ă���L�[�g�ݍ��킹�������ŕߑ�����
	if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {	// WM_CHAR �����s����Ȃ��悤�ɂ���
		if(msg.hwnd == buffers_->getActiveView().getHandle()) {
			KeyModifier modifiers = 0;
			if(toBoolean(::GetKeyState(VK_CONTROL) & 0x8000))
				modifiers |= KM_CTRL;
			if(toBoolean(::GetKeyState(VK_SHIFT) & 0x8000))
				modifiers |= KM_SHIFT;
			if(msg.message == WM_SYSKEYDOWN || toBoolean(::GetKeyState(VK_MENU) & 0x8000))
				modifiers |= KM_ALT;
			return handleKeyDown(static_cast<VirtualKey>(msg.wParam), modifiers);
		}
	} else if(msg.message == WM_SYSCHAR) {
		if(msg.hwnd == buffers_->getActiveView().getHandle()) {
			// �L�[�g�ݍ��킹���L�[�{�[�h�X�L�[���ɓo�^����Ă��邩���ׂ�B
			// �o�^����Ă���Ί���̏��� (���j���[�̃A�N�e�B�x�[�V����) ��W�Q
			const VirtualKey key = LOBYTE(::VkKeyScanExW(static_cast<WCHAR>(msg.wParam), ::GetKeyboardLayout(0)));
			KeyModifier modifiers = KM_ALT;
			if(toBoolean(::GetKeyState(VK_CONTROL) & 0x8000))	modifiers |= KM_CTRL;
			if(toBoolean(::GetKeyState(VK_SHIFT) & 0x8000))		modifiers |= KM_SHIFT;

			if(twoStroke1stKey_ == VK_NULL)
				return keyboardMap_.getCommand(KeyCombination(key, modifiers)) != 0;
			else
				return keyboardMap_.getCommand(
					KeyCombination(twoStroke1stKey_, twoStroke1stModifiers_), KeyCombination(key, modifiers)) != 0;
		}
	}
	return false;
}

/**
 * INI ���當���񃊃X�g��ǂݍ���
 * @param section �Z�N�V������
 * @param key �L�[��
 * @param[out] items ���X�g
 * @param[in] defaultValue �ݒ肪������Ȃ��Ƃ��Ɏg�p���镶����
 */
void Alpha::readProfileList(const wchar_t* section,
		const wchar_t* key, std::list<std::wstring>& items, const wchar_t* defaultValue /* = 0 */) {
	const std::wstring s = readStringProfile(section, key, defaultValue);

	items.clear();
	if(s.empty())
		return;
	std::wistringstream ss(s);
	std::wstring buffer;
	while(ss >> buffer)
		items.push_back(buffer);
}


/**
 * INI ���當����̏W����ǂݍ���
 * @param section �Z�N�V������
 * @param key �L�[��
 * @param[out] items �W��
 * @param[in] defaultValue �ݒ肪������Ȃ��Ƃ��Ɏg�p���镶����
 */
void Alpha::readProfileSet(const wchar_t* section,
		const wchar_t* key, std::set<std::wstring>& items, const wchar_t* defaultValue /* = 0 */) {
	const std::wstring s = readStringProfile(section, key, defaultValue);

	items.clear();
	if(s.empty())
		return;
	std::wistringstream	ss(s);
	std::wstring buffer;
	while(ss >> buffer)
		items.insert(buffer);
}

/// �X�N���v�g�G���W���ƃt�@�C���p�^�[���̊֘A�t��������
void Alpha::registerScriptEngineAssociations() {
	// �R���|�[�l���g�J�e�S������X�N���v�g�G���W����񋓂��AINI ����g���q���E��
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
			if(wchar_t* firstPeriod = wcschr(progID, L'.'))	// ProgID ���̃o�[�W�����ԍ����̂Ă�
				*firstPeriod = 0;
			const wstring pattern = readStringProfile(INI_SECTION_SCRIPTENGINES, progID);
			if(!pattern.empty())
				scriptSystem_->addEngineScriptNameAssociation(pattern.c_str(), clsid);
			::CoTaskMemFree(progID);
		}
	}
}

/**
 * @brief ���݈ʒu�ȍ~�̌����������S�Ēu������
 *
 * ���̃��\�b�h�͒u���_�C�A���O����\���ł��@�\����
 */
void Alpha::replaceAll() {
	FindAllCommand command(buffers_->getActiveView(), FindAllCommand::REPLACE,
						toBoolean(searchDialog_->isButtonChecked(IDC_RADIO_SELECTION)));
	ulong replacedCount = -1;
	searchDialog_->setOptions();
	try {
		replacedCount = command.execute();
	} catch(boost::regex_error& e) {
		if(showMessageBoxOnFind_)
			showRegexSearchError(e);
	} catch(runtime_error&) {
		if(showMessageBoxOnFind_)
			messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	}
	if(replacedCount == 0) {
		if(showMessageBoxOnFind_)
			messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
	} else if(replacedCount != -1) {
		if(showMessageBoxOnFind_)
			messageBox(MSG_SEARCH__REPLACE_DONE, MB_ICONINFORMATION, MARGS % replacedCount);
	}
	if(searchDialog_->isWindow()) {
		if(searchDialog_->isButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// [���ׂĒu����_�C�A���O�����]
			getMainWindow().sendMessage(WM_COMMAND, CMD_SEARCH_FIND);
		else
			::SetFocus(searchDialog_->getItem(IDC_COMBO_FINDWHAT));
	}
}

/**
 * @brief ���݂̑I��͈͂�����������ł���Βu�����s��
 *
 * �u����A�����͑I��͈͂�����������Ŗ����ꍇ�͎��̌�����������������I����Ԃɂ���B
 * ���̃��\�b�h�͒u���_�C�A���O����\���ł��@�\����
 */
void Alpha::replaceAndSearchNext() {
	FindNextCommand command(buffers_->getActiveView(), true,
		searchDialog_->isButtonChecked(IDC_CHK_SHIFT) ? BACKWARD : FORWARD);
	searchDialog_->setOptions();

	try {
		command.execute();
	} catch(boost::regex_error& e) {
		if(showMessageBoxOnFind_)
			showRegexSearchError(e);
	} catch(runtime_error&) {
		if(showMessageBoxOnFind_)
			messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	}
	if(searchDialog_->isVisible()) {
//		if(searchDialog_.isButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// [������_�C�A���O�����]
//			getMainWindow().sendMessage(WM_COMMAND, CMD_SEARCH_FIND);
//		else
			::SetFocus(searchDialog_->getItem(IDC_COMBO_FINDWHAT));
	}
}

/// INI �t�@�C���ɐݒ��ۑ�����
void Alpha::saveINISettings() {
	wchar_t keyName[30];

	// �o�[�̉����̕ۑ�
	AutoZero<::REBARBANDINFOW> rbbi;
	rbbi.fMask = RBBIM_STYLE;
	rebar_.getBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
	writeIntegerProfile(L"View", L"visibleToolbar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
	rebar_.getBandInfo(rebar_.idToIndex(IDC_BUFFERBAR), rbbi);
	writeIntegerProfile(L"View", L"visibleBufferBar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
	writeIntegerProfile(L"View", L"visibleStatusBar", statusBar_.isVisible() ? 1 : 0);

	// MRU ���X�g�̕ۑ�
	mruManager_->save();

	// ���������񗚗��̕ۑ�
	const searcher::TextSearcher& s = buffers_->getEditorSession().getTextSearcher();
	for(size_t i = 0; i < s.getNumberOfStoredPatterns(); ++i) {
		swprintf(keyName, L"findWhat(%u)", i);
		writeStringProfile(L"Find", keyName, s.getPattern(i).c_str());
	}
	swprintf(keyName, L"findWhat(%u)", s.getNumberOfStoredPatterns());
	writeStringProfile(L"Find", keyName, L"");
	for(size_t i = 0; i < s.getNumberOfStoredReplacements(); ++i) {
		swprintf(keyName, L"replaceWith(%u)", i);
		writeStringProfile(L"Find", keyName, s.getReplacement(i).c_str());
	}
	swprintf(keyName, L"replaceWith(%u)", s.getNumberOfStoredReplacements());
	writeStringProfile(L"Find", keyName, L"");
}

/// [���ׂă}�[�N]
void Alpha::searchAndBookmarkAll() {
	FindAllCommand command(buffers_->getActiveView(), FindAllCommand::BOOKMARK,
						toBoolean(searchDialog_->isButtonChecked(IDC_RADIO_SELECTION)));
	searchDialog_->setOptions();
	try {
		command.execute();
	} catch(boost::regex_error& e) {
		if(showMessageBoxOnFind_)
			showRegexSearchError(e);
	} catch(runtime_error&) {
		if(showMessageBoxOnFind_)
			messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	}
}

/**
 * @brief ���݂̌��������ɏ]���Ď��A�����͑O�̕����������
 *
 * ���̃��\�b�h�̓q�b�g���������������I����Ԃɂ���B
 * �܂��A�����_�C�A���O����\���ł��@�\����
 * @param forward �O������
 * @param messageOnFailure ���������s�����Ƃ��Ƀv�����v�g��\������
 * @return ���������s�����Ƃ� false
 */
bool Alpha::searchNext(bool forward, bool messageOnFailure) {
	FindNextCommand command(buffers_->getActiveView(), false, forward ? FORWARD : BACKWARD);
	searchDialog_->setOptions();
	try {
		if(command.execute() == 0)
			return true;
		else if(messageOnFailure)
			messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
	} catch(boost::regex_error& e) {
		if(messageOnFailure)
			showRegexSearchError(e);
	} catch(runtime_error&) {
		if(messageOnFailure)
			messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	}
	if(searchDialog_->isVisible()) {
		if(searchDialog_->isButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// [������_�C�A���O�����]
			getMainWindow().sendMessage(WM_COMMAND, CMD_SEARCH_FIND);
		else
			::SetFocus(searchDialog_->getItem(IDC_COMBO_FINDWHAT));
	}
	return false;
}

/// �S�ẴG�f�B�^�ƈꕔ�̃R���g���[���ɐV�����t�H���g��ݒ�
void Alpha::setFont(const ::LOGFONTW& font) {
	::LOGFONTW lf = font;

	lf.lfWeight = FW_NORMAL;
	::DeleteObject(editorFont_);
	editorFont_ = ::CreateFontIndirectW(&lf);

	// �S�Ẵr���[�̃t�H���g���X�V
	for(size_t i = 0; i < buffers_->getCount(); ++i) {
		presentation::Presentation& p = buffers_->getAt(i).getPresentation();
		for(presentation::Presentation::TextViewerIterator it = p.getFirstTextViewer(); it != p.getLastTextViewer(); ++it)
			(*it)->getTextRenderer().setFont(font.lfFaceName, font.lfHeight, 0);
	}

	// �ꕔ�̃R���g���[���ɂ��ݒ�
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
		if(bookmarkDialog_.get() != 0 && bookmarkDialog_->isWindow())
			bookmarkDialog_->sendItemMessage(IDC_LIST_BOOKMARKS, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
		if(searchDialog_.get() != 0 && searchDialog_->isWindow()) {
			searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
			searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
		}
	}

	// INI �t�@�C���ɕۑ�
	writeStructureProfile(L"View", L"Font.default", lf);

	// ���� <-> �ϕ��ŕ\�L��ς���K�v������
	updateStatusBarPaneSize();
}

/**
 * �X�e�[�^�X�o�[�̐擪�̃y�C���̃e�L�X�g��ݒ肷��
 * @param text �ݒ肷��e�L�X�g�B@c null ���Ɗ���̃e�L�X�g
 * @param font �t�H���g�B@c null ���Ɗ���
 */
void Alpha::setStatusText(const wchar_t* text, HFONT font /* = 0 */) {
	statusBar_.setText(0, (text != 0) ? text : IDS_DEFAULTSTATUSTEXT, SBT_NOBORDERS);
	statusBar_.setFont(font);
}

/// ���j���[�̏�����
void Alpha::setupMenus() {
	Menu menuBar(getMainWindow().getMenu());
	while(true) {
		const UINT c = menuBar.getNumberOfItems();
		if(c == 0 || c == -1)
			break;
		menuBar.remove<Menu::BY_POSITION>(0);
	}

	const Menu::SeparatorItem sep;

#define ITEM(id) Menu::StringItem(id, commandManager_->getMenuName(id).c_str())
#define RADIO_ITEM(id) Menu::StringItem(id, commandManager_->getMenuName(id).c_str(), MFS_ENABLED, true)

	// ���j���[�o�[
	menuBar << Menu::StringItem(CMD_FILE_TOP, loadString(CMD_FILE_TOP).c_str())
		<< Menu::StringItem(CMD_EDIT_TOP, loadString(CMD_EDIT_TOP).c_str())
		<< Menu::StringItem(CMD_SEARCH_TOP, loadString(CMD_SEARCH_TOP).c_str())
		<< Menu::StringItem(CMD_VIEW_TOP, loadString(CMD_VIEW_TOP).c_str())
		<< Menu::StringItem(CMD_MACRO_TOP, loadString(CMD_MACRO_TOP).c_str())
		<< Menu::StringItem(CMD_TOOL_TOP, loadString(CMD_TOOL_TOP).c_str())
		<< Menu::StringItem(CMD_WINDOW_TOP, loadString(CMD_WINDOW_TOP).c_str())
		<< Menu::StringItem(CMD_HELP_TOP, loadString(CMD_HELP_TOP).c_str());

	// [�t�@�C��]
	auto_ptr<Menu> popup(new PopupMenu);
	*popup << ITEM(CMD_FILE_NEW) << ITEM(CMD_FILE_NEWWITHFORMAT) << sep << ITEM(CMD_FILE_OPEN)
		<< ITEM(CMD_FILE_REOPEN) << ITEM(CMD_FILE_REOPENWITHCODEPAGE) << ITEM(CMD_FILE_MRU) << sep
		<< ITEM(CMD_FILE_CLOSE) << ITEM(CMD_FILE_CLOSEALL) << ITEM(CMD_FILE_CLOSEOTHERS) << sep
		<< ITEM(CMD_FILE_SAVE) << ITEM(CMD_FILE_SAVEAS) << ITEM(CMD_FILE_SAVEALL) << sep
		<< ITEM(CMD_FILE_PRINT) << /*ITEM(CMD_FILE_PRINTPREVIEW) <<*/ ITEM(CMD_FILE_PRINTSETUP) << sep
		<< ITEM(CMD_FILE_SENDMAIL) << sep << ITEM(CMD_FILE_EXIT);
	popup->setChildPopup<Menu::BY_COMMAND>(CMD_FILE_MRU, mruManager_->getPopupMenu());
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_FILE_TOP, popup);

	// [�ҏW]
	popup.reset(new PopupMenu);
	*popup << ITEM(CMD_EDIT_UNDO) << ITEM(CMD_EDIT_REDO) << sep << ITEM(CMD_EDIT_CUT)
		<< ITEM(CMD_EDIT_COPY) << ITEM(CMD_EDIT_PASTE) << ITEM(CMD_EDIT_PASTEFROMCLIPBOARDRING)
		<< ITEM(CMD_EDIT_DELETE) << ITEM(CMD_EDIT_SELECTALL) << sep << ITEM(CMD_EDIT_ADVANCED)
		<< ITEM(CMD_EDIT_OPENCANDIDATEWINDOW);
	auto_ptr<Menu> advMenu(new PopupMenu);	// [�ҏW]-[���x�ȑ���]
	*advMenu << ITEM(CMD_EDIT_CHARTOCODEPOINT) << ITEM(CMD_EDIT_CODEPOINTTOCHAR) << sep
		<< ITEM(CMD_EDIT_NARROWTOSELECTION) << ITEM(CMD_EDIT_WIDEN);
	popup->setChildPopup<Menu::BY_COMMAND>(CMD_EDIT_ADVANCED, advMenu);
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_EDIT_TOP, popup);

	// [����]
	popup.reset(new PopupMenu);
	*popup << ITEM(CMD_SEARCH_FIND) << ITEM(CMD_SEARCH_FINDNEXT) << ITEM(CMD_SEARCH_FINDPREV)
		<< ITEM(CMD_SEARCH_INCREMENTALSEARCH) << ITEM(CMD_SEARCH_REVOKEMARK) << sep
		<< ITEM(CMD_SEARCH_GOTOLINE) << ITEM(CMD_SEARCH_BOOKMARKS) << sep << ITEM(CMD_SEARCH_GOTOMATCHBRACKET)
		<< ITEM(CMD_SEARCH_EXTENDTOMATCHBRACKET) << sep << ITEM(CMD_SEARCH_FINDFILES)
		<< ITEM(CMD_SEARCH_SEARCHMULTIPLEFILES) << ITEM(CMD_SEARCH_REPLACEMULTIPLEFILES);
	// [����]-[�u�b�N�}�[�N]
	auto_ptr<PopupMenu> bookmarksMenu(new PopupMenu);
	*bookmarksMenu << ITEM(CMD_SEARCH_TOGGLEBOOKMARK) << ITEM(CMD_SEARCH_NEXTBOOKMARK)
		<< ITEM(CMD_SEARCH_PREVBOOKMARK) << ITEM(CMD_SEARCH_CLEARBOOKMARKS) << ITEM(CMD_SEARCH_MANAGEBOOKMARKS);
	popup->setChildPopup<Menu::BY_COMMAND>(CMD_SEARCH_BOOKMARKS, bookmarksMenu);
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_SEARCH_TOP, popup);


	// [�\��]
	popup.reset(new PopupMenu);
	*popup << ITEM(CMD_VIEW_TOOLBAR) << ITEM(CMD_VIEW_STATUSBAR) << ITEM(CMD_VIEW_BUFFERBAR)
		<< sep << ITEM(CMD_VIEW_BUFFERS) << ITEM(CMD_VIEW_NEXTBUFFER) << ITEM(CMD_VIEW_PREVBUFFER)
		<< sep << RADIO_ITEM(CMD_VIEW_WRAPNO) << RADIO_ITEM(CMD_VIEW_WRAPBYSPECIFIEDWIDTH)
		<< RADIO_ITEM(CMD_VIEW_WRAPBYWINDOWWIDTH) << sep << ITEM(CMD_VIEW_REFRESH);
	popup->setChildPopup<Menu::BY_COMMAND>(CMD_VIEW_BUFFERS, buffers_->getListMenu());
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_VIEW_TOP, popup);

	// [�}�N��]
	popup.reset(new PopupMenu);
	*popup << ITEM(CMD_MACRO_EXECUTE) << ITEM(CMD_MACRO_DEFINE) << ITEM(CMD_MACRO_APPEND)
		<< ITEM(CMD_MACRO_PAUSERESTART) << ITEM(CMD_MACRO_INSERTQUERY) << ITEM(CMD_MACRO_ABORT)
		<< ITEM(CMD_MACRO_SAVEAS) << ITEM(CMD_MACRO_LOAD) << sep << ITEM(CMD_MACRO_SCRIPTS);
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_MACRO_TOP, popup);

	// [�}�N��]-[�X�N���v�g] (�b��)
/*	popup.reset(new PopupMenu);
	*scriptMenu << ITEM(CMD_EDIT_RELOADPLUGIN) << sep;
	const size_t c = scriptMacroManager_->getCount();
	if(c != 0) {
		*scriptControls << ITEM(CMD_MACRO_INTERRUPT) << sep;
		for(size_t i = 0; i < c; ++i)
			*scriptMenu << ITEM(CMD_EDIT_PLUGINLIST_START + static_cast<UINT>(i));
	} else
		*scriptMenu << ITEM(CMD_EDIT_PLUGINLIST_START, MFS_GRAYED | MFS_DISABLED);
	macroMenu->setChildPopup<Menu::BY_COMMAND>(CMD_MACRO_SCRIPTS, *scriptMenu, true);
*/
	// [�c�[��]
	popup.reset(new PopupMenu);
	*popup << ITEM(CMD_TOOL_EXECUTE) << ITEM(CMD_TOOL_EXECUTECOMMAND) << sep
		<< ITEM(CMD_TOOL_APPDOCTYPES) << ITEM(CMD_TOOL_DOCTYPEOPTION)
		<< ITEM(CMD_TOOL_COMMONOPTION) << ITEM(CMD_TOOL_FONT);
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_TOOL_TOP, popup);
//	delete appDocTypeMenu_;
//	appDocTypeMenu_ = new Menu();	// [�K�p�����^�C�v]
//	popup->setChildPopup<Menu::BY_COMMAND>(CMD_TOOL_APPDOCTYPES, *appDocTypeMenu_, false);

	// [�E�B���h�E]
	popup.reset(new PopupMenu);
	*popup << ITEM(CMD_WINDOW_SPLITNS) << ITEM(CMD_WINDOW_SPLITWE) << ITEM(CMD_WINDOW_UNSPLITOTHERS)
		<< ITEM(CMD_WINDOW_UNSPLITACTIVE) << ITEM(CMD_WINDOW_NEXTPANE) << ITEM(CMD_WINDOW_PREVPANE)
		<< sep << ITEM(CMD_WINDOW_TOPMOSTALWAYS);
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_WINDOW_TOP, popup);

	// [�w���v]
	popup.reset(new PopupMenu);
	*popup << ITEM(CMD_HELP_ABOUT);
	menuBar.setChildPopup<Menu::BY_COMMAND>(CMD_HELP_TOP, popup);

#undef ITEM
#undef RADIO_ITEM

	getMainWindow().drawMenuBar();
}

/// �c�[���o�[�̏����� (1 �񂵂��Ăяo���Ă͂Ȃ�Ȃ�)
void Alpha::setupToolbar() {
	// �W���c�[���o�[
	CommandID* commands = 0;	// �c�[���o�[�ɏ悹��{�^���ɑΉ�����R�}���h
	size_t buttonCount;			// �{�^����
	list<wstring> buttonIDs;

	// �ݒ��ǂݍ���
	readProfileList(L"ToolbarButtons", L"standard", buttonIDs, L"");

	if(!buttonIDs.empty()) {	// �G���g�����������ꍇ
		list<wstring>::const_iterator it;
		size_t i = 0;

		buttonCount = buttonIDs.size();
		commands = new CommandID[buttonCount];
		for(i = 0, it = buttonIDs.begin(); it != buttonIDs.end(); ++i, ++it)
			commands[i] = static_cast<CommandID>(wcstoul(it->c_str(), 0, 10));
	} else {	// �f�t�H���g�̐ݒ���g��
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

	// �C���[�W���X�g���쐬����
	WCHAR iconDir[MAX_PATH];
	wcscpy(iconDir, getModuleFileName());
	::PathFindFileNameW(iconDir)[0] = 0;
	wcscat(iconDir, IDS_ICON_DIRECTORY_NAME);
	commandManager_->createImageList(iconDir);

	// �{�^�������
	::TBBUTTON* buttons = new ::TBBUTTON[buttonCount];
	bool hasDropArrow;
	for(size_t i = 0, j = 0; i < buttonCount; ++i, ++j) {
		hasDropArrow = commands[j] == CMD_FILE_NEW || commands[i] == CMD_FILE_OPEN;
		ZeroMemory(buttons + i, sizeof(::TBBUTTON));
		buttons[i].fsState = TBSTATE_ENABLED;
		if(commands[j] == 0)
			buttons[i].fsStyle = BTNS_SEP;
		else {
			size_t icon = commandManager_->getIconIndex(commands[j]);
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
	toolbar_.setImageList(commandManager_->getImageList(CommandManager::ICONSTATE_NORMAL).getHandle());
	toolbar_.setDisabledImageList(commandManager_->getImageList(CommandManager::ICONSTATE_DISABLED).getHandle());
	toolbar_.setHotImageList(commandManager_->getImageList(CommandManager::ICONSTATE_HOT).getHandle());
//	toolbar_.setPadding(6, 6);

	for(size_t i = 0; i < buttonCount; ++i) {
		delete[] reinterpret_cast<wchar_t*>(buttons[i].iString);
//		if(buttons[i].fsStyle != BTNS_SEP
//				&& buttons[i].iBitmap != I_IMAGENONE) {	// �A�C�R���t���{�^���̕��������ŌŒ肷��
//			AutoZeroCB<TBBUTTONINFOW> tbi;
//			tbi.dwMask = TBIF_SIZE;
//			tbi.cx = (buttons[i].idCommand != CMD_FILE_NEW && buttons[i].idCommand != CMD_FILE_OPEN) ? 22 : 38;
//			toolbar_.setButtonInfo(buttons[i].idCommand, tbi);
//		}
	}
	delete[] commands;
	delete[] buttons;

	// ���o�[�ɏ悹��
	AutoZeroCB<::REBARBANDINFOW> rbbi;
	const wstring caption = loadString(MSG_DIALOG__BUFFERBAR_CAPTION);
	rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;
	rbbi.fStyle = RBBS_GRIPPERALWAYS | RBBS_USECHEVRON;
	rbbi.wID = IDC_TOOLBAR;
	rbbi.hwndChild = toolbar_.getHandle();
	rbbi.cxMinChild = 0;
	rbbi.cyMinChild = 22;
	rebar_.insertBand(0, rbbi);

	// �c�[���o�[�̃V�F�u����������镝�̐ݒ�
	::RECT rect;
	toolbar_.getItemRect(toolbar_.getButtonCount() - 1, rect);
	rbbi.fMask = RBBIM_IDEALSIZE;
	rbbi.cxIdeal = rect.right;
	rebar_.setBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
}

/**
 * ���K�\�������G���[�_�C�A���O��\������
 * @param e �G���[���e
 */
void Alpha::showRegexSearchError(const boost::regex_error& e) {
	messageBox(MSG_SEARCH__INVALID_REGEX_PATTERN, MB_ICONEXCLAMATION,
		MARGS % loadString(MSG_SEARCH__BAD_REGEX_PATTERN_START + e.code()) % static_cast<long>(e.position()));
}

/// [�����ƒu��] �_�C�A���O��\�����ăp�^�[���ҏW�e�L�X�g�{�b�N�X�Ƀt�H�[�J�X��^����
void Alpha::showSearchDialog() {
	if(!searchDialog_->isVisible()) {
		if(initializeFindTextFromEditor_) {	// �A�N�e�B�u�ȃG�f�B�^���猟���p�^�[�������o��
			Caret& caret = buffers_->getActiveView().getCaret();
			if(caret.isSelectionEmpty()) {
				String s;
				// TODO: obtain the word nearest from the caret position.
//				caret.getNearestWordFromCaret(0, 0, &s);
				searchDialog_->setItemText(IDC_COMBO_FINDWHAT, s.c_str());
			} else if(caret.getAnchor().getLineNumber() != caret.getLineNumber())
				searchDialog_->setItemText(IDC_COMBO_FINDWHAT, L"");
			else
				searchDialog_->setItemText(IDC_COMBO_FINDWHAT, caret.getSelectionText().c_str());
		}
		searchDialog_->show(SW_SHOW);
	} else
		searchDialog_->setActive();
	::SetFocus(searchDialog_->getItem(IDC_COMBO_FINDWHAT));
}

void Alpha::updateStatusBarPaneSize() {
	if(!statusBar_.isWindow())
		return;
	static const ICON_WIDTH = 16;
	int parts[5], borders[3];
	statusBar_.getBorders(borders);
	const int padding = (borders[0] + borders[2]) * 2 + 5;
	::RECT rc;
	statusBar_.getRect(rc);

	ClientDC dc = statusBar_.getDC();
	HFONT oldFont = static_cast<HFONT>(dc.selectObject(statusFont_));

	parts[4] = rc.right - rc.left;
	if(!getMainWindow().isZoomed())
		parts[4] -= 20;
	// �i���[�C���O
	parts[3] = parts[4] - ICON_WIDTH - padding;
	// �㏑��/�}�����[�h
	const wstring overtypeMode = loadString(MSG_STATUS__OVERTYPE_MODE);
	const wstring insertMode = loadString(MSG_STATUS__INSERT_MODE);
	parts[2] = parts[3] - max(
		dc.getTextExtent(overtypeMode.data(), static_cast<int>(overtypeMode.length())).cx,
		dc.getTextExtent(insertMode.data(), static_cast<int>(insertMode.length())).cx) - padding;
	// �L�[�{�[�h�}�N��
	parts[1] = parts[2] - ICON_WIDTH - padding;
	// �L�����b�g�ʒu
	wchar_t format[256], text[256];
	loadString(MSG_STATUS__CARET_POSITION, format, countof(format));
	swprintf(text, format, 88888888, 88888888, 88888888);
	parts[0] = parts[1] - dc.getTextExtent(text, static_cast<int>(wcslen(text))).cx - padding;

	dc.selectObject(oldFont);
	statusBar_.setParts(countof(parts), parts);
}

/// @see WM_CLOSE
bool Alpha::onClose() {
	// TODO: invoke application teardown.
//	eventHandlerScript_->invoke(OLESTR("OnAlphaTerminating"), params);
	if(buffers_->closeAll(true)) {
		saveINISettings();
		getMainWindow().destroy();
		return true;
	}
	return false;	// ���[�U���L�����Z������
}

/// @see Window::onCommand
bool Alpha::onCommand(WORD id, WORD notifyCode, HWND control) {
//	if(::GetCurrentThreadId() != getMainWindow().getWindowThreadID())
//		getMainWindow().sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
	if(id == CMD_SPECIAL_ILLEGAL2STROKE) {
		::MessageBeep(MB_OK);
		return true;
	} else if(id >= CMD_VIEW_BUFFERLIST_START && id < CMD_VIEW_BUFFERLIST_END) {
		if(commandManager_->isEnabled(id, true))
			buffers_->setActive(buffers_->getAt(id - CMD_VIEW_BUFFERLIST_START));
	} else
		commandManager_->executeCommand(static_cast<CommandID>(id), true);
	return true;
}

/// @see WM_COPYDATA
void Alpha::onCopyData(HWND window, const COPYDATASTRUCT& cds) {
	const WCHAR* const data = static_cast<WCHAR*>(cds.lpData);
	parseCommandLine(data, data + MAX_PATH);
}

/// @see Window::onDestroy
void Alpha::onDestroy() {
	// ��n���Ȃ� (Alpha::onClose ���Q��)
	getMainWindow().killTimer(ID_TIMER_QUERYCOMMAND);

	delete mruManager_;
//	toolbar_.destroyWindow();
//	statusBar_.destroyWindow();

	::PostQuitMessage(0);	// STA �X���b�h�I��
}

/// @see WM_DRAWITEM
void Alpha::onDrawItem(UINT, const ::DRAWITEMSTRUCT& drawItem) {
	if(drawItem.CtlType != ODT_MENU)	// �����_�ł̓��j���[�̕`��̂�
		return;
	if(drawItem.itemID != 0) {
		const wstring text = commandManager_->getMenuName(drawItem.itemID);
		manah::AutoBuffer<wchar_t> caption(new wchar_t[text.length() + 1]);
		wcscpy(caption.get(), text.c_str());
		wchar_t* accel = wcschr(caption.get(), L'\t');
		if(accel == caption.get())
			accel = 0;
		else if(accel != 0)
			*(accel++) = 0;
		Menu::drawItem(drawItem, caption.get(), accel, 0, 0, getBufferList().getBufferIcon(drawItem.itemID - CMD_VIEW_BUFFERLIST_START));
	} else
		Menu::drawItem(drawItem, 0);
}

/// @see WM_DROPFILES
void Alpha::onDropFiles(HDROP drop) {
	const uint c = ::DragQueryFileW(drop, 0xFFFFFFFF, 0, 0);
	WCHAR filePath[MAX_PATH];

	for(uint i = 0; i < c; ++i) {
		::DragQueryFileW(drop, i, filePath, MAX_PATH);
		if(!toBoolean(::PathIsDirectoryW(filePath)))
			buffers_->open(filePath);
		else
			buffers_->openDialog(filePath);
	}
	::DragFinish(drop);

	EditorView& activeView = buffers_->getActiveView();
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

/// @see WM_INITMENUPOPUP
void Alpha::onInitMenuPopup(HMENU menu, UINT, bool sysMenu) {
	if(sysMenu)
		return;
	// TODO: handle message about mode list menu
/*	else if(appDocTypeMenu_->getSafeHmenu() == menu) {	// �K�p�����^�C�v
		const AlphaDoc& activeBuffer = buffers_->getActive();
		const DocumentTypeManager& types = buffers_->getDocumentTypeManager();
		const bool ctrlPressed = toBoolean(::GetKeyState(VK_CONTROL) & 0x8000);

		while(appDocTypeMenu_->getItemCount() > 0)	// ���ׂč폜
			appDocTypeMenu_->deleteMenuItem<Controls::Menu::BY_POSITION>(0);
		for(size_t i = 0; i < types.getCount(); ++i) {
			const DocumentType& type = types.getAt(i);
			if(type.hidden && !ctrlPressed)
				continue;
			appDocTypeMenu_->appendMenuItem(CMD_TOOL_DOCTYPELIST_START + static_cast<UINT>(i), type.name.c_str(), MFT_OWNERDRAW);
		}
		appDocTypeMenu_->checkMenuItem<Controls::Menu::BY_COMMAND>(
			CMD_TOOL_DOCTYPELIST_START + static_cast<UINT>(types.find(activeBuffer.getDocumentType())));
	}
*/	else {
		Menu popup;
		popup.attach(menu);
		const int c = popup.getNumberOfItems();
		for(int i = 0; i < c; ++i) {
			const CommandID id = popup.getID(i);
			popup.enable<Menu::BY_POSITION>(i, commandManager_->isEnabled(id, true));
			popup.check<Menu::BY_POSITION>(i, commandManager_->isChecked(id));
		}
	}
}

/// @see WM_MEASUREITEM
void Alpha::onMeasureItem(UINT id, ::MEASUREITEMSTRUCT& mi) {
	if(mi.CtlType == ODT_MENU) {
		if(mi.itemID == 0)
			Menu::measureItem(mi, 0);
		else {
			const wstring s = commandManager_->getMenuName(mi.itemID);
			manah::AutoBuffer<wchar_t> caption(new wchar_t[s.length() + 1]);
			wcscpy(caption.get(), s.c_str());
			wchar_t* accel = wcschr(caption.get(), _T('\t'));
			if(accel == caption.get())
				accel = 0;
			else if(accel != 0)
				*(accel++) = 0;
			Menu::measureItem(mi, caption.get(), accel);
		}
	}
}

/// @see WM_MENUCHAR
LRESULT Alpha::onMenuChar(wchar_t ch, UINT flags, Menu& menu) {
	return menu.handleMenuChar(ch, flags);
}

/// @see WM_MENUSELECT
void Alpha::onMenuSelect(UINT itemID, UINT flags, HMENU) {
	// �I�����ڂɑΉ�����������X�e�[�^�X�o�[�ɕ\��
	if(itemID >= CMD_EDIT_PLUGINLIST_START && itemID < CMD_EDIT_PLUGINLIST_END) {	// �}�N��
/*		statusBar_.setText(statusBar_.isSimple() ? SB_SIMPLEID : 0,
			(scriptMacroManager_->getCount() != 0) ?
			scriptMacroManager_->getDescription(itemID - CMD_EDIT_PLUGINLIST_START).c_str() : L"", SBT_NOBORDERS);
*/	} else if(itemID >= CMD_VIEW_BUFFERLIST_START && itemID < CMD_VIEW_BUFFERLIST_END)	// �o�b�t�@
		statusBar_.setText(statusBar_.isSimple() ? SB_SIMPLEID : 0,
			buffers_->getAt(itemID - CMD_VIEW_BUFFERLIST_START).getFilePathName(), SBT_NOBORDERS);
	else {
		const wstring prompt = (!toBoolean(flags & MF_POPUP) && !toBoolean(flags & MFT_SEPARATOR)) ? loadString(itemID) : L"";
		statusBar_.setText(statusBar_.isSimple() ? SB_SIMPLEID : 0,
			(!prompt.empty()) ? wcschr(prompt.data(), L'\n') + 1 : L"", SBT_NOBORDERS);
	}
}

/// @see Window#onNotify
bool Alpha::onNotify(int id, NMHDR& nmhdr) {
	if(id == IDC_BUFFERBAR)
		return toBoolean(buffers_->handleBufferBarNotification(*reinterpret_cast<NMTOOLBARW*>(&nmhdr)));
	else if(id == IDC_BUFFERBARPAGER)
		return toBoolean(buffers_->handleBufferBarPagerNotification(nmhdr));

	switch(nmhdr.code) {
	case RBN_HEIGHTCHANGE:	// ���o�[�̍������ς����
		onSize(0, -1, -1);
		return true;
	case RBN_CHEVRONPUSHED:	// �c�[���o�[�̃V�F�u�����������ꂽ
		onRebarChevronPushed(*reinterpret_cast<LPNMREBARCHEVRON>(&nmhdr));
		return true;
	case TBN_DROPDOWN: {	// �c�[���o�[�̃h���b�v�_�E��
		RECT rect;
		POINT pt;
		const bool ctrlPressed = toBoolean(::GetKeyState(VK_CONTROL) & 0x8000);

		toolbar_.getRect(reinterpret_cast<LPNMTOOLBAR>(&nmhdr)->iItem, rect);
		pt.x = rect.left;
		pt.y = rect.bottom;
		getMainWindow().clientToScreen(pt);
		switch(reinterpret_cast<LPNMTOOLBAR>(&nmhdr)->iItem) {
		case CMD_FILE_NEW:
/*			while(newDocTypeMenu_->getItemCount() > 0)	// ���ׂč폜
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
			mruManager_->getPopupMenu().trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow().getHandle());
			return true;
		}
		break;
	}
	case TBN_GETOBJECT:
		onCommand(reinterpret_cast<LPNMOBJECTNOTIFY>(&nmhdr)->iItem, 0, 0);
		return 0;
	case TTN_GETDISPINFOW:	// �c�[���o�[�̃c�[���`�b�v
		if(nmhdr.idFrom >= CMD_VIEW_BUFFERLIST_START && nmhdr.idFrom < CMD_VIEW_BUFFERLIST_END)
			return toBoolean(buffers_->handleBufferBarNotification(*reinterpret_cast<NMTOOLBARW*>(&nmhdr)));
		else {
			static wchar_t tipText[500];
			::NMTTDISPINFOW& nmttdi = *reinterpret_cast<NMTTDISPINFOW*>(&nmhdr);
			nmttdi.hinst = getHandle();
			wstring name = commandManager_->getName(static_cast<CommandID>(nmhdr.idFrom));
			const wstring key = keyboardMap_.getKeyString(static_cast<CommandID>(nmhdr.idFrom));
			if(!key.empty()) {
				name += L" (";
				name += key;
				name += L")";
			}
			wcscpy(nmttdi.lpszText = tipText, name.c_str());
		}
		return true;
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
				if(0 != loadString(nmtbhi.idNew, caption, countof(caption))) {
					if(const wchar_t* const lf = wcschr(caption, L'\n'))
						setStatusText(lf + 1);
				} else
					setStatusText(L"");
			}
		}
		break;
*/	}
	return false;
}

/// RBN_CHEVRONPUSHED �̏���
void Alpha::onRebarChevronPushed(const NMREBARCHEVRON& chevron) {
	AutoZeroCB<::REBARBANDINFOW> rbi;
	::RECT bandRect;

	// �c�[���o�[�𓾂� (�o�b�t�@�o�[�ł����ʂ̃R�[�h���g����)
	rebar_.getRect(chevron.uBand, bandRect);
	rbi.fMask = RBBIM_CHILD | RBBIM_IDEALSIZE;
	rebar_.getBandInfo(chevron.uBand, rbi);
	const long buttonCount = static_cast<long>(::SendMessage(rbi.hwndChild, TB_BUTTONCOUNT, 0, 0L));

	// ��\���̃{�^���܂Ői��
	long i;
	::RECT buttonRect;
	for(i = 0; i < buttonCount; ++i) {
		::SendMessage(rbi.hwndChild, TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&buttonRect));
		if(buttonRect.right + bandRect.left > chevron.rc.left)
			break;
	}

	// ��\���̃{�^�������j���[���ڂɕϊ�
	PopupMenu popup;
	::POINT pt = {chevron.rc.left, chevron.rc.bottom};
	AutoZeroCB<::TBBUTTONINFOW> tbbi;
    tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE;
	for(; i < buttonCount; ++i) {
		::SendMessage(rbi.hwndChild, TB_GETBUTTONINFOW, i, reinterpret_cast<LPARAM>(&tbbi));
		if(toBoolean(tbbi.fsStyle & TBSTYLE_SEP))
			popup << Menu::SeparatorItem();
		else
			popup << Menu::StringItem(tbbi.idCommand, commandManager_->getMenuName(tbbi.idCommand).c_str(),
				(commandManager_->isEnabled(tbbi.idCommand, true) ? MFS_ENABLED : MFS_DISABLED)
				| ((commandManager_->isChecked(tbbi.idCommand)) ? MFS_CHECKED : 0));
	}
	rebar_.clientToScreen(pt);
	popup.trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow().getHandle());
}

/// @see WM_SETCURSOR
bool Alpha::onSetCursor(HWND hWnd, UINT nHitTest, UINT message) {
	POINT pt;	// �J�[�\���ʒu
	RECT clientRect, statusBarRect;

	getMainWindow().getClientRect(clientRect);
	if(statusBar_.isVisible())	statusBar_.getRect(statusBarRect);
	else						::SetRect(&statusBarRect, 0, 0, 0, 0);
	::GetCursorPos(&pt);
	getMainWindow().screenToClient(pt);

	if(pt.y >= clientRect.bottom - (statusBarRect.bottom - statusBarRect.top) - 3
			&& pt.y <= clientRect.bottom - (statusBarRect.bottom - statusBarRect.top)) {
		::SetCursor(loadStandardCursor(IDC_SIZENS));
		return true;
	}

	return false;
}

/// @see WM_SETTINGCHNAGE
void Alpha::onSettingChange(UINT, const wchar_t*) {
	AutoZeroCB<NONCLIENTMETRICSW> ncm;
	ascension::updateSystemSettings();
	::DeleteObject(statusFont_);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
	statusFont_ = ::CreateFontIndirectW(&ncm.lfStatusFont);
	updateStatusBarPaneSize();
}

/**
 *	@see			Window::onSize
 *	@param type		Window::onSize �Q��
 *	@param cx, cy	-1�ɂ���Ƃ��̃��\�b�h�͌��݂̃E�B���h�E�T�C�Y���g���B
 *					���̃��\�b�h���s������̏�����P���Ɏg�p�������ꍇ�ɂ��Ăяo�����Ƃ��ł���
 */
void Alpha::onSize(UINT type, int cx, int cy) {
	RECT rebarRect, statusBarRect, editorRect;

	if(cx == -1 && cy == -1) {
		RECT rect;
		getMainWindow().getClientRect(rect);
		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;
	}

	if(statusBar_.isVisible()) {
		statusBar_.sendMessage(WM_SIZE, cx, cy);
		statusBar_.getRect(statusBarRect);
		updateStatusBarPaneSize();
	} else
		::SetRect(&statusBarRect, 0, 0, 0, 0);

	if(rebar_.isVisible()) {
		rebar_.sendMessage(WM_SIZE, cx, cy);
		rebar_.getRect(rebarRect);
		toolbar_.sendMessage(WM_SIZE, cx, rebarRect.bottom - rebarRect.top - 2);
	} else
		::SetRect(&rebarRect, 0, 0, 0, 0);

	editorRect.left =  0;
	editorRect.top = rebarRect.bottom - rebarRect.top;
	editorRect.right = cx;
	editorRect.bottom = cy
		- (statusBar_.isVisible() ? statusBarRect.bottom - statusBarRect.top : 0);
//	if(outputWindow.isWindow() && outputWindow.isVisible())
//		editorRect.bottom -= outputWndHeight_;
	if(buffers_->getEditorWindow().isWindow())
		buffers_->getEditorWindow().move(editorRect, true);

//	if(outputWindow_.isWindow()) {
//		outputWindow_.move(0, editorRect.bottom + 2, cx, outputWndHeight_);
//		outputWindow_.show(SW_SHOW);
//	}
}

/// @see WM_TIMER
void Alpha::onTimer(UINT timerID) {
	if(timerID == ID_TIMER_QUERYCOMMAND && buffers_->getCount() != 0) {
		// �c�[���o�[�A�C�e���̗L����/������
		if(toolbar_.isVisible()) {
			const size_t buttonCount = toolbar_.getButtonCount();
			TBBUTTON button;

//			toolbar_.lockWindowUpdate();	// ���ꂪ����ƃc�[���o�[���X�N���[���O�ɂ��鎞�ɉ�ʂ������
			for(size_t i = 0; i < buttonCount; ++i) {
				toolbar_.getButton(static_cast<int>(i), button);
				toolbar_.checkButton(button.idCommand, commandManager_->isChecked(button.idCommand));
				toolbar_.enableButton(button.idCommand, commandManager_->isEnabled(button.idCommand, true));
			}
//			toolbar_.unlockWindowUpdate();
		}
	} else if(timerID == ID_TIMER_MOUSEMOVE) {	// �q���g�\��
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
						&& wcscmp(dpi.m_bstrType, L"�G���[") != 0)	// VC7
					ssResult << dpi.m_bstrName << L" = " << dpi.m_bstrValue;
				if(dpi.m_dwValidFields & DBGPROP_INFO_NAME)		::SysFreeString(dpi.m_bstrName);
				if(dpi.m_dwValidFields & DBGPROP_INFO_TYPE)		::SysFreeString(dpi.m_bstrType);
				if(dpi.m_dwValidFields & DBGPROP_INFO_VALUE)	::SysFreeString(dpi.m_bstrValue);
			}
		}
*/		getMainWindow().killTimer(ID_TIMER_MOUSEMOVE);
	}
}
