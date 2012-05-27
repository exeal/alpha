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
#include <vector>
#if defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
#	include <ascension/win32/handle.hpp>	// win32.Handle
#endif
#include <boost/operators.hpp>

namespace ascension {
	namespace graphics {
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

			class FontFamily {
				ASCENSION_NONCOPYABLE_TAG(FontFamily);
			public:
				explicit FontFamily(const String& name) : name_(name) {
					if(name.empty())
						throw std::length_error("name");
				}
				FontFamily& append(std::unique_ptr<FontFamily> family) /*throw()*/ {
					next_.reset(family.release());
					return *this;
				}
				const String& name() const /*throw()*/ {return name_;}
				FontFamily* next() /*throw()*/ {return next_.get();}
				const FontFamily* next() const /*throw()*/ {return next_.get();}
			private:
				const String name_;
				std::unique_ptr<FontFamily> next_;
			};

			class FontFamilies : public FontFamily {
				ASCENSION_NONCOPYABLE_TAG(FontFamilies);
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
				explicit FontFamilies(const String& firstName, GenericFamily genericFamily = UNSPECIFIED) : FontFamily(firstName), genericFamily_(genericFamily) {
				}
				GenericFamily genericFamily() const /*throw()*/ {return genericFamily_;}
				FontFamilies& setGenericFamily(GenericFamily genericFamily) {
					if(genericFamily < SERIF || genericFamily > UNSPECIFIED)
						throw UnknownValueException("genericFamily");
					genericFamily_ = genericFamily;
					return *this;
				}
			private:
				GenericFamily genericFamily_;
			};

			/**
			 * 
			 * @see FontProperties
			 */
			class FontPropertiesBase {
			public:
				enum Weight {
					NORMAL_WEIGHT = 400,
					BOLD = 700,
					BOLDER,
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
				};
				enum Stretch {
					NORMAL_STRETCH = 1000,
					WIDER,
					NARROWER,
					ULTRA_CONDENSED = 500,
					EXTRA_CONDENSED = 625,
					CONDENSED = 750,
					SEMI_CONDENSED = 875,
					SEMI_EXPANDED = 1125,
					EXPANDED = 1250,
					EXTRA_EXPANDED = 1500,
					ULTRA_EXPANDED = 2000
				};
				enum Style {
					NORMAL_STYLE,
					ITALIC,
					OBLIQUE,
					BACKSLANT
				};
				enum Variant {
					NORMAL_VARIANT,
					SMALL_CAPS
				};
				enum Orientation {
					HORIZONTAL,
					VERTICAL
				};
			};

			/**
			 * Set of font properties without the family name.
			 * @see FontDescription
			 */
			template<template<typename> class PropertyHolder = detail::Type2Type>
			class FontProperties : public FontPropertiesBase,
					private boost::equality_comparable<FontProperties<PropertyHolder>> {
			public:
				typedef typename PropertyHolder<Weight>::Type WeightType;
				typedef typename PropertyHolder<Stretch>::Type StretchType;
				typedef typename PropertyHolder<Style>::Type StyleType;
				typedef typename PropertyHolder<Variant>::Type VariantType;
				typedef typename PropertyHolder<Orientation>::Type OrientationType;
				typedef typename PropertyHolder<double>::Type PixelSizeType;
			public:
				/**
				 * Constructor.
				 * @param weight
				 * @param stretch
				 * @param style
				 * @param variant
				 * @param orientation
				 * @param size
				 */
				explicit FontProperties(
					WeightType weight = WeightType(), StretchType stretch = StretchType(),
					StyleType style = StyleType(), VariantType variant = VariantType(),
					OrientationType orientation = OrientationType(), PixelSizeType pixelSize = PixelSizeType())
					: weight_(weight), stretch_(stretch), style_(style), orientation_(orientation), pixelSize_(pixelSize) {}
				/// Implicit conversion operator.
				template<template<typename> class T> inline operator FontProperties<T>() const {
					return FontProperties<T>(weight(), stretch(), style(), variant(), orientation(), pixelSize());
				}
				/// Equality operator.
				bool operator==(const FontProperties& other) const /*throw()*/ {
					return weight_ == other.weight_ && stretch_ == other.stretch_
						&& style_ == other.style_ && equals(pixelSize_, other.pixelSize_);
				}
				/// Returns the hash value for this object.
				std::size_t hash() const /*throw()*/ {
					const std::collate<char>& coll(std::use_facet<std::collate<char>>(std::locale::classic()));
					// bad idea :(
					const char* temp = reinterpret_cast<const char*>(&pixelSize_);
					return coll.hash(temp, temp + sizeof(size_) / sizeof(char))
						+ (orientation() << 2) + (stretch() << 4) + (style() << 6) + (weight() << 8);
				}
				/// Returns the orientation.
				OrientationType orientation() const /*throw()*/ {return orientation_;}
				/// Returns the size in pixels. Zero means that inherit the parent.
				PixelSizeType pixelSize() const /*throw()*/ {return pixelSize_;}
				/// Returns the stretch.
				StretchType stretch() const /*throw()*/ {return stretch_;}
				/// Returns the style.
				StyleType style() const /*throw()*/ {return style_;}
				/// Returns the variant.
				VariantType variant() const /*throw()*/ {return variant_;}
				/// Returns the weight.
				WeightType weight() const /*throw()*/ {return weight_;}
			private:
				WeightType weight_;
				StretchType stretch_;
				StyleType style_;
				VariantType variant_;
				OrientationType orientation_;
				PixelSizeType pixelSize_;
			};

