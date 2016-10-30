/**
 * @file font-family-other.cpp
 * @author exeal
 * @date 2014-05-13 Created.
 */

#include <ascension/graphics/font/font-family.hpp>
#if !ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE) && !ASCENSION_SELECTS_SHAPING_ENGINE(PANGO) && !ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
#include <boost/core/null_deleter.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			namespace {
				template<std::size_t length>
				inline String makeString(const char (&s)[length]) {
					return String(std::begin(s), std::end(s) - 1);
				}
			}

			std::shared_ptr<const FontFamily> FontFamily::createCursiveInstance() {
				static const FontFamily instance(makeString("cursive"));
				return std::shared_ptr<const FontFamily>(&instance, boost::null_deleter());
			}

			std::shared_ptr<const FontFamily> FontFamily::createFantasyInstance() {
				static const FontFamily instance(makeString("fantasy"));
				return std::shared_ptr<const FontFamily>(&instance, boost::null_deleter());
			}

			std::shared_ptr<const FontFamily> FontFamily::createMonospaceInstance() {
				static const FontFamily instance(makeString("monospace"));
				return std::shared_ptr<const FontFamily>(&instance, boost::null_deleter());
			}

			std::shared_ptr<const FontFamily> FontFamily::createSansSerifInstance() {
				static const FontFamily instance(makeString("sans-serif"));
				return std::shared_ptr<const FontFamily>(&instance, boost::null_deleter());
			}

			std::shared_ptr<const FontFamily> FontFamily::createSerifInstance() {
				static const FontFamily instance(makeString("serif"));
				return std::shared_ptr<const FontFamily>(&instance, boost::null_deleter());
			}

			FontFamily::FontFamily(const String& name) : name_(name) {
				if(name.empty())
					throw std::length_error("name");
			}

			FontFamily& FontFamily::operator=(const FontFamily& other) {
				return (name_ = other.name_), *this;
			}

			String FontFamily::name(const std::locale& lc /* = std::locale::classic() */) const BOOST_NOEXCEPT {
				return name_;
			}
		}
	}
}

#endif // !ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE) && !ASCENSION_SELECTS_SHAPING_ENGINE(PANGO) && !ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
