// object.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_OBJECT_HPP
#define MANAH_OBJECT_HPP


// modern types /////////////////////////////////////////////////////////////

typedef unsigned char	byte;
typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;


// basic classes ////////////////////////////////////////////////////////////

namespace manah {

	// provides convenient base classes for defining unassignable/uncopyable classes.
	// however, MANAH_***ABLE_TAG macros are preferrable...
#if 0
	namespace able_ {
		// unassignable object's base class
		class Unassignable {
		protected:
			Unassignable() throw() {}
			Unassignable(const Unassignable&) throw() {}
			~Unassignable() throw() {}
		private:
			const Unassignable& operator=(const Unassignable&);
		};

		// noncopyable object's base class (from Boost.Noncopyable)
		class Noncopyable {
		protected:
			Noncopyable() throw() {}
			~Noncopyable() throw() {}
		private:
			Noncopyable(const Noncopyable&);
			const Noncopyable& operator=(const Noncopyable&);
		};
	}
	typedef able_::Unassignable Unassignable;
	typedef able_::Noncopyable Noncopyable;
#endif /* 0 */

	// OR-combinations of enum values (from Qt.QFlags)
	template<typename Enum> class Flags {
	public:
		Flags(Enum value) : value_(value) {}
		Flags(int value = 0) : value_(static_cast<Enum>(value)) {}
		Flags(const Flags<Enum>& rhs) throw() : value_(rhs.value_) {}
		Flags<Enum>& operator=(const Flags<Enum>& rhs) throw() {value_ = rhs.value_; return *this;}
		Flags<Enum> operator&(int rhs) const throw() {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator&(uint rhs) const throw() {Flags<Enum> temp(*this); return temp &= rhs;}
		Flags<Enum> operator|(Enum rhs) const {Flags<Enum> temp(*this); return temp |= rhs;}
		Flags<Enum> operator^(Enum rhs) const {Flags<Enum> temp(*this); return temp ^= rhs;}
		Flags<Enum>& operator&=(int rhs) throw() {value_ &= rhs; return *this;}
		Flags<Enum>& operator&=(uint rhs) throw() {value_ &= rhs; return *this;}
		Flags<Enum>& operator|=(Enum rhs) {value_ |= rhs; return *this;}
		Flags<Enum>& operator^=(Enum rhs) {value_ ^= rhs; return *this;}
		Flags<Enum>& operator~() const throw() {return ~value_;}
		bool operator!() const throw() {return value_ == 0;}
		operator Enum() const {return static_cast<Enum>(value_);}
		void clear() throw() {value_ = 0;}
		bool has(Enum e) const {return (value_ & e) != 0;}
		Flags<Enum>& set(Enum e, bool value = true) {if(value) value_ |= e; else value_ &= ~e; return *this;}
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

#define MANAH_UNASSIGNABLE_TAG(className)	private: className& operator=(const className&)
#define MANAH_NONCOPYABLE_TAG(className)	MANAH_UNASSIGNABLE_TAG(className); className(const className&)

#endif /* !MANAH_OBJECT_HPP */
