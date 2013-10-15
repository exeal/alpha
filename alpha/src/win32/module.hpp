/**
 * @file module.hpp
 * @author exeal
 * @date 2003-2007, 2013
 */

#ifndef ALPHA_WIN32_MODULE_HPP
#define ALPHA_WIN32_MODULE_HPP

#include <ascension/win32/window.hpp>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>


namespace alpha {
	namespace win32 {

		class Module {
		public:
			// message arguments
			class MessageArguments {
				ASCENSION_NONCOPYABLE_TAG(MessageArguments);
			public:
				/// Default constructor.
				MessageArguments() BOOST_NOEXCEPT {}
				template<typename T> MessageArguments& operator%(const T& rhs) {
					std::basic_ostringstream<WCHAR> ss;
					ss << rhs;
					args_.push_back(ss.str());
					return *this;
				}
			private:
				std::list<std::basic_string<WCHAR>> args_;
				friend class Module;
			};

			/**
			 * Constructor.
			 * @param handle A handle to the module
			 * @throw ascension#NullPointerException @a handle is @c null
			 */
			explicit Module(HMODULE handle) : handle_(handle), accelerators_(nullptr) {
				if(handle_ == nullptr)
					throw ascension::NullPointerException("handle");
				WCHAR temp[MAX_PATH];
				if(0 == ::GetModuleFileNameW(handle_, temp, std::extent<decltype(temp)>::value))
					throw ascension::makePlatformError();
				fileName_.assign(temp);
			}

			HCURSOR createCursor(int xHotSpot, int yHotSpot, int width, int height, const void* andPlane, const void* xorPlane);
			HICON createIcon(int width, int height, BYTE planeCount, BYTE bitsPixel, const BYTE* andBits, const BYTE* xorBits);
			bool enumResourceLanguages(const WCHAR* type, const WCHAR* name, ENUMRESLANGPROCW enumFunc, LONG_PTR param);
			bool enumResourceNames(const WCHAR* type, ENUMRESNAMEPROCW enumFunc, LONG_PTR param);
			bool enumResourceTypes(ENUMRESTYPEPROCW enumProc, LONG_PTR param);
			HICON extractIcon(const WCHAR* exeFileName, UINT index);

			/// Returns the module file name.
			const std::basic_string<WCHAR>& fileName() const BOOST_NOEXCEPT {
				return fileName_;
			}

			/// Calls @c FindResourceW.
			ascension::win32::Handle<HRSRC>::Type findResource(const ascension::win32::ResourceID& id, const WCHAR* type) {
				if(HRSRC temp = ::FindResourceW(handle_, id, type))
					return ascension::win32::Handle<HRSRC>::Type(temp);
				throw ascension::makePlatformError();
			}

			/// Calls @c FindResourceExW.
			ascension::win32::Handle<HRSRC>::Type findResource(const ascension::win32::ResourceID& id, const WCHAR* type, WORD language) {
				if(HRSRC temp = ::FindResourceExW(handle_, id, type, language))
					return ascension::win32::Handle<HRSRC>::Type(temp);
				throw ascension::makePlatformError();
			}

			/// Calls @c LoadAccelerators.
			void loadAccelerators(const ascension::win32::ResourceID& id) {
				decltype(accelerators_) temp(::LoadAcceleratorsW(handle_, id), &ascension::detail::NullDeleter());	// MSDN says "are freed automatically when the application terminates."
				if(temp.get() == nullptr)
					throw ascension::makePlatformError();
				std::swap(accelerators_, temp);
			}

			/// Calls @c LoadBitmapW.
			ascension::win32::Handle<HBITMAP>::Type loadBitmap(const ascension::win32::ResourceID& id) const {
				if(auto temp = ascension::win32::Handle<HBITMAP>::Type(::LoadBitmapW(handle_, id), &::DeleteObject))
					return temp;
				throw ascension::makePlatformError();
			}

			/// Calls @c LoadCursorW.
			ascension::win32::Handle<HCURSOR>::Type loadCursor(const ascension::win32::ResourceID& id) const {
				if(auto temp = ascension::win32::Handle<HCURSOR>::Type(::LoadCursorW(handle_, id), &ascension::detail::NullDeleter()))
					return temp;
				throw ascension::makePlatformError();
			}

			/// Calls @c LoadIconW.
			ascension::win32::Handle<HICON>::Type loadIcon(const ascension::win32::ResourceID& id) const {
				if(auto temp = ascension::win32::Handle<HICON>::Type(::LoadIconW(handle_, id), &ascension::detail::NullDeleter()))
					return temp;
				throw ascension::makePlatformError();
			}

