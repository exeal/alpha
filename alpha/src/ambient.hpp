/**
 * @file ambient.hpp
 */

#ifndef ALPHA_AMBIENT_HPP
#define ALPHA_AMBIENT_HPP
#include <manah/object.hpp>	// MANAH_NONCOPYABLE_TAG
#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <list>

namespace alpha {
	namespace ambient {

		std::wstring toWideString(PyObject* object);

		class Interpreter {
			MANAH_NONCOPYABLE_TAG(Interpreter);
		public:
			~Interpreter() /*throw()*/;
			void addInstaller(void (*installer)(), manah::uint order);
			boost::python::object executeFile(boost::python::str fileName);
			void handleException();
			void install();
			static Interpreter& instance();
			boost::python::object module(const char* name);
			void throwLastWin32Error();
			boost::python::object toplevelPackage();
		private:
			Interpreter();
		private:
			struct Installer {
				manah::uint order;
				void (*function)();
			};
			boost::python::object package_;
			std::list<Installer> installers_;
		};

		inline void Interpreter::throwLastWin32Error() {
			::PyErr_SetFromWindowsErr(0);
			boost::python::throw_error_already_set();
		}

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
