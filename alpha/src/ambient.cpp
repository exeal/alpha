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
	py::object o(py::handle<>(PyUnicode_Check(object) ? py::expect_non_null(object) : ::PyObject_Unicode(object)));
	if(py::ssize_t n = ::PyUnicode_GetSize(o.ptr())) {
		static wchar_t s[0x100];
		const bool large = n > MANAH_COUNTOF(s);
		manah::AutoBuffer<wchar_t> buffer(large ? new wchar_t[n] : 0);
		wchar_t* const p = large ? buffer.get() : s;
		n = ::PyUnicode_AsWideChar(reinterpret_cast<PyUnicodeObject*>(o.ptr()), p, n);
		if(n == -1)
			py::throw_error_already_set();
		p[static_cast<size_t>(n)] = 0;
		return p;
	}
	return wstring();
}

py::object alpha::ambient::convertWideStringToUnicodeObject(const wstring& s) {
	return py::object(py::handle<>(::PyUnicode_FromWideChar(s.data(), s.length())));
}


Interpreter::Interpreter() {
//	::Py_SetProgramName();
	::Py_Initialize();
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

void Interpreter::handleException() {
	if(0 == ::PyErr_Occurred())
		return;
	PyObject* type;
	PyObject* value;
	PyObject* traceback;
	::PyErr_Fetch(&type, &value, &traceback);
	if(type != 0 && value != 0) {
		ostringstream out;
		py::object tracebackModule(py::import("traceback"));
		if(tracebackModule != py::object()) {
			py::object formatException(tracebackModule.attr("format_exception"));
			if(formatException != py::object()) {
				const py::list li(formatException(py::handle<>(py::borrowed(type)),
					py::handle<>(py::borrowed(value)), py::handle<>(py::borrowed(py::allow_null(traceback)))));
				if(li != py::object()) {
					for(py::ssize_t i = 0, c = py::len(li); i < c; ++i)
						out << ::PyString_AsString(py::object(li[i]).ptr());
				}
			}
		}
		::MessageBoxA(Alpha::instance().getMainWindow().use(), out.str().c_str(), "Alpha", MB_ICONEXCLAMATION);
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
					::Py_InitModule(missingModule.c_str(), 0);
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

py::object Interpreter::toplevelPackage() {
	if(package_ == py::object()) {
		package_ = py::object(py::borrowed(::Py_InitModule("ambient", 0)));
		installException("RecoverableError", py::object(py::borrowed(PyExc_Exception)));
		py::scope temp(package_);
		py::def("error", &raiseRecoverableError);
	}
	return package_;
}
