/**
 * @file encoding-detector.cpp
 * Implements @c EncodingDetector class.
 * @author exeal
 * @date 2004-2014 Was encoder.hpp.
 * @date 2016-09-22 Separated from encoder.cpp.
 */

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-factory.hpp>
#include <ascension/corelib/encoding/encoding-detector.hpp>
#include <ascension/corelib/text/utf.hpp>	// text.isScalarValue, text.utf.encode
#include <algorithm>
#include <memory>							// std.unique_ptr
#include <boost/foreach.hpp>


namespace ascension {
	namespace encoding {
		/**
		 * Constructor.
		 * @param name The name of the encoding detector
		 * @throw std#invalid_argument @a name is invalid
		 */
		EncodingDetector::EncodingDetector(const boost::string_ref& name) : name_(name.to_string()) {
		}

		/// Destructor.
		EncodingDetector::~EncodingDetector() BOOST_NOEXCEPT {
		}

		/**
		 * Detects the encoding of the string buffer.
		 * @param bytes The byte character sequence to test
		 * @return A tuple of: (0) The MIBenum value, (1) the encoding name and (2) the number of initial bytes
		 *         absolutely detected. The value can't exceed the result of `bytes.size()`
		 * @throw NullPointerException @a bytes is @c null
		 * @throw std#invalid_argument @a bytes is not ordered
		 */
		std::tuple<MIBenum, std::string, std::size_t> EncodingDetector::detect(const boost::iterator_range<const Byte*>& bytes) const {
			if(boost::const_begin(bytes) == nullptr || boost::const_end(bytes) == nullptr)
				throw NullPointerException("bytes");
			else if(boost::const_begin(bytes) > boost::const_end(bytes))
				throw std::invalid_argument("bytes is not ordered.");
			return doDetect(bytes);
		}

		/**
		 * Returns the encoding detector which matches the given name.
		 * @param name The name
		 * @return The encoding detector or @c null if not registered
		 */
		std::shared_ptr<const EncodingDetector> EncodingDetector::forName(const boost::string_ref& name) BOOST_NOEXCEPT {
			BOOST_FOREACH(const std::shared_ptr<const EncodingDetector>detector, registry()) {
				const std::string canonicalName(detector->name());
				if(compareEncodingNames(std::begin(name), std::end(name), std::begin(canonicalName), std::end(canonicalName)) == 0)
					return detector;
			}
			return nullptr;
		}

#if BOOST_OS_WINDOWS
		/**
		 * Returns the encoding detector which has the given Windows code page.
		 * @param codePage The code page
		 * @return The encoding detector or @c null if not registered
		 */
		std::shared_ptr<const EncodingDetector> EncodingDetector::forWindowsCodePage(unsigned int codePage) BOOST_NOEXCEPT {
			switch(codePage) {
				case 50001:
					return forName("UniversalAutoDetect");
				case 50932:
					return forName("JISAutoDetect");
				case 50949:
					return forName("KSAutoDetect");
				default:
					return std::shared_ptr<const EncodingDetector>();
			}
		}
#endif // BOOST_OS_WINDOWS

		std::vector<std::shared_ptr<const EncodingDetector>>& EncodingDetector::registry() {
			static std::vector<std::shared_ptr<const EncodingDetector>> singleton;
			return singleton;
		}

		/**
		 * Registers the new encoding detector.
		 * @param newDetector The encoding detector
		 * @throw NullPointerException @a detector is @c null
		 */
		void EncodingDetector::registerDetector(std::shared_ptr<const EncodingDetector> newDetector) {
			if(newDetector.get() == nullptr)
				throw NullPointerException("newDetector");
			registry().push_back(newDetector);
		}


		// UniversalDetector //////////////////////////////////////////////////////////////////////////////////////////

		namespace {
			class UniversalDetector : public EncodingDetector {
			public:
				UniversalDetector() : EncodingDetector("UniversalAutoDetect") {}
			private:
				std::tuple<MIBenum, std::string, std::size_t> doDetect(const boost::iterator_range<const Byte*>& bytes) const BOOST_NOEXCEPT override;
			};
//			ASCENSION_DEFINE_ENCODING_DETECTOR(SystemLocaleBasedDetector, "SystemLocaleAutoDetect");
//			ASCENSION_DEFINE_ENCODING_DETECTOR(UserLocaleBasedDetector, "UserLocaleAutoDetect");

			struct UniversalDetectorInstaller {
				UniversalDetectorInstaller() BOOST_NOEXCEPT {
					EncodingDetector::registerDetector(std::make_shared<UniversalDetector>());
				}
			} unused;
		} // namespace @0

		/// @see EncodingDetector#doDetect
		std::tuple<MIBenum, std::string, std::size_t> UniversalDetector::doDetect(const boost::iterator_range<const Byte*>& bytes) const BOOST_NOEXCEPT {
			// try all detectors
			std::vector<std::string> names;
			availableNames(std::back_inserter(names));

			auto result(std::make_tuple(
				Encoder::defaultInstance().properties().mibEnum(), Encoder::defaultInstance().properties().name(), static_cast<std::size_t>(0)));
			std::size_t bestScore = 0;
			BOOST_FOREACH(const std::string& name, names) {
				if(const std::shared_ptr<const EncodingDetector> detector = forName(name)) {
					if(detector.get() == this)
						continue;
					const auto detectedEncoding(detector->detect(bytes));
					if(std::get<2>(detectedEncoding) > bestScore) {
						result = detectedEncoding;
						if(std::get<2>(detectedEncoding) == boost::size(bytes))
							break;
						bestScore = std::get<2>(detectedEncoding);
					}
				}
			}

			return result;
		}
	}
}
