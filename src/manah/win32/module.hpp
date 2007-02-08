// module.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_MODULE_HPP
#define MANAH_MODULE_HPP

#include "ui/window.hpp"
#include <vector>
#include <set>
#include <sstream>


namespace manah {
	namespace windows {

		class Module : public HandleHolder<HMODULE> {
		public:
			// メッセージ引数
			class MessageArguments : public Noncopyable {
			public:
				MessageArguments() {}
				template<class T> MessageArguments& operator %(const T& rhs) {
					std::basic_ostringstream<TCHAR> ss;
					ss << rhs;
					args_.push_back(ss.str());
					return *this;
				}
			private:
				std::list<std::basic_string<TCHAR> > args_;
				friend class Module;
			};

			// コンストラクタ
			explicit Module(HMODULE handle);
			// メソッド
			HCURSOR						createCursor(int xHotSpot, int yHotSpot, int width, int height, const void* andPlane, const void* xorPlane);
			HICON						createIcon(int width, int height, BYTE planeCount, BYTE bitsPixel, const BYTE* andBits, const BYTE* xorBits);
			bool						enumResourceLanguages(const TCHAR* type, const TCHAR* name, ENUMRESLANGPROC enumFunc, LONG_PTR param);
			bool						enumResourceNames(const TCHAR* type, ENUMRESNAMEPROC enumFunc, LONG_PTR param);
			bool						enumResourceTypes(ENUMRESTYPEPROC enumProc, LONG_PTR param);
			HICON						extractIcon(const TCHAR* exeFileName, UINT index);
			HRSRC						findResource(const ResourceID& id, const TCHAR* type);
			HRSRC						findResource(const ResourceID& id, const TCHAR* type, WORD language);
			const TCHAR*				getModuleFileName() const;
			bool						loadAccelerators(const ResourceID& id);
			HBITMAP						loadBitmap(const ResourceID& id) const;
			HCURSOR						loadCursor(const ResourceID& id) const;
			HICON						loadIcon(const ResourceID& id) const;
			HANDLE						loadImage(const ResourceID& id, UINT type, int desiredWidth, int desiredHeight, UINT loadOptions) const;
			HMENU						loadMenu(const ResourceID& id) const;
			std::basic_string<TCHAR>	loadMessage(DWORD id, const MessageArguments& args = MessageArguments()) const;
			HGLOBAL						loadResource(HRSRC resource);
			static HCURSOR				loadStandardCursor(const ResourceID& id);
			static HICON				loadStandardIcon(const ResourceID& id);
			int							loadString(UINT id, TCHAR* buffer, int maxLength) const;
			std::basic_string<TCHAR>	loadString(UINT id, const MessageArguments& args = MessageArguments()) const;
			DWORD						sizeofResource(HRSRC resource);