			/// Calls @c LoadImageW.
			ascension::win32::Handle<HANDLE>::Type loadImage(const ascension::win32::ResourceID& id, UINT type, int desiredWidth, int desiredHeight, UINT options) const {
				ascension::win32::Handle<HANDLE>::Type image;
				if((options & LR_SHARED) != 0)
					image = ascension::win32::Handle<HANDLE>::Type(::LoadImageW(handle_, id, type, desiredWidth, desiredHeight, options), &ascension::detail::NullDeleter());
				else {
					switch(type) {
						case IMAGE_BITMAP:
							image = ascension::win32::Handle<HANDLE>::Type(::LoadImageW(handle_, id, type, desiredWidth, desiredHeight, options), &::DeleteObject);
							break;
						case IMAGE_ICON:
							image = ascension::win32::Handle<HANDLE>::Type(::LoadImageW(handle_, id, type, desiredWidth, desiredHeight, options), &::DestroyIcon);
							break;
						case IMAGE_CURSOR:
							image = ascension::win32::Handle<HANDLE>::Type(::LoadImageW(handle_, id, type, desiredWidth, desiredHeight, options), &::DestroyCursor);
							break;
						default:
							::SetLastError(ERROR_BAD_ARGUMENTS);
							break;
					}
				}
				if(image.get() == nullptr)
					throw ascension::makePlatformError();
				return image;
			}

			/// Calls @c LoadMenuW. The returned value is deleted by @c DestroyMenu.
			ascension::win32::Handle<HMENU>::Type loadMenu(const ascension::win32::ResourceID& id) const {
				if(auto temp = ascension::win32::Handle<HMENU>::Type(::LoadMenuW(handle_, id), &::DestroyMenu))
					return temp;
				throw ascension::makePlatformError();
			}

			std::basic_string<WCHAR> loadMessage(DWORD id, const MessageArguments& args = MessageArguments()) const {
				void* buffer = nullptr;
				std::unique_ptr<WCHAR*[]> inserts(new WCHAR*[args.args_.size()]);
				std::size_t i = 0;

				for(auto insert(std::begin(args.args_)), e(std::end(args.args_)); insert != e; ++insert, ++i)
					inserts[i] = const_cast<WCHAR*>(insert->c_str());
				const DWORD n = ::FormatMessageW(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
					handle_, id, 0, reinterpret_cast<WCHAR*>(&buffer), 0, reinterpret_cast<va_list*>(inserts.get()));
				std::shared_ptr<void> holder(buffer, &::LocalFree);
				if(n == 0 || buffer == nullptr)
					throw ascension::makePlatformError();
				return static_cast<WCHAR*>(buffer);
			}

			/// Calls @c LoadResource.
			HGLOBAL loadResource(HRSRC resource) {
				if(HGLOBAL temp = ::LoadResource(handle_, resource))
					return temp;
				throw ascension::makePlatformError();
			}

			/// Calls @c LoadCursorW with @c null @c HMODULE.
			static ascension::win32::Handle<HCURSOR>::Type loadStandardCursor(const ascension::win32::ResourceID& id) {
				if(auto temp = ascension::win32::Handle<HCURSOR>::Type(::LoadCursorW(nullptr, id), &ascension::detail::NullDeleter()))
					return temp;
				throw ascension::makePlatformError();
			}

			/// Calls @c LoadIconW with @c null @c HMODULE.
			static ascension::win32::Handle<HICON>::Type loadStandardIcon(const ascension::win32::ResourceID& id) {
				if(auto temp = ascension::win32::Handle<HICON>::Type(::LoadIconW(nullptr, id), &ascension::detail::NullDeleter()))
					return temp;
				throw ascension::makePlatformError();
			}

			/// Calls @c LoadStringW.
			int loadString(UINT id, WCHAR* buffer, int maxLength) const {
				if(const int temp = ::LoadStringW(handle_, id, buffer, maxLength))
					return temp;
				throw ascension::makePlatformError();
			}

