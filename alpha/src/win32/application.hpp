/**
 * @file win32/application.hpp
 * Defines @c win32#Application class.
 * @author exeal
 * @date 2003-2007, 2013
 * @date 2017-01-20 Separated from win32/module.hpp.
 */

#ifndef ALPHA_WIN32_APPLICATION_HPP
#define ALPHA_WIN32_APPLICATION_HPP
#include "win32/module.hpp"
#include <set>


namespace alpha {
	namespace win32 {
		/// The application class.
		class Application : public Module {
		public:
			/// Creates a @c Application instance.
			Application() : Module(ascension::win32::borrowed(::GetModuleHandle(nullptr))), running_(false) {}

			/// Destructor.
			virtual ~Application() BOOST_NOEXCEPT {}

			/**
			 * Runs the application.
			 * @param showCommand The window showing command which is passed to @c #initialize method
			 * @return The exit code
			 */
			virtual int run(int showCommand) {
				if(running_)
					return -1;

				if(!initialize(showCommand))
					return -1;
				running_ = true;

				MSG message;
				while(true) {
					bool dialogMessage = false;
					const auto f = ::GetMessageW(&message, nullptr, 0, 0);
					assert(f != -1);
					if(f == 0)
						return static_cast<int>(message.wParam);
					for(auto i(std::begin(modelessDialogs_)), e(std::end(modelessDialogs_)); i != e; ) {
						if(!ascension::win32::boole(::IsWindow(i->get())))	// pop an invalid handle automati
							i = modelessDialogs_.erase(i);
						else if(ascension::win32::boole(::IsDialogMessage(i->get(), &message))) {
							dialogMessage = true;
							break;
						} else
							++i;
					}
					if(!dialogMessage && accelerators().get() != nullptr && !ascension::win32::boole(::TranslateAcceleratorW(message.hwnd, accelerators().get(), &message))) {
						bool consumed = false;
						preTranslateMessage(message, consumed);
						if(!consumed) {
							::TranslateMessage(&message);
							::DispatchMessageW(&message);
						}
					}
				}

				running_ = false;
				return 0;
			}

		protected:
			/**
			 * This method is called in @c #run method once.
			 * @param command The window showing command
			 * @return Set @c false to interrupt running
			 */
			virtual bool initialize(int command) {
				return true;
			}
//			void popModelessDialog(HWND dialog);
			/**
			 * Handles the message before the message is passed to @c TranslateMessage.
			 * @param message The message
			 * @param[out] consumed If @c true, this message is neither translated nor dispatched
			 */
			virtual void preTranslateMessage(const MSG& message, bool& consumed) {
				consumed = false;
			}
			/**
			 * Registers the modeless dialog box.
			 * @param dialog A handle to the dialog box
			 * @throw std#invalid_argument @a dialog is not a valid handle value
			 */
			void pushModelessDialog(ascension::win32::Handle<HWND> dialog) {
				assert(ascension::win32::boole(::IsWindow(dialog.get())));
				modelessDialogs_.insert(dialog);
			}

		private:
			std::set<ascension::win32::Handle<HWND>> modelessDialogs_;	// modeless dialogs to be called isDialogMessage
			bool running_;
		};

		/**
		 * The application class which has the main window.
		 * @tparam MainWindow The type of the main window
		 */
		template<class MainWindow>
		class WindowApplication : public Application {
		public:
			/**
			 * Creates a @c WindowApplication instance.
			 * @param window The main window
			 * @throw ascension#NullPointerException @a window is @c null
			 */
			explicit WindowApplication(std::unique_ptr<MainWindow> window) : window_(std::move(window)) {
				if(window_.get() == nullptr)
					throw ascension::NullPointerException("window");
			}

			/// Returns the main window.
			MainWindow& mainWindow() const {
				return *window_;
			}

		private:
			std::unique_ptr<MainWindow> window_;
		};
	}
} // namespace alpha.win32

#endif // !ALPHA_WIN32_APPLICATION_HPP
