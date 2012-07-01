/**
 * @file font.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2010-2012
 */

#ifndef ASCENSION_FONT_HPP
#define ASCENSION_FONT_HPP

#include <ascension/config.hpp>	// ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/basic-types.hpp>		// uint32_t, std.tr1.shared_ptr, ...
#include <ascension/graphics/geometry.hpp>
#include <cstring>	// std.strlen
#include <locale>	// std.collate
#include <memory>	// std.unique_ptr, std.shared_ptr
#include <set>
#include <unordered_map>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#if defined(ASCENSION_SHAPING_ENGINE_CAIRO)
#	include <cairomm.h>
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)
#	include <CGFont.h>
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
#	include <CTFont.h>
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
#	include <dwrite.h>
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
#	include <hb.h>
#	include <boost/intrusive_ptr.hpp>
namespace boost {
	inline void intrusive_ptr_add_ref(hb_font_t* p) {::hb_font_reference(p);}
	inline void intrusive_ptr_release(hb_font_t* p) {::hb_font_destroy(p);}
}
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
#	include <pangomm.h>
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
#	include <QFont>
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
#	include <ascension/win32/handle.hpp>	// win32.Handle
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
#	include <GdiPlus.h>
#endif

namespace ascension {
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	namespace detail {
		struct IdeographicVariationSequences {
			std::vector<uint32_t> defaultMappings;
			std::unordered_map<uint32_t, uint16_t> nonDefaultMappings;
		};
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	namespace graphics {

		/**
		 * @a font namespace is...
		 * Font properties specifications are designed based on "CSS Fonts Module Level 3"
		 * (http://dev.w3.org/csswg/css3-fonts/).
		 */
		namespace font {

			/// TrueType/OpenType font tag.
			typedef uint32_t TrueTypeFontTag;

			template<uint8_t c1, uint8_t c2 = ' ', uint8_t c3 = ' ', uint8_t c4 = ' '>
			struct MakeTrueTypeFontTag {
				static const TrueTypeFontTag value = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
			};

			/**
			 * Returns an 32-bit integer represents the given TrueType tag.
			 * @tparam Character The character type of @a name
			 * @param name The TrueType tag name
			 * @param validate Set @c true to validate characters in @a name
			 * @return The 32-bit integral TrueType tag value
			 * @throw std#length_error The length of @a name is zero or greater four
			 * @throw std#invalid_argument @a validate is @c true and any character in @a name was
			 *                             invalid
			 */
			template<typename Character>
			inline TrueTypeFontTag makeTrueTypeFontTag(const Character name[], bool validate = true) {
				const std::size_t len = std::char_traits<Character>::length(name);
				if(len == 0 || len > 4)
					throw std::length_error("name");
				TrueTypeFontTag tag = 0;
				std::size_t i = 0;
				for(; i < len; ++i) {
					if(validate && (name[i] < 32 || name[i] > 126))
						throw std::invalid_argument("name");
					tag |= name[i] << ((3 - i) * 8);
				}
				for(; i < 4; ++i)
					tag |= ' ' << ((3 - i) * 8);
				return tag;
			}

			template<typename T> inline int round(T value) {
				return static_cast<int>(std::floor(value + 0.5));
			}
			bool supportsComplexScripts() /*throw()*/;
			bool supportsOpenTypeFeatures() /*throw()*/;

			typedef uint16_t GlyphCode;

			/**
			 * Represents information for a single glyph.
			 * @see GlyphVector#metrics
			 */
			class GlyphMetrics {
			public:
				virtual Scalar advanceX() const = 0;
				virtual Scalar advanceY() const = 0;
				virtual NativeSize bounds() const = 0;
				virtual Scalar leftTopSideBearing() const = 0;
				virtual Scalar rightBottomSideBearing() const = 0;
			};

			class GlyphVector {
			public:
				/// Destructor.
				virtual ~GlyphVector() /*throw()*/ {}
				GlyphCode operator[](std::ptrdiff_t index) const {return at(index);}
				virtual GlyphCode at(std::size_t index) const = 0;
				std::size_t length() const {return size();}
				virtual NativeSize logicalBounds() const = 0;
				virtual NativeSize logicalGlyphBounds(std::size_t index) const = 0;
				virtual std::shared_ptr<GlyphMetrics> metrics(std::size_t index) const = 0;
//				virtual std::shared_ptr<Shape> outline(std::size_t index) const = 0;
				virtual NativePoint position(std::size_t index) const = 0;
				/// Returns
				virtual std::size_t size() const = 0;
				virtual NativeSize visualGlyphBounds(std::size_t index) const = 0;
				virtual NativeSize visualBounds() const = 0;
			};

			class FontFace;

			/**
			 * @c FontFamily represents a family of related font faces. A font family is a group of
			 * font faces that share a common design, but differ in styles.
			 * @see FontFamilies, FontFace
			 */
			class FontFamily {
			public:
#if defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				explicit FontFamily(const String& name);
				explicit FontFamily(win32::com::SmartPointer<IDWriteFontFamily> nativeObject);
				win32::com::SmartPointer<IDWriteFontFamily> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				explicit FontFamily(const String& name);
				explicit FontFamily(Glib::RefPtr<Pango::FontFamily> nativeObject);
				Glib::RefPtr<Pango::FontFamily> asNativeObject();
				Glib::RefPtr<const Pango::FontFamily> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				explicit FontFamily(const String& name);
				explicit FontFamily(std::unique_ptr<Gdiplus::FontFamily>&& nativeObject);
				explicit FontFamily(std::shared_ptr<Gdiplus::FontFamily> nativeObject);
				explicit FontFamily(Gdiplus::FontFamily& nativeObject);	// weak ref.
				std::shared_ptr<Gdiplus::FontFamily> asNativeObject() /*noexcept*/;
				std::shared_ptr<const Gdiplus::FontFamily> asNativeObject() const /*noexcept*/;
#else
				explicit FontFamily(const String& name) : name_(name) {
					if(name.empty())
						throw std::length_error("name");
				}
#endif
				const String& name() const /*noexcept*/ {return name_;}
			private:
#if defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFontFamily> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				Glib::RefPtr<Pango::FontFamily> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::FontFamily> nativeObject_;
#else
				const String name_;
#endif
			};

			class FontFamilySpecification {
				ASCENSION_NONCOPYABLE_TAG(FontFamilySpecification);
			public:
				explicit FontFamilySpecification(const String& name) : name_(name) {
					if(name.empty())
						throw std::length_error("name");
				}
				/**
				 * Appends the new font family to this object.
				 * @param family The font family to append
				 * @return This object
				 */
				FontFamilySpecification& append(std::unique_ptr<FontFamilySpecification>&& family) /*noexcept*/ {
					next_ = std::move(family);
					return *this;
				}
				/// Returns the next font family.
				FontFamilySpecification* next() /*noexcept*/ {return next_.get();}
				/// Returns the next font family.
				const FontFamilySpecification* next() const /*noexcept*/ {return next_.get();}
			private:
				const String name_;
				std::unique_ptr<FontFamilySpecification> next_;
			};

			class FontFamiliesSpecification : public FontFamilySpecification {
				ASCENSION_NONCOPYABLE_TAG(FontFamiliesSpecification);
			public:
				enum GenericFamily {
					SERIF,
					SANS_SERIF,
					CURSIVE,
					FANTASY,
					MONOSPACE,
					UNSPECIFIED
				};
			public:
				explicit FontFamiliesSpecification(const String& firstName, GenericFamily genericFamily = UNSPECIFIED) : FontFamilySpecification(firstName), genericFamily_(genericFamily) {
				}
				GenericFamily genericFamily() const /*throw()*/ {return genericFamily_;}
				FontFamiliesSpecification& setGenericFamily(GenericFamily genericFamily) {
					if(genericFamily < SERIF || genericFamily > UNSPECIFIED)
						throw UnknownValueException("genericFamily");
					genericFamily_ = genericFamily;
					return *this;
				}
			private:
				GenericFamily genericFamily_;
			};

			/**
			 * Weight of glyphs in the font, their degree of blackness or stroke thickness.
			 * @see http://dev.w3.org/csswg/css3-fonts/#font-weight-prop
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(FontWeight)
				/// Same as 400.
				NORMAL = 400,
				/// Same as 700.
				BOLD = 700,
				/// Specifies the weight of the face bolder than the inherited value.
				BOLDER,
				/// Specifies the weight of the face lighter than the inherited value.
				LIGHTER,
				THIN = 100,
				EXTRA_LIGHT = 200,
				ULTRA_LIGHT = 200,
				LIGHT = 300, 
				MEDIUM = 500,
				SEMI_BOLD = 600,
				DEMI_BOLD = 600,
				EXTRA_BOLD = 800,
				ULTRA_BOLD = 800,
				BLACK = 900,
				HEAVY = 900
			ASCENSION_END_SCOPED_ENUM

			/**
			 * Width of glyphs in the font.
			 * @see http://dev.w3.org/csswg/css3-fonts/#font-stretch-prop
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(FontStretch)
				NORMAL = 1000,			///< Normal.
//				WIDER,
//				NARROWER,
				ULTRA_CONDENSED = 500,	///< Ultra Condensed.
				EXTRA_CONDENSED = 625,	///< Extra Condensed.
				CONDENSED = 750,		///< Condensed.
				SEMI_CONDENSED = 875,	///< Semi Condensed.
				SEMI_EXPANDED = 1125,	///< Semi Expanded.
				EXPANDED = 1250,		///< Expanded.
				EXTRA_EXPANDED = 1500,	///< Extra Expanded.
				ULTRA_EXPANDED = 2000	///< Ultra Expanded.
			ASCENSION_END_SCOPED_ENUM

			/**
			 * Style of the font.
			 * @see http://dev.w3.org/csswg/css3-fonts/#font-style-prop
			 */
			ASCENSION_BEGIN_SCOPED_ENUM(FontStyle)
				NORMAL,		///< Selects a face that is classified as 'normal'.
				ITALIC,		///< Selects a face that is labeled 'italic' or 'oblique'.
				OBLIQUE,	///< Selects a face that is labeled 'oblique'.
				BACKSLANT	///< Selects a face that is labeled 'backslant'.
			ASCENSION_END_SCOPED_ENUM

			ASCENSION_BEGIN_SCOPED_ENUM(FontVariant)
				NORMAL,
				SMALL_CAPS
			ASCENSION_END_SCOPED_ENUM

			ASCENSION_BEGIN_SCOPED_ENUM(FontOrientation)
				HORIZONTAL,
				VERTICAL
			ASCENSION_END_SCOPED_ENUM

			/**
			 * Set of font properties without the family name.
			 * @tparam PropertyHolder
			 * @see FontDescription
			 */
			template<template<typename> class PropertyHolder = detail::Type2Type>
			struct FontProperties : private boost::equality_comparable<FontProperties<PropertyHolder>> {
				typedef typename PropertyHolder<FontWeight>::Type WeightType;
				typedef typename PropertyHolder<FontStretch>::Type StretchType;
				typedef typename PropertyHolder<FontStyle>::Type StyleType;
				typedef typename PropertyHolder<FontVariant>::Type VariantType;
				typedef typename PropertyHolder<FontOrientation>::Type OrientationType;

				WeightType weight;				///< The font weight.
				StretchType stretch;			///< The font stretch.
				StyleType style;				///< The font style.
				VariantType variant;			///< The font variant.
				OrientationType orientation;	///< The font orientation.

				/**
				 * Constructor.
				 * @param weight
				 * @param stretch
				 * @param style
				 * @param variant
				 * @param orientation
				 */
				explicit FontProperties(
					WeightType weight = WeightType(), StretchType stretch = StretchType(),
					StyleType style = StyleType(), VariantType variant = VariantType(),
					OrientationType orientation = OrientationType())
					: weight(weight), stretch(stretch), style(style), orientation(orientation) {}
				/// Implicit conversion operator.
				template<template<typename> class T>
				operator FontProperties<T>() const {
					return FontProperties<T>(weight, stretch, style, variant, orientation);
				}
				/// Equality operator.
				bool operator==(const FontProperties& other) const {
					return weight_ == other.weight_ && stretch_ == other.stretch_ && style_ == other.style_;
				}
			};

			template<template<typename> class PropertyHolder = detail::Type2Type>
			class FontDescription {
			public:
				typedef typename PropertyHolder<String>::Type FamilyNameType;
				typedef typename PropertyHolder<double>::Type PixelSizeType;
				typedef FontProperties<PropertyHolder> PropertiesType;
			public:
				/**
				 * Constructor.
				 * @param familyName The family name
				 * @param pixelSize The size in pixels
				 * @param properties The other properties
				 */
				explicit FontDescription(const String& familyName,
					PixelSizeType pixelSize = PixelSizeType(),
					const PropertiesType& properties = PropertiesType())
					: family_(family), pixelSize_(pixelSize), properties_(properties) {}
				/// Returns the family name.
				const FamilyNameType& familyName() const /*noexcept*/ {return familyName_;}
				/// Returns the size in pixels.
				PixelSizeType pixelSize() const /*noexcept(...)*/ {return pixelSize_;}
				/// Returns the other properties.
				PropertiesType& properties() /*noexcept*/ {return properties_;}
				/// Returns the other properties.
				const PropertiesType& properties() const /*noexcept*/ {return properties_;}
				/**
				 * Sets the family name.
				 * @param familyName The new family name
				 * @throw std#length_error @a familyName is empty
				 */
				FontDescription& setFamilyName(const FamilyNameType& familyName) {
					if(familyName.empty())
						throw std::length_error("familyName");
					return (familyName_ = familyName), *this;
				}
				/// Sets the size in pixels.
				FontDescription& setPixelSize(PixelSizeType newValue) /*noexcept(...)*/ {
					return (pixelSize_ = newValue), *this;
				}
			private:
				FamilyNameType familyName_;
				PixelSizeType pixelSize_;
				FontProperties<PropertyHolder> properties_;
			};
		}
	}
}

namespace std {
	template<>
	class hash<ascension::graphics::font::FontFamily> : public std::hash<ascension::String> {
	public:
		typedef ascension::graphics::font::FontFamily argument_type;
		result_type operator()(const argument_type& key) const {
			return std::hash<ascension::String>::operator()(key.name());
		}
	};