			std::basic_string<WCHAR> loadString(UINT id, const MessageArguments& args = MessageArguments()) const {
				WCHAR buffer[1024];
				loadString(id, buffer, 1024);
				if(args.args_.empty())
					return buffer;

				void* p = nullptr;
				std::unique_ptr<WCHAR*[]> pp(new WCHAR*[args.args_.size()]);
				std::size_t i = 0;

				for(auto it = std::begin(args.args_); it != std::end(args.args_); ++it, ++i)
					pp[i] = const_cast<WCHAR*>(it->c_str());
				const DWORD n = ::FormatMessageW(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
						buffer, 0, 0, reinterpret_cast<WCHAR*>(&p), 0, reinterpret_cast<va_list*>(pp.get()));
				std::shared_ptr<void> holder(p, &::LocalFree);
				if(n == 0 || p == nullptr)
					throw ascension::makePlatformError();
				return static_cast<WCHAR*>(p);
			}

			/// Calls @c SizeofResource.
			DWORD sizeofResource(HRSRC resource) {
				if(const DWORD temp = ::SizeofResource(handle_, resource))
					return temp;
				throw ascension::makePlatformError();
			}

		protected:
			ascension::win32::Handle<HACCEL>::Type accelerators() const BOOST_NOEXCEPT {return accelerators_;}
		private:
			HMODULE handle_;
			std::basic_string<WCHAR> fileName_;
			ascension::win32::Handle<HACCEL>::Type accelerators_;
		};

		template<class TopWindow = ascension::win32::Window>
		class Application : public Module {
		public:
			/// Constructor.
			explicit Application(TopWindow* topWindow = nullptr) :
				Module(borrowed(::GetModuleHandle(nullptr))), mainWindow_(topWindow), running_(false) {}

			/// Destructor.
			virtual ~Application() BOOST_NOEXCEPT {}

			static void commandLineArguments(const ascension::win32::StringPiece& cmdLine, std::vector<std::basic_string<WCHAR>>& args)  {
				assert(cmdLine.data() != nullptr);
				std::unique_ptr<WCHAR[]> each(new WCHAR[std::wcslen(cmdLine) + 1]);
				std::size_t i = 0, eachLength = 0;
				bool inQuote = false;

				args.clear();
				while(true) {
					const WCHAR c = cmdLine[i];
					if(c == 0) {	// terminator
						if(eachLength != 0) {
							each[eachLength] = 0;
							args.push_back(each);
						}
						break;
					}
					if(!inQuote) {
						if(c == L' ') {
							if(eachLength != 0) {
								*(each + eachLength) = 0;
								args.push_back(each);
								eachLength = 0;
							}
						} else if(c == L'\"')
							inQuote = true;
						else {
							*(each + eachLength) = c;
							++eachLength;
						}
					} else {
						if(c == L'\"')
							inQuote = false;
						else {
							*(each + eachLength) = c;
							++eachLength;
						}
					}
					++i;
				}
			}

			void commandLineArguments(std::vector<std::wstring>& args) const {
				const WCHAR* const cmdLine = ::GetCommandLineW();
				const WCHAR* p = cmdLine;
				while(true) {
					switch(*p) {
						case 0:
							args.clear();
							return;
						case L'\"':
						case L'\'': {
							const WCHAR* const quote = std::wcschr(p + 1, *p);
							if(quote == nullptr) {
								args.clear();
								return;
							}
							p = quote + 1;
							break;
						}
						case L' ':
							exit;
						default:
							++p;
							break;
					}
				}
				commandLineArguments(p, args);
			}

			/// Returns the main window.
			TopWindow& mainWindow() const {
				if(mainWindow_ == nullptr)
					throw std::logic_error("The application does not have a window.");
				return *mainWindow_;
			}
			virtual int run(int showCommand) {
				if(running_)
					return -1;

				if(!initInstance(showCommand))
					return -1;
				running_ = true;

				MSG message;
				while(true) {
					bool dialogMessage = false;
					const int f = ::GetMessageW(&message, nullptr, 0, 0);
					assert(f != -1);
					if(f == 0)
						return static_cast<int>(message.wParam);
					for(std::set<HWND>::iterator i(std::begin(modelessDialogs_)), e(std::end(modelessDialogs_)); i != e; ) {
						if(!ascension::win32::boole(::IsWindow(*i))) {	// auto pop
							modelessDialogs_.erase(i++);
						} else if(ascension::win32::boole(::IsDialogMessage(*i, &message))) {
							dialogMessage = true;
							break;
						} else
							++i;
					}
					if(!dialogMessage) {
						if(accelerators() != 0) {
							if(!ascension::win32::boole(::TranslateAcceleratorW(message.hwnd, accelerators(), &message))
									&& !preTranslateMessage(message)) {
								::TranslateMessage(&message);
								::DispatchMessageW(&message);
							}
						} else if(!preTranslateMessage(message)) {
							::TranslateMessage(&message);
							::DispatchMessageW(&message);
						}
					}
				}

				running_ = false;
				return 0;
			}
		protected:
			virtual LRESULT dispatchEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam) = 0;
			virtual bool initInstance(int showCommand) = 0;
//			void popModelessDialog(HWND dialog);
			virtual bool preTranslateMessage(const MSG& msg) {return false;}
			void pushModelessDialog(HWND dialog) {
				assert(ascension::win32::boole(::IsWindow(dialog)));
				modelessDialogs_.insert(dialog);
			}
			void setMainWindow(TopWindow& window) {mainWindow_ = &window;}
		private:
			TopWindow* mainWindow_;
			std::set<HWND> modelessDialogs_;	// modeless dialogs to be called isDialogMessage
			bool running_;
		};


