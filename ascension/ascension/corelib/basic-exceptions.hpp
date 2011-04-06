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
	 * A platform-dependent error whose detail can be obtained by POSIX @c errno or Win32
	 * @c GetLastError.
	 * @tparam Base the base exception class
	 */
	template<typename Base = std::runtime_error>
	class PlatformDependentError : public Base {
	public:
#ifdef ASCENSION_OS_WINDOWS
		typedef DWORD Code;
#else
		typedef int Code;
#endif
	public:
		/**
		 * Constructor.
		 * @param code the error code
		 */
		explicit PlatformDependentError(Code code
#ifdef ASCENSION_OS_WINDOWS
			= ::GetLastError()
#else
			= errno
#endif
			) : Base("platform-dependent error occurred."), code_(code) {}
		/// Returns the error code.
		Code code() const /*throw()*/;
	private:
		const Code code_;
	};

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