	template<>
	class hash<ascension::graphics::font::FontProperties<>> :
		public std::unary_function<ascension::graphics::font::FontProperties<>, std::hash<void*>::result_type> {
	public:
		result_type operator()(const argument_type& key) const {
			// TODO: use boost.hash_combine.
			return std::hash<argument_type::WeightType>()(key.weight)
				+ std::hash<argument_type::StretchType>()(key.stretch)
				+ std::hash<argument_type::StyleType>()(key.style)
				+ std::hash<argument_type::VariantType>()(key.variant)
				+ std::hash<argument_type::OrientationType>()(key.orientation);
		}
	};

	template<>
	class hash<ascension::graphics::font::FontDescription<>> :
		public std::unary_function<ascension::graphics::font::FontDescription<>, std::hash<void*>::result_type> {
	public:
		result_type operator()(const argument_type& key) const {
			// TODO: use boost.hash_combine.
			return std::hash<ascension::String>()(key.familyName())
				+ std::hash<argument_type::PixelSizeType>()(key.pixelSize())
				+ std::hash<argument_type::PropertiesType>()(key.properties());
		}
	};
}

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Used to represent a group of fonts with the same family, slant, weight, width, but
			 * varying sizes.
			 */
			class FontFace {
			public:
				void availableSizes(std::set<int>& sizes) const;
				const FontDescription<>& describe() const;
				/// Returns the face name.
				const String& name() const;
			private:
				const FontDescription<> description_;
				const String name_;
			};

