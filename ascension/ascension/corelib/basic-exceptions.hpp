/**
 * @file basic-exceptions.hpp
 * @author exeal
 * @date 2010-11-07 separated from basic-types.hpp
 */

#ifndef ASCENSION_BASIC_EXCEPTIONS_HPP
#define ASCENSION_BASIC_EXCEPTIONS_HPP

#include <ascension/platforms.hpp>
#include <stdexcept>
#include <sstream>	// std.ostringstream
#include <string>	// std.string
#ifdef ASCENSION_OS_WINDOWS
#	include <ascension/win32/windows.hpp>	// DWORD, GetLastError
#endif // ASCENSION_OS_WINDOWS

namespace ascension {

	/// Pointer argument is @c null but that is not allowed.
	class NullPointerException : public std::invalid_argument {
	public:
		/// Constructor.
		explicit NullPointerException(const std::string& message) : std::invalid_argument(message) {}
		/// Destructor.
		~NullPointerException() throw() {}
	};

	/// The operation was performed in an illegal state.
	class IllegalStateException : public std::logic_error {
	public:
		/// Constructor.
		explicit IllegalStateException(const std::string& message) : std::logic_error(message) {}
	};

	/// The specified index was out of bounds.
	class IndexOutOfBoundsException : public std::out_of_range {
	public:
		/// Default constructor.
		IndexOutOfBoundsException() : std::out_of_range("the index is out of range.") {}
		/// Constructor.
		explicit IndexOutOfBoundsException(const std::string& message) : std::out_of_range(message) {}
	};

	/**
	 * The iterator has reached the end of the enumeration.
	 * @note Not all iterator classes defined in Ascension throw this exception.
	 */
	class NoSuchElementException : public std::runtime_error {
	public:
		/// Default constructor.
		NoSuchElementException() : std::runtime_error("the iterator is end.") {}
		/// Constructor takes an error message.
		explicit NoSuchElementException(const std::string& message) : std::runtime_error(message) {}
	};

	/// Specified value is invalid for enumeration or constant.
	class UnknownValueException : public std::invalid_argument {
	public:
		/// Constructor.
		explicit UnknownValueException(const std::string& message) : invalid_argument(message) {}
	};

	/**
	 * An error whose detail can be identified by an integer (ex. POSIX @c errno, Win32
	 * @c GetLastError, ...
	 * @tparam Code The error code type
	 * @tparam Base The base exception class
	 */
	template<typename Code, typename Base = std::runtime_error>
	class IntegralError : public Base {
	public:
		typedef Code value_type;
	public:
		/**
		 * Constructor.
		 * @param code The error code
		 * @param message The error message
		 */
		explicit IntegralError(Code code,
			const std::string& message = "") : Base(message), code_(code) {}
		/// Returns the error code.
		Code code() const /*throw()*/;
	private:
		const Code code_;
	};

#if defined(ASCENSION_OS_WINDOWS)
	template<typename Base = std::runtime_error>
	class PlatformDependentError : public IntegralError<DWORD, Base> {
	public:
		PlatformDependentError(DWORD code = ::GetLastError()) : IntegralError<DWORD, Base>(code) {
			void* buffer;
			if(0 != ::FormatMessageA(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					0, code, LANG_USER_DEFAULT, reinterpret_cast<char*>(&buffer), 0, 0)) {
				message_ = static_cast<char*>(buffer);
				::LocalFree(buffer);
			}
		}
		const char* what() const {return message_.c_str();}
	private:
		std::string message_;
	};
#elif defined(ASCENSION_OS_POSIX)
	template<typename Base = std::runtime_error>
	class PlatformDependentError : public PlatformDependentError<int, Base> {
	public:
		PlatformDependentError(int code = errno) : IntegralError<int, Base>(code) {
			const char* const s = ::strerror(code);
			message_ = (s != 0) ? s : "";
		}
		const char* what() const {return message_.c_str();}
	private:
		std::string message_;
	};
#endif

	namespace detail {
		/// @internal
		class UnreachableCode : public std::logic_error {	// should derive from std.runtime_error???
		public:
			explicit UnreachableCode(const char* file, int line) : std::logic_error("") {
				std::ostringstream temp;
				temp << file << ":" << line;
				message_ = temp.str();
			}
			const char* what() const {
				return message_.c_str();
			}
		private:
			std::string message_;
		};
	}

}

#define ASCENSION_ASSERT_NOT_REACHED()	\
	throw ascension::detail::UnreachableCode(__FILE__, __LINE__)

#endif // !ASCENSION_BASIC_EXCEPTIONS_HPP