		template<class TopWindow = ascension::win32::Window>
		class ProfilableApplication : public Application<TopWindow> {
		public:
			/// Constructor.
			explicit ProfilableApplication(TopWindow* topWindow = nullptr, const ascension::win32::StringPiece& iniFileName = ascension::win32::StringPiece()) : Application<TopWindow>(topWindow) {
				if(iniFileName.data() != nullptr)
					iniFileName_.assign(iniFileName);
				else {
					WCHAR temp[MAX_PATH];
					if(0 == ::GetModuleFileNameW(nullptr, temp, std::extent<decltype(temp)>::value))
						throw ascension::makePlatformError();
					if(WCHAR* const dot = std::wcsrchr(iniFileName_, L'.')) {
						std::wcscpy(dot + 1, L"ini");
						iniFileName_.assign(temp);
					}
				}
			}

			/// Returns the INI file name.
			const std::basic_string<WCHAR>& iniFileName() const BOOST_NOEXCEPT {
				return iniFileName_;
			}

			/// Calls @c GetPrivateProfileIntW.
			UINT readIntegerProfile(const ascension::win32::StringPiece& section, const ascension::win32::StringPiece& key, UINT defaultValue = 0) BOOST_NOEXCEPT {
				return ::GetPrivateProfileIntW(section.data(), key.data(), defaultValue, iniFileName().c_str());
			}

			/// Calls @c GetPrivateProfileStringW.
			std::basic_string<WCHAR> readStringProfile(const ascension::win32::StringPiece& section,
					const ascension::win32::StringPiece& key, const ascension::win32::StringPiece& defaultValue = ascension::win32::StringPiece()) {
				WCHAR buffer[1024];
				::SetLastError(ERROR_SUCCESS);
				::GetPrivateProfileStringW(section.data(), key.data(), defaultValue.data(), buffer, std::extent<decltype(buffer)>::value, iniFileName());
				if(::GetLastError() != ERROR_SUCCESS)
					throw ascension::makePlatformError();
				return buffer;
			}

			/// Calls @c GetPrivateProfileStructW.
			template<typename T> void readStructureProfile(const ascension::win32::StringPiece& section, const ascension::win32::StringPiece& key, T& value) {
				if(!ascension::win32::boole(::GetPrivateProfileStructW(section.data(), key.data(), &value, sizeof(T), iniFileName())))
					throw ascension::makePlatformError();
			}

			/// Calls @c WritePrivateProfileStringW.
			void writeIntegerProfile(const ascension::win32::StringPiece& section, const ascension::win32::StringPiece& key, UINT value) {
				WCHAR buffer[32];
				::wsprintfW(buffer, L"%u", value);
				return writeStringProfile(section, key, buffer);
			}

			/// Calls @c WritePrivateProfileStringW.
			void writeStringProfile(const ascension::win32::StringPiece& section, const ascension::win32::StringPiece& key, const ascension::win32::StringPiece& value) {
				if(!ascension::win32::boole(::WritePrivateProfileStringW(section.data(), key.data(), value.data(), iniFileName())))
					throw ascension::makePlatformError();
			}

			/// Calls @c WritePrivateProfileStructW.
			template<typename T> void writeStructureProfile(const ascension::win32::StringPiece& section, const ascension::win32::StringPiece& key, const T& value) {
				if(!ascension::win32::boole(::WritePrivateProfileStructW(section.data(), key.data(), const_cast<T*>(&value), sizeof(T), iniFileName())))
					throw ascension::makePlatformError();
			}
		private:
			std::basic_string<WCHAR> iniFileName_;
		};

	}
} // namespace alpha.win32

#endif // !ALPHA_WIN32_MODULE_HPP
