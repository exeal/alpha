/**
 * @file ambient.cpp
 */

#ifndef ALPHA_NO_AMBIENT
#include "ambient.hpp"
#include "application.hpp"
//#include "../resource/messages.h"
#include <ascension/log.hpp>
//#include <boost/foreach.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <glibmm/i18n.h>
#include <glibmm/ustring.h>
#include <gtkmm/messagedialog.h>
#include <codecvt>

namespace alpha {
	namespace ambient {
		// Glib.ustring <=> PyObject conversion
		namespace {
			struct UstringToPython {
				static PyObject* convert(const Glib::ustring& s) {
					return ::PyUnicode_FromStringAndSize(s.data(), boost::implicit_cast<boost::python::ssize_t>(s.length()));
				}
				static const PyTypeObject* get_pytype() {
					return &PyUnicode_Type;
				}
			};

			struct UstringFromPython {
				UstringFromPython() {
					boost::python::converter::registry::push_back(&convertible, &construct, boost::python::type_id<Glib::ustring>(), &UstringToPython::get_pytype);
				}

			private:
				static void construct(PyObject* object, boost::python::converter::rvalue_from_python_stage1_data* data) {
					typedef PyObject* (*convertibleFunction)(PyObject*);
					convertibleFunction creator = *static_cast<convertibleFunction>(data->convertible);
					boost::python::handle<> intermediate(creator(object));

					void* const storage = reinterpret_cast<boost::python::converter::rvalue_from_python_storage<Glib::ustring>*>(data)->storage.bytes;
					new(storage) Glib::ustring(::PyBytes_AsString(intermediate.get()), ::PyBytes_Size(intermediate.get()));
					data->convertible = storage;
				}
				static void* convertible(PyObject* object) {
					if(PyUnicode_Check(object))
						return &::PyUnicode_AsUTF8String;
					if(PyBytes_Check(object))
						return &identity;
					return nullptr;
				}
				static PyObject* identity(PyObject* p) {
					boost::python::incref(p);
					return p;
				}
			};
		}
/*
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
*/
		Interpreter::Interpreter() {
//			::Py_SetProgramName();
			::Py_Initialize();
			assert(::Py_IsInitialized() != 0);
		}

		Interpreter::~Interpreter() {
			::PyErr_Clear();
//			::Py_Finalize();
		}

		void Interpreter::addInstaller(void (*installer)(), std::uint32_t order) {
			std::list<Installer>::iterator i(std::begin(installers_));
			for(const std::list<Installer>::iterator e(std::end(installers_)); i != e; ++i) {
				if(order != LOWEST_INSTALLATION_ORDER && i->order == order)
					throw std::invalid_argument("");
				else if(i->order > order)
					break;
			}
			Installer newInstaller = {order, installer};
			installers_.insert(i, newInstaller);
		}

		namespace {
			void raiseRecoverableError(boost::python::object value) {
				static boost::python::object klass(Interpreter::instance().exceptionClass("RecoverableError"));
				::PyErr_SetObject(klass.ptr(), value.ptr());
				boost::python::throw_error_already_set();
			}
		}

		boost::python::object Interpreter::exceptionClass(const std::string& name) const {
			std::map<const std::string, boost::python::object>::const_iterator i(exceptionClasses_.find(name));
			if(i == std::end(exceptionClasses_))
				throw std::invalid_argument("the exception class with the given name not found.");
			return i->second;
		}

