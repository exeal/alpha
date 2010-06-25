// module.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_MODULE_HPP
#define MANAH_MODULE_HPP

#include "ui/window.hpp"
#include <vector>
#include <set>
#include <sstream>


namespace manah {
	namespace win32 {

		class Module : public Object<HMODULE, 0> {
		public:
			// message arguments
			class MessageArguments {
				MANAH_NONCOPYABLE_TAG(MessageArguments);
			public:
				MessageArguments() {}
				template<typename T> MessageArguments& operator%(const T& rhs) {
					std::wostringstream ss;
					ss << rhs;
					args_.push_back(ss.str());
					return *this;
				}
			private:
				std::list<std::wstring> args_;
				friend class Module;
			};

			// constructor
			template<typename T> explicit Module(T* handle);
			// methods
			HCURSOR createCursor(int xHotSpot, int yHotSpot, int width, int height, const void* andPlane, const void* xorPlane);
			HICON createIcon(int width, int height, BYTE planeCount, BYTE bitsPixel, const BYTE* andBits, const BYTE* xorBits);
			bool enumResourceLanguages(const WCHAR* type, const WCHAR* name, ENUMRESLANGPROCW enumFunc, LONG_PTR param);
			bool enumResourceNames(const WCHAR* type, ENUMRESNAMEPROCW enumFunc, LONG_PTR param);
			bool enumResourceTypes(ENUMRESTYPEPROCW enumProc, LONG_PTR param);
			HICON extractIcon(const WCHAR* exeFileName, UINT index);
			HRSRC findResource(const ResourceID& id, const WCHAR* type);
			HRSRC findResource(const ResourceID& id, const WCHAR* type, WORD language);
			const WCHAR* getModuleFileName() const;
			bool loadAccelerators(const ResourceID& id);
			HBITMAP loadBitmap(const ResourceID& id) const;
			HCURSOR loadCursor(const ResourceID& id) const;
			HICON loadIcon(const ResourceID& id) const;
			HANDLE loadImage(const ResourceID& id, UINT type, int desiredWidth, int desiredHeight, UINT loadOptions) const;
			HMENU loadMenu(const ResourceID& id) const;
			std::wstring loadMessage(DWORD id, const MessageArguments& args = MessageArguments()) const;
			HGLOBAL loadResource(HRSRC resource);
			static HCURSOR loadStandardCursor(const ResourceID& id);
			static HICON loadStandardIcon(const ResourceID& id);
			int loadString(UINT id, WCHAR* buffer, int maxLength) const;
			std::wstring loadString(UINT id, const MessageArguments& args = MessageArguments()) const;
			DWORD sizeofResource(HRSRC resource);

