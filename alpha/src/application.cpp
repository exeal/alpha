/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2009
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
using namespace alpha;
//using namespace alpha::command;
using namespace ascension;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace ascension::encoding;
using namespace ascension::texteditor::commands;
using namespace manah;
using namespace std;
namespace py = boost::python;


// �O���[�o���֐�//////////////////////////////////////////////////////////

void callOleUninitialize() {
	::OleUninitialize();
}

/// The entry point.
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

	// �ȒP�ȑ��d�N���}�~ (Ctrl �L�[�������Ȃ���N������Ƒ��d�N������悤�ɂ��Ă݂�)
	if(::GetLastError() != ERROR_ALREADY_EXISTS || toBoolean(::GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
		::OleInitialize(0);	// STA �ɓ��� + �������T�[�r�X�̏�����
		atexit(&callOleUninitialize);
		win32::ui::initCommonControls(ICC_COOL_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_WIN95_CLASSES);
		ambient::Interpreter::instance().install();
		ambient::Interpreter::instance().toplevelPackage();
		Alpha* application = new Alpha();
		exitCode = application->run(nCmdShow);
		delete application;
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

	/// ChooseFontW �̂��߂̃t�b�N�v���V�W��
	UINT_PTR CALLBACK chooseFontHookProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
		if(message == WM_COMMAND && LOWORD(wParam) == psh3) {	// [�K�p] �{�^��
			LOGFONTW lf;
			::SendMessageW(dialog, WM_CHOOSEFONT_GETLOGFONT, 0, reinterpret_cast<LPARAM>(&lf));
			Alpha::instance().setFont(lf);
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
Alpha::Alpha() : editorFont_(0) {
	assert(Alpha::instance_ == 0);
	Alpha::instance_ = this;
	searchDialog_.reset(new ui::SearchDialog());	// ctor of SearchDialog calls Alpha
	onSettingChange(0, 0);	// statusFont_ �̏�����
}

/// �f�X�g���N�^
Alpha::~Alpha() {
	::DeleteObject(statusFont_);
	Alpha::instance_ = 0;
}

LRESULT CALLBACK Alpha::appWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	return (instance_ != 0) ?
		instance_->dispatchEvent(window, message, wParam, lParam) : ::DefWindowProc(window, message, wParam, lParam);
}

/// [�t�H���g] �_�C�A���O��\�����ăG�f�B�^�̃t�H���g��ύX����
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

/// ���b�Z�[�W�̐U�蕪��
LRESULT	Alpha::dispatchEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
	case WM_ACTIVATE:
		if(wParam == WA_ACTIVE) {
			BufferList& buffers = BufferList::instance();
			for(size_t i = 0; i < buffers.numberOfBuffers(); ++i)
				buffers.at(i).textFile().checkTimeStamp();
		}
		return 0L;
	case WM_COMMAND:
		return onCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
	case WM_CLOSE:
		teardown();
		return 0;
	case WM_COPYDATA:
		onCopyData(reinterpret_cast<HWND>(wParam), *reinterpret_cast<COPYDATASTRUCT*>(lParam));
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
		onEnterMenuLoop(toBoolean(wParam));	// ??? C4244@MSVC9
		break;
	case WM_EXITMENULOOP:
		onExitMenuLoop(toBoolean(wParam));	// ??? C4244@MSVC9
		break;
	case WM_INITMENUPOPUP:
		ui::handleINITMENUPOPUP(wParam, lParam);
		break;
	case WM_MEASUREITEM:
		onMeasureItem(static_cast<UINT>(wParam), *reinterpret_cast<::MEASUREITEMSTRUCT*>(lParam));
		break;
//	case WM_MENUCHAR: {
//		auto_ptr<Menu> activePopup(new Menu(reinterpret_cast<HMENU>(lParam)));
//		return onMenuChar(LOWORD(wParam), HIWORD(wParam), *activePopup);
//	}
	case WM_MENUCOMMAND:
		ui::handleMENUCOMMAND(wParam, lParam);
		break;
	case WM_MENUSELECT:
		ui::handleMENUSELECT(wParam, lParam);
		break;
	case WM_NOTIFY:
		onNotify(static_cast<UINT>(wParam), *reinterpret_cast<NMHDR*>(lParam));
		break;
	case WM_QUERYENDSESSION:
		return teardown();
	case WM_SETCURSOR:
		if(onSetCursor(reinterpret_cast<HWND>(wParam),
				static_cast<UINT>(LOWORD(lParam)), static_cast<UINT>(HIWORD(lParam))))
			return false;
		break;
	case WM_SETFOCUS:
		// m_iActiveView �������悤�Ƃ��Ă���r���[���w���\��������
//		if(m_buffers.GetCount() != 0 && m_buffers.GetActiveDocumentIndex() != -1
//				&& reinterpret_cast<HWND>(wParam) != m_documents.GetActiveDocument()->GetWindow())
//			::SendMessageW(m_documents.GetActiveDocument()->GetWindow(), message, wParam, lParam);
		EditorWindows::instance().setFocus();
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

/*	// �R�[�h�y�[�W���̓ǂݍ���
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
	// ����̏�����ǂݍ���
	try {
		kernel::Newline newline =
			static_cast<kernel::Newline>(readIntegerProfile(L"File", L"defaultNewline", kernel::NLF_CR_LF));
		if(newline == kernel::NLF_RAW_VALUE)
			newline = kernel::NLF_CR_LF;
//		kernel::Document::setDefaultCode(readIntegerProfile(L"File", L"defaultCodePage", ::GetACP()), newline);
	} catch(invalid_argument&) {
		// TODO: �ݒ肪�Ԉ���Ă��邱�Ƃ����[�U�ɒʒm
	}

	// �g�b�v���x���E�B���h�E
	if(!applicationWindow.create(IDS_APPNAME, reinterpret_cast<HWND>(use()),
			win32::ui::DefaultWindowRect(), 0, /*WS_VISIBLE |*/ WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW))
		return false;
	setMainWindow(applicationWindow);

	// ���o�[�̍쐬
	REBARINFO rbi = {sizeof(REBARINFO), 0, 0};
	rebar_.create(applicationWindow.use(), win32::ui::DefaultWindowRect(), 0, 0,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | RBS_BANDBORDERS | RBS_VARHEIGHT | CCS_NODIVIDER,
		WS_EX_TOOLWINDOW);
	rebar_.setBarInfo(rbi);

	// �c�[���o�[�̍쐬
