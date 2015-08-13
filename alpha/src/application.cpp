/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2009, 2014-2015
 */

#include "application.hpp"
#include "ambient.hpp"
#include "buffer-list.hpp"
#include "input.hpp"
//#include "ui.hpp"
#include "editor-panes.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
//#include "search.hpp"
//#include <ascension/text-editor.hpp>
#include <ascension/corelib/regex.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/native-conversion.hpp>
#include <ascension/viewer/text-area.hpp>
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

#ifdef _DEBUG
#	include <boost/log/core.hpp>
#	include <boost/log/expressions.hpp>
#	include <boost/log/sinks.hpp>
#	include <boost/log/sinks/debug_output_backend.hpp>
#	include <boost/make_shared.hpp>
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
#ifdef _DEBUG
	{
		auto loggingBackend(boost::make_shared<boost::log::sinks::debug_output_backend>());
		auto loggingSink(boost::make_shared<boost::log::sinks::synchronous_sink<boost::log::sinks::debug_output_backend>>(loggingBackend));
		loggingSink->set_formatter(
			boost::log::expressions::format("%1%(%2%) [%3%] : %4%")
			% boost::log::expressions::attr<std::string>("file")
			% boost::log::expressions::attr<int>("line")
			% boost::log::expressions::attr<std::string>("function")
			% boost::log::expressions::message);
//		boost::log::add_common_attributes();
		boost::log::core::get()->add_sink(loggingSink);
	}
#endif // _DEBUG

	int	exitCode = 0/*EXIT_SUCCESS*/;

