/**
 * @file encoding-detector.hpp
 * Defines @c EncodingDetector class.
 * @author exeal
 * @date 2004-2014 Was encoder.hpp.
 * @date 2016-09-22 Separated from encoder.hpp.
 */

#ifndef ASCENSION_ENCODING_DETECTOR_HPP
#define ASCENSION_ENCODING_DETECTOR_HPP
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/encoding/mib-enum.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <memory>
#include <tuple>
#include <vector>

namespace ascension {
	namespace encoding {
		class EncodingDetector : private boost::noncopyable {
		public:
			virtual ~EncodingDetector() BOOST_NOEXCEPT;
			std::tuple<MIBenum, std::string, std::size_t> detect(const boost::iterator_range<const Byte*>& bytes) const;
			const std::string& name() const BOOST_NOEXCEPT;

			/// @name Factory
			/// @{
			static std::shared_ptr<const EncodingDetector> forName(const boost::string_ref& name) BOOST_NOEXCEPT;
#if BOOST_OS_WINDOWS
			static std::shared_ptr<const EncodingDetector> forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT;
#endif // BOOST_OS_WINDOWS
			template<typename OutputIterator> static void availableNames(OutputIterator out);
			static void registerDetector(std::shared_ptr<const EncodingDetector> newDetector);
			/// @}

		protected:
			explicit EncodingDetector(const boost::string_ref& name);
		private:
			/**
			 * Detects the encoding of the given character sequence.
			 * @param bytes The byte character sequence to test
			 * @return See @c #detect
			 */
			virtual std::tuple<MIBenum, std::string, std::size_t> doDetect(
				const boost::iterator_range<const Byte*>& bytes) const BOOST_NOEXCEPT = 0;
		private:
			static std::vector<std::shared_ptr<const EncodingDetector>>& registry();
			const std::string name_;
		};

		/**
		 * Returns names for all available encoding detectors.
		 * @tparam OutputIterator The type of @a out
		 * @param[out] out The output iterator to receive names
		 */
		template<typename OutputIterator>
		inline void EncodingDetector::availableNames(OutputIterator out) {
			for(std::vector<std::shared_ptr<const EncodingDetector>>::const_iterator i(std::begin(registry())), e(std::end(registry())); i != e; ++i, ++out)
				*out = (*i)->name();
		}

		/// Returns the name of the encoding detector.
		inline const std::string& EncodingDetector::name() const BOOST_NOEXCEPT {
			return name_;
		}
	}
} // namespace ascension.encoding

#endif // !ASCENSION_ENCODING_DETECTOR_HPP