			class Font : public std::enable_shared_from_this<Font> {
			public:
				/**
				 * Abstract class provides physical font metrics information.
				 * @see Font#metrics
				 */
				class Metrics {
				public:
					/// Returns the ascent of the text in font coordinate units.
					virtual int ascent() const /*noexcept*/ = 0;
					/// Returns the average width of a character in font coordinate units.
					virtual int averageCharacterWidth() const /*noexcept*/ = 0;
					/// Returns the cell height in font coordinate units.
					int cellHeight() const /*throw()*/ {return ascent() + descent();}
					/// Returns the descent of the text in font coordinate units.
					virtual int descent() const /*noexcept*/ = 0;
					/// Returns the em height in font coordinate units.
					int emHeight() const /*noexcept*/ {return cellHeight() - internalLeading();}
					/// Returns the external leading in font coordinate units.
					/// @note In Ascension, external leadings are placed below characters.
					virtual int externalLeading() const /*noexcept*/ = 0;
					/// Returns the internal leading in font coordinate units.
					virtual int internalLeading() const /*noexcept*/ = 0;
					/// Returns the gap of the lines (external leading) in font coordinate units.
					int lineGap() const /*noexcept*/ {return externalLeading();}
					/// Returns the pitch of lines in pixels.
					/// @note This method ignores @c LayoutSettings#lineSpacing value.
					int linePitch() const /*noexcept*/ {return cellHeight() + lineGap();}
					/// Returns the x-height of the font in font coordinate units.
					virtual int xHeight() const /*noexcept*/ = 0;
				};
			public:
#if defined(ASCENSION_SHAPING_ENGINE_CAIRO)
				explicit Font(Cairo::RefPtr<Cairo::ScaledFont> nativeObject);
				Glib::RefPtr<Cairo::ScaledFont> asNativeObject();
				Glib::RefPtr<const Cairo::ScaledFont> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)
				explicit Font(cg::Reference<CGFontRef>&& nativeObject);
				cg::Reference<CGFontRef>& asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
				explicit Font(cg::Reference<CTFontRef>&& nativeObject);
				cg::Reference<CTFontRef>& asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				explicit Font(win32::com::SmartPointer<IDWriteFont> nativeObject);
				win32::com::SmartPointer<IDWriteFont> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
				explicit Font(boost::intrusive_ptr<hb_font_t> nativeObject);
				boost::intrusive_ptr<hb_font_t> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				explicit Font(Glib::RefPtr<Pango::Font> nativeObject);
				Glib::RefPtr<Pango::Font> asNativeObject();
				Glib::RefPtr<const Pango::Font> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				explicit Font(std::shared_ptr<QFont> nativeObject);
				std::shared_ptr<QFont> asNativeObject();
				std::shared_ptr<const QFont> asNativeObject() const;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				explicit Font(win32::Handle<HFONT>&& nativeObject) /*noexcept*/;
				const win32::Handle<HFONT>& asNativeObject() const /*noexcept*/;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				explicit Font(std::shared_ptr<Gdiplus::Font> nativeObject);
				std::shared_ptr<Gdiplus::Font> asNativeObject();
				std::shared_ptr<const Gdiplus::Font> asNativeObject() const;
#endif
				/**
				 * Creates a @c GlyphVector by mapping characters to glyphs one-to-one based on the
				 * Unicode cmap in this font. This method does no other processing besides the
				 * mapping of glyphs to characters. This means that this method is not useful for
				 * some scripts, such as Arabic, Hebrew, Thai, and Indic, that require reordering,
				 * shaping, or ligature substitution.
				 * @param text The text string
				 * @return A new @c GlyphVector created with the specified string
				 */
				std::unique_ptr<const GlyphVector> createGlyphVector(const String& text) const;
				/// Returns the description of this font.
				FontDescription<>&& describe() const /*noexcept*/;
				/**
				 * Returns the family name of this font.
				 * @param lc The locale for which to get the font family name. If this value is
				 *           C or unsupported locale, this method returns an unlocalized name
				 * @return The family name of this font
				 * @see #faceName
				 */
				String familyName(const std::locale& lc = std::locale::classic()) const /*noexcept*/;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				boost::optional<GlyphCode> ivsGlyph(CodePoint baseCharacter,
					CodePoint variationSelector, GlyphCode defaultGlyph) const;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				/// Returns the metrics of the font.
				std::unique_ptr<Metrics> metrics() const /*noexcept*/;
			private:
#if defined(ASCENSION_SHAPING_ENGINE_CAIRO)
				Cairo::RefPtr<Cairo::ScaledFont> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)
				cg::Reference<CGFontRef> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
				cg::Reference<CTFontRef> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFont> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
				boost::intrusive_ptr<hb_font_t> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				Glib::RefPtr<Pango::Font> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				std::shared_ptr<QFont> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
				win32::Handle<HFONT> nativeObject_;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				std::unique_ptr<detail::IdeographicVariationSequences> ivs_;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::Font> nativeObject_;
