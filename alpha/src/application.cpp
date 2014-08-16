/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2009, 2014
 */

#include "application.hpp"
#include "ambient.hpp"
#include "buffer-list.hpp"
#include "input.hpp"
//#include "ui.hpp"
#include "editor-view.hpp"
#include "editor-window.hpp"
#include "function-pointer.hpp"
//#include "search.hpp"
//#include <ascension/text-editor.hpp>
#include <ascension/corelib/regex.hpp>
#include <ascension/graphics/native-conversion.hpp>
#include <gtkmm/fontchooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <glibmm/i18n.h>
//#include "../resource/messages.h"
//#include <manah/win32/dc.hpp>
//#include <manah/win32/gdi-object.hpp>
//#include <manah/win32/ui/wait-cursor.hpp>
#include <algorithm>
#include <codecvt>
#include <fstream>
//#include <commdlg.h>	// ChooseFont
//#include <shlwapi.h>	// PathXxxx
//#include <comcat.h>		// ICatInformation
#ifdef BOOST_OS_WINDOWS
#	include <CommCtrl.h>	// InitMUILanguage
#	include <Ole2.h>		// OleInitialize, OleUninitialize
#endif
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <CommDlg.h>	// ChooseFontW
#	include <Dlgs.h>
#endif


#ifdef BOOST_OS_WINDOWS
namespace alpha {
	namespace {
		void callOleUninitialize() {
			::OleUninitialize();
		}
	}
}
#endif

