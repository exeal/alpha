// object.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_OBJECT_HPP
#define MANAH_OBJECT_HPP

#define NOMINMAX
#include <cassert>
#include <cstring>	// prevent C header inclusion
#include <cwchar>	// prevent C header inclusion
#include <cstdlib>	// prevent C header inclusion
#undef min
#undef max


// modern types /////////////////////////////////////////////////////////////

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;


// basic classes ////////////////////////////////////////////////////////////

namespace manah {

	// unassignable object's base class
	class Unassignable {
	protected:
		Unassignable() throw() {}
		Unassignable(const Unassignable& rhs) throw() {}
		~Unassignable() throw() {}
	private:
		const Unassignable& operator =(const Unassignable&);
	};

	// noncopyable object's base class (from Boost.Noncopyable)
	class Noncopyable {
	protected:
		Noncopyable() throw() {}
		~Noncopyable() throw() {}
	private:
		Noncopyable(const Noncopyable&);
		const Noncopyable& operator =(const Noncopyable&);
	};

	// OR-combinations of enum values (from Qt.QFlags)
	template<class Enum> class Flags {
	public:
		Flags(Enum value) throw() : value_(value) {}
		Flags(int value = 0) throw() : value_(static_cast<Enum>(value)) {}
		Flags(const Flags<Enum>& rhs) throw() : value_(rhs.value_) {}
		Flags<Enum>& operator =(const Flags<Enum>& rhs) throw() {value_ = rhs.value_; return *this;}
		Flags<Enum> operator &(int rhs) const throw() {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator &(uint rhs) const throw() {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator |(Enum rhs) const throw() {Flags<Enum> temp(*this); return temp |= rhs;}
		Flags<Enum> operator ^(Enum rhs) const throw() {Flags<Enum> temp(*this); return temp ^= rhs;}
		Flags<Enum>& operator &=(int rhs) throw() {value_ &= rhs; return *this;}
		Flags<Enum>& operator &=(uint rhs) throw() {value_ &= rhs; return *this;}
		Flags<Enum>& operator |=(Enum rhs) throw() {value_ |= rhs; return *this;}
		Flags<Enum>& operator ^=(Enum rhs) throw() {value_ ^= rhs; return *this;}
		Flags<Enum>& operator ~() const throw() {return ~value_;}
		bool operator !() const throw() {return value_ == 0;}
		operator Enum() const throw() {return static_cast<Enum>(value_);}
		void clear() throw() {value_ = 0;}
		bool has(Enum e) const throw() {return (value_ & e) != 0;}
	private:
		int value_;
	};

} // namespace manah


// macros ///////////////////////////////////////////////////////////////////

#define toBoolean(exp) ((exp) != 0)
#ifndef countof
#define countof(array) (sizeof(array) / sizeof(array[0]))
#endif /* !countof */
#ifndef endof
#define endof(array) (array + countof(array))
#endif /* !endof */

#endif /* MANAH_OBJECT_HPP */