#ifdef BOOST_OS_WINDOWS
	// Shift キーを押しながら起動すると英語モードになるようにしてみた
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

	// NT 系か調べる
	OSVERSIONINFOA osvi;
	osvi.dwOSVersionInfoSize = sizeof(decltype(osvi));
	::GetVersionExA(&osvi);
	if(!ascension::win32::boole(osvi.dwPlatformId & VER_PLATFORM_WIN32_NT)) {
		Gtk::MessageDialog dialog(_("Alpha does not support your platform."), false, Gtk::MESSAGE_ERROR);
		dialog.run();
		return exitCode = -1;
	}
	ascension::win32::Handle<HANDLE>::Type mutex(::CreateMutexW(0, false, IDS_APPFULLVERSION), &::CloseHandle);

	// 簡単な多重起動抑止 (Ctrl キーを押しながら起動すると多重起動するようにしてみた)
	if(::GetLastError() != ERROR_ALREADY_EXISTS || ascension::win32::boole(::GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
		::OleInitialize(nullptr);	// enter STA and initialize high-level services
		std::atexit(&alpha::callOleUninitialize);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		ascension::win32::ui::initCommonControls(ICC_COOL_CLASSES | ICC_PAGESCROLLER_CLASS | ICC_WIN95_CLASSES);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#endif
#ifndef ALPHA_NO_AMBIENT
		alpha::ambient::Interpreter::instance().install();
		alpha::ambient::Interpreter::instance().toplevelPackage();
#endif

		const Glib::RefPtr<alpha::Application> application(alpha::Application::create(argc, argv));
		exitCode = application->run(application->window());
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
	// Application ////////////////////////////////////////////////////////////////////////////////////////////////////

	Glib::RefPtr<Application> Application::instance_;

	/// Private constructor.
	Application::Application(Gio::ApplicationFlags flags /* = Gio::APPLICATION_FLAGS_NONE */)
			: Gtk::Application("alpha", flags), window_(new ui::MainWindow) {
		Glib::set_application_name("alpha");
//		searchDialog_.reset(new ui::SearchDialog());	// ctor of SearchDialog calls Alpha
//		onSettingChange(0, 0);	// statusFont_ の初期化
	}

	/// Private constructor.
	Application::Application(int& argc, char**& argv, Gio::ApplicationFlags flags /* = Gio::APPLICATION_FLAGS_NONE */)
			: Gtk::Application(argc, argv, "alpha", flags), window_(new ui::MainWindow) {
		Glib::set_application_name("alpha");
//		searchDialog_.reset(new ui::SearchDialog());	// ctor of SearchDialog calls Alpha
//		onSettingChange(0, 0);	// statusFont_ の初期化
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
		dialog.set_font_desc(ascension::graphics::toNative<Pango::FontDescription>(activeView.textArea().textRenderer().defaultFont()->describe()));
		if(dialog.run() == Gtk::RESPONSE_ACCEPT)
			setFont(ascension::graphics::fromNative<ascension::graphics::font::FontDescription>(dialog.get_font_desc()));
#endif
	}

	/**
	 * Creates a new @c Application instance.
	 * @param flags The application flags
	 * @return The instance
	 * @throw ascension#IllegalStateException The instance is already exist.
	 */
	Glib::RefPtr<Application> Application::create(Gio::ApplicationFlags flags /* = Gio::APPLICATION_FLAGS_NONE */) {
		if(instance_)
			throw ascension::IllegalStateException("");
		instance_ = Glib::RefPtr<Application>(new Application(flags));
		return instance_;
	}

	/**
	 * Creates a new @c Application instance.
	 * @param argc The parameter received by your main() function
	 * @param argv The parameter received by your main() function
	 * @param flags The application flags
	 * @return The instance
	 * @throw ascension#IllegalStateException The instance is already exist.
	 */
	Glib::RefPtr<Application> Application::create(int& argc, char**& argv, Gio::ApplicationFlags flags /* = Gio::APPLICATION_FLAGS_NONE */) {
		if(instance_)
			throw ascension::IllegalStateException("");
		instance_ = Glib::RefPtr<Application>(new Application(argc, argv, flags));
		return instance_;
	}

	/// Loads settings from the file.
	void Application::loadSettings() {
		// 表示に関する設定
		ascension::graphics::font::FontDescription fd(*ascension::graphics::font::FontFamily::createMonospaceInstance(), 0.0);
		readStructureProfile(L"View", L"Font.default", fd);	// this can fail
		setFont(fd);

		// Migemo DLL & 辞書パス
		const Glib::ustring migemoRuntimePath(boost::get_optional_value_or(readStringProfile("Find", "migemoRuntimePath"), Glib::ustring()));
		const Glib::ustring migemoDictionaryPath(boost::get_optional_value_or(readStringProfile("Find", "migemoDictionaryPath"), Glib::ustring()));
		if(!migemoRuntimePath.empty() && !migemoDictionaryPath.empty()) {
			ascension::regex::MigemoPattern::initialize(
				boost::filesystem::path(migemoRuntimePath.c_str(), std::codecvt_utf8_utf16<wchar_t>()),
				boost::filesystem::path(migemoDictionaryPath.c_str(), std::codecvt_utf8_utf16<wchar_t>()));
		}

		// 検索文字列の履歴
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
		window_->show();
//		BufferList::instance().addNew("*Messages*");
//		window_->statusBar().push("Ready");
	}

	/// Overrides @c Gio#Application#on_open method.
	void Application::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) {
		// TODO: Not implemented.
		return on_open(files, hint);
	}

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

/// 全てのエディタと一部のコントロールに新しいフォントを設定
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

	// 一部のコントロールにも設定
	if(ascension::win32::boole(readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1))) {
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
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
}

	bool Application::teardown(bool callHook /* = true */) {
#ifndef ALPHA_NO_AMBIENT
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
#endif
		saveSettings();
		quit();
//		::PostQuitMessage(0);
		return true;
	}


#ifndef ALPHA_NO_AMBIENT
	ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
		ambient::Interpreter& interpreter = ambient::Interpreter::instance();
		boost::python::scope scope(interpreter.toplevelPackage());

		boost::python::def("kill_alpha",
			ambient::makeFunctionPointer([](bool callHook) -> bool {
				return Application::instance()->teardown(callHook);
			}),
			boost::python::arg("call_hook") = true);
	ALPHA_EXPOSE_EPILOGUE()
#endif
}