/// The entry point.
int main(int argc, char* argv[]) {
	int	exitCode = 0/*EXIT_SUCCESS*/;

#ifdef BOOST_OS_WINDOWS
	// Shift �L�[�������Ȃ���N������Ɖp�ꃂ�[�h�ɂȂ�悤�ɂ��Ă݂�
	if(ascension::win32::boole(::GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
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
#endif // _DEBUG

	// NT �n�����ׂ�
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(decltype(osvi));
	::GetVersionExA(&osvi);
	if(!ascension::win32::boole(osvi.dwPlatformId & VER_PLATFORM_WIN32_NT)) {
		Gtk::MessageDialog dialog(_("Alpha does not support your platform."), false, Gtk::MESSAGE_ERROR);
		dialog.run();
		return exitCode = -1;
	}
	ascension::win32::Handle<HANDLE>::Type mutex(::CreateMutexW(0, false, IDS_APPFULLVERSION), &::CloseHandle);

	// �ȒP�ȑ��d�N���}�~ (Ctrl �L�[�������Ȃ���N������Ƒ��d�N������悤�ɂ��Ă݂�)
	if(::GetLastError() != ERROR_ALREADY_EXISTS || ascension::win32::boole(::GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
		::OleInitialize(nullptr);	// enter STA and initialize high-level services
		std::atexit(&alpha::callOleUninitialize);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		ascension::win32::ui::initCommonControls(ICC_COOL_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_WIN95_CLASSES);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#endif
		alpha::ambient::Interpreter::instance().install();
		alpha::ambient::Interpreter::instance().toplevelPackage();

		const Glib::RefPtr<alpha::Application> application(alpha::Application::create());
		exitCode = application->run(argc, argv);
#ifdef BOOST_OS_WINDOWS
	} else {	// pass the command line arguments to the existing process
		HWND existWnd = ::FindWindowW(IDS_APPNAME, nullptr);
		while(!ascension::win32::boole(::IsWindow(existWnd))) {
			::Sleep(1000);
			existWnd = ::FindWindowW(IDS_APPNAME, nullptr);
		}
		const WCHAR* const commandLine = ::GetCommandLineW();
		const std::size_t commandLineLength = std::wcslen(commandLine);
		std::unique_ptr<WCHAR[]> data(new WCHAR[commandLineLength + 1 + MAX_PATH]);
		::GetCurrentDirectoryW(MAX_PATH, data.get());
		std::wcscpy(data.get() + MAX_PATH, commandLine);
		ascension::win32::AutoZero<COPYDATASTRUCT> cd;
		cd.lpData = data.get();
		cd.cbData = static_cast<DWORD>(sizeof(WCHAR) * (commandLineLength + 1 + MAX_PATH));
		::SendMessageW(existWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cd));
		::Sleep(300);
		::SetForegroundWindow(existWnd);
	}
#endif

	return exitCode;
}

namespace alpha {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	namespace {
		/// Hook procedure for @c ChooseFontW.
		UINT_PTR CALLBACK chooseFontHookProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
			if(message == WM_COMMAND && LOWORD(wParam) == psh3) {	// "Apply" button
				LOGFONTW lf;
				::SendMessageW(dialog, WM_CHOOSEFONT_GETLOGFONT, 0, reinterpret_cast<LPARAM>(&lf));
				Application::instance().setFont(lf);
				return true;
			} else if(message == WM_INITDIALOG) {
				::EnableWindow(::GetDlgItem(dialog, stc2), false);	// disable "Font style"
				::EnableWindow(::GetDlgItem(dialog, cmb2), false);
			}
			return 0;
		}
	} // namespace @0
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

	// Application ////////////////////////////////////////////////////////////////////////////////////////////////////

	Glib::RefPtr<Application> Application::instance_;

	/// Private constructor.
	Application::Application() : Gtk::Application("alpha", Gio::APPLICATION_HANDLES_OPEN) {
		Glib::set_application_name("alpha");
//		searchDialog_.reset(new ui::SearchDialog());	// ctor of SearchDialog calls Alpha
//		onSettingChange(0, 0);	// statusFont_ �̏�����
	}

	/// Shows font-chooser user interface and changes the font of the selected editor.
	void Application::changeFont() {
		EditorView& activeView = EditorPanes::instance().activePane().selectedView();
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		LOGFONTW font;
		ascension::win32::AutoZeroSize<CHOOSEFONTW> cf;

		::GetObjectW(editorFont_, sizeof(decltype(LOGFONTW)), &font);
		cf.hwndOwner = getMainWindow().use();
		cf.lpLogFont = &font;
		cf.lpfnHook = chooseFontHookProc;
		cf.Flags = CF_APPLY | CF_ENABLEHOOK | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_SCREENFONTS;
		cf.hInstance = use();

		if(ascension::win32::boole(::ChooseFontW(&cf))) {
			font.lfItalic = false;
			font.lfWeight = FW_REGULAR;
			setFont(font);
		}
#else
		Gtk::FontChooserDialog dialog(Glib::ustring(), window());
		dialog.set_font_desc(ascension::graphics::toNative<Pango::FontDescription>(activeView.textRenderer().defaultFont()->describe()));
		if(dialog.run() == Gtk::RESPONSE_ACCEPT)
			setFont(ascension::graphics::fromNative<ascension::graphics::font::FontDescription>(dialog.get_font_desc()));
#endif
	}

	/// Creates a new @c Application instance.
	Glib::RefPtr<Application> Application::create() {
		if(instance_)
			throw ascension::IllegalStateException("");
		instance_ = Glib::RefPtr<Application>(new Application);
		return instance_;
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	/// @see alpha#win32#Application#initInstance
	bool Application::initInstance(int showCommand) {
		// setup the script engine
//		ambient::ScriptEngine::instance().installModules();

	// register the top level window class
		ascension::win32::AutoZeroSize<WNDCLASSEXW> wc;
		wc.style = CS_DBLCLKS/* | CS_DROPSHADOW*/;
		wc.lpfnWndProc = Alpha::appWndProc;
//		wc.cbClsExtra = 0;
//		wc.cbWndExtra = 0;
		wc.hInstance = use();
		wc.hIcon = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
		wc.hIconSm = static_cast<HICON>(loadImage(IDR_ICONS, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
		wc.hCursor = loadStandardCursor(MAKEINTRESOURCEW(32512));	// IDC_ARROW
		wc.hbrBackground = manah::win32::ui::BrushHandleOrColor(COLOR_3DFACE).get();
//		wc.lpszMenuName = nullptr;
		wc.lpszClassName = IDS_APPNAME;
		if(!ascension::win32::boole(::RegisterClassExW(&wc)))
			return false;

		static win32::ui::Window applicationWindow;

/*		// �R�[�h�y�[�W���̓ǂݍ���
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
//					::UnlockResource(buffer);
				}
				::FreeResource(buffer);
			}
		}
*/
		// ����̏�����ǂݍ���
		try {
			ascension::text::Newline newline =
				static_cast<ascension::text::Newline>(readIntegerProfile(L"File", L"defaultNewline", ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED));
			if(newline == ascension::text::Newline::USE_INTRINSIC_VALUE)
				newline = ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED;
//			ascension::kernel::Document::setDefaultCode(readIntegerProfile(L"File", L"defaultCodePage", ::GetACP()), newline);
		} catch(const std::invalid_argument&) {
			// TODO: Report the error to user.
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
//		setupToolbar();
		BufferList::instance().createBar(rebar_);

		// �G�f�B�^�E�B���h�E�̍쐬
		// (WS_CLIPCHILDREN ��t����ƕ����E�B���h�E�̃T�C�Y�ύX�g���s���ɂȂ�...)
		EditorWindows::instance().create(getMainWindow().use(),
			win32::ui::DefaultWindowRect(), WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, *(new EditorWindow));
		assert(EditorWindows::instance().isWindow());

		// ��ʐݒ�̓ǂݍ���
		loadSettings();

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

//		setupMenus();
		if(!ascension::win32::boole(readIntegerProfile(L"View", L"visibleToolbar", true)))
			rebar_.showBand(rebar_.idToIndex(IDC_TOOLBAR), false);
		if(!ascension::win32::boole(readIntegerProfile(L"View", L"visibleStatusBar", true)))
			statusBar_.hide();
		if(!ascension::win32::boole(readIntegerProfile(L"View", L"visibleBufferBar", true)))
			rebar_.showBand(rebar_.idToIndex(IDC_BUFFERBAR), false);
		applicationWindow.show(showCommand);

		// �R�}���h���C������^����ꂽ�t�@�C�����J��
		WCHAR cd[MAX_PATH];
		::GetCurrentDirectoryW(MAX_PATH, cd);
		parseCommandLine(cd, ::GetCommandLineW());

		// ...
		statusBar_.setText(0);

		// �A�E�g�v�b�g�E�B���h�E�̍쐬
//		outputWindow.create(getMainWindow());
//		outputWindow.writeLine(OTT_GENERAL, IDS_APPFULLVERSION);

		// �c�[���_�C�A���O�̍쐬
		searchDialog_->doModeless(applicationWindow.get(), false);
		pushModelessDialog(searchDialog_->get());
		if(toBoolean(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
//			searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//			searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
		}

		applicationWindow.setFocus();
		return true;
	}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

	/// Loads settings from the file.
	void Application::loadSettings() {
		// �\���Ɋւ���ݒ�
		ascension::graphics::font::FontDescription fd(*ascension::graphics::font::FontFamily::createMonospaceInstance(), 0.0);
		readStructureProfile(L"View", L"Font.default", fd);	// this can fail
		setFont(fd);

		// Migemo DLL & �����p�X
		const Glib::ustring migemoRuntimePath(boost::get_optional_value_or(readStringProfile("Find", "migemoRuntimePath"), Glib::ustring()));
		const Glib::ustring migemoDictionaryPath(boost::get_optional_value_or(readStringProfile("Find", "migemoDictionaryPath"), Glib::ustring()));
		if(!migemoRuntimePath.empty() && !migemoDictionaryPath.empty()) {
			ascension::regex::MigemoPattern::initialize(
				boost::filesystem::path(migemoRuntimePath.c_str(), std::codecvt_utf8_utf16<wchar_t>()),
				boost::filesystem::path(migemoDictionaryPath.c_str(), std::codecvt_utf8_utf16<wchar_t>()));
		}

		// ����������̗���
		std::list<ascension::String> findWhats, replacesWiths;
		for(unsigned short i = 0; i < 16; ++i) {
			std::ostringstream keyName("findWhat(");
			keyName << i << ")";
			const boost::optional<Glib::ustring> v(readStringProfile("Find", keyName.str()));
			if(v == boost::none || v->empty())
				break;
			findWhats.push_back(ascension::fromGlibUstring(*v));
		}
		for(unsigned short i = 0; i < 16; ++i) {
			std::ostringstream keyName("replaceWith(");
			keyName << i << ")";
			const boost::optional<Glib::ustring> v(readStringProfile("Find", keyName.str()));
			if(v == boost::none || v->empty())
				break;
			replacesWiths.push_back(ascension::fromGlibUstring(*v));
		}
		ascension::searcher::TextSearcher& s = BufferList::instance().editorSession().textSearcher();
		s.setMaximumNumberOfStoredStrings(16);
		s.setStoredStrings(std::begin(findWhats), std::end(findWhats), false);
		s.setStoredStrings(std::begin(replacesWiths), std::end(replacesWiths), true);
	}

	/// Overrides @c Gio#Application#on_activate method.
	void Application::on_activate() {
		assert(window_.get() == nullptr);
		window_.reset(new ui::MainWindow);

		add_window(*window_);
		window_->show();
	}

	/// Overrides @c Gio#Application#on_open method.
	void Application::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) {
		// TODO: Not implemented.
		return on_open(files, hint);
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

	/// Saves the settings into the file.
	void Application::saveSettings() {
		// visibility of bars
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		ascension::win32::AutoZero<REBARBANDINFOW> rbbi;
		rbbi.fMask = RBBIM_STYLE;
		rebar_.getBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
		writeIntegerProfile(L"View", L"visibleToolbar", ascension::win32::boole(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
		rebar_.getBandInfo(rebar_.idToIndex(IDC_BUFFERBAR), rbbi);
		writeIntegerProfile(L"View", L"visibleBufferBar", ascension::win32::boole(rbbi.fStyle & RBBS_HIDDEN) ? 0 : 1);
		writeIntegerProfile(L"View", L"visibleStatusBar", statusBar_.isVisible() ? 1 : 0);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

		// search and replacement strings
		const ascension::searcher::TextSearcher& s = BufferList::instance().editorSession().textSearcher();
		for(std::size_t i = 0, c = s.numberOfStoredPatterns(); ; ++i) {
			std::ostringstream keyName("findWhat(");
			keyName << i << ")";
			if(i < c)
				writeStringProfile("Find", keyName.str(), s.pattern(i).c_str());
			else {
				writeStringProfile("Find", keyName.str(), "");
				break;
			}
		}
		for(std::size_t i = 0, c = s.numberOfStoredReplacements(); ; ++i) {
			std::ostringstream keyName("replaceWith(");
			keyName << i << ")";
			if(i < c)
				writeStringProfile("Find", keyName.str(), s.replacement(i).c_str());
			else {
				writeStringProfile("Find", keyName.str(), "");
				break;
			}
		}
	}

/// �S�ẴG�f�B�^�ƈꕔ�̃R���g���[���ɐV�����t�H���g��ݒ�
void Application::setFont(const ascension::graphics::font::FontDescription& font) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	LOGFONTW lf = font;

	lf.lfWeight = FW_NORMAL;
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

	// �ꕔ�̃R���g���[���ɂ��ݒ�
	if(ascension::win32::boole(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
//		if(bookmarkDialog_.get() != 0 && bookmarkDialog_->isWindow())
//			bookmarkDialog_->sendItemMessage(IDC_LIST_BOOKMARKS, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		if(searchDialog_.get() != 0 && searchDialog_->isWindow()) {
//			searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//			searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//		}
	}

	// INI �t�@�C���ɕۑ�
	writeStructureProfile("View", "Font.default", lf);

	// ���� <-> �ϕ��ŕ\�L��ς���K�v������
	statusBar_.adjustPaneWidths();
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
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

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
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
	ascension::updateSystemSettings();
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
#endif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

	bool Application::teardown(bool callHook /* = true */) {
		if(callHook) {
			boost::python::object toplevel(ambient::Interpreter::instance().toplevelPackage());
			if(::PyObject_HasAttrString(toplevel.ptr(), "about_to_be_killed_hook") != 0) {
				try {
					if(::PyObject_IsTrue(toplevel.attr("about_to_be_killed_hook")().ptr()) == 0)
						return false;
				} catch(const boost::python::error_already_set&) {
					ambient::Interpreter::instance().handleException();
				}
			}
		}
		saveSettings();
		quit();
//		::PostQuitMessage(0);
		return true;
	}


	ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
		ambient::Interpreter& interpreter = ambient::Interpreter::instance();
		boost::python::scope scope(interpreter.toplevelPackage());

		boost::python::def("kill_alpha",
			ambient::makeFunctionPointer([](bool callHook) -> bool {
				return Application::instance()->teardown(callHook);
			}),
			boost::python::arg("call_hook") = true);
	ALPHA_EXPOSE_EPILOGUE()
}