		protected:
			HACCEL getAccelerators() const throw() {return accelerators_;}
		private:
			TCHAR fileName_[MAX_PATH];
			HACCEL accelerators_;
		};

#define MARGS manah::windows::Module::MessageArguments()


template<class TopWindow = ui::Window> class Application : public Module {
public:
	// コンストラクタ
	explicit Application(TopWindow* topWindow = 0);
	virtual ~Application() {}
	// メソッド
	static void		getCommandLineArguments(const TCHAR* cmdLine, std::vector<std::basic_string<TCHAR> >& args);
	void			getCommandLineArguments(std::vector<std::basic_string<TCHAR> >& args) const;
	TopWindow&		getMainWindow() const;
	virtual int		run(int showCommand);
protected:
	virtual LRESULT	dispatchEvent(HWND window, UINT message, WPARAM wParam, LPARAM lParam) = 0;
	virtual bool	initInstance(int showCommand) = 0;
	void			popModelessDialog(HWND dialog);
	virtual bool	preTranslateMessage(const MSG& msg);
	void			pushModelessDialog(HWND dialog);
	void			setMainWindow(TopWindow& window) {mainWindow_ = &window;}
private:
	TopWindow* mainWindow_;
	std::set<HWND> modelessDialogs_;	// modeless dialogs to be called isDialogMessage
	bool running_;
};


template<class TopWindow = ui::Window>
class ProfilableApplication : public Application<TopWindow> {
public:
	// コンストラクタ
	explicit ProfilableApplication(TopWindow* topWindow = 0, const TCHAR* iniFileName = 0);
	// メソッド
	const TCHAR*				getINIFileName() const throw() {return iniFileName_;}
	UINT						readIntegerProfile(const TCHAR* section, const TCHAR* key, UINT defaultValue = 0);
	std::basic_string<TCHAR>	readStringProfile(const TCHAR* section, const TCHAR* key, const TCHAR* defaultValue = 0);
	template<class T> bool		readStructureProfile(const TCHAR* section, const TCHAR* key, T& value) {
		return toBoolean(::GetPrivateProfileStruct(section, key, &value, sizeof(T), iniFileName_));}
	bool						writeIntegerProfile(const TCHAR* section, const TCHAR* key, UINT value);
	bool						writeStringProfile(const TCHAR* section, const TCHAR* key, const TCHAR* value);
	template<class T> bool		writeStructureProfile(const TCHAR* section, const TCHAR* key, const T& value) {
		return toBoolean(::WritePrivateProfileStruct(section, key, const_cast<T*>(&value), sizeof(T), iniFileName_));}
private:
	TCHAR iniFileName_[MAX_PATH];
};


// Module ///////////////////////////////////////////////////////////////////

inline Module::Module(HMODULE handle) : HandleHolder<HMODULE>(handle), accelerators_(0) {
	assert(handle != 0); ::GetModuleFileName(handle, fileName_, MAX_PATH);}

inline HRSRC Module::findResource(const ResourceID& id, const TCHAR* type) {return ::FindResource(get(), id.name, type);}

inline HRSRC Module::findResource(const ResourceID& id, const TCHAR* type, WORD language) {return ::FindResourceEx(get(), id.name, type, language);}

inline const TCHAR* Module::getModuleFileName() const {return fileName_;}

inline bool Module::loadAccelerators(const ResourceID& id) {return (accelerators_ = ::LoadAccelerators(get(), id.name)) != 0;}

inline HBITMAP Module::loadBitmap(const ResourceID& id) const {return ::LoadBitmap(get(), id.name);}

inline HCURSOR Module::loadCursor(const ResourceID& id) const {return ::LoadCursor(get(), id.name);}

inline HICON Module::loadIcon(const ResourceID& id) const {return ::LoadIcon(get(), id.name);}

inline HANDLE Module::loadImage(const ResourceID& id, UINT type, int desiredWidth, int desiredHeight, UINT options) const {
	return ::LoadImage(get(), id.name, type, desiredWidth, desiredHeight, options);}

inline HMENU Module::loadMenu(const ResourceID& id) const {return ::LoadMenu(get(), id.name);}

inline std::basic_string<TCHAR> Module::loadMessage(DWORD id, const MessageArguments& args /* = MessageArguments() */) const {
	void* p = 0;
	TCHAR** const pp = new TCHAR*[args.args_.size()];
	std::list<std::basic_string<TCHAR> >::const_iterator it;
	std::size_t i;

	for(it = args.args_.begin(), i = 0; it != args.args_.end(); ++it, ++i)
		pp[i] = const_cast<TCHAR*>(it->c_str());
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE
		| FORMAT_MESSAGE_ARGUMENT_ARRAY, get(), id, LANG_USER_DEFAULT,
		reinterpret_cast<TCHAR*>(&p), 0, reinterpret_cast<va_list*>(pp));
	delete[] pp;
	if(p != 0) {
		TCHAR*		psz_ = static_cast<TCHAR*>(p);
		std::size_t	cch = std::_tcslen(psz_);
		if(cch >= 2 && psz_[cch - 2] == _T('\r') && psz_[cch - 1] == _T('\n'))
			psz_[cch - 2] = 0;
		std::basic_string<TCHAR> temp = psz_;
		::LocalFree(p);
		return temp;
	} else
		return _T("");
}

inline HGLOBAL Module::loadResource(HRSRC resource) {return ::LoadResource(get(), resource);}

inline HCURSOR Module::loadStandardCursor(const ResourceID& id) {return ::LoadCursor(0, id.name);}

inline HICON Module::loadStandardIcon(const ResourceID& id) {return ::LoadIcon(0, id.name);}

inline int Module::loadString(UINT id, TCHAR* buffer, int maxLength) const {return ::LoadString(get(), id, buffer, maxLength);}

inline std::basic_string<TCHAR> Module::loadString(UINT id, const MessageArguments& args /* = MessageArguments() */) const {
	TCHAR buffer[1024];
	loadString(id, buffer, 1024);
	if(args.args_.empty())
		return buffer;

	void* p = 0;
	TCHAR**	pp = new TCHAR*[args.args_.size()];
	std::list<std::basic_string<TCHAR> >::const_iterator it;
	std::size_t i;

	for(it = args.args_.begin(), i = 0; it != args.args_.end(); ++it, ++i)
		pp[i] = const_cast<TCHAR*>(it->c_str());
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING
		| FORMAT_MESSAGE_ARGUMENT_ARRAY, buffer, 0, 0,
		reinterpret_cast<TCHAR*>(&p), 0, reinterpret_cast<va_list*>(pp));
	delete[] pp;

	std::basic_string<TCHAR> temp = static_cast<TCHAR*>(p);
	::LocalFree(p);
	return temp;
}

inline DWORD Module::sizeofResource(HRSRC resource) {return ::SizeofResource(get(), resource);}


// Application //////////////////////////////////////////////////////////////

template<class TopWindow>
inline Application<TopWindow>::Application(TopWindow* topWindow /* = 0 */)
	: Module(::GetModuleHandle(0)), mainWindow_(topWindow), running_(false) {}