			template<template<typename> class PropertyHolder = detail::Type2Type>
			class FontDescription {
			public:
				/**
				 * Constructor.
				 * @param families The family names
				 * @param properties The properties other than the family names
				 */
				explicit FontDescription(
					std::unique_ptr<FontFamilies> families = std::unique_ptr<FontFamilies>(),
					const FontProperties<PropertyHolder>& properties = FontProperties<PropertyHolder>())
					: families_(families), properties_(properties) {}
				/// Returns the family names or @c null.
				const std::unique_ptr<FontFamilies>& families() const /*throw()*/ {return families_;}
				/// Returns the properties other than the family names.
				const FontProperties<PropertyHolder>& properties() const /* throw() */ {return properties_;}
			private:
				const std::unique_ptr<FontFamilies> families_;
				const FontProperties<PropertyHolder> properties_;
			};

#if defined(ASCENSION_SHAPING_ENGINE_CORE_GRAPHICS)
			typedef CGFontRef NativeFont;
#elif defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
			typedef CTFontRef NativeFont;
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
			typedef win32::com::ComPtr<IDWriteFont> NativeFont;
#elif defined(ASCENSION_SHAPING_ENGINE_HARFBUZZ)
			typedef hb_font_t* NativeFont;
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
			typedef PangoFont* NativeFont;
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
			typedef QFont* NativeFont;
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI)
			typedef win32::Handle<HFONT> NativeFont;
#endif
			/**
			 * Used to represent a group of fonts with the same family, slant, weight, width, but
			 * varying sizes.
			 */
			class FontFace {
			public:
				void availableSizes(std::vector<int>& sizes) const;
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
				 * Provides physical font metrics information.
				 * @see LineMetrics
				 */
				class Metrics {
				public:
					/// Destructor.
					virtual ~Metrics() /*throw()*/ {}
					/// Returns the ascent of the text in pixels.
					virtual int ascent() const /*throw()*/ = 0;
					/// Returns the average width of a character in pixels.
					virtual int averageCharacterWidth() const /*throw()*/ = 0;
					/// Returns the cell height in pixels.
					int cellHeight() const /*throw()*/ {return ascent() + descent();}
					/// Returns the descent of the text in pixels.
					virtual int descent() const /*throw()*/ = 0;
					/// Returns the em height in pixels.
					int emHeight() const /*throw()*/ {return cellHeight() - internalLeading();}
					/// Returns the external leading in pixels.
					/// @note In Ascension, external leadings are placed below characters.
					virtual int externalLeading() const /*throw()*/ = 0;
					/// Returns the internal leading in pixels.
					virtual int internalLeading() const /*throw()*/ = 0;
					/// Returns the gap of the lines (external leading) in pixels.
					int lineGap() const /*throw()*/ {return externalLeading();}
					/// Returns the pitch of lines in pixels.
					/// @note This method ignores @c LayoutSettings#lineSpacing value.
					int linePitch() const /*throw()*/ {return cellHeight() + lineGap();}
					/// Returns the x-height of the font in pixels.
					virtual int xHeight() const /*throw()*/ = 0;
				};
			public:
				/// Destructor.
				virtual ~Font() /*throw()*/ {}
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
				/**
				 * Returns the face name (logical name) of this font.
				 * @param lc The locale for which to get the font face name. If this value is
				 *           C or unsupported locale, this method returns an unlocalized name
				 * @return The face name of this font
				 * @see #familyName
				 */
				virtual String faceName(const std::locale& lc = std::locale::classic()) const /*throw()*/ = 0;
				/**
				 * Returns the family name of this font.
				 * @param lc The locale for which to get the font family name. If this value is
				 *           C or unsupported locale, this method returns an unlocalized name
				 * @return The family name of this font
				 * @see #faceName
				 */
				virtual String familyName(const std::locale& lc = std::locale::classic()) const /*throw()*/ = 0;
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				virtual bool ivsGlyph(CodePoint baseCharacter, CodePoint variationSelector, GlyphCode& glyph) const = 0;
#endif //ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
				/// Returns the metrics of the font.
				virtual const Metrics& metrics() const /*throw()*/ = 0;
				/// Returns the platform-dependent native object.
				virtual const NativeFont& nativeObject() const /*throw()*/ = 0;
			};

			/// An interface represents an object provides a set of fonts.
			class FontCollection {
			public:
				/// Destructor.
				virtual ~FontCollection() /*throw()*/ {}
				/**
				 * Returns the font matches the given properties.
				 * @param familyName The font family name
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested properties or the default one
				 */
				virtual std::shared_ptr<const Font> get(const String& familyName,
					const FontProperties<>& properties, double sizeAdjust = 0.0) const = 0;
				/**
				 * Returns the font for last resort fallback.
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested property
				 */
				virtual std::shared_ptr<const Font> lastResortFallback(
					const FontProperties<>& properties, double sizeAdjust = 0.0) const = 0;
			};

			const FontCollection& systemFonts() /*throw()*/;

		}
	}
}

#endif // !ASCENSION_FONT_HPP
