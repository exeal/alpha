/**
 * @file ambient.hpp
 */

#ifndef ALPHA_AMBIENT_HPP
#define ALPHA_AMBIENT_HPP
#include <manah/object.hpp>	// MANAH_NONCOPYABLE_TAG
#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <list>
#include <map>

namespace alpha {
	namespace ambient {

		std::wstring convertUnicodeObjectToWideString(PyObject* object);
		template<typename T> T convertWideStringToUnicodeObject(const std::wstring& s);
		template<typename Exception> class CppStdExceptionTranslator {
		public:
			explicit CppStdExceptionTranslator(boost::python::object type) : type_(type) {assert(type != 0);}
			void operator()(const Exception& e) const {::PyErr_SetString(type_.ptr(), e.what());}
		private:
			const boost::python::object type_;
		};

		class Interpreter {
			MANAH_NONCOPYABLE_TAG(Interpreter);
		public:
			static const manah::uint LOWEST_INSTALLATION_ORDER = static_cast<manah::uint>(-1);
		public:
			~Interpreter() /*throw()*/;
			void addInstaller(void (*installer)(), manah::uint order);
			boost::python::object executeFile(const std::wstring& fileName);
			void install();
			static Interpreter& instance();
			// package and modules
			boost::python::object module(const std::string& name);
			boost::python::object toplevelPackage();
			// exceptions
			boost::python::object exceptionClass(const std::string& name) const;
			void handleException();
			void installException(const std::string& name, boost::python::object base = boost::python::object());
			void raiseException(const std::string& name, boost::python::object value);
			void raiseLastWin32Error();
			// commands
			boost::python::object executeCommand(boost::python::object command);
//			boost::python::ssize_t numericPrefix() const /*throw()*/;
			void setNumericPrefix(boost::python::ssize_t n) /*throw()*/;
			void unsetNumericPrefix() /*throw()*/;
		private:
			Interpreter();
		private:
			struct Installer {
				manah::uint order;
				void (*function)();
			};
			boost::python::object package_;
			std::list<Installer> installers_;
			std::map<const std::string, boost::python::object> exceptionClasses_;
			std::pair<bool, boost::python::ssize_t> numericPrefix_;
		};

		template<> inline boost::python::object convertWideStringToUnicodeObject<boost::python::object>(const std::wstring& s) {
			return boost::python::object(boost::python::handle<>(::PyUnicode_FromWideChar(s.data(), s.length())));
		}

		template<> inline boost::python::str convertWideStringToUnicodeObject<boost::python::str>(const std::wstring& s) {
			return boost::python::str(boost::python::handle<>(::PyUnicode_FromWideChar(s.data(), s.length())));
		}
	}
}

#define ALPHA_EXPOSE_PROLOGUE(order)				\
	namespace {										\
	const manah::uint installationOrder = order;	\
		void installAPIs() {

#define ALPHA_EXPOSE_EPILOGUE()																			\
		}																								\
		struct Exposer {																				\
			Exposer() {																					\
				alpha::ambient::Interpreter::instance().addInstaller(&installAPIs, installationOrder);	\
			}																							\
		} exposer;																						\
	}



#endif // !ALPHA_AMBIENT_HPP
