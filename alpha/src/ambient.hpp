/**
 * @file ambient.hpp
 */

#ifndef ALPHA_AMBIENT_HPP
#define ALPHA_AMBIENT_HPP
#include <ascension/corelib/basic-types.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#define BOOST_PYTHON_STATIC_LIB
#include <boost/python.hpp>
#include <list>
#include <map>

namespace alpha {
	namespace ambient {
/*
		std::wstring convertUnicodeObjectToWideString(PyObject* object);
		inline boost::python::str makePythonString(const std::string& source) {
			return boost::python::str(boost::python::handle<>(::PyUnicode_FromStringAndSize(source.data(), source.length())));
//			return boost::python::object(boost::python::handle<>(::PyUnicode_FromStringAndSize(source.data(), source.length())));
		}
		inline boost::python::str makePythonString(const std::wstring& source) {
			return boost::python::str(boost::python::handle<>(::PyUnicode_FromWideChar(source.data(), source.length())));
//			return boost::python::object(boost::python::handle<>(::PyUnicode_FromWideChar(source.data(), source.length())));
		}
		inline boost::python::str makePythonString(const Glib::ustring& source) {
			return makePythonString(source.raw());
		}
*/
		class Interpreter : private boost::noncopyable {
		public:
			static const std::uint32_t LOWEST_INSTALLATION_ORDER = static_cast<std::uint32_t>(-1);
		public:
			~Interpreter() BOOST_NOEXCEPT;
			void addInstaller(void (*installer)(), std::uint32_t order);
			boost::python::object executeFile(const boost::filesystem::path& fileName);
			void install();
			static Interpreter& instance();
			// package and modules
			boost::python::object module(const std::string& name);
			boost::python::object toplevelPackage();
			// exceptions
			boost::python::object exceptionClass(const std::string& name) const;
			void handleException();
			template<typename Exception>
			void installException(const std::string& name, boost::python::object base = boost::python::object());
			void raiseException(const std::string& name, boost::python::object value);
			void raiseLastSystemError(const boost::filesystem::path& fileName = boost::filesystem::path());
			// commands
			boost::python::object executeCommand(boost::python::object command);
//			boost::python::ssize_t numericPrefix() const /*throw()*/;
			void setNumericPrefix(boost::python::ssize_t n) BOOST_NOEXCEPT;
			void unsetNumericPrefix() BOOST_NOEXCEPT;
		private:
			Interpreter();
		private:
			struct Installer {
				std::uint32_t order;
				void (*function)();
			};
			boost::python::object package_;
			std::list<Installer> installers_;
			std::map<const std::string, boost::python::object> exceptionClasses_;
			boost::optional<boost::python::ssize_t> numericPrefix_;
		};

		/**
		 * @tparam Exception The exception type to install
		 * @param name The name of the exception type. The qualified name is prefixed by "ambient."
		 * @param base The base type of the exception registered newly
		 */
		template<typename Exception>
		inline void Interpreter::installException(const std::string& name, boost::python::object base /* = py::object() */) {
			if(exceptionClasses_.find(name) != std::end(exceptionClasses_))
				throw std::invalid_argument("the exception with the given name has already been installed.");

			const std::string fullName("ambient." + name);
			boost::python::object newException(boost::python::handle<>(::PyErr_NewException(fullName.c_str(), base.ptr(), nullptr)));
			if(-1 == ::PyModule_AddObject(toplevelPackage().ptr(), name.c_str(), newException.ptr()))
				throw std::runtime_error("PyModule_AddObject failed.");

			boost::python::register_exception_translator<Exception>([&newException](const Exception& e) {
				::PyErr_SetString(newException.ptr(), e.what());
			});

			exceptionClasses_.insert(std::make_pair(name, newException));
		}
	}
}

#define ALPHA_EXPOSE_PROLOGUE(order)				\
	namespace {										\
	const std::uint32_t installationOrder = order;	\
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
