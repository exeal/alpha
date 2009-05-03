/**
 * @file ambient.cpp
 */

#include "ambient.hpp"
#include "application.hpp"
#include "../resource/messages.h"
using namespace alpha::ambient;
using namespace std;
namespace py = boost::python;


wstring alpha::ambient::toWideString(PyObject* object) {
	py::object o(py::handle<>(PyUnicode_Check(object) ? py::expect_non_null(object) : ::PyObject_Unicode(object)));
	if(py::ssize_t n = ::PyUnicode_GetSize(o.ptr())) {
		static wchar_t s[0x100];
		const bool large = n > MANAH_COUNTOF(s);
		manah::AutoBuffer<wchar_t> buffer(large ? new wchar_t[n] : 0);
		wchar_t* const p = large ? buffer.get() : s;
		n = ::PyUnicode_AsWideChar(reinterpret_cast<PyUnicodeObject*>(o.ptr()), p, n);
		p[static_cast<size_t>(n)] = 0;
		return p;
	}
	return wstring();
}


Interpreter::Interpreter() {
//	::Py_SetProgramName();
	::Py_Initialize();
}

Interpreter::~Interpreter() {
	::PyErr_Clear();
//	::Py_Finalize();
}

void Interpreter::addInstaller(void (*installer)()) {
	installers_.push(installer);
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
				const py::list li(formatException(py::object(py::borrowed<>(type)),
					py::object(py::borrowed<>(value)), py::object(py::borrowed<>(traceback))));
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
	while(!installers_.empty()) {
		(*installers_.top())();
		installers_.pop();
	}
}

Interpreter& Interpreter::instance() {
	static Interpreter singleton;
	return singleton;
}

py::object Interpreter::module(const char* name) {
	py::object ambient(toplevelPackage());
	py::dict d(ambient.attr("__dict__"));
	if(!d.has_key(name)) {
		string fullName("ambient.");
		fullName += name;
		if(PyObject* newModule = ::PyImport_AddModule(fullName.c_str())) {
			if(0 == ::PyModule_AddObject(ambient.ptr(), name, newModule)) {
				::Py_InitModule(fullName.c_str(), 0);
				return py::object(py::handle<>(py::borrowed(newModule)));
			}
		}
		throw runtime_error("failed to initialize the module.");
	}
	return ambient.attr(name);
}
