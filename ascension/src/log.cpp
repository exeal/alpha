/**
 * @file log.cpp
 * Defines @c ASCENSION_TRIVIAL_LOG macro.
 * @author exeal
 * @date 2015-06-27 Created.
 */

#ifdef _DEBUG
#include <ascension/log.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace ascension {
	BOOST_LOG_GLOBAL_LOGGER_INIT(globalLogger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>) {
		auto record = boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>();
		boost::log::add_common_attributes();
		return std::move(record);
	}
}

#endif // _DEBUG