		boost::python::object Interpreter::executeCommand(boost::python::object command) {
			try {
				if(numericPrefix_ != boost::none) {
					unsetNumericPrefix();
					// use numeric prefix?
					if(PyFunction_Check(command.ptr())) {
						// check if this function has a argument named "n"
						const boost::python::object code(boost::python::borrowed<>(::PyFunction_GetCode(command.ptr())));
						if(PyCode_Check(code.ptr())) {
							bool hasN = false;
							if((boost::python::extract<int>(code.attr("co_flags")) & 0x0008/*CO_VARKEYWORDS*/) != 0)
								hasN = true;
							else {
								const boost::python::str STRING_N("n");
								const boost::python::tuple argumentNames(boost::python::extract<boost::python::tuple>(code.attr("co_varnames")));
								const boost::python::ssize_t c = boost::python::extract<boost::python::ssize_t>(code.attr("co_argcount"));
								for(boost::python::ssize_t i = 0; i < c; ++i) {
									if(::PyObject_RichCompareBool(static_cast<boost::python::object>(argumentNames[i]).ptr(), STRING_N.ptr(), Py_EQ) == 1) {
										hasN = true;
										break;
									}
								}
							}
							if(hasN) {
								boost::python::tuple args;
								boost::python::dict kw;
								kw["n"] = boost::get(numericPrefix_);
								return boost::python::object(boost::python::handle<>(::PyObject_Call(command.ptr(), args.ptr(), kw.ptr())));
							}
						}
					}
				}
				return command();
			} catch(boost::python::error_already_set&) {
				if(::PyErr_ExceptionMatches(exceptionClass("RecoverableError").ptr()) != 0) {
					PyObject* type;
					PyObject* value;
					PyObject* traceback;
					::PyErr_Fetch(&type, &value, &traceback);
					if(type != nullptr && value != nullptr) {
						const boost::python::extract<Glib::ustring> message(value);
						if(message.check()) {
							Gtk::MessageDialog dialog(Application::instance()->window(), static_cast<Glib::ustring>(message).c_str(), false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
							dialog.set_title(_("Alpha"));
							dialog.run();
							return boost::python::object();
						}
					}
					::PyErr_Restore(type, value, traceback);
				}
				handleException();
			}
			return boost::python::object();
		}

		boost::python::object Interpreter::executeFile(const boost::filesystem::path& fileName) {
			if(!boost::filesystem::exists(fileName)) {
				const std::string s = "No such file or directory: '"
#ifdef BOOST_OS_WINDOWS
					+ fileName.string(std::codecvt_utf8_utf16<wchar_t>());
#else
					+ fileName.native();
#endif
				::PyErr_SetString(PyExc_IOError, s.c_str());
				boost::python::throw_error_already_set();
			}
			boost::filesystem::ifstream file(fileName, std::ios_base::in /*| std::ios_base::binary*/);
			if(!file.is_open())
				raiseLastSystemError(fileName);

			boost::python::handle<> result;
			try {
				// read the input file
				std::string script;
				script.reserve(static_cast<std::string::size_type>(boost::filesystem::file_size(fileName)));	// TODO: May overflow.
				for(std::string line; std::getline(file, line); )
					script += line;

				std::string fileNameString;
#ifdef BOOST_OS_WINDOWS
				{
					boost::python::object s(fileName.native());
					boost::python::handle<> a(::PyUnicode_EncodeFSDefault(s.ptr()));
					fileNameString.assign(::PyByteArray_AsString(a.get()), ::PyByteArray_Size(a.get()));
				}
#else
				fileNameString = fileName.native();
#endif

				const boost::python::handle<> code(::Py_CompileString(script.c_str(), fileNameString.c_str(), Py_file_input));
				const boost::python::object ns(boost::python::import("__main__").attr("__dict__"));
				wchar_t* argv[1] = {const_cast<wchar_t*>(fileName.wstring().c_str())};
				::PySys_SetArgv(1, argv);
				result = boost::python::handle<>(::PyEval_EvalCode(code.get(), ns.ptr(), ns.ptr()));
				static wchar_t emptyString[] = L"";
				argv[0] = emptyString;
				::PySys_SetArgv(1, argv);
			} catch(const std::bad_alloc& e) {
				::PyErr_SetString(PyExc_MemoryError, e.what());
				boost::python::throw_error_already_set();
			} catch(const boost::filesystem::filesystem_error&) {
				raiseLastSystemError(fileName);
			} catch(const boost::python::error_already_set&) {
				throw;
			}

			if(result == 0)
				boost::python::throw_error_already_set();
			return boost::python::object(boost::python::handle<>(result));
		}

		void Interpreter::handleException() {
			if(::PyErr_Occurred() == nullptr)
				return;
			PyObject* type;
			PyObject* value;
			PyObject* traceback;
			::PyErr_Fetch(&type, &value, &traceback);
			if(type != nullptr && value != nullptr) {
				if(::PyErr_GivenExceptionMatches(value, type) == 0)
					::PyErr_NormalizeException(&type, &value, &traceback);
				Glib::ustring message;
				const boost::python::object none;
				boost::python::object tracebackModule(boost::python::import("traceback"));
				if(tracebackModule != none) {
					const boost::python::object formatException(tracebackModule.attr("format_exception"));
					if(formatException != none) {
#ifdef _DEBUG
						try {
#endif // _DEBUG
							const boost::python::list lines(formatException(
								boost::python::borrowed(type), boost::python::borrowed(value),
								boost::python::handle<>(boost::python::allow_null(boost::python::borrowed(traceback)))));
							if(lines != none) {
								for(boost::python::ssize_t i = 0, c = boost::python::len(lines); i < c; ++i)
									message += boost::python::extract<Glib::ustring>(boost::python::object(lines[i]));
							}
#ifdef _DEBUG
						} catch(const boost::python::error_already_set&) {
							boost::python::decref(type);
							boost::python::decref(value);
							boost::python::xdecref(traceback);
							type = value = traceback = nullptr;
							::PyErr_Fetch(&type, &value, &traceback);
							ASCENSION_LOG_TRIVIAL(debug) << static_cast<std::string>(boost::python::extract<std::string>(value));
						}
#endif // _DEBUG
					}
				}
				Gtk::MessageDialog dialog(Application::instance()->window(), message, false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
				dialog.set_title(_("Alpha"));
				dialog.run();
			}
			boost::python::xdecref(type);
			boost::python::xdecref(value);
			boost::python::xdecref(traceback);
			::PyErr_Clear();
		}

		void Interpreter::install() {
			boost::python::to_python_converter<Glib::ustring, UstringToPython, true>();
			UstringFromPython();
			boost::for_each(installers_, [](Installer& installer) {
				(*installer.function)();
			});
			installers_.clear();
		}

		Interpreter& Interpreter::instance() {
			static Interpreter singleton;
			return singleton;
		}

		boost::python::object Interpreter::module(const std::string& name) {
			boost::python::object parent(toplevelPackage());
			std::string moduleName;
			for(std::string::size_type dot = name.find('.'), previousDot = 0; ; previousDot = dot + 1, dot = name.find('.', previousDot)) {
				moduleName = name.substr(previousDot, (dot != std::string::npos) ? dot - previousDot : std::string::npos);
				if(!boost::python::dict(parent.attr("__dict__")).has_key(moduleName)) {
					const std::string missingModule("ambient." + name.substr(0, dot));
					if(PyObject* newModule = ::PyImport_AddModule(missingModule.c_str())) {
						if(0 == ::PyModule_AddObject(parent.ptr(), moduleName.c_str(), newModule)) {
							::PyImport_AddModule(missingModule.c_str());	// Py_InitModule was used in 2.x
							if(dot == std::string::npos)
								return boost::python::object(boost::python::handle<>(boost::python::borrowed(newModule)));
						}
					}
					throw std::runtime_error("failed to initialize the module.");
				}
				if(dot == std::string::npos)
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
		void Interpreter::raiseException(const std::string& name, boost::python::object value) {
			::PyErr_SetObject(exceptionClass(name).ptr(), value.ptr());
			boost::python::throw_error_already_set();
		}

		/**
		 * Sets error indicator with the last system error (POSIX errno or Win32 *etLastError).
		 * @param fileName
		 */
		void Interpreter::raiseLastSystemError(const boost::filesystem::path& fileName /* = boost::filesystem::path() */) {
			const boost::python::str fileNameObject(fileName.native());
			::PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, fileNameObject.ptr());
			boost::python::throw_error_already_set();
		}

		void Interpreter::setNumericPrefix(boost::python::ssize_t n) BOOST_NOEXCEPT {
			numericPrefix_ = n;
		}

		boost::python::object Interpreter::toplevelPackage() {
			if(package_ == boost::python::object()) {
				package_ = boost::python::object(boost::python::borrowed(::PyImport_AddModule("ambient")));	// Py_InitModule was used in 2.x
//				installException<int>("RecoverableError", boost::python::object(boost::python::borrowed(PyExc_Exception)));
				boost::python::scope temp(package_);
				boost::python::def("error", &raiseRecoverableError);
			}
			return package_;
		}

		void Interpreter::unsetNumericPrefix() BOOST_NOEXCEPT {
			numericPrefix_ = boost::none;
		}
	}
}

#endif // !ALPHA_NO_AMBIENT
