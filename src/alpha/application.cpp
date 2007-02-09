/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2006
 */

#include "stdafx.h"
#include "application.hpp"
#include "ankh.hpp"
#include "command.hpp"
#include "mru-manager.hpp"
#include "search-dialog.hpp"
#include "bookmark-dialog.hpp"
//#include "DebugDlg.h"
#include "ascension/text-editor.hpp"
#include "ascension/regex.hpp"
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
using namespace manah::windows;
using namespace manah::windows::ui;
using namespace manah::windows::gdi;
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
	::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_EVERY_1024_DF);
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
Alpha::Alpha()
		: menu_(0), newDocTypeMenu_(new Menu), appDocTypeMenu_(0), editorFont_(0),
		scriptSystem_(new ankh::ScriptSystem), mruManager_(0), twoStroke1stKey_(VK_NULL), twoStroke1stModifiers_(0),
		temporaryMacroDefiningIcon_(0), temporaryMacroPausingIcon_(0), narrowingIcon_(0) {
	assert(Alpha::instance_ == 0);
	Alpha::instance_ = this;
	scriptSystem_->AddRef();
	commandManager_.reset(new CommandManager(*this));
	searchDialog_.reset(new ui::SearchDlg(*this));
	bookmarkDialog_.reset(new ui::BookmarkDlg(*this));
	registerScriptEngineAssociations();
	onSettingChange(0, 0);	// statusFont_ �̏�����
}

/// �f�X�g���N�^
Alpha::~Alpha() {
	buffers_.reset(0);	// ��ɉ�̂���
	::DeleteObject(statusFont_);
	::DestroyIcon(temporaryMacroDefiningIcon_);
	::DestroyIcon(temporaryMacroPausingIcon_);
	::DestroyIcon(narrowingIcon_);
	delete newDocTypeMenu_;
	delete appDocTypeMenu_;
	scriptSystem_->shutdown();
	scriptSystem_->Release();
	Alpha::instance_ = 0;
}

/// @see IActiveBufferListener#activeBufferSwitched
void Alpha::activeBufferSwitched() {
	updateTitleBar();
	updateStatusBar(SBP_ALL);
}

/// @see BufferList::IActiveBufferListener::activeBufferPropertyChanged
void Alpha::activeBufferPropertyChanged() {
	updateTitleBar();
	updateStatusBar(SBP_ALL);
}

LRESULT CALLBACK Alpha::appWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	return (instance_ != 0) ?
		instance_->dispatchEvent(window, message, wParam, lParam) : ::DefWindowProc(window, message, wParam, lParam);
}

/// @see ascension#viewers#ICaretListener#caretMoved
void Alpha::caretMoved(const Caret&, const Region&) {
	updateStatusBar(SBP_POSITION);
}

/// [�t�H���g] �_�C�A���O��\�����ăG�f�B�^�̃t�H���g��ύX����
void Alpha::changeFont() {
	EditorView& activeView = buffers_->getActiveView();
	::LOGFONTW font;
	AutoZeroLS<::CHOOSEFONTW> cf;

	getTextEditorFont(font);
	cf.hwndOwner = getMainWindow().getSafeHwnd();
	cf.lpLogFont = &font;
	cf.lpfnHook = chooseFontHookProc;
	cf.Flags = CF_APPLY | CF_ENABLEHOOK | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_SCREENFONTS;
	cf.hInstance = get();

	if(toBoolean(::ChooseFontW(&cf))) {
		font.lfItalic = false;
		font.lfWeight = FW_REGULAR;
		setFont(font);
	}
}

/// @see ascension#IClipboardRingListener::clipboardRingChanged
void Alpha::clipboardRingChanged() {
}

