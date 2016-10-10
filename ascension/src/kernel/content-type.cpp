/**
 * @file content-type.cpp
 */

#include <ascension/kernel/content-type.hpp>
#ifndef BOOST_NO_CXX11_HDR_MUTEX
#	include <mutex>
#else
#	include <boost/thread/mutex.hpp>
#endif

namespace ascension {
	namespace kernel {
		/// A special value which represents DEFAULT content type.
		const ContentType ContentType::DEFAULT_CONTENT(0);
		/// A special value which represents PARENT (means "transition source") content.
		const ContentType ContentType::PARENT_CONTENT(1);

		namespace {
#ifndef BOOST_NO_CXX11_HDR_MUTEX
			static std::mutex mutex;
#else
			static boost::mutex mutex;
#endif
		}

		/**
		 * Private constructor creates a @c ContentType with the given integer value.
		 * @param value The value
		 */
		ContentType::ContentType(std::uint32_t value) BOOST_NOEXCEPT : value_(value) {
		}

		/// Returns an unique new @c ContentType.
		ContentType ContentType::newValue() BOOST_NOEXCEPT {
			static std::uint32_t next = MAXIMUM_SPECIAL_VALUE_;
			mutex.lock();
			const auto v = ++next;
			mutex.unlock();
			return ContentType(v);
		}
	}
}