#endif
			};

			/// An interface represents an object provides a set of fonts.
			class FontCollection {
			public:
				/// Destructor.
				virtual ~FontCollection() /*throw()*/ {}
				std::set<FontFamily>&& families() const;
				/**
				 * Returns the font matches the given properties.
				 * @param familyName The font family name
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested properties or the default one
				 */
				std::shared_ptr<const Font> get(
						const FontDescription<>& description, double sizeAdjust = 0.0) const {
					CachedFonts::const_iterator i(cachedFonts_.find(description));
					if(i != cachedFonts_.end())
						return i->second;
					return const_cast<FontCollection*>(this)->cache(description, sizeAdjust);
				}
				/**
				 * Returns the font for last resort fallback.
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested property
				 */
				std::shared_ptr<const Font> lastResortFallback(
					const FontDescription<>& description, double sizeAdjust = 0.0) const;
			private:
				std::shared_ptr<const Font> cache(
					const FontDescription<>& description, double sizeAdjust);
#if defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
				cg::Reference<CTFontCollectionRef> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
				win32::com::SmartPointer<IDWriteFontCollection> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
				Glib::RefPtr<Pango::FontMap> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
				std::shared_ptr<QFontDatabase> nativeObject_;
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
				std::shared_ptr<Gdiplus::FontCollection> nativeObject_;
#endif
				typedef std::unordered_map<FontDescription<>, std::shared_ptr<Font>> CachedFonts;
				CachedFonts cachedFonts_;
			};

			const FontCollection& installedFonts();

		}
	}
}

#endif // !ASCENSION_FONT_HPP
