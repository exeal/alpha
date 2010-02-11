/**
 * @file ambient.cpp
 */

#include "ambient.hpp"
#include "application.hpp"
#include "../resource/messages.h"
using namespace alpha::ambient;
using namespace std;
namespace py = boost::python;


wstring alpha::ambient::convertUnicodeObjectToWideString(PyObject* object) {
	const py::object o(PyUnicode_Check(object) ?
		py::object(py::borrowed(py::expect_non_null(object)))
		: py::object(py::handle<>(::PyObject_Str(object))));
	if(py::ssize_t n = ::PyUnicode_GetSize(o.ptr())) {
		static wchar_t s[0x100];
		const bool large = n > MANAH_COUNTOF(s);
		manah::AutoBuffer<wchar_t> buffer(large ? new wchar_t[n + 1] : 0);
		wchar_t* const p = large ? buffer.get() : s;
		n = ::PyUnicode_AsWideChar(reinterpret_cast<PyUnicodeObject*>(o.ptr()), p, n);
		if(n == -1)
			py::throw_error_already_set();
		return wstring(p, n);
	}
	return wstring();
}

Interpreter::Interpreter() : numericPrefix_(make_pair(false, 0)) {
//	::Py_SetProgramName();
	::Py_Initialize();
	assert(::Py_IsInitialized() != 0);
}

Interpreter::~Interpreter() {
	::PyErr_Clear();
//	::Py_Finalize();
}

void Interpreter::addInstaller(void (*installer)(), manah::uint order) {
	list<Installer>::iterator i(installers_.begin());
	for(const list<Installer>::iterator e(installers_.end()); i != e; ++i) {
		if(order != LOWEST_INSTALLATION_ORDER && i->order == order)
			throw invalid_argument("");
		else if(i->order > order)
			break;
	}
	Installer newInstaller = {order, installer};
	installers_.insert(i, newInstaller);
}

namespace {
	void raiseRecoverableError(py::object value) {
		static py::object klass(Interpreter::instance().exceptionClass("RecoverableError"));
		::PyErr_SetObject(klass.ptr(), value.ptr());
		py::throw_error_already_set();
	}
}

py::object Interpreter::exceptionClass(const string& name) const {
	map<const string, py::object>::const_iterator i(exceptionClasses_.find(name));
	if(i == exceptionClasses_.end())
		throw invalid_argument("the exception class with the given name not found.");
	return i->second;
}

py::object Interpreter::executeCommand(py::object command) {
	try {
		if(numericPrefix_.first) {
			unsetNumericPrefix();
			// use numeric prefix?
			if(PyFunction_Check(command.ptr())) {
				// check if this function has a argument named "n"
				const py::object code(py::borrowed<>(::PyFunction_GetCode(command.ptr())));
				if(PyCode_Check(code.ptr())) {
					bool hasN = false;
					if((py::extract<int>(code.attr("co_flags")) & 0x0008/*CO_VARKEYWORDS*/) != 0)
						hasN = true;
					else {
						const py::str STRING_N("n");
						const py::tuple argumentNames(py::extract<py::tuple>(code.attr("co_varnames")));
						const py::ssize_t c = py::extract<py::ssize_t>(code.attr("co_argcount"));
						for(py::ssize_t i = 0; i < c; ++i) {
							if(::PyObject_RichCompareBool(static_cast<py::object>(argumentNames[i]).ptr(), STRING_N.ptr(), Py_EQ) == 1) {
								hasN = true;
								break;
							}
						}
					}
					if(hasN) {
						py::tuple args;
						py::dict kw;
						kw["n"] = numericPrefix_.second;
						return py::object(py::handle<>(::PyObject_Call(command.ptr(), args.ptr(), kw.ptr())));
					}
				}
			}
		}
		return command();
	} catch(py::error_already_set&) {
		if(::PyErr_ExceptionMatches(exceptionClass("RecoverableError").ptr()) != 0) {
			PyObject* type;
			PyObject* value;
			PyObject* traceback;
			::PyErr_Fetch(&type, &value, &traceback);
			if(type != 0 && value != 0) {
				::MessageBoxW(Alpha::instance().getMainWindow().use(),
					convertUnicodeObjectToWideString(value).c_str(), L"Alpha", MB_ICONEXCLAMATION);
				return py::object();
			}
			::PyErr_Restore(type, value, traceback);
		}
		handleException();
	}
	return py::object();
}