		protected:
			HACCEL getAccelerators() const throw() {return accelerators_;}
		private:
			WCHAR fileName_[MAX_PATH];
			HACCEL accelerators_;
		};

#define MARGS manah::win32::Module::MessageArguments()


template<class TopWindow = ui::Window> class Application : public Module {
public:
	// constructors
	explicit Application(TopWindow* topWindow = 0);
	virtual ~Application() {}
	// methods
	static void getCommandLineArguments(const WCHAR* cmdLine, std::vector<std::wstring>& args);
	void getCommandLineArguments(std::vector<std::wstring>& args) const;
	TopWindow& getMainWindow() const;
	virtual int run(int showCommand);
protected:
	virtual LRESULT dispatchEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam) = 0;
	virtual bool initInstance(int showCommand) = 0;
//	void popModelessDialog(HWND dialog);
	virtual bool preTranslateMessage(const MSG& msg);
	void pushModelessDialog(HWND dialog);
	void setMainWindow(TopWindow& window) {mainWindow_ = &window;}
private:
	TopWindow* mainWindow_;
	std::set<HWND> modelessDialogs_;	// modeless dialogs to be called isDialogMessage
	bool running_;
};


template<class TopWindow = ui::Window>
class ProfilableApplication : public Application<TopWindow> {
public:
	// constructor
	explicit ProfilableApplication(TopWindow* topWindow = 0, const WCHAR* iniFileName = 0);
	// methods
	const WCHAR* getINIFileName() const throw() {return iniFileName_;}
	UINT readIntegerProfile(const WCHAR* section, const WCHAR* key, UINT defaultValue = 0);
	std::wstring readStringProfile(const WCHAR* section, const WCHAR* key, const WCHAR* defaultValue = 0);
	template<typename T> bool readStructureProfile(const WCHAR* section, const WCHAR* key, T& value) {
		return toBoolean(::GetPrivateProfileStructW(section, key, &value, sizeof(T), iniFileName_));}
	bool writeIntegerProfile(const WCHAR* section, const WCHAR* key, UINT value);
	bool writeStringProfile(const WCHAR* section, const WCHAR* key, const WCHAR* value);
	template<typename T> bool writeStructureProfile(const WCHAR* section, const WCHAR* key, const T& value) {
		return toBoolean(::WritePrivateProfileStructW(section, key, const_cast<T*>(&value), sizeof(T), iniFileName_));}
private:
	WCHAR iniFileName_[MAX_PATH];
};


// Module ///////////////////////////////////////////////////////////////////

template<typename T>
inline Module::Module(T* handle) : Object<HMODULE, 0>(handle), accelerators_(0) {::GetModuleFileNameW(get(), fileName_, MAX_PATH);}

inline HRSRC Module::findResource(const ResourceID& id, const WCHAR* type) {return ::FindResourceW(use(), id, type);}

inline HRSRC Module::findResource(const ResourceID& id, const WCHAR* type, WORD language) {return ::FindResourceExW(use(), id, type, language);}

inline const WCHAR* Module::getModuleFileName() const {return fileName_;}

inline bool Module::loadAccelerators(const ResourceID& id) {return (accelerators_ = ::LoadAcceleratorsW(use(), id)) != 0;}

inline HBITMAP Module::loadBitmap(const ResourceID& id) const {return ::LoadBitmapW(use(), id);}

inline HCURSOR Module::loadCursor(const ResourceID& id) const {return ::LoadCursorW(use(), id);}

inline HICON Module::loadIcon(const ResourceID& id) const {return ::LoadIconW(use(), id);}

inline HANDLE Module::loadImage(const ResourceID& id, UINT type, int desiredWidth, int desiredHeight, UINT options) const {
	return ::LoadImageW(use(), id, type, desiredWidth, desiredHeight, options);}

inline HMENU Module::loadMenu(const ResourceID& id) const {return ::LoadMenuW(use(), id);}

inline std::wstring Module::loadMessage(DWORD id, const MessageArguments& args /* = MessageArguments() */) const {
	void* buffer = 0;
	AutoBuffer<WCHAR*> inserts(new WCHAR*[args.args_.size()]);
	std::list<std::wstring>::const_iterator it;
	std::size_t i = 0;

	for(std::list<std::wstring>::const_iterator insert(args.args_.begin()), e(args.args_.end()); insert != e; ++insert, ++i)
		inserts[i] = const_cast<WCHAR*>(insert->c_str());
	::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		use(), id, 0, reinterpret_cast<WCHAR*>(&buffer), 0, reinterpret_cast<va_list*>(inserts.get()));
	if(buffer != 0) {
		std::wstring temp(static_cast<WCHAR*>(buffer));
		::LocalFree(buffer);
		return temp;
	} else
		return L"";
}

inline HGLOBAL Module::loadResource(HRSRC resource) {return ::LoadResource(use(), resource);}

inline HCURSOR Module::loadStandardCursor(const ResourceID& id) {return ::LoadCursorW(0, id);}

inline HICON Module::loadStandardIcon(const ResourceID& id) {return ::LoadIconW(0, id);}

inline int Module::loadString(UINT id, WCHAR* buffer, int maxLength) const {return ::LoadStringW(use(), id, buffer, maxLength);}

inline std::wstring Module::loadString(UINT id, const MessageArguments& args /* = MessageArguments() */) const {
	WCHAR buffer[1024];
	loadString(id, buffer, 1024);
	if(args.args_.empty())
		return buffer;

	void* p = 0;
	WCHAR** pp = new WCHAR*[args.args_.size()];
	std::list<std::wstring>::const_iterator it;
	std::size_t i;

	for(it = args.args_.begin(), i = 0; it != args.args_.end(); ++it, ++i)
		pp[i] = const_cast<WCHAR*>(it->c_str());
	::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING
		| FORMAT_MESSAGE_ARGUMENT_ARRAY, buffer, 0, 0,
		reinterpret_cast<WCHAR*>(&p), 0, reinterpret_cast<va_list*>(pp));
	delete[] pp;

	std::wstring temp = static_cast<WCHAR*>(p);
	::LocalFree(p);
	return temp;
}

inline DWORD Module::sizeofResource(HRSRC resource) {return ::SizeofResource(use(), resource);}


// Application //////////////////////////////////////////////////////////////

template<class TopWindow>
inline Application<TopWindow>::Application(TopWindow* topWindow /* = 0 */)
	: Module(borrowed(::GetModuleHandle(0))), mainWindow_(topWindow), running_(false) {}

template<class TopWindow> inline void Application<TopWindow>::getCommandLineArguments(const WCHAR* cmdLine, std::vector<std::wstring>& args) {
	assert(cmdLine != 0);
	WCHAR* each = new WCHAR[std::wcslen(cmdLine) + 1];
	WCHAR ch;
	std::size_t i = 0;
	std::size_t eachLength = 0;
	bool inQuote = false;

	args.clear();
	while(true) {
		ch = cmdLine[i];
		if(ch == 0) {	// terminator
			if(eachLength != 0) {
				each[eachLength] = 0;
				args.push_back(each);
			}
			break;
		}
		if(!inQuote) {
			if(ch == L' ') {
				if(eachLength != 0) {
					*(each + eachLength) = 0;
					args.push_back(each);
					eachLength = 0;
				}
			} else if(ch == L'\"')
				inQuote = true;
			else {
				*(each + eachLength) = ch;
				++eachLength;
			}
		} else {
			if(ch == L'\"')
				inQuote = false;
			else {
				*(each + eachLength) = ch;
				++eachLength;
			}
		}
		++i;
	}
	delete[] each;
}

