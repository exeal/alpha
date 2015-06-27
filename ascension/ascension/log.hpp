/**
 * @file log.hpp
 * Defines @c ASCENSION_TRIVIAL_LOG macro.
 * @author exeal
 * @date 2015-06-27 Created.
 */

#if !defined(ASCENSION_LOG_HPP) && defined(_DEBUG)
#define ASCENSION_LOG_HPP
#include <boost/current_function.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

namespace ascension {
	BOOST_LOG_GLOBAL_LOGGER(globalLogger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>);
}

/**
 * Ascension-variant of @c BOOST_LOG_TRIVIAL.
 * This includes the following attributes:
 * - file : The file name
 * - line : The line number
 * - function : The function name
 * @param severityLevel The severity level. This is one of @c boost#log#trivial#severity_level enums
 */
#define ASCENSION_LOG_TRIVIAL(severityLevel)											\
	BOOST_LOG_SEV(ascension::globalLogger::get(), boost::log::trivial::severityLevel)	\
		<< boost::log::add_value("file", __FILE__)										\
		<< boost::log::add_value("line", __LINE__)										\
		<< boost::log::add_value("function", BOOST_CURRENT_FUNCTION)

#endif // !ASCENSION_LOG_HPP && !_DEBUG
