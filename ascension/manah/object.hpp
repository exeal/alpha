/**
 * @file object.hpp
 * @date 2003-2009 exeal
 */

#ifndef MANAH_OBJECT_HPP
#define MANAH_OBJECT_HPP

namespace manah {

	/// Converts an integral or float into a boolean value.
	template<typename T> inline bool toBoolean(T value) {return value != 0;}

	// provides convenient base classes for defining unassignable/uncopyable classes.
	// however, MANAH_***ABLE_TAG macros are preferrable...
#if 0
	namespace able_ {
		// unassignable object's base class
		class Unassignable {
		protected:
			Unassignable() /*throw()*/ {}
			Unassignable(const Unassignable&) /*throw()*/ {}
			~Unassignable() /*throw()*/ {}
		private:
			const Unassignable& operator=(const Unassignable&);
		};

		// noncopyable object's base class (from Boost.Noncopyable)
		class Noncopyable {
		protected:
			Noncopyable() /*throw()*/ {}
			~Noncopyable() /*throw()*/ {}
		private:
			Noncopyable(const Noncopyable&);
			const Noncopyable& operator=(const Noncopyable&);
		};
	}
	typedef able_::Unassignable Unassignable;
	typedef able_::Noncopyable Noncopyable;
#endif /* 0 */

} // namespace manah

#endif // !MANAH_OBJECT_HPP
