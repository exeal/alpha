/**
 * @file scoped-enum-emulation.hpp
 * Defines macros emulate C++11 scoped enums.
 * @author exeal
 * @date 2012-05-31 Separated from future.hpp
 */

#ifndef ASCENSION_SCOPED_ENUM_EMULATION_HPP
#define ASCENSION_SCOPED_ENUM_EMULATION_HPP
#include <boost/config.hpp>
#include <boost/core/scoped_enum.hpp>

#ifndef ASCENSION_SCOPED_ENUMS_BEGIN
#	if defined(BOOST_NO_SCOPED_ENUMS) || defined(BOOST_NO_CXX11_SCOPED_ENUMS)

#		define ASCENSION_SCOPED_ENUMS_BEGIN(name) BOOST_SCOPED_ENUM_START(name) {

#		define ASCENSION_SCOPED_ENUMS_END										\
			};																	\
			self_type(enum_type v) : v_(v) {}									\
			self_type& operator=(underlying_type v) {return (v_ = v), *this;}	\
			BOOST_SCOPED_ENUM_END

#	else

#		define ASCENSION_SCOPED_ENUMS_BEGIN(name) enum class {

#		define ASCENSION_SCOPED_ENUMS_END };

#	endif // BOOST_NO_SCOPED_ENUMS
#endif // !ASCENSION_SCOPED_ENUMS_BEGIN

#endif // !ASCENSION_SCOPED_ENUM_EMULATION_HPP
