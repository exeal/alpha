/**
 * @file application.cpp
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
#include "localized-string.hpp"
//#include "search.hpp"
#include "ui/main-window.hpp"
//#include <ascension/text-editor.hpp>
#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/text/utf-string.hpp>
#include <ascension/graphics/font/font.hpp>
//#include <ascension/graphics/native-conversion.hpp>
#include <ascension/kernel/searcher.hpp>
#include <ascension/viewer/text-area.hpp>
//#include "../resource/messages.h"
//#include <manah/win32/dc.hpp>
//#include <manah/win32/gdi-object.hpp>
//#include <manah/win32/ui/wait-cursor.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <algorithm>
#include <codecvt>
#include <fstream>
#if BOOST_OS_WINDOWS
#	include <CommCtrl.h>	// InitMUILanguage
#	include <Ole2.h>		// OleInitialize, OleUninitialize
#endif
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <Dlgs.h>
#endif

#ifdef _DEBUG
#	include <boost/log/core.hpp>
#	include <boost/log/expressions.hpp>
#	include <boost/log/sinks.hpp>
#	include <boost/log/sinks/debug_output_backend.hpp>
#	include <boost/make_shared.hpp>
#endif


#if BOOST_OS_WINDOWS
namespace alpha {
	namespace {
		void callOleUninitialize() {
			::OleUninitialize();
		}
	}
}
#endif

/// The entry point.
#if 0
int main(int argc, char* argv[]) {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int showCommand) {
#endif
#ifdef _DEBUG
	{
		std::locale::global(std::locale("Japanese_Japan.932"));
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

#if BOOST_OS_WINDOWS
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
		static const auto message(alpha::localizedString("Alpha does not support your platform."));
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		Gtk::MessageDialog dialog(message.c_str(), false, Gtk::MESSAGE_ERROR);
		dialog.run();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		::MessageBoxW(nullptr, message.c_str(), IDS_APPNAME, MB_ICONERROR);
#endif
		return exitCode = -1;
	}
	auto mutex(ascension::win32::makeHandle(::CreateMutexW(0, false, IDS_APPFULLVERSION), &::CloseHandle));

	// 簡単な多重起動抑止 (Ctrl キーを押しながら起動すると多重起動するようにしてみた)
	if(::GetLastError() != ERROR_ALREADY_EXISTS || ascension::win32::boole(::GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
		::OleInitialize(nullptr);	// enter STA and initialize high-level services
		std::atexit(&alpha::callOleUninitialize);
#endif
#ifndef ALPHA_NO_AMBIENT
		alpha::ambient::Interpreter::instance().install();
		alpha::ambient::Interpreter::instance().toplevelPackage();
#endif

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		const Glib::RefPtr<alpha::Application> application(alpha::Application::create(argc, argv));
		exitCode = application->run(application->mainWindow());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		std::unique_ptr<alpha::ui::MainWindow> window(new alpha::ui::MainWindow);
		alpha::Application application(std::move(window));
		ascension::win32::realize(application.mainWindow(), ascension::win32::Window::Type::toplevel());
		application.run(showCommand);
#else
#endif
#if BOOST_OS_WINDOWS
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
		auto cd(ascension::win32::makeZero<COPYDATASTRUCT>());
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
	/**
	 * @fn alpha::Application::changeFont
	 * Shows font-chooser user interface and changes the font of the selected editor.
	 */

	/// Loads settings from the file.
	void Application::loadSettings() {
		// 表示に関する設定
		ascension::graphics::font::FontDescription fd(*ascension::graphics::font::FontFamily::createMonospaceInstance(), 0.0);
//		fd = settings().get<ascension::graphics::font::FontDescription>("view.font.default");	// this can fail
		setFont(fd);

		// Migemo DLL & 辞書パス
		const auto migemoRuntimePath(settings().get<std::string>("find.migemo-runtime-path", std::string()));	// UTF-8
		const auto migemoDictionaryPath(settings().get<std::string>("find.migemo-dictionary-path", std::string()));	// UTF-8
		if(!migemoRuntimePath.empty() && !migemoDictionaryPath.empty())
			ascension::regex::MigemoPattern::initialize(
				boost::filesystem::path(migemoRuntimePath), boost::filesystem::path(migemoDictionaryPath));

		// search and replacement strings
		std::list<ascension::String> findWhats, replacesWiths;
		BOOST_FOREACH(auto& s, settings().get_child("find.find-what"))
			findWhats.push_back(ascension::text::utf::toString(std::get<1>(s).data()));
		BOOST_FOREACH(auto&s, settings().get_child("find.replace-with"))
			replacesWiths.push_back(ascension::text::utf::toString(std::get<1>(s).data()));
		auto& s = BufferList::instance().editorSession().textSearcher();
		s.setMaximumNumberOfStoredStrings(16);
		s.setStoredStrings(std::begin(findWhats), std::end(findWhats), false);
		s.setStoredStrings(std::begin(replacesWiths), std::end(replacesWiths), true);
	}

	/// Saves the settings into the file.
	void Application::saveSettings() {
		// visibility of bars
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && 0
		auto rbbi(ascension::win32::makeZero<REBARBANDINFOW>());
		rbbi.fMask = RBBIM_STYLE;
		rebar_.getBandInfo(rebar_.idToIndex(IDC_TOOLBAR), rbbi);
		settings().put("view.visible-toolbar", (rbbi.fStyle & RBBS_HIDDEN) != 0);
		rebar_.getBandInfo(rebar_.idToIndex(IDC_BUFFERBAR), rbbi);
		settings().put("view.visible-buffer-bar", (rbbi.fStyle & RBBS_HIDDEN) != 0);
		settings().put("view.visible-status-bar", ascension::viewer::widgetapi::isVisible(mainWindow().statusBar()));
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

		// search and replacement strings
		const auto& s = BufferList::instance().editorSession().textSearcher();
		for(std::size_t i = 0, c = s.numberOfStoredPatterns(); ; ++i)
			settings().put("find.find-what", ascension::text::utf::fromString<std::string>(s.pattern(i)));
		for(std::size_t i = 0, c = s.numberOfStoredReplacements(); ; ++i)
			settings().put("find.replace-with", ascension::text::utf::fromString<std::string>(s.replacement(i)));

		boost::property_tree::write_xml("./settings.xml", settings());
	}

	/**
	 * @fn alpha::Application::setFont
	 * Sets the specified font into the all editors and some widgets.
	 * @param font
	 */

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
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		::PostQuitMessage(0);
#else
		quit();
#endif
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
