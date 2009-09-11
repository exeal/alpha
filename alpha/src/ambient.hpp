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
		boost::python::object convertWideStringToUnicodeObject(const std::wstring& s);
		template<typename Exception> class CppStdExceptionTranslator {
		public:
			explicit CppStdExceptionTranslator(PyObject* type) : type_(type) {assert(type != 0);}
			void operator()(const Exception& e) const {::PyErr_SetString(type_, e.what());}
		private:
			PyObject* const type_;
		};

		class Interpreter {
			MANAH_NONCOPYABLE_TAG(Interpreter);
		public:
			static const manah::uint LOWEST_INSTALLATION_ORDER = static_cast<manah::uint>(-1);
		public:
			~Interpreter() /*throw()*/;
			void addInstaller(void (*installer)(), manah::uint order);
			boost::python::object executeCommand(boost::python::object command);
			boost::python::object executeFile(boost::python::str fileName);
			void install();
			static Interpreter& instance();
			// package and modules
			boost::python::object module(const std::string& name);
			boost::python::object toplevelPackage();
			// exceptions
			void handleException();
			void installException(const std::string& name, boost::python::object base = boost::python::object());
			void raiseException(const std::string& name, boost::python::object value);
			void raiseLastWin32Error();
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
		};

		inline boost::python::object Interpreter::executeFile(boost::python::str fileName) {
			boost::python::object ns(boost::python::import("__main__").attr("__dict__"));
			return boost::python::exec_file(fileName, ns, ns);
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