template<class TopWindow> inline void Application<TopWindow>::getCommandLineArguments(
		const TCHAR* cmdLine, std::vector<std::basic_string<TCHAR> >& args) {
	assert(cmdLine != 0);
	TCHAR* each = new TCHAR[std::_tcslen(cmdLine) + 1];
	TCHAR ch;
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
			if(ch == _T(' ')) {
				if(eachLength != 0) {
					*(each + eachLength) = 0;
					args.push_back(each);
					eachLength = 0;
				}
			} else if(ch == _T('\"'))
				inQuote = true;
			else {
				*(each + eachLength) = ch;
				++eachLength;
			}
		} else {
			if(ch == _T('\"'))
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

template<class TopWindow> inline void Application<TopWindow>::getCommandLineArguments(std::vector<std::basic_string<TCHAR> >& args) const {
	const TCHAR* cmdLine = ::GetCommandLine();
	std::size_t	 i = 0;
	TCHAR ch;
	TCHAR* quote;

	while(true) {
		ch = cmdLine[i];
		if(ch == 0) {
			args.clear();
			return;
		} else if(ch == _T('\"')) {
			quote = STD_::_tcschr(cmdLine + i + 1, _T('\"'));
			if(quote == 0) {
				args.clear();
				return;
			}
			i = quote - cmdLine + 1;
		} else if(ch == _T('\'')) {
			quote = STD_::_tcschr(cmdLine + i + 1, _T('\''));
			if(quote == 0) {
				args.clear();
				return;
			}
			i = quote - cmdLine + 1;
		} else if(ch == _T(' ')) {
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

template<class TopWindow> inline void Application<TopWindow>::popModelessDialog(HWND dialog) {modelessDialogs_.erase(dialog);}

template<class TopWindow> inline void Application<TopWindow>::pushModelessDialog(HWND dialog) {assert(toBoolean(::IsWindow(dialog))); modelessDialogs_.insert(dialog);}

template<class TopWindow> inline int Application<TopWindow>::run(int showCommand) {
	if(running_)
		return -1;
	MSG msg;
	int f;
	bool isDlgMsg;

	if(!initInstance(showCommand))
		return -1;
	running_ = true;
	while(true) {
		isDlgMsg = false;
		f = ::GetMessage(&msg, 0, 0, 0);
		assert(f != -1);
		if(f == 0)
			return static_cast<int>(msg.wParam);
		for(std::set<HWND>::iterator it = modelessDialogs_.begin(); it != modelessDialogs_.end(); ++it) {
			if(!toBoolean(::IsWindow(*it)))	// auto pop
				it = modelessDialogs_.erase(it);
			else if(toBoolean(::IsDialogMessage(*it, &msg))) {
				isDlgMsg = true;
				break;
			}
		}
		if(!isDlgMsg) {
			if(getAccelerators() != 0) {
				if(!toBoolean(::TranslateAccelerator(msg.hwnd, getAccelerators(), &msg))
						&& !preTranslateMessage(msg)) {
					::TranslateMessage(&msg);
					::DispatchMessage(&msg);
				}
			} else if(!preTranslateMessage(msg)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
	}

	running_ = false;
	return 0;
}


// ProfilableApplication ////////////////////////////////////////////////////

template<class TopWindow> inline ProfilableApplication<TopWindow>::ProfilableApplication(
		TopWindow* topWindow /* = 0 */, const TCHAR* iniFileName /* = 0 */) : Application<TopWindow>(topWindow) {
	if(iniFileName != 0)
		std::_tcscpy(iniFileName_, iniFileName);
	else {
		::GetModuleFileName(0, iniFileName_, MAX_PATH);
		if(TCHAR* const dot = std::_tcsrchr(iniFileName_, _T('.')))
			std::_tcscpy(dot + 1, _T("ini"));
		else
			iniFileName_[0] = 0;
	}
}

template<class TopWindow>
inline UINT ProfilableApplication<TopWindow>::readIntegerProfile(const TCHAR* section, const TCHAR* key, UINT defaultValue /* 0 */) {
	return ::GetPrivateProfileInt(section, key, defaultValue, iniFileName_);}

template<class TopWindow> inline std::basic_string<TCHAR>
ProfilableApplication<TopWindow>::readStringProfile(const TCHAR* section, const TCHAR* key, const TCHAR* defaultValue /* = _T("") */) {
	TCHAR buffer[1024];
	::GetPrivateProfileString(section, key, defaultValue, buffer, countof(buffer), iniFileName_);
	return buffer;
}

template<class TopWindow>
inline bool ProfilableApplication<TopWindow>::writeIntegerProfile(const TCHAR* section, const TCHAR* key, UINT value) {
	TCHAR buffer[32];
	::wsprintf(buffer, _T("%u"), value);
	return writeStringProfile(section, key, buffer);
}

template<class TopWindow>
inline bool ProfilableApplication<TopWindow>::writeStringProfile(const TCHAR* section, const TCHAR* key, const TCHAR* value) {
	return toBoolean(::WritePrivateProfileString(section, key, value, iniFileName_));}

}} // namespace manah::windows

#endif /* MANAH_MODULE_HPP */
