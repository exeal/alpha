/**
 * @file basic-types.hpp
 * @author exeal
 * @date 2010-10-21 separated from common.hpp
 */

#ifndef ASCENSION_BASIC_TYPES_HPP
#define ASCENSION_BASIC_TYPES_HPP
#include <ascension/common.hpp>

namespace ascension {

	/**
	 * OR-combinations of enum values (from Qt.QFlags).
	 * @deprecated 0.8 Bad design idea.
	 */
	template<typename Enum> class Flags {
	public:
		Flags(Enum value) : value_(value) {}
		Flags(int value = 0) : value_(static_cast<Enum>(value)) {}
		Flags(const Flags<Enum>& rhs) /*throw()*/ : value_(rhs.value_) {}
		Flags<Enum>& operator=(const Flags<Enum>& rhs) /*throw()*/ {value_ = rhs.value_; return *this;}
		Flags<Enum> operator&(int rhs) const /*throw()*/ {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator&(uint rhs) const /*throw()*/ {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator|(Enum rhs) const {Flags<Enum> temp(*this); return temp |= rhs;}
		Flags<Enum> operator^(Enum rhs) const {Flags<Enum> temp(*this); return temp ^= rhs;}
		Flags<Enum>& operator&=(int rhs) /*throw()*/ {value_ &= rhs; return *this;}
		Flags<Enum>& operator&=(uint rhs) /*throw()*/ {value_ &= rhs; return *this;}
		Flags<Enum>& operator|=(Enum rhs) {value_ |= rhs; return *this;}
		Flags<Enum>& operator^=(Enum rhs) {value_ ^= rhs; return *this;}
		Flags<Enum>& operator~() const /*throw()*/ {return ~value_;}
		bool operator!() const /*throw()*/ {return value_ == 0;}
		operator Enum() const {return static_cast<Enum>(value_);}
		void clear() /*throw()*/ {value_ = 0;}
		bool has(Enum e) const {return (value_ & e) != 0;}
		Flags<Enum>& set(Enum e, bool value = true) {if(value) value_ |= e; else value_ &= ~e; return *this;}
	private:
		int value_;
	};

	/**
	 * Represents direction in a text or a document (not visual orientation. See
	 * @c #presentation#ReadingDirection).
	 * @see ascension#text, ascension#searcher
	 */
	class Direction {
	public:
		static const Direction FORWARD;		///< Direction to the end.
		static const Direction BACKWARD;	///< Direction to the start.
		/// Copy-constructor.
		Direction(const Direction& other) /*throw()*/ : value_(other.value_) {}
		/// Assignment operator.
		Direction& operator=(const Direction& other) /*throw()*/ {value_ = other.value_; return *this;}
		/// Negation operator returns the complement of this.
		Direction operator!() const /*throw()*/ {return (*this == FORWARD) ? BACKWARD : FORWARD;}
		/// Equality operator.
		bool operator==(const Direction& other) const /*throw()*/ {return value_ == other.value_;}
		/// Inequality operator.
		bool operator!=(const Direction& other) const /*throw()*/ {return !(*this == other);}
	private:
		explicit Direction(bool value) /*throw()*/ : value_(value) {}
		bool value_;
	};

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
#ifdef ASCENSION_WINDOWS
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
#ifdef ASCENSION_WINDOWS
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

	/**
	 * Represents an invariant range.
	 * @tparam T the element type
	 * @note This class is not compatible with Boost.Range.
	 * @see kernel#Region
	 */
	template<typename T> class Range : protected std::pair<T, T> {
	public:
		typedef T ValueType;
	public:
		/// Default constructor.
		Range() {}
		/// Constructor.
		Range(ValueType v1, ValueType v2) :
			std::pair<ValueType, ValueType>(std::min(v1, v2), std::max(v1, v2)) {}
		/// Returns the beginning (minimum) of the range.
		ValueType beginning() const {return std::pair<T, T>::first;}
		/// Returns the end (maximum) of the range.
		ValueType end() const {return std::pair<T, T>::second;}
		/// Returns @c true if the given value is included by this range.
		bool includes(ValueType v) const {return v >= beginning() && v < end();}
		/// Returns @c true if the given range is included by this range.
		template<typename U> bool includes(const Range<U>& other) const {
			return other.beginning() >= beginning() && other.end() <= end();}
		/// Returns @c true if the range is empty.
		bool isEmpty() const {return beginning() == end();}
		/// Returns the length of the range.
		/// @note This class does not define a method named "size".
		typename std::iterator_traits<T>::difference_type length() const {return end() - beginning();}
	};

	/// Returns a @c Range object using the given two values.
	template<typename T> inline Range<T> makeRange(T v1, T v2) {return Range<T>(v1, v2);}

} // namespace ascension

#endif // !ASCENSION_BASIC_TYPES_HPP
