/**
 * @file application-gtk.hpp
 * @author exeal
 * @date 2003-2009, 2014-2015
 * @date 2017-02-20 Separated from application.cpp.
 */

#include "application.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#include "ambient.hpp"
#include "buffer-list.hpp"
#include "input.hpp"
//#include "ui.hpp"
#include "editor-panes.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
//#include "search.hpp"
#include "ui/main-window.hpp"
//#include <ascension/text-editor.hpp>
#include <ascension/corelib/regex.hpp>
#include <ascension/graphics/font/font.hpp>
//#include <ascension/graphics/native-conversion.hpp>
#include <ascension/viewer/text-area.hpp>
#include <gtkmm/fontchooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <glibmm/i18n.h>
#include <algorithm>
#include <codecvt>
#include <fstream>
#if BOOST_OS_WINDOWS
#	include <CommCtrl.h>	// InitMUILanguage
#	include <Ole2.h>		// OleInitialize, OleUninitialize
#endif


namespace alpha {
	Glib::RefPtr<Application> Application::instance_;

	/// Private constructor.
	Application::Application(Gio::ApplicationFlags flags /* = Gio::APPLICATION_FLAGS_NONE */)
			: Gtk::Application("alpha", flags), window_(new ui::MainWindow) {
		Glib::set_application_name("alpha");
//		searchDialog_.reset(new ui::SearchDialog());	// ctor of SearchDialog calls Alpha
//		onSettingChange(0, 0);	// statusFont_ ÇÃèâä˙âª
	}

	/// Private constructor.
	Application::Application(int& argc, char**& argv, Gio::ApplicationFlags flags /* = Gio::APPLICATION_FLAGS_NONE */)
			: Gtk::Application(argc, argv, "alpha", flags), window_(new ui::MainWindow) {
		Glib::set_application_name("alpha");
//		searchDialog_.reset(new ui::SearchDialog());	// ctor of SearchDialog calls Alpha
//		onSettingChange(0, 0);	// statusFont_ ÇÃèâä˙âª
	}

	void Application::changeFont() {
		EditorView& activeView = EditorPanes::instance().activePane().selectedView();
		Gtk::FontChooserDialog dialog(Glib::ustring(), window());
		dialog.set_font_desc(ascension::graphics::toNative<Pango::FontDescription>(activeView.textArea()->textRenderer()->defaultFont()->describe()));
		if(dialog.run() == Gtk::RESPONSE_ACCEPT)
			setFont(ascension::graphics::fromNative<ascension::graphics::font::FontDescription>(dialog.get_font_desc()));
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

	void Application::setFont(const ascension::graphics::font::FontDescription& font) {
	}

#endif