template<class TopWindow> inline void Application<TopWindow>::getCommandLineArguments(std::vector<std::wstring>& args) const {
	const WCHAR* cmdLine = ::GetCommandLineW();
	std::size_t	 i = 0;
	WCHAR ch;
	WCHAR* quote;

	while(true) {
		ch = cmdLine[i];
		if(ch == 0) {
			args.clear();
			return;
		} else if(ch == L'\"') {
			quote = std::wcschr(cmdLine + i + 1, L'\"');
			if(quote == 0) {
				args.clear();
				return;
			}
			i = quote - cmdLine + 1;
		} else if(ch == L'\'') {
			quote = std::wcschr(cmdLine + i + 1, '\'');
			if(quote == 0) {
				args.clear();
				return;
			}
			i = quote - cmdLine + 1;
		} else if(ch == L' ') {
			++i;
			break;
		} else
			++i;
	}
	getCommandLineArguments(cmdLine + i, args);
}

template<class TopWindow> inline TopWindow& Application<TopWindow>::getMainWindow() const {
	if(mainWindow_ == 0)
		throw std::logic_error("The application does not have a window.");
	return *mainWindow_;
}

template<class TopWindow> inline bool Application<TopWindow>::preTranslateMessage(const MSG& msg) {return false;}

template<class TopWindow> inline void Application<TopWindow>::pushModelessDialog(HWND dialog) {assert(toBoolean(::IsWindow(dialog))); modelessDialogs_.insert(dialog);}

template<class TopWindow> inline int Application<TopWindow>::run(int showCommand) {
	if(running_)
		return -1;
	MSG message;
	int f;
	bool dialogMessage;

	if(!initInstance(showCommand))
		return -1;
	running_ = true;
	while(true) {
		dialogMessage = false;
		f = ::GetMessageW(&message, 0, 0, 0);
		assert(f != -1);
		if(f == 0)
			return static_cast<int>(message.wParam);
		for(std::set<HWND>::iterator i(modelessDialogs_.begin()); i != modelessDialogs_.end(); ) {
			if(!toBoolean(::IsWindow(*i))) {	// auto pop
				modelessDialogs_.erase(i++);
			} else if(toBoolean(::IsDialogMessage(*i, &message))) {
				dialogMessage = true;
				break;
			} else
				++i;
		}
		if(!dialogMessage) {
			if(getAccelerators() != 0) {
				if(!toBoolean(::TranslateAcceleratorW(message.hwnd, getAccelerators(), &message))
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


// ProfilableApplication ////////////////////////////////////////////////////

template<class TopWindow> inline ProfilableApplication<TopWindow>::ProfilableApplication(
		TopWindow* topWindow /* = 0 */, const WCHAR* iniFileName /* = 0 */) : Application<TopWindow>(topWindow) {
	if(iniFileName != 0)
		std::wcscpy(iniFileName_, iniFileName);
	else {
		::GetModuleFileNameW(0, iniFileName_, MAX_PATH);
		if(WCHAR* const dot = std::wcsrchr(iniFileName_, L'.'))
			std::wcscpy(dot + 1, L"ini");
		else
			iniFileName_[0] = 0;
	}
}

template<class TopWindow>
inline UINT ProfilableApplication<TopWindow>::readIntegerProfile(const WCHAR* section, const WCHAR* key, UINT defaultValue /* 0 */) {
	return ::GetPrivateProfileIntW(section, key, defaultValue, iniFileName_);}

template<class TopWindow> inline std::wstring
ProfilableApplication<TopWindow>::readStringProfile(const WCHAR* section, const WCHAR* key, const WCHAR* defaultValue /* = L"" */) {
	WCHAR buffer[1024];
	::GetPrivateProfileStringW(section, key, defaultValue, buffer, MANAH_COUNTOF(buffer), iniFileName_);
	return buffer;
}

template<class TopWindow>
inline bool ProfilableApplication<TopWindow>::writeIntegerProfile(const WCHAR* section, const WCHAR* key, UINT value) {
	WCHAR buffer[32];
	::wsprintfW(buffer, L"%u", value);
	return writeStringProfile(section, key, buffer);
}

template<class TopWindow>
inline bool ProfilableApplication<TopWindow>::writeStringProfile(const WCHAR* section, const WCHAR* key, const WCHAR* value) {
	return toBoolean(::WritePrivateProfileStringW(section, key, value, iniFileName_));}

}} // namespace manah.win32

#endif // !MANAH_MODULE_HPP
