/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2009, 2013-2014
 */

#ifndef ALPHA_APPLICATION_HPP
#define ALPHA_APPLICATION_HPP
#include "resource.h"
//#include "search.hpp"	// ui.SearchDialog
#include <ascension/graphics/font/font-description.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/application.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "win32/application.hpp"
#else
#	error "Not implemented"
#endif


// �^�C�g���o�[�Ƃ��Ɏg���A�v���P�[�V������
#define IDS_APPNAME		L"Alpha"
#define IDS_APPVERSION	L"0.7.94.0"
#define IDS_APPFULLVERSION	IDS_APPNAME L" " IDS_APPVERSION

#define IDS_DEFAULTSTATUSTEXT	L""
#define IDS_EVENTSCRIPTFILENAME	L"events.*"
#define IDS_MACRO_DIRECTORY_NAME			L"macros\\"
#define IDS_KEYBOARDSCHEME_DIRECTORY_NAME	L"keyboard-schemes\\"
#define IDS_ICON_DIRECTORY_NAME				L"icons\\"

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
	namespace ui {
		class MainWindow;
	}

	/// The application class of Alpha.
	class Application :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		public Gtk::Application
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		public win32::WindowApplication<ui::MainWindow>
#endif
	{
	public:
		/// @name Instance
		/// @{
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		static Glib::RefPtr<Application> create(Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE);
		static Glib::RefPtr<Application> create(int& argc, char**& argv, Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE);
		static Glib::RefPtr<Application> instance();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		static std::shared_ptr<Application> create(std::unique_ptr<ui::MainWindow> window);
		static std::shared_ptr<Application> instance();
#endif
		/// @}

		void setFont(const ascension::graphics::font::FontDescription& font);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		ui::MainWindow& mainWindow() BOOST_NOEXCEPT;
		const ui::MainWindow& mainWindow() const BOOST_NOEXCEPT;
#endif
		// operations
		bool teardown(bool callHook = true);

		/// @name Settings
		/// @{
		void saveSettings();
		boost::property_tree::ptree& settings() BOOST_NOEXCEPT;
		const boost::property_tree::ptree& settings() const BOOST_NOEXCEPT;
		/// @}

	private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		explicit Application(Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE);
		Application(int& argc, char**& argv, Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		explicit Application(std::unique_ptr<ui::MainWindow> window);
#endif
		void changeFont();
		bool initInstance(int showCommand);
		void loadSettings();
		void updateTitleBar();
		template<typename Section, typename Key, typename Value>
		inline void writeIntegerProfile(Section section, Key key, Value value) {}	// dummy
		template<typename Section, typename Key, typename Value>
		inline void writeStringProfile(Section section, Key key, const Value& value) const {}	// dummy
		template<typename Section, typename Key, typename T>
		inline void writeStructureProfile(Section section, Key key, const T& data) const {}	// dummy

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		// Gio.Application
		void on_activate() override;
		void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;
#endif

	private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		static Glib::RefPtr<Application> instance_;
		std::unique_ptr<ui::MainWindow> window_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		static std::shared_ptr<Application> instance_;
#endif
		std::unique_ptr<boost::property_tree::ptree> settings_;
	};


	/// Returns the singleton application object.
	inline
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		Glib::RefPtr<Application>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		std::shared_ptr<Application>
#endif
		Application::instance() {
		if(!instance_)
			throw ascension::NullPointerException("There is no singleton instance.");
		return instance_;
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
	/// Returns the main window.
	inline ui::MainWindow& Application::mainWindow() BOOST_NOEXCEPT {
		assert(window_.get() != nullptr);
		return *window_;
	}

	/// Returns the main window.
	inline const ui::MainWindow& Application::mainWindow() const BOOST_NOEXCEPT {
		assert(window_.get() != nullptr);
		return *window_;
	}
#endif

	/// Returns the application-global settings in UTF-8.
	inline boost::property_tree::ptree& Application::settings() BOOST_NOEXCEPT {
		return *settings_;
	}

	/// @overload
	inline const boost::property_tree::ptree& Application::settings() const BOOST_NOEXCEPT {
		return *settings_;
	}
} // namespace alpha

#endif // ALPHA_APPLICATION_HPP