py::object Interpreter::executeFile(const wstring& fileName) {
	HANDLE file = ::CreateFileW(fileName.c_str(), GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if(file == INVALID_HANDLE_VALUE) {
		char s[MAX_PATH + 64];
		sprintf(s, "No such file or directory: '%s'", fileName.c_str());
		::PyErr_SetString(PyExc_IOError, s);
		py::throw_error_already_set();
	}

	PyObject* result = 0;
	HANDLE mapping = ::CreateFileMappingW(file, 0, PAGE_READONLY, 0, 0, 0);
	if(mapping != 0) {
		// read the input file
		const char* const content = static_cast<const char*>(::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
		if(content != 0) {
			LARGE_INTEGER fileSize;
			if(!manah::toBoolean(::GetFileSizeEx(file, &fileSize)))
				raiseLastWin32Error();
			manah::AutoBuffer<char> script;
			try {
				script.reset(new char[fileSize.QuadPart + 1]);
			} catch(const bad_alloc& e) {
				::PyErr_SetString(PyExc_MemoryError, e.what());
			}
			// convert the file content into script text
			char* e = script.get();
			for(const char* p = content; ; ++e) {
				if(const char* eol = strpbrk(p, "\n\r")) {
					memcpy(e, p, eol - p);
					*(e += (eol - p)) = '\n';
					p = eol + ((*eol == '\r' && eol[1] == '\n') ? 2 : 1);
				} else {
					strcpy(e, p);
					break;
				}
			}
			PyObject* const code = ::Py_CompileString(
				script.get(), u2a(fileName.c_str(), fileName.c_str() + fileName.length() + 1).get(), Py_file_input);
			if(code == 0)
				py::throw_error_already_set();
			const py::object ns(py::import("__main__").attr("__dict__"));
			manah::AutoBuffer<wchar_t> arg0(new wchar_t[fileName.length() + 1]);
			wcscpy(arg0.get(), fileName.c_str());
			wchar_t* argv[1] = {arg0.get()};
			::PySys_SetArgv(1, argv);
			result = ::PyEval_EvalCode(reinterpret_cast<PyCodeObject*>(code), ns.ptr(), ns.ptr());
			static wchar_t emptyString[] = L"";
			argv[0] = emptyString;
			::PySys_SetArgv(1, argv);
			::UnmapViewOfFile(content);
		}
		::CloseHandle(mapping);
	}
	::CloseHandle(file);

	if(result == 0)
		py::throw_error_already_set();
	return py::object(py::handle<>(result));
}

void Interpreter::handleException() {
	if(0 == ::PyErr_Occurred())
		return;
	PyObject* type;
	PyObject* value;
	PyObject* traceback;
	::PyErr_Fetch(&type, &value, &traceback);
	if(type != 0 && value != 0) {
		if(::PyErr_GivenExceptionMatches(value, type) == 0)
			::PyErr_NormalizeException(&type, &value, &traceback);
		wostringstream out;
		const py::object none;
		py::object tracebackModule(py::import("traceback"));
		if(tracebackModule != none) {
			const py::object formatException(tracebackModule.attr("format_exception"));
			if(formatException != none) {
#ifdef _DEBUG
				try {
#endif // _DEBUG
					const py::list lines(formatException(
						py::borrowed(type), py::borrowed(value), py::handle<>(py::allow_null(py::borrowed(traceback)))));
					if(lines != none) {
						for(py::ssize_t i = 0, c = py::len(lines); i < c; ++i)
							out << convertUnicodeObjectToWideString(py::object(lines[i]).ptr());
					}
#ifdef _DEBUG
				} catch(py::error_already_set&) {
					Py_DECREF(type);
					Py_DECREF(value);
					Py_XDECREF(traceback);
					type = value = traceback = 0;
					::PyErr_Fetch(&type, &value, &traceback);
					::OutputDebugStringW(convertUnicodeObjectToWideString(value).c_str());
					::DebugBreak();
				}
#endif // _DEBUG
			}
		}
		::MessageBoxW(Alpha::instance().getMainWindow().use(), out.str().c_str(), L"Alpha", MB_ICONEXCLAMATION);
	}
	Py_XDECREF(type);
	Py_XDECREF(value);
	Py_XDECREF(traceback);
	::PyErr_Clear();
}

void Interpreter::install() {
	for(list<Installer>::const_iterator i(installers_.begin()), e(installers_.end()); i != e; ++i)
		(*i->function)();
	installers_.clear();
}

void Interpreter::installException(const string& name, py::object base /* = py::object() */) {
	if(exceptionClasses_.find(name) != exceptionClasses_.end())
		throw invalid_argument("the exception with the given name has already been installed.");
	manah::AutoBuffer<char> fullName(new char[name.length() + 9]);
	strcpy(fullName.get(), "ambient.");
	strcat(fullName.get(), name.c_str());
	py::object newException(py::handle<>(::PyErr_NewException(fullName.get(), base.ptr(), 0)));
	if(-1 == ::PyModule_AddObject(toplevelPackage().ptr(), name.c_str(), newException.ptr()))
		throw runtime_error("PyModule_AddObject failed.");
	exceptionClasses_.insert(make_pair(name, newException));
}

Interpreter& Interpreter::instance() {
	static Interpreter singleton;
	return singleton;
}

py::object Interpreter::module(const string& name) {
	py::object parent(toplevelPackage());
	string moduleName;
	for(string::size_type dot = name.find('.'), previousDot = 0; ; previousDot = dot + 1, dot = name.find('.', previousDot)) {
		moduleName = name.substr(previousDot, (dot != string::npos) ? dot - previousDot : string::npos);
		if(!py::dict(parent.attr("__dict__")).has_key(moduleName)) {
			const string missingModule("ambient." + name.substr(0, dot));
			if(PyObject* newModule = ::PyImport_AddModule(missingModule.c_str())) {
				if(0 == ::PyModule_AddObject(parent.ptr(), moduleName.c_str(), newModule)) {
					::PyImport_AddModule(missingModule.c_str());	// Py_InitModule was used in 2.x
					if(dot == string::npos)
						return py::object(py::handle<>(py::borrowed(newModule)));
				}
			}
			throw runtime_error("failed to initialize the module.");
		}
		if(dot == string::npos)
			break;
		parent = parent.attr(moduleName.c_str());
	}
	return parent.attr(moduleName.c_str());
}

/**
 * Sets error indicator with the installed exception.
 * @param name the name of the installed exception type
 * @param value the exception value
 * @throw std#invalid_argument the exception type specified by @a name is not installed
 */
void Interpreter::raiseException(const string& name, py::object value) {
	map<const string, py::object>::const_iterator exception(exceptionClasses_.find(name));
	if(exception == exceptionClasses_.end())
		throw invalid_argument("specified exception is not installed.");
	::PyErr_SetObject(exception->second.ptr(), value.ptr());
	py::throw_error_already_set();
}

void Interpreter::raiseLastWin32Error() {
	::PyErr_SetFromWindowsErr(0);
	py::throw_error_already_set();
}

void Interpreter::setNumericPrefix(py::ssize_t n) {
	numericPrefix_.first = true;
	numericPrefix_.second = n;
}

py::object Interpreter::toplevelPackage() {
	if(package_ == py::object()) {
		package_ = py::object(py::borrowed(::PyImport_AddModule("ambient")));	// Py_InitModule was used in 2.x
		installException("RecoverableError", py::object(py::borrowed(PyExc_Exception)));
		py::scope temp(package_);
		py::def("error", &raiseRecoverableError);
	}
	return package_;
}

void Interpreter::unsetNumericPrefix() {
	numericPrefix_.first = false;
}
