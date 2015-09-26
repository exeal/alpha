/**
 * @file font-pango.cpp
 * Implementes font-related stuffs on pangomm.
 * @author exeal
 * @date 2013-10-29 Created.
 * @date 2013-2014
 */

#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-description.hpp>

namespace ascension {
	namespace graphics {
		namespace font {

#if ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
			/**
			 * Creates @c Font instance from native @c Pango#Fontset object.
			 * @param nativeObject
			 */
			Font::Font(Glib::RefPtr<Pango::Fontset> nativeObject) : nativeObject_(nativeObject) {
			}

			/// Returns the underlying native object.
			Glib::RefPtr<Pango::Fontset> Font::asNativeObject() {
				return nativeObject_;
			}

			/// Returns the underlying native object.
			Glib::RefPtr<const Pango::Fontset> Font::asNativeObject() const {
				return nativeObject_;
			}

			void Font::buildDescription() {
			}
#endif	// ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)

		}
#if ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
		namespace detail {
			template<> font::FontDescription fromNative<font::FontDescription>(const Pango::FontDescription& object) {
				font::FontProperties properties;
				switch(object.get_weight()) {
					case Pango::WEIGHT_ULTRALIGHT:
						properties.weight = font::FontWeight::ULTRA_LIGHT;
						break;
					case Pango::WEIGHT_LIGHT:
						properties.weight = font::FontWeight::LIGHT;
						break;
					case Pango::WEIGHT_NORMAL:
						properties.weight = font::FontWeight::NORMAL;
						break;
					case Pango::WEIGHT_SEMIBOLD:
						properties.weight = font::FontWeight::SEMI_BOLD;
						break;
					case Pango::WEIGHT_BOLD:
						properties.weight = font::FontWeight::BOLD;
						break;
					case Pango::WEIGHT_ULTRABOLD:
						properties.weight = font::FontWeight::ULTRA_BOLD;
						break;
					case Pango::WEIGHT_HEAVY:
						properties.weight = font::FontWeight::HEAVY;
						break;
				}
				switch(object.get_stretch()) {
					case Pango::STRETCH_ULTRA_CONDENSED:
						properties.stretch = font::FontStretch::ULTRA_CONDENSED;
						break;
					case Pango::STRETCH_EXTRA_CONDENSED:
						properties.stretch = font::FontStretch::EXTRA_CONDENSED;
						break;
					case Pango::STRETCH_CONDENSED:
						properties.stretch = font::FontStretch::CONDENSED;
						break;
					case Pango::STRETCH_SEMI_CONDENSED:
						properties.stretch = font::FontStretch::SEMI_CONDENSED;
						break;
					case Pango::STRETCH_NORMAL:
						properties.stretch = font::FontStretch::NORMAL;
						break;
					case Pango::STRETCH_SEMI_EXPANDED:
						properties.stretch = font::FontStretch::SEMI_EXPANDED;
						break;
					case Pango::STRETCH_EXPANDED:
						properties.stretch = font::FontStretch::EXPANDED;
						break;
					case Pango::STRETCH_EXTRA_EXPANDED:
						properties.stretch = font::FontStretch::EXTRA_EXPANDED;
						break;
					case Pango::STRETCH_ULTRA_EXPANDED:
						properties.stretch = font::FontStretch::ULTRA_EXPANDED;
						break;
				}
				switch(object.get_style()) {
					case Pango::STYLE_NORMAL:
						properties.style = font::FontStyle::NORMAL;
						break;
					case Pango::STYLE_OBLIQUE:
						properties.style = font::FontStyle::OBLIQUE;
						break;
					case Pango::STYLE_ITALIC:
						properties.style = font::FontStyle::ITALIC;
						break;
				}
/*				switch(object.get_variant()) {
					case Pango::VARIANT_NORMAL:
						properties.variant = font::FontVariant::NORMAL;
						break;
					case Pango::VARIANT_SMALL_CAPS:
						properties.variant = font::FontVariant::SMALL_CAPS;
						break;
				}
*/
				return font::FontDescription(font::FontFamily(fromGlibUstring(object.get_family())), !object.get_size_is_absolute() ? object.get_size() : 0, properties);
			}

			Pango::FontDescription toNative(const font::FontDescription& object, const Pango::FontDescription* /* = nullptr */) {
				Pango::FontDescription result(toGlibUstring(object.family().name()));
				switch(boost::native_value(object.properties().style)) {
					case font::FontStyle::NORMAL:
						result.set_style(Pango::STYLE_NORMAL);
						break;
					case font::FontStyle::ITALIC:
						result.set_style(Pango::STYLE_ITALIC);
						break;
					case font::FontStyle::OBLIQUE:
						result.set_style(Pango::STYLE_OBLIQUE);
						break;
				}
/*				switch(object.properties().variant) {
					case font::FontVariant::NORMAL:
						result.set_variant(Pango::VARIANT_NORMAL);
						break;
					case font::FontVariant::SMALL_CAPS:
						result.set_variant(Pango::VARIANT_SMALL_CAPS);
						break;
				}
*/				switch(boost::native_value(object.properties().weight)) {
					case font::FontWeight::ULTRA_LIGHT:
						result.set_weight(Pango::WEIGHT_ULTRALIGHT);
						break;
					case font::FontWeight::LIGHT:
						result.set_weight(Pango::WEIGHT_LIGHT);
						break;
					case font::FontWeight::NORMAL:
						result.set_weight(Pango::WEIGHT_NORMAL);
						break;
					case font::FontWeight::SEMI_BOLD:
						result.set_weight(Pango::WEIGHT_SEMIBOLD);
						break;
					case font::FontWeight::BOLD:
						result.set_weight(Pango::WEIGHT_BOLD);
						break;
					case font::FontWeight::ULTRA_BOLD:
						result.set_weight(Pango::WEIGHT_ULTRABOLD);
						break;
					case font::FontWeight::HEAVY:
						result.set_weight(Pango::WEIGHT_HEAVY);
						break;
				}
				switch(boost::native_value(object.properties().stretch)) {
					case font::FontStretch::ULTRA_CONDENSED:
						result.set_stretch(Pango::STRETCH_ULTRA_CONDENSED);
						break;
					case font::FontStretch::EXTRA_CONDENSED:
						result.set_stretch(Pango::STRETCH_EXTRA_CONDENSED);
						break;
					case font::FontStretch::CONDENSED:
						result.set_stretch(Pango::STRETCH_CONDENSED);
						break;
					case font::FontStretch::SEMI_CONDENSED:
						result.set_stretch(Pango::STRETCH_SEMI_CONDENSED);
						break;
					case font::FontStretch::NORMAL:
						result.set_stretch(Pango::STRETCH_NORMAL);
						break;
					case font::FontStretch::SEMI_EXPANDED:
						result.set_stretch(Pango::STRETCH_SEMI_EXPANDED);
						break;
					case font::FontStretch::EXPANDED:
						result.set_stretch(Pango::STRETCH_EXPANDED);
						break;
					case font::FontStretch::EXTRA_EXPANDED:
						result.set_stretch(Pango::STRETCH_EXTRA_EXPANDED);
						break;
					case font::FontStretch::ULTRA_EXPANDED:
						result.set_stretch(Pango::STRETCH_ULTRA_EXPANDED);
						break;
				}
				result.set_size(static_cast<int>(object.pointSize() * PANGO_SCALE));

				return result;
			}
		}
#endif	// ASCENSION_SUPPORTS_SHAPING_ENGINE(PANGO)
	}
}