//	setupToolbar();
	BufferList::instance().createBar(rebar_);

	// �G�f�B�^�E�B���h�E�̍쐬
	// (WS_CLIPCHILDREN ��t����ƕ����E�B���h�E�̃T�C�Y�ύX�g���s���ɂȂ�...)
	EditorWindows::instance().create(getMainWindow().use(),
		win32::ui::DefaultWindowRect(), WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, *(new EditorWindow));
	assert(EditorWindows::instance().isWindow());

	// ��ʐݒ�̓ǂݍ���
	loadINISettings();

	// �X�N���v�g�ɂ��ݒ�
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

	// �X�e�[�^�X�o�[�̍쐬
	statusBar_.create(applicationWindow.use());
	statusBar_.adjustPaneWidths();

	// ���̑��̏�����
	applicationWindow.dragAcceptFiles(true);
	applicationWindow.setTimer(ID_TIMER_QUERYCOMMAND, 200, 0);
	applicationWindow.setPosition(0, 0, 0, 760, 560, SWP_NOMOVE | SWP_NOZORDER);
	applicationWindow.center();

	// TODO: invoke the initialize script

	// �����̃r���[�̍쐬
	BufferList::instance().addNew();

//	setupMenus();
	if(!toBoolean(readIntegerProfile(L"View", L"visibleToolbar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_TOOLBAR), false);
	if(!toBoolean(readIntegerProfile(L"View", L"visibleStatusBar", true)))
		statusBar_.hide();
	if(!toBoolean(readIntegerProfile(L"View", L"visibleBufferBar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_BUFFERBAR), false);
	applicationWindow.show(showCommand);

	// �R�}���h���C������^����ꂽ�t�@�C�����J��
	WCHAR cd[MAX_PATH];
	::GetCurrentDirectoryW(MAX_PATH, cd);
	parseCommandLine(cd, ::GetCommandLineW());

	// ...
	statusBar_.setText(0);

	// �A�E�g�v�b�g�E�B���h�E�̍쐬
//	outputWindow.create(getMainWindow());
//	outputWindow.writeLine(OTT_GENERAL, IDS_APPFULLVERSION);

	// �c�[���_�C�A���O�̍쐬
	searchDialog_->doModeless(applicationWindow.get(), false);
	pushModelessDialog(searchDialog_->get());
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
//		searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
	}

	applicationWindow.setFocus();
	return true;
}

/// INI �t�@�C������ݒ��ǂݍ���
void Alpha::loadINISettings() {
	// �\���Ɋւ���ݒ�
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
#if(_MSC_VER < 1400)
		swprintf(keyName, L"findWhat(%u)", i);
#else
		swprintf(keyName, MANAH_COUNTOF(keyName), L"findWhat(%u)", i);
#endif // _MSC_VER < 1400
		value = readStringProfile(L"Find", keyName);
		if(value.empty())
			break;
		findWhats.push_back(value);
	}
	for(ushort i = 0; i < 16; ++i) {
#if(_MSC_VER < 1400)
		swprintf(keyName, L"replaceWith(%u)", i);
#else
		swprintf(keyName, MANAH_COUNTOF(keyName), L"replaceWith(%u)", i);
#endif // _MSC_VER < 1400
		value = readStringProfile(L"Find", keyName);
		if(value.empty())
			break;
		replacesWiths.push_back(value);
	}
	searcher::TextSearcher& s = BufferList::instance().editorSession().textSearcher();
	s.setMaximumNumberOfStoredStrings(16);
	s.setStoredStrings(findWhats.begin(), findWhats.end(), false);
	s.setStoredStrings(replacesWiths.begin(), replacesWiths.end(), true);
}

/**
 * ���b�Z�[�W�e�[�u���̕���������b�Z�[�W�{�b�N�X�ɕ\������
 * @param id ���b�Z�[�W���ʎq
 * @param type �_�C�A���O�̃^�C�v (::MessageBox �Ɠ���)
 * @param args ���b�Z�[�W�̈���
 * @return ���[�U�̕ԓ� (::MessageBox �Ɠ���)
 */
int Alpha::messageBox(DWORD id, UINT type, MessageArguments& args /* = MessageArguments() */) {
	return getMainWindow().messageBox(loadMessage(id, args).c_str(), IDS_APPNAME, type);
}

/**
 * �R�}���h���C������͂��Ď��s����B�����Ȉ����͖�������
 * @param currentDirectory �J�����g�f�B���N�g��
 * @param commandLine �R�}���h���C��
 * @see Alpha#onCopyData, WinMain
 */
void Alpha::parseCommandLine(const WCHAR* currentDirectory, const WCHAR* commandLine) {
	// TODO: this code should be reimplemented by Python.
	// CommandLineToArgvW �� argv[0] �ɑ΂����d���p���̈����ɖ�肪����悤��...
/*	int argc;
	WCHAR** argv = ::CommandLineToArgvW(commandLine, &argc);
	WCHAR canonical[MAX_PATH];
	for(int i = 1; i < argc; ++i) {
		if(toBoolean(::PathIsRelativeW(argv[i])))
			::PathCombineW(canonical, currentDirectory, argv[i]);
		else
			wcscpy(canonical, argv[i]);
		if(toBoolean(::PathIsDirectoryW(canonical)))
			BufferList::instance().openDialog(canonical);
		else
			BufferList::instance().open(canonical);
	}
	::LocalFree(argv);
*/}

/// @see manah#win32#Application#preTranslateMessage
bool Alpha::preTranslateMessage(const MSG& message) {
	return (message.message >= WM_KEYFIRST && message.message <= WM_KEYLAST) ? ui::InputManager::instance().input(message) : false;
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

#if 0
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
#endif // 0

/// INI �t�@�C���ɐݒ��ۑ�����
void Alpha::saveINISettings() {
	wchar_t keyName[30];

	// �o�[�̉����̕ۑ�
	win32::AutoZero<REBARBANDINFOW> rbbi;
	rbbi.fMask = RBBIM_STYLE;
	rebar_.getBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
	writeIntegerProfile(L"View", L"visibleToolbar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
	rebar_.getBandInfo(rebar_.idToIndex(IDC_BUFFERBAR), rbbi);
	writeIntegerProfile(L"View", L"visibleBufferBar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
	writeIntegerProfile(L"View", L"visibleStatusBar", statusBar_.isVisible() ? 1 : 0);

	// ���������񗚗��̕ۑ�
	const searcher::TextSearcher& s = BufferList::instance().editorSession().textSearcher();
	for(size_t i = 0; i < s.numberOfStoredPatterns(); ++i) {
#if(_MSC_VER < 1400)
		swprintf(keyName, L"findWhat(%u)", i);
#else
		swprintf(keyName, MANAH_COUNTOF(keyName), L"findWhat(%u)", i);
#endif // _MSC_VER < 1400
		writeStringProfile(L"Find", keyName, s.pattern(i).c_str());
	}
#if(_MSC_VER < 1400)
	swprintf(keyName, L"findWhat(%u)", s.numberOfStoredPatterns());
#else
	swprintf(keyName, MANAH_COUNTOF(keyName), L"findWhat(%u)", s.numberOfStoredPatterns());
#endif // _MSC_VER < 1400
	writeStringProfile(L"Find", keyName, L"");
	for(size_t i = 0; i < s.numberOfStoredReplacements(); ++i) {
#if(_MSC_VER < 1400)
		swprintf(keyName, L"replaceWith(%u)", i);
#else
		swprintf(keyName, MANAH_COUNTOF(keyName), L"replaceWith(%u)", i);
#endif // _MSC_VER < 1400
		writeStringProfile(L"Find", keyName, s.replacement(i).c_str());
	}
#if(_MSC_VER < 1400)
	swprintf(keyName, L"replaceWith(%u)", s.numberOfStoredReplacements());
#else
	swprintf(keyName, MANAH_COUNTOF(keyName), L"replaceWith(%u)", s.numberOfStoredReplacements());
#endif // _MSC_VER < 1400
	writeStringProfile(L"Find", keyName, L"");
}

/// �S�ẴG�f�B�^�ƈꕔ�̃R���g���[���ɐV�����t�H���g��ݒ�
void Alpha::setFont(const LOGFONTW& font) {
	LOGFONTW lf = font;

	lf.lfWeight = FW_NORMAL;
	::DeleteObject(editorFont_);
	editorFont_ = ::CreateFontIndirectW(&lf);

	// �S�Ẵr���[�̃t�H���g���X�V
	BufferList& buffers = BufferList::instance();
	for(size_t i = 0; i < buffers.numberOfBuffers(); ++i) {
		presentation::Presentation& p = buffers.at(i).presentation();
		for(presentation::Presentation::TextViewerIterator it(p.firstTextViewer()); it != p.lastTextViewer(); ++it)
			(*it)->textRenderer().setFont(font.lfFaceName, font.lfHeight, 0);
	}

	// �ꕔ�̃R���g���[���ɂ��ݒ�
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
//		if(bookmarkDialog_.get() != 0 && bookmarkDialog_->isWindow())
//			bookmarkDialog_->sendItemMessage(IDC_LIST_BOOKMARKS, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		if(searchDialog_.get() != 0 && searchDialog_->isWindow()) {
//			searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//			searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		}
	}

	// INI �t�@�C���ɕۑ�
	writeStructureProfile(L"View", L"Font.default", lf);

	// ���� <-> �ϕ��ŕ\�L��ς���K�v������
	statusBar_.adjustPaneWidths();
}
#if 0
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
	for(size_t i = 0; i < buttonCount; ++i) {
		if(commands[i] != 0)
			commands[i] -= CMD_SPECIAL_START;
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
	MANAH_AUTO_STRUCT_SIZE(::REBARBANDINFOW, rbbi);
	const wstring caption = loadMessage(MSG_DIALOG__BUFFERBAR_CAPTION);
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
	if(drawItem.CtlType != ODT_MENU)	// �����_�ł̓��j���[�̕`��̂�
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
	case RBN_HEIGHTCHANGE:	// ���o�[�̍������ς����
		onSize(0, -1, -1);
		return true;
	case RBN_CHEVRONPUSHED:	// �c�[���o�[�̃V�F�u�����������ꂽ
		onRebarChevronPushed(*reinterpret_cast<LPNMREBARCHEVRON>(&nmhdr));
		return true;
#if 0
	case TBN_DROPDOWN: {	// �c�[���o�[�̃h���b�v�_�E��
		RECT rect;
		POINT pt;
		const bool ctrlPressed = toBoolean(::GetKeyState(VK_CONTROL) & 0x8000);

		toolbar_.getRect(reinterpret_cast<LPNMTOOLBAR>(&nmhdr)->iItem, rect);
		pt.x = rect.left;
		pt.y = rect.bottom;
		getMainWindow().clientToScreen(pt);
		switch(reinterpret_cast<LPNMTOOLBAR>(&nmhdr)->iItem + CMD_SPECIAL_START) {
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
//			mruManager_->popupMenu().trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow().getHandle());
			return true;
		}
		break;
	}
#endif // 0
	case TBN_GETOBJECT:
		onCommand(reinterpret_cast<::LPNMOBJECTNOTIFY>(&nmhdr)->iItem, 0, 0);
		return 0;
/*	case TTN_GETDISPINFOW: {	// �c�[���o�[�̃c�[���`�b�v
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

/// RBN_CHEVRONPUSHED �̏���
void Alpha::onRebarChevronPushed(const ::NMREBARCHEVRON& chevron) {
	win32::AutoZeroSize<REBARBANDINFOW> rbi;
	RECT bandRect;

	// �c�[���o�[�𓾂� (�o�b�t�@�o�[�ł����ʂ̃R�[�h���g����)
	rebar_.getRect(chevron.uBand, bandRect);
	rbi.fMask = RBBIM_CHILD | RBBIM_IDEALSIZE;
	rebar_.getBandInfo(chevron.uBand, rbi);
	const long buttonCount = static_cast<long>(::SendMessageW(rbi.hwndChild, TB_BUTTONCOUNT, 0, 0L));

	// ��\���̃{�^���܂Ői��
	long i;
	RECT buttonRect;
	for(i = 0; i < buttonCount; ++i) {
		::SendMessageW(rbi.hwndChild, TB_GETITEMRECT, i, reinterpret_cast<LPARAM>(&buttonRect));
		if(buttonRect.right + bandRect.left > chevron.rc.left)
			break;
	}

	// ��\���̃{�^�������j���[���ڂɕϊ�
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
	POINT pt;	// �J�[�\���ʒu
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
		// �c�[���o�[�A�C�e���̗L����/������
		if(toolbar_.isWindow() && toolbar_.isVisible()) {
			const size_t buttonCount = toolbar_.getButtonCount();
			TBBUTTON button;

//			toolbar_.lockWindowUpdate();	// ���ꂪ����ƃc�[���o�[���X�N���[���O�ɂ��鎞�ɉ�ʂ������
			for(size_t i = 0; i < buttonCount; ++i) {
				toolbar_.getButton(static_cast<int>(i), button);
//				toolbar_.checkButton(button.idCommand, commandManager_->isChecked(button.idCommand + CMD_SPECIAL_START));
//				toolbar_.enableButton(button.idCommand, commandManager_->isEnabled(button.idCommand + CMD_SPECIAL_START, true));
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

bool Alpha::teardown(bool callHook /* = true */) {
	if(callHook) {
		py::object toplevel(ambient::Interpreter::instance().toplevelPackage());
		if(toBoolean(::PyObject_HasAttrString(toplevel.ptr(), "about_to_be_killed_hook"))) {
			if(!toBoolean(::PyObject_IsTrue(toplevel.attr("about_to_be_killed_hook")().ptr())))
				return false;
		}
	}
	saveINISettings();
	getMainWindow().destroy();
//	::PostQuitMessage(0);
	return true;
}


// StatusBar ////////////////////////////////////////////////////////////////

StatusBar::StatusBar() : columnStartValue_(1) {
}

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
	// �i���[�C���O
	parts[3] = parts[4] - ICON_WIDTH - padding;
	// �㏑��/�}�����[�h
	const wstring overtypeMode = app.loadMessage(MSG_STATUS__OVERTYPE_MODE);
	const wstring insertMode = app.loadMessage(MSG_STATUS__INSERT_MODE);
	parts[2] = parts[3] - max(
		dc.getTextExtent(overtypeMode.data(), static_cast<int>(overtypeMode.length())).cx,
		dc.getTextExtent(insertMode.data(), static_cast<int>(insertMode.length())).cx) - padding;
	// �L�[�{�[�h�}�N��
	parts[1] = parts[2] - ICON_WIDTH - padding;
	// �L�����b�g�ʒu
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

bool StatusBar::create(HWND parent) {
	return win32::ui::StatusBar::create(parent, win32::ui::DefaultWindowRect(), 0, IDC_STATUSBAR,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_BOTTOM | CCS_NODIVIDER | SBARS_SIZEGRIP | SBT_TOOLTIPS);
}

void StatusBar::hide() {
	win32::ui::StatusBar::show(SW_HIDE);
}

/**
 * Sets the text of the first pane.
 * @param text the text to set. if this is @c null, the default text is used
 * @param font the font to draw the text. if this is @c null, the default font is used
 */
void StatusBar::setText(const WCHAR* text, HFONT font /* = 0 */) {
	win32::ui::StatusBar::setText(0, (text != 0) ? text : IDS_DEFAULTSTATUSTEXT, SBT_NOBORDERS);
	if(font != 0) {
		LOGFONTW lf;
		if(::GetObjectW(font, sizeof(LOGFONTW), &lf) == 0)
			throw win32::InvalidHandleException("font");
		if(HFONT newFont = ::CreateFontIndirectW(&lf))
			font_.reset(win32::managed(newFont));
	} else
		font_.reset();
	setFont(font_.get());
}

void StatusBar::show() {
	win32::ui::StatusBar::show(SW_SHOWNA);
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


namespace {
	bool killAlpha(bool callHook) {return Alpha::instance().teardown(callHook);}
}

ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
	ambient::Interpreter& interpreter = ambient::Interpreter::instance();
	py::scope scope(interpreter.toplevelPackage());

	py::def("kill_alpha", &killAlpha, py::arg("call_hook") = true);
ALPHA_EXPOSE_EPILOGUE()
