/**
 * @file font-collection.hpp
 * @author exeal
 * @date 2010-11-06 Created
 * @date 2010-2014 Was font.hpp
 * @date 2014-06-15 Separated from font.hpp
 */

#ifndef ASCENSION_FONT_COLLECTION_HPP
#define ASCENSION_FONT_COLLECTION_HPP
#include <ascension/graphics/affine-transform.hpp>
#include <ascension/graphics/font/font.hpp>
#include <boost/optional.hpp>
#include <boost/range/iterator.hpp>
#include <memory>
#include <set>

namespace ascension {
	namespace graphics {
		namespace font {
			class FontDescription;

			/**
			 * @c FontCollection represents the set of fonts available for a particular graphics
			 * context, and provides a method to enumerate font families.
			 * @see Fontset, RenderingContext2D
			 */
			class FontCollection : public Wrapper<FontCollection> {
			public:
#if ASCENSION_SELECTS_SHAPING_ENGINE(CORE_TEXT)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(HARFBUZZ)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(QT)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
				explicit FontCollection(win32::Handle<HDC>::Type deviceContext) BOOST_NOEXCEPT;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
#endif
				/// Returns a set of font families available in this collection.
				std::set<FontFamily>&& families() const;
				/**
				 * Returns the fontset matches the given description.
				 * @param description The font description
				 * @param transform The transform associated with the font
				 * @param sizeAdjust The 'font-size-adjust' value. Set @c boost#none for 'none'
				 * @return The font has the requested description or the default one
				 */
				std::shared_ptr<const Font> get(
					const FontDescription& description,
					const AffineTransform& transform = AffineTransform(),
					boost::optional<Scalar> sizeAdjust = boost::none) const;
				/**
				 * Returns the fontset for last resort fallback.
				 * @param description The font description
				 * @param transform The transform associated with the font
				 * @param sizeAdjust The 'font-size-adjust' value. Set @c boost#none for 'none'
				 * @return The font has the requested property
				 */
				std::shared_ptr<const Font> lastResortFallback(
					const FontDescription& description,
					const AffineTransform& transform = AffineTransform(),
					boost::optional<Scalar> sizeAdjust = boost::none) const;
			private:
#if ASCENSION_SELECTS_SHAPING_ENGINE(CORE_TEXT)
				cg::Reference<CTFontCollectionRef> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFontCollection> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(HARFBUZZ)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
				Glib::RefPtr<Pango::FontMap> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(QT)
				std::shared_ptr<QFontDatabase> nativeObject_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
				win32::Handle<HDC>::Type deviceContext_;
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::FontCollection> nativeObject_;
#endif
			};

			template<typename SinglePassReadableRange>
			inline typename boost::range_iterator<const SinglePassReadableRange>::type findMatchingFontFamily(
					const FontCollection& fontCollection, const SinglePassReadableRange& fontFamilies) {
				// TODO: This code is adhoc. Should be rewritten according to CSS Fonts Module Level 3,
				//       5 Font Matching Algorithm (http://www.w3.org/TR/css3-fonts/#font-matching-algorithm).
				assert(boost::const_begin(fontFamilies) != boost::const_end(fontFamilies));
				return boost::const_begin(fontFamilies);
			}
		}
	}
}

#endif // !ASCENSION_FONT_COLLECTION_HPP