/// @see ascension#IClipboardRingListener#clipboardRingAddingDenied
void Alpha::clipboardRingAddingDenied() {
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

/**
 * �R�}���h�̃��j���[�L���v�V������Ԃ�
 * @param id �R�}���h ID
 */
const wchar_t* Alpha::getMenuLabel(CommandID id) const {
	static wchar_t buffer[MAX_PATH + 8];

	// [�}�N��]
/*	if(id >= CMD_EDIT_PLUGINLIST_START && id < CMD_EDIT_PLUGINLIST_END) {
		if(scriptMacroManager_->getCount() != 0) {
			wcscpy(buffer, scriptMacroManager_->getName(id - CMD_EDIT_PLUGINLIST_START).c_str());
			wcscat(buffer, L"\t");
			wcscat(buffer, keyboardMap_.getKeyString(id, useShortKeyNames_).c_str());
		} else
			wcscpy(buffer, loadString(MSG_ERROR__FAILED_TO_LOAD_SOMETHING).c_str());
		return buffer;
	}
	
	else*/ if(id >= CMD_FILE_MRULIST_START && id < CMD_FILE_MRULIST_END) {
		const MRU& file = mruManager_->getFileInfoAt(id - CMD_FILE_MRULIST_START);
		swprintf(buffer, L"&%X  %s", id - CMD_FILE_MRULIST_START, file.fileName.c_str());
		return buffer;
	}
	
	else if(id >= CMD_VIEW_BUFFERLIST_START && id < CMD_VIEW_BUFFERLIST_END) {
		assert(static_cast<size_t>(id - CMD_VIEW_BUFFERLIST_START) < buffers_->getCount());
		if(id - CMD_VIEW_BUFFERLIST_START < 0x10)
			swprintf(buffer, L"&%X  %s", id - CMD_VIEW_BUFFERLIST_START,
				buffers_->getAt(id - CMD_VIEW_BUFFERLIST_START).getFilePathName());
		else
			wcscpy(buffer, buffers_->getAt(id - CMD_VIEW_BUFFERLIST_START).getFilePathName());
		return buffer;
	}

	loadString(id, buffer, countof(buffer));
	wchar_t* const delimiter = wcschr(buffer, L'\n');
	if(delimiter != 0)
		*delimiter = 0;
	wcscat(buffer, L"\t");
	wcscat(buffer, keyboardMap_.getKeyString(id, useShortKeyNames_).c_str());
	return buffer;
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
		getMainWindow().sendMessage(WM_INITMENU, reinterpret_cast<WPARAM>(menu_->getSafeHmenu()));
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
				MARGS % KeyboardMap::getStrokeString(KeyCombination(key, modifiers), useShortKeyNames_));
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
					KeyCombination(twoStroke1stKey_, twoStroke1stModifiers_),
					KeyCombination(key, modifiers), useShortKeyNames_));
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
	wc.hInstance = get();
	wc.hIcon = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	wc.hIconSm = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	wc.hCursor = loadStandardCursor(IDC_ARROW);
	wc.hbrBackground = manah::windows::BrushHandleOrColor(COLOR_3DFACE).brush;
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
		text::LineBreak lineBreak =
			static_cast<text::LineBreak>(readIntegerProfile(L"File", L"defaultBreakType", text::LB_CRLF));
		if(lineBreak == text::LB_AUTO)
			lineBreak = text::LB_CRLF;
		text::Document::setDefaultCode(readIntegerProfile(L"File", L"defaultCodePage", ::GetACP()), lineBreak);
	} catch(invalid_argument&) {
		// TODO: �ݒ肪�Ԉ���Ă��邱�Ƃ����[�U�ɒʒm
	}

	// �g�b�v���x���E�B���h�E
	if(!applicationWindow.create(IDS_APPNAME, reinterpret_cast<HWND>(get()),
			DefaultWindowRect(), 0, /*WS_VISIBLE |*/ WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW))
		return false;
	setMainWindow(applicationWindow);

	// ���o�[�̍쐬
	::REBARINFO rbi = {sizeof(::REBARINFO), 0, 0};
	rebar_.create(applicationWindow, DefaultWindowRect(), 0, 0,
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
	mruManager_ = new MRUManager(readIntegerProfile(L"File", L"mruLimit", 8), CMD_FILE_MRULIST_START, true);
	wchar_t keyName[30];
	stack<MRU> files;
	for(uint i = 0; ; ++i) {
		MRU file;
		swprintf(keyName, L"strPath(%u)", i);
		file.fileName = readStringProfile(L"MRU", keyName);
		if(file.fileName.empty())
			break;
		swprintf(keyName, L"nCodePage(%u)", i);
		file.codePage = readIntegerProfile(L"MRU", keyName, CPEX_AUTODETECT_USERLANG);
		files.push(file);
	}
	while(!files.empty()) {
		mruManager_->add(files.top().fileName, files.top().codePage);
		files.pop();
	}

	// �A�C�R���̗p��
	temporaryMacroDefiningIcon_ = static_cast<HICON>(loadImage(IDR_ICON_TEMPMACRODEFINING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	temporaryMacroPausingIcon_ = static_cast<HICON>(loadImage(IDR_ICON_TEMPMACROPAUSING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	narrowingIcon_ = static_cast<HICON>(loadImage(IDR_ICON_NARROWING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

	// �X�e�[�^�X�o�[�̍쐬
	statusBar_.create(applicationWindow, DefaultWindowRect(), 0, IDC_STATUSBAR,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_BOTTOM | CCS_NODIVIDER | SBARS_SIZEGRIP | SBT_TOOLTIPS);
//	statusBar_.setMinHeight(4);

	// ���̑��̏�����
	applicationWindow.dragAcceptFiles(true);
	applicationWindow.setTimer(ID_TIMER_QUERYCOMMAND, 200, 0);
	applicationWindow.setWindowPos(0, 0, 0, 760, 560, SWP_NOMOVE | SWP_NOZORDER);
	applicationWindow.centerWindow();

	// TODO: invoke the initialize script

	// �����̃r���[�̍쐬
	buffers_->addNew();

	setupMenus();
	if(!toBoolean(readIntegerProfile(L"View", L"visibleToolbar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_TOOLBAR), false);
	if(!toBoolean(readIntegerProfile(L"View", L"visibleStatusBar", true)))
		statusBar_.showWindow(SW_HIDE);
	if(!toBoolean(readIntegerProfile(L"View", L"visibleBufferBar", true)))
		rebar_.showBand(rebar_.idToIndex(IDC_BUFFERBAR), false);
	applicationWindow.showWindow(showCommand);

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
	searchDialog_->doModeless(applicationWindow, false);
	pushModelessDialog(*searchDialog_);
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
		searchDialog_->sendDlgItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
		searchDialog_->sendDlgItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
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
	searchDialog_->clearHistory(false);
	searchDialog_->clearHistory(true);
	searchDialog_->setHistory(findWhats, replacesWiths);

	// ���̑�
	useShortKeyNames_ = toBoolean(readIntegerProfile(L"Edit", L"useShortKeyNames", 0));
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
	if(menu_ != 0)
		setupMenus();
}

/// @see ascension#viewers#ICaretListener#matchBracketsChanged
void Alpha::matchBracketsChanged(const Caret& self, const pair<Position, Position>& oldPair, bool outsideOfView) {
	if(!outsideOfView || &self.getTextViewer() != &buffers_->getActiveView())
		return;
	const pair<Position, Position>& brackets = self.getMatchBrackets();
	if(brackets.second == Position::INVALID_POSITION)
		setStatusText(0);
	else {
/*		const AlphaDoc& activeDocument = buffers_->getActive();
		const AlphaView& activeView = buffers_->getActiveView();
		const Lexer& lexer = activeView.getLexer();
		const length_t lineCount = activeDocument.getLineCount();
		const EditView::Options& options = activeView.getOptions();
		char_t prompt[50] = {0};

		// �Ί��ʂ̎��ӂ�\�� (NUL �������܂ރe�L�X�g�͖��Ή�)
		for(length_t i = pos.line; i < lineCount; ++i) {
			const string_t line = activeDocument.getLine(i);
			const length_t indentLength =
				lexer.getCharacterClassification().eatWhiteSpaces(line.data(), line.data() + line.length(), true) - line.data();

			if(i == pos.line) {	// �Ί��ʂ̌��������s
				// �Ί��ʂ̑O
				if(pos.row - indentLength <= 20)
					wcsncat(prompt, line.c_str() + indentLength, pos.row - indentLength + 1);
				else {
					wcscpy(prompt, L"... ");
					wcsncat(prompt, line.c_str() + pos.row - 16, 17);
				}
				// ���ʂ̌��
				if(line.length() - pos.row - 1 <= 20)
					wcscat(prompt, line.c_str() + pos.row + 1);
				else {
					wcsncat(prompt, line.c_str() + pos.row + 1, 16);
					wcscat(prompt, L" ...");
					break;
				}
			} else {
				wcscat(prompt, L"^J");	// ���s
				if(indentLength != 0)
					wcscat(prompt, L" ");
				if(wcslen(prompt) >= 37) {
					wcscat(prompt, L" ...");
					break;
				}
				if(wcslen(prompt) + line.length() - indentLength >= 41) {
					wcsncat(prompt, line.c_str() + indentLength, 41 - wcslen(prompt));
					wcscat(prompt, L" ...");
					break;
				} else
					wcscat(prompt, line.c_str() + indentLength);
			}

			// �Ί��ʂ̌��
		}
		for(char_t* p = prompt; *p != 0; ++p) {
			if(*p == L'\t')
				*p = L' ';
		}
		const length_t startLine = activeView.getLayoutSetter().getSettings().lineNumberLayout.startLine;
		const wstring s = loadString(MSG_STATUS__MATCH_BRACKET_OUT_OF_VIEW, MARGS % (pos.line + startLine) % prompt);
		setStatusText(s.c_str());
*/	}
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

/// @see ascension#viewers#ICaretListener#overtypeModeChanged
void Alpha::overtypeModeChanged(const Caret& self) {
	if(&self.getTextViewer() == &buffers_->getActiveView())
		updateStatusBar(SBP_OVERTYPEMODE);
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
	if(msg.hwnd == buffers_->getActiveView()) {
		if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {	// WM_CHAR �����s����Ȃ��悤�ɂ���
			KeyModifier modifiers = 0;
			if(toBoolean(::GetKeyState(VK_CONTROL) & 0x8000))
				modifiers |= KM_CTRL;
			if(toBoolean(::GetKeyState(VK_SHIFT) & 0x8000))
				modifiers |= KM_SHIFT;
			if(msg.message == WM_SYSKEYDOWN || toBoolean(::GetKeyState(VK_MENU) & 0x8000))
				modifiers |= KM_ALT;
			return handleKeyDown(static_cast<VirtualKey>(msg.wParam), modifiers);
		} else if(msg.message == WM_SYSCHAR) {
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
						toBoolean(searchDialog_->isDlgButtonChecked(IDC_RADIO_SELECTION)));

	searchDialog_->updateOptions();
	buffers_->getEditorSession().getTextSearcher().setPattern(searchDialog_->getFindText());
	buffers_->getEditorSession().getTextSearcher().setReplacement(searchDialog_->getReplaceText());
	ulong replacedCount = -1;
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
		// �����ɒǉ�
		searchDialog_->addToHistory(searchDialog_->getFindText(), false);
		searchDialog_->addToHistory(searchDialog_->getReplaceText(), true);
	}
	if(searchDialog_->isWindow()) {
		if(searchDialog_->isDlgButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// [���ׂĒu����_�C�A���O�����]
			getMainWindow().sendMessage(WM_COMMAND, CMD_SEARCH_FIND);
		else
			::SetFocus(searchDialog_->getDlgItem(IDC_COMBO_FINDWHAT));
	}
}

/**
 * @brief ���݂̑I��͈͂�����������ł���Βu�����s��
 *
 * �u����A�����͑I��͈͂�����������Ŗ����ꍇ�͎��̌�����������������I����Ԃɂ���B
 * ���̃��\�b�h�͒u���_�C�A���O����\���ł��@�\����
 */
void Alpha::replaceAndSearchNext() {
	searchDialog_->updateOptions();
	buffers_->getEditorSession().getTextSearcher().setPattern(searchDialog_->getFindText());
	buffers_->getEditorSession().getTextSearcher().setReplacement(searchDialog_->getReplaceText());

	FindNextCommand command(buffers_->getActiveView(), true,
		searchDialog_->isDlgButtonChecked(IDC_CHK_SHIFT) ? BACKWARD : FORWARD);
	bool succeeded = false;

	try {
		succeeded = command.execute() == 0;
	} catch(boost::regex_error& e) {
		if(showMessageBoxOnFind_)
			showRegexSearchError(e);
	} catch(runtime_error&) {
		if(showMessageBoxOnFind_)
			messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	}
	if(succeeded)	// �u��
		searchDialog_->addToHistory(searchDialog_->getReplaceText(), true);
	if(searchDialog_->isWindowVisible()) {
//		if(searchDialog_.isDlgButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// [������_�C�A���O�����]
//			getMainWindow().sendMessage(WM_COMMAND, CMD_SEARCH_FIND);
//		else
			::SetFocus(searchDialog_->getDlgItem(IDC_COMBO_FINDWHAT));
	}
}

/// INI �t�@�C���ɐݒ��ۑ�����
void Alpha::saveINISettings() {
	wchar_t keyName[30];
	unsigned short i;

	// �o�[�̉����̕ۑ�
	AutoZero<::REBARBANDINFOW> rbbi;
	rbbi.fMask = RBBIM_STYLE;
	rebar_.getBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
	writeIntegerProfile(L"View", L"visibleToolbar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
	rebar_.getBandInfo(rebar_.idToIndex(IDC_BUFFERBAR), rbbi);
	writeIntegerProfile(L"View", L"visibleBufferBar", toBoolean(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
	writeIntegerProfile(L"View", L"visibleStatusBar", statusBar_.isWindowVisible() ? 1 : 0);

	// MRU ���X�g�̕ۑ�
	for(i = 0; ; ++i) {
		swprintf(keyName, L"pathName(%u)", i);
		if(i == mruManager_->getCount()) {
			writeStringProfile(L"MRU", keyName, L"");	// ���X�g�̏I�[��\��
			break;
		} else {
			const MRU& file = mruManager_->getFileInfoAt(i);
			writeStringProfile(L"MRU", keyName, file.fileName.c_str());
			swprintf(keyName, L"codePage(%u)", i);
			writeIntegerProfile(L"MRU", keyName, file.codePage);
		}
	}

	// ���������񗚗��̕ۑ�
	list<wstring> findWhats, replaceWiths;
	list<wstring>::const_iterator it;
	searchDialog_->getHistory(findWhats, replaceWiths);
	for(i = 0, it = findWhats.begin(); it != findWhats.end(); ++i, ++it) {
		swprintf(keyName, L"findWhat(%u)", i);
		writeStringProfile(L"Find", keyName, it->c_str());
	}
	swprintf(keyName, L"findWhat(%u)", i);
	writeStringProfile(L"Find", keyName, L"");
	for(i = 0, it = replaceWiths.begin(); it != replaceWiths.end(); ++i, ++it) {
		swprintf(keyName, L"replaceWith(%u)", i);
		writeStringProfile(L"Find", keyName, it->c_str());
	}
	swprintf(keyName, L"replaceWith(%u)", i);
	writeStringProfile(L"Find", keyName, L"");
}

/// [���ׂă}�[�N]
void Alpha::searchAndBookmarkAll() {
	FindAllCommand command(buffers_->getActiveView(), FindAllCommand::BOOKMARK,
						toBoolean(searchDialog_->isDlgButtonChecked(IDC_RADIO_SELECTION)));
	searchDialog_->updateOptions();
	buffers_->getEditorSession().getTextSearcher().setPattern(searchDialog_->getFindText());
	try {
		if(command.execute() > 0)
			searchDialog_->addToHistory(searchDialog_->getFindText(), false);
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
	searchDialog_->updateOptions();
	buffers_->getEditorSession().getTextSearcher().setPattern(searchDialog_->getFindText());

	try {
		if(command.execute() == 0) {
			if(searchDialog_->isWindow())
				searchDialog_->checkDlg2StateButton(IDC_CHK_SHIFT, !forward);
			searchDialog_->addToHistory(searchDialog_->getFindText(), false);
			return true;
		} else if(messageOnFailure)
			messageBox(MSG_SEARCH__PATTERN_NOT_FOUND, MB_ICONINFORMATION);
	} catch(boost::regex_error& e) {
		if(messageOnFailure)
			showRegexSearchError(e);
	} catch(runtime_error&) {
		if(messageOnFailure)
			messageBox(MSG_ERROR__REGEX_UNKNOWN_ERROR, MB_ICONEXCLAMATION);
	}
	if(searchDialog_->isWindowVisible()) {
		if(searchDialog_->isDlgButtonChecked(IDC_CHK_AUTOCLOSE) == BST_CHECKED)	// [������_�C�A���O�����]
			getMainWindow().sendMessage(WM_COMMAND, CMD_SEARCH_FIND);
		else
			::SetFocus(searchDialog_->getDlgItem(IDC_COMBO_FINDWHAT));
	}
	return false;
}

/// @see ascension#viewers#ICaretListener#selectionShapeChanged
void Alpha::selectionShapeChanged(const Caret&) {
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
			bookmarkDialog_->sendDlgItemMessage(IDC_LIST_BOOKMARKS, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
		if(searchDialog_.get() != 0 && searchDialog_->isWindow()) {
			searchDialog_->sendDlgItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
			searchDialog_->sendDlgItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
		}
	}

	// INI �t�@�C���ɕۑ�
	writeStructureProfile(L"View", L"oFont.pLogfont", lf);

	// ���� <-> �ϕ��ŕ\�L��ς���K�v������
	updateStatusBar(SBP_POSITION);
}

/// ���j���[�̏�����
void Alpha::setupMenus() {
	if(menu_ != 0) {
		while(true) {
			const UINT c = menu_->getItemCount();
			if(c == 0 || c == -1)
				break;
			menu_->removeMenuItem<Menu::BY_POSITION>(0);
		}
		delete menu_;
	}
	menu_ = new Menu(getMainWindow().getMenu());
	menu_->deleteMenuItem<Menu::BY_POSITION>(0);	// �_�~�[������ (�Ӗ�������̂͏���̂�)

	const Menu::SeparatorItem sep(MFT_OWNERDRAW);

	// ���j���[�o�[
	*menu_ << Menu::StringItem(CMD_FILE_TOP, loadString(CMD_FILE_TOP).c_str())
		<< Menu::StringItem(CMD_EDIT_TOP, loadString(CMD_EDIT_TOP).c_str())
		<< Menu::StringItem(CMD_SEARCH_TOP, loadString(CMD_SEARCH_TOP).c_str())
		<< Menu::StringItem(CMD_VIEW_TOP, loadString(CMD_VIEW_TOP).c_str())
		<< Menu::StringItem(CMD_MACRO_TOP, loadString(CMD_MACRO_TOP).c_str())
		<< Menu::StringItem(CMD_TOOL_TOP, loadString(CMD_TOOL_TOP).c_str())
		<< Menu::StringItem(CMD_HELP_TOP, loadString(CMD_HELP_TOP).c_str());

	// [�t�@�C��]
	Menu* fileMenu = new Menu();
	*fileMenu << Menu::OwnerDrawnItem(CMD_FILE_NEW)
		<< Menu::OwnerDrawnItem(CMD_FILE_NEWWITHFORMAT) << sep
		<< Menu::OwnerDrawnItem(CMD_FILE_OPEN) << Menu::OwnerDrawnItem(CMD_FILE_REOPEN)
		<< Menu::OwnerDrawnItem(CMD_FILE_REOPENWITHCODEPAGE) << Menu::OwnerDrawnItem(CMD_FILE_MRU) << sep
		<< Menu::OwnerDrawnItem(CMD_FILE_CLOSE) << Menu::OwnerDrawnItem(CMD_FILE_CLOSEALL)
		<< Menu::OwnerDrawnItem(CMD_FILE_CLOSEOTHERS) << sep
		<< Menu::OwnerDrawnItem(CMD_FILE_SAVE) << Menu::OwnerDrawnItem(CMD_FILE_SAVEAS)
		<< Menu::OwnerDrawnItem(CMD_FILE_SAVEALL) << sep << Menu::OwnerDrawnItem(CMD_FILE_SENDMAIL)
		<< sep << Menu::OwnerDrawnItem(CMD_FILE_EXIT);
	fileMenu->setChildPopup<Menu::BY_COMMAND>(CMD_FILE_MRU, mruManager_->getPopupMenu(), false);
	menu_->setChildPopup<Menu::BY_COMMAND>(CMD_FILE_TOP, *fileMenu, true);

	// [�ҏW]
	Menu* editMenu = new Menu();
	*editMenu << Menu::OwnerDrawnItem(CMD_EDIT_UNDO)
		<< Menu::OwnerDrawnItem(CMD_EDIT_REDO) << Menu::SeparatorItem(MFT_OWNERDRAW)
		<< Menu::OwnerDrawnItem(CMD_EDIT_CUT) << Menu::OwnerDrawnItem(CMD_EDIT_COPY)
		<< Menu::OwnerDrawnItem(CMD_EDIT_PASTE) << Menu::OwnerDrawnItem(CMD_EDIT_PASTEFROMCLIPBOARDRING)
		<< Menu::OwnerDrawnItem(CMD_EDIT_DELETE) << Menu::OwnerDrawnItem(CMD_EDIT_SELECTALL)
		<< sep << Menu::OwnerDrawnItem(CMD_EDIT_ADVANCED)
		<< Menu::OwnerDrawnItem(CMD_EDIT_OPENCANDIDATEWINDOW) << Menu::OwnerDrawnItem(CMD_EDIT_SHOWABBREVIATIONDLG);
	menu_->setChildPopup<Menu::BY_COMMAND>(CMD_EDIT_TOP, *editMenu, true);

	// [�ҏW]-[���x�ȑ���]
	Menu* advEditMenu = new Menu();
	*advEditMenu << Menu::OwnerDrawnItem(CMD_EDIT_CHARTOCODEPOINT)
		<< Menu::OwnerDrawnItem(CMD_EDIT_CODEPOINTTOCHAR) << sep
		<< Menu::OwnerDrawnItem(CMD_EDIT_NARROWTOSELECTION) << Menu::OwnerDrawnItem(CMD_EDIT_WIDEN);
	editMenu->setChildPopup<Menu::BY_COMMAND>(CMD_EDIT_ADVANCED, *advEditMenu, true);

	// [����]
	Menu* findMenu = new Menu();
	*findMenu << Menu::OwnerDrawnItem(CMD_SEARCH_FIND)
		<< Menu::OwnerDrawnItem(CMD_SEARCH_FINDNEXT) << Menu::OwnerDrawnItem(CMD_SEARCH_FINDPREV)
		<< Menu::OwnerDrawnItem(CMD_SEARCH_INCREMENTALSEARCH) << Menu::OwnerDrawnItem(CMD_SEARCH_INCREMENTALSEARCHR)
		<< Menu::OwnerDrawnItem(CMD_SEARCH_INCREMENTALSEARCHRF) << Menu::OwnerDrawnItem(CMD_SEARCH_INCREMENTALSEARCHRR)
		<< Menu::OwnerDrawnItem(CMD_SEARCH_INCREMENTALSEARCHMF) << Menu::OwnerDrawnItem(CMD_SEARCH_INCREMENTALSEARCHMR)
		<< Menu::OwnerDrawnItem(CMD_SEARCH_REVOKEMARK) << sep << Menu::OwnerDrawnItem(CMD_SEARCH_GOTOLINE)
		<< sep << Menu::OwnerDrawnItem(CMD_SEARCH_TOGGLEBOOKMARK)
		<< Menu::OwnerDrawnItem(CMD_SEARCH_NEXTBOOKMARK) << Menu::OwnerDrawnItem(CMD_SEARCH_PREVBOOKMARK)
		<< Menu::OwnerDrawnItem(CMD_SEARCH_CLEARBOOKMARKS) << Menu::OwnerDrawnItem(CMD_SEARCH_MANAGEBOOKMARKS)
		<< sep << Menu::OwnerDrawnItem(CMD_SEARCH_GOTOMATCHBRACKET) << Menu::OwnerDrawnItem(CMD_SEARCH_EXTENDTOMATCHBRACKET);
	menu_->setChildPopup<Menu::BY_COMMAND>(CMD_SEARCH_TOP, *findMenu, true);

	// [�\��]
	Menu* viewMenu = new Menu();
	*viewMenu << Menu::OwnerDrawnItem(CMD_VIEW_TOOLBAR)
		<< Menu::OwnerDrawnItem(CMD_VIEW_STATUSBAR) << Menu::OwnerDrawnItem(CMD_VIEW_BUFFERBAR)
		<< sep << Menu::OwnerDrawnItem(CMD_VIEW_BUFFERS) << Menu::OwnerDrawnItem(CMD_VIEW_NEXTBUFFER)
		<< Menu::OwnerDrawnItem(CMD_VIEW_PREVBUFFER) << Menu::SeparatorItem(MFT_OWNERDRAW)
		<< Menu::OwnerDrawnItem(CMD_VIEW_SPLITNS) << Menu::OwnerDrawnItem(CMD_VIEW_SPLITWE)
		<< Menu::OwnerDrawnItem(CMD_VIEW_UNSPLITOTHERS) << Menu::OwnerDrawnItem(CMD_VIEW_UNSPLITACTIVE)
		<< Menu::OwnerDrawnItem(CMD_VIEW_NEXTPANE) << Menu::OwnerDrawnItem(CMD_VIEW_PREVPANE)
		<< sep << Menu::OwnerDrawnItem(CMD_VIEW_WRAPNO)
		<< Menu::OwnerDrawnItem(CMD_VIEW_WRAPBYSPECIFIEDWIDTH)
		<< Menu::OwnerDrawnItem(CMD_VIEW_WRAPBYWINDOWWIDTH)
		<< Menu::SeparatorItem(MFT_OWNERDRAW) << Menu::OwnerDrawnItem(CMD_VIEW_TOPMOSTALWAYS)
		<< Menu::OwnerDrawnItem(CMD_VIEW_REFRESH);
	viewMenu->setChildPopup<Menu::BY_COMMAND>(CMD_VIEW_BUFFERS, buffers_->getListMenu(), false);
	menu_->setChildPopup<Menu::BY_COMMAND>(CMD_VIEW_TOP, *viewMenu, true);

	// [�}�N��]
	Menu* macroMenu = new Menu();
	*macroMenu << Menu::OwnerDrawnItem(CMD_MACRO_EXECUTE)
		<< Menu::OwnerDrawnItem(CMD_MACRO_DEFINE) << Menu::OwnerDrawnItem(CMD_MACRO_APPEND)
		<< Menu::OwnerDrawnItem(CMD_MACRO_PAUSERESTART) << Menu::OwnerDrawnItem(CMD_MACRO_INSERTQUERY)
		<< Menu::OwnerDrawnItem(CMD_MACRO_ABORT) << Menu::OwnerDrawnItem(CMD_MACRO_SAVEAS)
		<< Menu::OwnerDrawnItem(CMD_MACRO_LOAD) << sep << Menu::OwnerDrawnItem(CMD_MACRO_SCRIPTS);
	menu_->setChildPopup<Menu::BY_COMMAND>(CMD_MACRO_TOP, *macroMenu, true);

	// [�}�N��]-[�X�N���v�g] (�b��)
	Menu* scriptMenu = new Menu();/*
//	*scriptMenu << Menu::OwnerDrawnItem(CMD_EDIT_RELOADPLUGIN) << sep;
	const size_t c = scriptMacroManager_->getCount();
	if(c != 0) {
//		*scriptControls::Menu << Menu::OwnerDrawnItem(CMD_MACRO_INTERRUPT) << sep;
		for(size_t i = 0; i < c; ++i)
			*scriptMenu << Menu::OwnerDrawnItem(CMD_EDIT_PLUGINLIST_START + static_cast<UINT>(i));
	} else
		*scriptMenu << Menu::OwnerDrawnItem(CMD_EDIT_PLUGINLIST_START, MFS_GRAYED | MFS_DISABLED);
*/	macroMenu->setChildPopup<Menu::BY_COMMAND>(CMD_MACRO_SCRIPTS, *scriptMenu, true);

	// [�c�[��]
	Menu* toolMenu = new Menu();
	*toolMenu << Menu::OwnerDrawnItem(CMD_TOOL_EXECUTE)
		<< Menu::OwnerDrawnItem(CMD_TOOL_EXECUTECOMMAND) << sep
		<< Menu::OwnerDrawnItem(CMD_TOOL_APPDOCTYPES) << Menu::OwnerDrawnItem(CMD_TOOL_DOCTYPEOPTION)
		<< Menu::OwnerDrawnItem(CMD_TOOL_COMMONOPTION) << Menu::OwnerDrawnItem(CMD_TOOL_FONT);
	menu_->setChildPopup<Menu::BY_COMMAND>(CMD_TOOL_TOP, *toolMenu, true);
	delete appDocTypeMenu_;
	appDocTypeMenu_ = new Menu();	// [�K�p�����^�C�v]
	toolMenu->setChildPopup<Menu::BY_COMMAND>(CMD_TOOL_APPDOCTYPES, *appDocTypeMenu_, false);

	// [�w���v]
	Menu* helpMenu = new Menu();
	*helpMenu << Menu::OwnerDrawnItem(CMD_HELP_ABOUT);
	menu_->setChildPopup<Menu::BY_COMMAND>(CMD_HELP_TOP, *helpMenu, true);

	getMainWindow().drawMenuBar();
}

/// �c�[���o�[�̏����� (1�񂵂��Ăяo���Ă͂Ȃ�Ȃ�)
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
		buttonCount = 16;
		commands = new CommandID[buttonCount];
		commands[0] = CMD_FILE_NEW;			commands[1] = CMD_FILE_OPEN;
		commands[2] = CMD_FILE_SAVE;		commands[3] = CMD_FILE_SAVEAS;
		commands[4] = CMD_FILE_SAVEALL;		commands[5] = 0;
		commands[6] = CMD_EDIT_CUT;			commands[7] = CMD_EDIT_COPY;
		commands[8] = CMD_EDIT_PASTE;		commands[9] = 0;
		commands[10] = CMD_EDIT_UNDO;		commands[11] = CMD_EDIT_REDO;
		commands[12] = 0;					commands[13] = CMD_SEARCH_FIND;
		commands[14] = CMD_SEARCH_FINDNEXT;	commands[15] = CMD_SEARCH_FINDPREV;
	}

	// �C���[�W���X�g���쐬����
	WCHAR iconDir[MAX_PATH];
	wcscpy(iconDir, getModuleFileName());
	::PathFindFileNameW(iconDir)[0] = 0;
	wcscat(iconDir, IDS_ICON_DIRECTORY_NAME);
	commandManager_->createImageList(iconDir);

	// �{�^�������
	TBBUTTON* buttons = new TBBUTTON[buttonCount];
	bool hasDropArrow;
	for(size_t i = 0; i < buttonCount; ++i) {
		hasDropArrow = commands[i] == CMD_FILE_NEW || commands[i] == CMD_FILE_OPEN;
		ZeroMemory(buttons + i, sizeof(TBBUTTON));
		buttons[i].fsState = TBSTATE_ENABLED;
		if(commands[i] == 0)
			buttons[i].fsStyle = BTNS_SEP;
		else {
			size_t icon = commandManager_->getIconIndex(commands[i]);

			if(hasDropArrow)	buttons[i].fsStyle = BTNS_BUTTON | BTNS_DROPDOWN;
			else if(icon == -1)	buttons[i].fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON;
			else				buttons[i].fsStyle = BTNS_BUTTON;
			if(icon != -1 /*&& !bHasDropArrow*/)
				buttons[i].iBitmap = static_cast<int>(icon);
			else {
				const wstring caption = commandManager_->getCaption(commands[i]);
				wchar_t* p = new wchar_t[caption.length() + 1];
				wcscpy(p, caption.c_str());
				buttons[i].iString = reinterpret_cast<INT_PTR>(p);
				buttons[i].iBitmap = hasDropArrow ? static_cast<int>(icon) : I_IMAGENONE;
			}
		}
		buttons[i].idCommand = commands[i];
	}

	if(!toolbar_.isWindow()) {
		toolbar_.create(rebar_, DefaultWindowRect(), L"", IDC_TOOLBAR,
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
	toolbar_.addButtons(static_cast<int>(buttonCount), buttons);
	toolbar_.setImageList(commandManager_->getImageList(CommandManager::ICONSTATE_NORMAL).get());
	toolbar_.setDisabledImageList(commandManager_->getImageList(CommandManager::ICONSTATE_DISABLED).get());
	toolbar_.setHotImageList(commandManager_->getImageList(CommandManager::ICONSTATE_HOT).get());
//	toolbar_.setPadding(6, 6);

	for(size_t i = 0; i < buttonCount; ++i) {
		delete[] reinterpret_cast<wchar_t*>(buttons[i].iString);
		if(buttons[i].fsStyle != BTNS_SEP
				&& buttons[i].iBitmap != I_IMAGENONE) {	// �A�C�R���t���{�^���̕��������ŌŒ肷��
			AutoZeroCB<TBBUTTONINFOW> tbi;
			tbi.dwMask = TBIF_SIZE;
			tbi.cx = (buttons[i].idCommand != CMD_FILE_NEW && buttons[i].idCommand != CMD_FILE_OPEN) ? 22 : 38;
			toolbar_.setButtonInfo(buttons[i].idCommand, tbi);
		}
	}
	delete[] commands;
	delete[] buttons;

	// ���o�[�ɏ悹��
	AutoZeroCB<REBARBANDINFOW> rbbi;
	const wstring caption = loadString(MSG_DIALOG__BUFFERBAR_CAPTION);
	rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;
	rbbi.fStyle = RBBS_GRIPPERALWAYS | RBBS_USECHEVRON;
	rbbi.wID = IDC_TOOLBAR;
	rbbi.hwndChild = toolbar_.getSafeHwnd();
	rbbi.cxMinChild = 0;
	rbbi.cyMinChild = 22;
	rebar_.insertBand(0, rbbi);

	// �c�[���o�[�̃V�F�u����������镝�̐ݒ�
	RECT rect;
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
	if(!searchDialog_->isWindowVisible()) {
		if(initializeFindTextFromEditor_) {	// �A�N�e�B�u�ȃG�f�B�^���猟���p�^�[�������o��
			Caret& caret = buffers_->getActiveView().getCaret();
			if(caret.isSelectionEmpty()) {
				String s;
				// TODO: obtain the word nearest from the caret position.
//				caret.getNearestWordFromCaret(0, 0, &s);
				searchDialog_->setDlgItemText(IDC_COMBO_FINDWHAT, s.c_str());
			} else if(caret.getAnchor().getLineNumber() != caret.getLineNumber())
				searchDialog_->setDlgItemText(IDC_COMBO_FINDWHAT, L"");
			else
				searchDialog_->setDlgItemText(IDC_COMBO_FINDWHAT, caret.getSelectionText().c_str());
		}
		searchDialog_->showWindow(SW_SHOW);
	} else
		searchDialog_->setActiveWindow();
	::SetFocus(searchDialog_->getDlgItem(IDC_COMBO_FINDWHAT));
}

/// @see ITemporaryMacroListener#temporaryMacroStateChanged
void Alpha::temporaryMacroStateChanged() {
	using ascension::presentation::Presentation;
	for(size_t i = 0; i < buffers_->getCount(); ++i) {
		Presentation& p = buffers_->getAt(i).getPresentation();
		for(Presentation::TextViewerIterator it = p.getFirstTextViewer(); it != p.getLastTextViewer(); ++it)
			(*it)->enableMouseOperation(!commandManager_->getTemporaryMacro().isDefining());
	}
	updateStatusBar(SBP_TEMPORARYMACRO);
}

/**
 * �X�e�[�^�X�o�[�̍X�V
 * @param panes �X�V����y�C��
 */
void Alpha::updateStatusBar(StatusBarPane panes) {
	if(!statusBar_.isWindowVisible())
		return;

	const int ICON_WIDTH = 16;
	Buffer& activeBuffer = buffers_->getActive();
	const EditorView& activeView = buffers_->getActiveView();

	// �p�[�c�̕����X�V����K�v����
	if(toBoolean(panes & (SBP_DOCUMENTTYPE | SBP_ENCODING))) {
		ClientDC dc = statusBar_.getDC();
		HFONT oldFont = static_cast<HFONT>(dc.selectObject(statusFont_));
		int parts[9];
		int borders[3];
		::RECT rect;

		statusBar_.setSimple(false);
		statusBar_.getWindowRect(rect);
		statusBar_.getBorders(borders);
		const int padding = (borders[0] + borders[2]) * 2 + 5;

		// �E�[
		parts[8] = rect.right - rect.left;
		// �T�C�Y�O���b�v
		parts[7] = parts[8] - (getMainWindow().isZoomed() ? 0 : (rect.bottom - rect.top - borders[1] * 2));
		// �i���[�C���O
		parts[6] = parts[7] - ICON_WIDTH - padding;
		// �㏑��/�}�����[�h
		const wstring overtypeMode = loadString(MSG_STATUS__OVERTYPE_MODE);
		const wstring insertMode = loadString(MSG_STATUS__INSERT_MODE);
		parts[5] = parts[6] - max(
			dc.getTextExtent(overtypeMode.data(), static_cast<int>(overtypeMode.length())).cx,
			dc.getTextExtent(insertMode.data(), static_cast<int>(insertMode.length())).cx) - padding;
		// �f�o�b�O���[�h
		parts[4] = parts[5] - ICON_WIDTH - padding;
		// �L�[�{�[�h�}�N��
		parts[3] = parts[4] - ICON_WIDTH - padding;
		// �G���R�[�f�B���O
		const wstring* encoding = getCodePageName(activeBuffer.getCodePage());
		parts[2] = parts[3] - dc.getTextExtent(encoding->data(), static_cast<int>(encoding->length())).cx - padding;
		// �����^�C�v
		// TODO: show current mode.
		parts[1] = parts[2] - 20 - padding;
//		parts[1] = parts[2] - dc.getTextExtent(activeBuffer.getDocumentType().data(),
//			static_cast<int>(activeBuffer.getDocumentType().length())).cx - padding;
		// �L�����b�g�ʒu
		const wstring caretPosition = activeView.getCurrentPositionString();
		parts[0] = parts[1] - dc.getTextExtent(caretPosition.data(), static_cast<int>(caretPosition.length())).cx - padding;

		statusBar_.setParts(countof(parts), parts);
		statusBar_.setText(8 | SBT_NOBORDERS, L"");
		dc.selectObject(oldFont);
	} else if(toBoolean(panes & SBP_POSITION)) {
		ClientDC dc = statusBar_.getDC();
		int parts[9];
		int borders[3];
		const wstring caretPosition = activeView.getCurrentPositionString();
		HFONT oldFont = static_cast<HFONT>(dc.selectObject(statusFont_));

		statusBar_.getBorders(borders);
		statusBar_.getParts(countof(parts), parts);
		const int oldWidth = parts[0];
		parts[0] = parts[1] - dc.getTextExtent(caretPosition.data(),
			static_cast<int>(caretPosition.length())).cx - (borders[0] + borders[2]) * 2 - 6;
		if(parts[0] != oldWidth)
			statusBar_.setParts(countof(parts), parts);
		dc.selectObject(oldFont);
	}

#define UPDATE_PANE(index, pane, text)	if(toBoolean(panes & pane))	statusBar_.setText(index, text)
#define UPDATE_PANE_WITH_ICON(index, pane, text, icon)	\
	if(toBoolean(panes & pane)) {						\
		statusBar_.setText(index, text);				\
		statusBar_.setTipText(index, text);				\
		statusBar_.setIcon(index, icon);				\
	}

	UPDATE_PANE_WITH_ICON(7, SBP_NARROWING,
		activeBuffer.isNarrowed() ? loadString(MSG_STATUS__NARROWING).c_str() : L"",
		activeBuffer.isNarrowed() ? narrowingIcon_ : 0);
	UPDATE_PANE(6, SBP_OVERTYPEMODE,
		loadString(activeView.getCaret().isOvertypeMode() ? MSG_STATUS__OVERTYPE_MODE : MSG_STATUS__INSERT_MODE).c_str());
//	UPDATE_PANE(5, SBP_DEBUGMODE, /*(activeDebugger_ != 0 && activeDebugger_->isDebugging()) ? loadString(MSG_DEBUGGING) :*/ L"");
	if(toBoolean(panes & SBP_TEMPORARYMACRO)) {
		if(commandManager_->getTemporaryMacro().getState() == TemporaryMacro::DEFINING) {
			UPDATE_PANE_WITH_ICON(4, SBP_TEMPORARYMACRO,
				loadString(MSG_STATUS__TEMP_MACRO_DEFINING).c_str(), temporaryMacroDefiningIcon_);
		} else if(commandManager_->getTemporaryMacro().getState() == TemporaryMacro::PAUSING) {
			UPDATE_PANE_WITH_ICON(4, SBP_TEMPORARYMACRO,
				loadString(MSG_STATUS__TEMP_MACRO_PAUSING).c_str(), temporaryMacroPausingIcon_);
		} else
			UPDATE_PANE_WITH_ICON(4, SBP_TEMPORARYMACRO, L"", 0);
	}
	UPDATE_PANE(3, SBP_ENCODING, getCodePageName(activeBuffer.getCodePage())->c_str());
	// TODO: show current mode.
	UPDATE_PANE(2, SBP_DOCUMENTTYPE, L" ");
//	UPDATE_PANE(2, SBP_DOCUMENTTYPE, activeBuffer.getDocumentType().c_str());
	UPDATE_PANE(1, SBP_POSITION, activeView.getCurrentPositionString());

#undef UPDATE_PANE
#undef UPDATE_PANE_WITH_ICON
}

/// �A�N�e�B�u�ȃo�b�t�@�Ɋ�Â��ă^�C�g���o�[�̍X�V
void Alpha::updateTitleBar() {
	static const Buffer* lastBuffer;
	static wstring titleCache;

	if(!getMainWindow().isWindow())
		return;

	const Buffer& activeBuffer = buffers_->getActive();
	wstring title = BufferList::getDisplayName(activeBuffer);

	if(&activeBuffer == lastBuffer && title == titleCache)
		return;
	titleCache = title;
	lastBuffer = &activeBuffer;

	// �^�C�g���o�[
//	title += L" - " IDS_APPFULLVERSION;
	title += L" - " IDS_APPNAME;
	getMainWindow().setWindowText(title.c_str());
}

/// @see ascension#searcher#IIncrementalSearcherListener#incrementalSearchAborted
void Alpha::incrementalSearchAborted() {
}

/// @see ascension#searcher#IIncrementalSearcherListener#incrementalSearchCompleted
void Alpha::incrementalSearchCompleted() {
	setStatusText(0);
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)))
		statusBar_.setFont(0);
}

/// @see ascension#searcher#IIncrementalSearcherListener#incrementalSearchPatternChanged
void Alpha::incrementalSearchPatternChanged(searcher::IIncrementalSearcherListener::Result result) {
	using namespace ascension::searcher;
	const IncrementalSearcher& isearch = buffers_->getEditorSession().getIncrementalSearcher();
	UINT msg;
	const bool forward = isearch.getDirection() == FORWARD;

	if(isearch.getPattern().empty()) {
		msg = forward ? MSG_STATUS__ISEARCH_EMPTY_PATTERN : MSG_STATUS__RISEARCH_EMPTY_PATTERN;
		setStatusText(loadString(msg).c_str());
		return;
	} else if(result == IIncrementalSearcherListener::FOUND)
		msg = forward ? MSG_STATUS__ISEARCH : MSG_STATUS__RISEARCH;
	else {
		if(result == IIncrementalSearcherListener::NOT_FOUND)
			msg = forward ? MSG_STATUS__ISEARCH_NOT_FOUND : MSG_STATUS__RISEARCH_NOT_FOUND;
		else
			msg = forward ? MSG_STATUS__ISEARCH_BAD_PATTERN : MSG_STATUS__RISEARCH_BAD_PATTERN;
		buffers_->getActiveView().beep();
	}

	String prompt = loadString(msg, MARGS % isearch.getPattern());
	replace_if(prompt.begin(), prompt.end(), bind2nd(equal_to<wchar_t>(), L'\t'), L' ');
	setStatusText(prompt.c_str());
}

/// @see ascension#searcher#IIncrementalSearcherListener#incrementalSearchStarted
void Alpha::incrementalSearchStarted() {
	// �X�e�[�^�X�o�[�̃t�H���g���G�f�B�^�̂��̂Ɉ�v������
	if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)))
		statusBar_.setFont(editorFont_);
}
/*
/// @see IEditViewEventListener::onInvokeURILink
void Alpha::onInvokeURILink(const char_t* uri) {
	::ShellExecuteW(0, 0, uri, 0, 0, SW_SHOWNORMAL);
}
*/
/// @see WM_CLOSE
bool Alpha::onClose() {
	// TODO: invoke application teardown.
//	eventHandlerScript_->invoke(OLESTR("OnAlphaTerminating"), params);
	if(buffers_->closeAll(true)) {
		saveINISettings();
		getMainWindow().destroyWindow();
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

	delete menu_;
	delete mruManager_;
//	toolbar_.destroyWindow();
//	statusBar_.destroyWindow();

	::PostQuitMessage(0);	// STA �X���b�h�I��
}

/// @see WM_DRAWITEM
void Alpha::onDrawItem(UINT, const DRAWITEMSTRUCT& drawItem) {
	if(drawItem.CtlType != ODT_MENU)	// �����_�ł̓��j���[�̕`��̂�
		return;

	// wingdi.h �� GetXValue �͌^���S�łȂ�
#define R_VALUE(rgb) static_cast<uchar>((rgb & 0x000000FF) >> 0)
#define G_VALUE(rgb) static_cast<uchar>((rgb & 0x0000FF00) >> 8)
#define B_VALUE(rgb) static_cast<uchar>((rgb & 0x00FF0000) >> 16)

	AutoDC dc(drawItem.hDC);
	const RECT& itemRect = drawItem.rcItem;
	RECT captionRect;
	HPEN oldPen, pen;
	HBRUSH oldBrush, brush;
	wchar_t* caption = 0;
	wchar_t* accel = 0;	// �A�N�Z�����[�^�A�t�@�C����

	const bool checked = toBoolean(drawItem.itemState & ODS_CHECKED);
	const bool disabled = toBoolean(drawItem.itemState & ODS_DISABLED);
	const bool selected = toBoolean(drawItem.itemState & ODS_SELECTED);
	const CommandID id = drawItem.itemID;
	const COLORREF highlightFgColor = ::GetSysColor(COLOR_HIGHLIGHT);
	const COLORREF highlightBgColor =
		RGB(R_VALUE(highlightFgColor) / 3 + 170, G_VALUE(highlightFgColor) / 3 + 170, B_VALUE(highlightFgColor) / 3 + 170);
	const COLORREF highlightIconBgColor =
		RGB((R_VALUE(highlightBgColor) + 0xFF) / 2, (G_VALUE(highlightBgColor) + 0xFF) / 2, (B_VALUE(highlightBgColor) + 0xFF) / 2);

	// ���̃��j���[�̕`��f�U�C���͓K���ɍl�������̂���
	// WTL �̂悤�Ȃ��̂��W���Ǝv����̂ŁA�����͕ύX���������ǂ��Ǝv��

	// �w�i�̕`��
	if(selected) {
		pen = ::CreatePen(PS_SOLID, 1, highlightFgColor);
		brush = ::CreateSolidBrush(highlightBgColor);
		oldPen = dc.selectObject(pen);
		oldBrush = dc.selectObject(brush);
		dc.rectangle(itemRect);
		dc.selectObject(oldPen);
		dc.selectObject(oldBrush);
		::DeleteObject(pen);
		::DeleteObject(brush);
	} else {
		dc.fillSolidRect(itemRect, ::GetSysColor(COLOR_MENU));
//		dc.fillSolidRect(itemRect.left, itemRect.top,
//			itemRect.bottom - itemRect.top, itemRect.bottom - itemRect.top,
//			::GetSysColor(COLOR_3DFACE));
	}

	if(id >= CMD_FILE_MRULIST_START && id < CMD_FILE_MRULIST_END) {	// MRU ���X�g
		const wchar_t* number = getMenuLabel(id);
		const wchar_t* filePath = wcsstr(number, L"  ") + 2;
		const wchar_t* fileName = ::PathFindFileNameW(filePath);
		captionRect = itemRect;
		captionRect.left += 14;
		captionRect.right -= 14;
		dc.setBkMode(TRANSPARENT);

		// �ԍ��̕`��
		dc.drawText(number, static_cast<int>(filePath - number), captionRect, DT_SINGLELINE | DT_VCENTER);
		dc.drawText(number, static_cast<int>(filePath - number), captionRect, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER);
		::SetRect(&captionRect, captionRect.right, itemRect.top, itemRect.right, itemRect.bottom);

		// �p�X�̕`��
		if(filePath != fileName) {
			dc.drawText(filePath,
				static_cast<int>(fileName - filePath), captionRect, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
			dc.drawText(filePath,
				static_cast<int>(fileName - filePath), captionRect, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
			::SetRect(&captionRect, captionRect.right, itemRect.top, itemRect.right, itemRect.bottom);
		}

		// �t�@�C�����̕`��
		dc.drawText(fileName, -1, captionRect, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
		dc.drawText(fileName, -1, captionRect, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

		// �t�@�C�����̉��ɉ���������
		pen = ::CreatePen(PS_SOLID, 1, dc.getTextColor());
		oldPen = dc.selectObject(pen);
		dc.moveTo(captionRect.left, captionRect.bottom - 1);
		dc.lineTo(captionRect.right, captionRect.bottom - 1);
		dc.selectObject(oldPen);
		::DeleteObject(pen);
	} else {
		// �A�C�R���̕`��
		const bool isBuffers = id >= CMD_VIEW_BUFFERLIST_START && id < CMD_VIEW_BUFFERLIST_END;
		const size_t icon = !isBuffers ? commandManager_->getIconIndex(id) : id - CMD_VIEW_BUFFERLIST_START;
		RECT iconRect;

		iconRect.left = itemRect.left + (itemRect.bottom - itemRect.top - 16) / 2 - 1;
		iconRect.top = itemRect.top + (itemRect.bottom - itemRect.top - 16) / 2 - 1;
		iconRect.right = iconRect.left + 18;
		iconRect.bottom = iconRect.top + 18;
		if(icon != -1 || isBuffers) {	// �A�C�R�������p�ł���ꍇ
			if(checked) {	// �`�F�b�N���
				pen = ::CreatePen(PS_SOLID, 1, highlightFgColor);
				brush = ::CreateSolidBrush(highlightIconBgColor);
				oldPen = dc.selectObject(pen);
				oldBrush = dc.selectObject(brush);
				dc.rectangle(iconRect);
				dc.selectObject(oldPen);
				dc.selectObject(oldBrush);
				::DeleteObject(pen);
				::DeleteObject(brush);
			}
			::InflateRect(&iconRect, -1, -1);
			if(isBuffers)
				dc.drawIconEx(iconRect.left, iconRect.top, buffers_->getBufferIcon(icon), 16, 16, 0, 0, DI_NORMAL);
			else {
				const CommandManager::IconState state =
					disabled ? CommandManager::ICONSTATE_DISABLED :
						(selected ? CommandManager::ICONSTATE_HOT : CommandManager::ICONSTATE_NORMAL);
				commandManager_->getImageList(state).draw(dc.get(), static_cast<int>(icon), iconRect.left, iconRect.top, ILD_NORMAL);
			}
			::InflateRect(&iconRect, 1, 1);
		} else if(checked) {
			const int checkBoxWidth = itemRect.bottom - itemRect.top;
			pen = ::CreatePen(PS_SOLID, 1, highlightFgColor);
			brush = ::CreateSolidBrush(highlightIconBgColor);
			oldPen = dc.selectObject(pen);
			oldBrush = dc.selectObject(brush);
			dc.rectangle(itemRect.left + 1, itemRect.top + 1, itemRect.left + itemRect.bottom - itemRect.top - 1, itemRect.bottom - 1);
			dc.selectObject(oldPen);
			dc.selectObject(oldBrush);
			::DeleteObject(pen);
			::DeleteObject(brush);
			pen = ::CreatePen(PS_SOLID, 1, RGB(0x00, 0x00, 0x00));	// �v��񂩂�?
			oldPen = dc.selectObject(pen);
			dc.moveTo(itemRect.left + checkBoxWidth / 2 - 3, itemRect.top + checkBoxWidth / 2 - 1);	// �`�F�b�N�}�[�N
			dc.lineTo(itemRect.left + checkBoxWidth / 2 - 1, itemRect.top + checkBoxWidth / 2 + 1);
			dc.lineTo(itemRect.left + checkBoxWidth / 2 + 4, itemRect.top + checkBoxWidth / 2 - 4);
			dc.moveTo(itemRect.left + checkBoxWidth / 2 - 3, itemRect.top + checkBoxWidth / 2 + 0);
			dc.lineTo(itemRect.left + checkBoxWidth / 2 - 1, itemRect.top + checkBoxWidth / 2 + 2);
			dc.lineTo(itemRect.left + checkBoxWidth / 2 + 4, itemRect.top + checkBoxWidth / 2 - 3);
			dc.selectObject(oldPen);
			::DeleteObject(pen);
		}

		if(id == 0	// ��؂���̕`��
				&& (drawItem.itemData == 0 || wcslen(reinterpret_cast<wchar_t*>(drawItem.itemData)) == 0)) {
			oldPen = dc.selectObject(::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW)));

			dc.moveTo(itemRect.left + 1, itemRect.top + 1);
			dc.lineTo(itemRect.right - 1, itemRect.top + 1);
			::DeleteObject(dc.selectObject(oldPen));
			dc.selectObject(::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DHIGHLIGHT)));
			dc.moveTo(itemRect.left + 1, itemRect.top + 2);
			dc.lineTo(itemRect.right - 1, itemRect.top + 2);
			::DeleteObject(dc.selectObject(oldPen));
		} else {	// �L���v�V�����̕`��
			dc.setBkMode(TRANSPARENT);
			captionRect = itemRect;
//			if(reinterpret_cast<HMENU>(drawItem.hwndItem) != menu_->getSafeHmenu()) {
				COLORREF captionColor = ::GetSysColor(disabled ? COLOR_GRAYTEXT : COLOR_MENUTEXT);
				const wchar_t* orgCaption = (drawItem.itemData == 0) ?
					getMenuLabel(drawItem.itemID) : reinterpret_cast<const wchar_t*>(drawItem.itemData);
				if(id != 0 && commandManager_->getLastCommand() == id)
					captionColor = RGB(R_VALUE(captionColor),
						(G_VALUE(captionColor) + 0xFF) / 2, B_VALUE(captionColor));
				dc.setTextColor(captionColor);
				captionRect.left = iconRect.right + 6;
				captionRect.right -= 6;
				caption = new wchar_t[wcslen(orgCaption) + 1];
				wcscpy(caption, orgCaption);
				accel = wcschr(caption, L'\t');
				if(accel != 0) {
					*accel = 0;
					dc.drawText(caption, -1, captionRect, DT_SINGLELINE | DT_VCENTER);
					dc.drawText(accel + 1, -1, captionRect, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
				} else
					dc.drawText(caption, -1, captionRect, DT_SINGLELINE | DT_VCENTER);
				delete[] caption;
//			} else {	// ���j���[�o�[�̏ꍇ
//				if(selected || drawItem.itemState & ODS_HOTLIGHT) {
//					dc.fillSolidRect(&captionRect, ::GetSysColor(COLOR_HIGHLIGHT));
//					dc.setTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
//				} else {
//					dc.fillSolidRect(&captionRect, ::GetSysColor(COLOR_BTNFACE));
//					dc.setTextColor(::GetSysColor(COLOR_MENUTEXT));
//				}
//				dc.drawText(reinterpret_cast<const wchar_t*>(drawItem.itemData),
//					-1, &captionRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
//			}
		}
	}

#undef R_VALUE
#undef G_VALUE
#undef B_VALUE
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
		Menu popup(menu);
		const int c = popup.getItemCount();
		for(int i = 0; i < c; ++i) {
			const CommandID id = popup.getMenuItemID(i);
			popup.enableMenuItem<Menu::BY_POSITION>(i, commandManager_->isEnabled(id, true));
			popup.checkMenuItem<Menu::BY_POSITION>(i, commandManager_->isChecked(id));
		}
	}
}

/// @see WM_MEASUREITEM
void Alpha::onMeasureItem(UINT id, MEASUREITEMSTRUCT& mi) {
	// ���j���[
	if(mi.CtlType == ODT_MENU) {
		if(mi.itemID == 0	// ��؂��
				&& (mi.itemData == 0 || wcslen(reinterpret_cast<wchar_t*>(mi.itemData)) == 0)) {
			mi.itemHeight = 4;
			mi.itemWidth = 30;	// ���̒l�͑����g���Ȃ�
		} else {
			// ���j���[�̃t�H���g���擾
			AutoZeroCB<NONCLIENTMETRICSW> ncm;
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
			if(mi.itemID == CMD_FILE_CLOSE)	// ��������
				ncm.lfMenuFont.lfWeight = FW_BOLD;
			HFONT menuFont = ::CreateFontIndirectW(&ncm.lfMenuFont);
			ClientDC dc = getMainWindow().getDC();
			HFONT oldFont = static_cast<HFONT>(dc.selectObject(menuFont));
			RECT rect;
			dc.drawText(mi.itemData == 0 ?
				getMenuLabel(mi.itemID) : reinterpret_cast<wchar_t*>(mi.itemData),
				-1, rect, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
			dc.selectObject(oldFont);
			::DeleteObject(menuFont);

			if(mi.itemID == 0)	// �|�b�v�A�b�v�̏ꍇ�A���̕��𑫂�
				rect.right += 24;
			if(mi.itemID > COMMAND_END) {
				mi.itemHeight = rect.bottom - rect.top + 4;
				mi.itemWidth = rect.right - rect.left + 28;
			} else {
				mi.itemHeight = max(rect.bottom - rect.top, 20L);
				mi.itemWidth = rect.right - rect.left + ((mi.itemID >= COMMAND_START) ? 24 : 0);
			}
		}
	}
}

/// @see WM_MENUCHAR
LRESULT Alpha::onMenuChar(wchar_t ch, UINT flags, Menu& menu) {
	const UINT c = menu.getItemCount();
	Menu::ItemInfo item;

	item.fMask = MIIM_FTYPE | MIIM_ID;
	if(ch >= L'a' && ch <= L'z')	// �啶���ɂ���
		ch -= 0x20;
	for(UINT i = 0; i < c; ++i) {
		menu.getMenuItemInfo<Menu::BY_POSITION>(i, item);
		if(item.wID != 0 && !toBoolean(item.fType & MFT_SEPARATOR)) {
			const wchar_t* accel = wcschr(getMenuLabel(item.wID), L'&');
			if(accel != 0) {
				if(accel[1] == ch)
					return (i | 0x00020000);
			}
		}
	}
	return MNC_IGNORE;
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
			mruManager_->getPopupMenu().trackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow());
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
			NMTTDISPINFOW& nmttdi = *reinterpret_cast<NMTTDISPINFOW*>(&nmhdr);
			nmttdi.hinst = get();
			wstring name = commandManager_->getName(static_cast<CommandID>(nmhdr.idFrom));
			const wstring key = keyboardMap_.getKeyString(static_cast<CommandID>(nmhdr.idFrom), false);
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
	Menu popup;
	::POINT pt = {chevron.rc.left, chevron.rc.bottom};
	AutoZeroCB<::TBBUTTONINFOW> tbbi;
	Menu::ItemInfo item;

    tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE;
	item.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE;
	for(; i < buttonCount; ++i) {
		::SendMessage(rbi.hwndChild, TB_GETBUTTONINFOW, i, reinterpret_cast<LPARAM>(&tbbi));
		if(toBoolean(tbbi.fsStyle & TBSTYLE_SEP))
			popup.appendSeparator(MFT_OWNERDRAW);
		else {
			item.fType = MFT_OWNERDRAW;
			item.fState = commandManager_->isEnabled(tbbi.idCommand, true) ? MFS_ENABLED : MFS_DISABLED;
			item.fState |= (commandManager_->isChecked(tbbi.idCommand)) ? MFS_CHECKED : 0;
			item.wID = tbbi.idCommand;
			item.dwItemData = 0/*reinterpret_cast<DWORD>(getControls::MenuLabel(tbbi.idCommand))*/;
			popup.insertMenuItem<Menu::BY_POSITION>(popup.getItemCount(), item);
		}
	}
	rebar_.clientToScreen(pt);
	popup.trackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, getMainWindow());
}

/// @see Window::onSetCursor
bool Alpha::onSetCursor(HWND hWnd, UINT nHitTest, UINT message) {
	POINT pt;		// �J�[�\���ʒu
	RECT clientRect, statusBarRect;

	getMainWindow().getClientRect(clientRect);
	if(statusBar_.isWindowVisible())	statusBar_.getWindowRect(statusBarRect);
	else								::SetRect(&statusBarRect, 0, 0, 0, 0);
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

	::DeleteObject(statusFont_);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
	statusFont_ = ::CreateFontIndirectW(&ncm.lfStatusFont);
	updateStatusBar(SBP_ALL);
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

	if(statusBar_.isWindowVisible()) {
		statusBar_.sendMessage(WM_SIZE, cx, cy);
		statusBar_.getWindowRect(statusBarRect);
		updateStatusBar(SBP_ALL);
	} else
		::SetRect(&statusBarRect, 0, 0, 0, 0);

	if(rebar_.isWindowVisible()) {
		rebar_.sendMessage(WM_SIZE, cx, cy);
		rebar_.getWindowRect(rebarRect);
		toolbar_.sendMessage(WM_SIZE, cx, rebarRect.bottom - rebarRect.top - 2);
	} else
		::SetRect(&rebarRect, 0, 0, 0, 0);

	editorRect.left =  0;
	editorRect.top = rebarRect.bottom - rebarRect.top;
	editorRect.right = cx;
	editorRect.bottom = cy
		- (statusBar_.isWindowVisible() ? statusBarRect.bottom - statusBarRect.top : 0);
//	if(outputWindow.isWindow() && outputWindow.isWindowVisible())
//		editorRect.bottom -= outputWndHeight_;
	if(buffers_->getEditorWindow().isWindow())
		buffers_->getEditorWindow().moveWindow(editorRect, true);

//	if(outputWindow_.isWindow()) {
//		outputWindow_.moveWindow(0, editorRect.bottom + 2, cx, outputWndHeight_);
//		outputWindow_.showWindow(SW_SHOW);
//	}
}

/// @see WM_TIMER
void Alpha::onTimer(UINT timerID) {
	if(timerID == ID_TIMER_QUERYCOMMAND && buffers_->getCount() != 0) {
		// �c�[���o�[�A�C�e���̗L����/������
		if(toolbar_.isWindowVisible()) {
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
