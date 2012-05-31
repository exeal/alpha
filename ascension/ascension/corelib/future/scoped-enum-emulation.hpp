/**
 * @file scoped-enum-emulation.hpp
 * Defines macros emulate C++11 scoped enums.
 * @author exeal
 * @date 2012-05-31 separated from future.hpp
 */

#ifndef ASCENSION_SCOPED_ENUM_EMULATION_HPP
#define ASCENSION_SCOPED_ENUM_EMULATION_HPP
#include <boost/config.hpp>

#ifdef BOOST_NO_SCOPED_ENUMS

#	define ASCENSION_BEGIN_SCOPED_ENUM(typeName)			\
	class typeName {										\
	public:													\
		typeName() /*noexcept*/ : value_() {}				\
		template<typename T>								\
		typeName(T value) /*noexcept*/ : value_(value) {}	\
		enum Type {
#	define ASCENSION_END_SCOPED_ENUM						\
		};													\
		operator Type() const /*noexcept*/ {return value_;}	\
	private:												\
		Type value_;										\
	};

#else

#	define ASCENSION_BEGIN_SCOPED_ENUM(typeName)	\
	enum class typeName
#	define ASCENSION_END_SCOPED_ENUM

#endif // BOOST_NO_SCOPED_ENUMS

#endif // !ASCENSION_SCOPED_ENUM_EMULATION_HPP
