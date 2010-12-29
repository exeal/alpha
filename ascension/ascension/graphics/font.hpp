/**
 * @file font.hpp
 * @author exeal
 * @date 2010-11-06 created
 */

#ifndef ASCENSION_FONT_HPP
#define ASCENSION_FONT_HPP

#include <ascension/config.hpp>	// ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#include <ascension/platforms.hpp>
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/basic-types.hpp>		// uint32_t
#include <ascension/graphics/geometry.hpp>
#include <cstring>	// std.strlen
#include <locale>	// std.collate
#ifdef ASCENSION_WINDOWS
#	include <ascension/win32/windows.hpp>	// win32.Handle
#endif // ASCENSION_WINDOWS

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
						throw std::invalid_argument(std::string("name[") + i + "]");
					tag[i] |= name[i] << ((3 - i) * 8);
				}
				for(; i < 4; ++i)
					tag[i] |= ' ' << ((3 - i) * 8);
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
				virtual Dimension<> bounds() const = 0;
				virtual Scalar leftTopSideBearing() const = 0;
				virtual Scalar rightBottomSideBearing() const = 0;
			};

			class GlyphVector {
			public:
				/// Destructor.
				virtual ~GlyphVector() /*throw()*/ {}
				GlyphCode operator[](std::ptrdiff_t index) const {}
				virtual GlyphCode at(std::size_t index) const = 0;
				std::size_t length() const {return size();}
				virtual Dimension<> logicalBounds() const = 0;
				virtual Dimension<> logicalGlyphBounds(std::size_t index) const = 0;
				virtual std::tr1::shared_ptr<GlyphMetrics> metrics(std::size_t index) const = 0;
//				virtual std::tr1::shared_ptr<Shape> outline(std::size_t index) const = 0;
				virtual Point<> position(std::size_t index) const = 0;
				/// Returns
				virtual std::size_t size() const = 0;
				virtual Dimension<> visualGlyphBounds(std::size_t index) const = 0;
				virtual Dimension<> visualBounds() const = 0;
			};

			class FontFamily {
				ASCENSION_NONCOPYABLE_TAG(FontFamily);
			public:
				explicit FontFamily(const String& name) : name_(name) {
					if(name.empty())
						throw std::length_error("name");
				}
				FontFamily& append(std::auto_ptr<FontFamily> family) /*throw()*/ {next_ = family; return *this;}
				const String& name() const /*throw()*/ {name_;}
				FontFamily* next() /*throw()*/ {return next_.get();}
				const FontFamily* next() const /*throw()*/ {return next_.get();}
			private:
				const String name_;
				std::auto_ptr<FontFamily> next_;
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
			 * Set of font properties without the family name.
			 * @see FontDescription
			 */
			class FontProperties {
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
					HEAVY = 900,
					INHERIT_WEIGHT
				};
				enum Stretch {
					NORMAL_STRETCH,
					WIDER,
					NARROWER,
					ULTRA_CONDENSED,
					EXTRA_CONDENSED,
					CONDENSED,
					SEMI_CONDENSED,
					SEMI_EXPANDED,
					EXPANDED,
					EXTRA_EXPANDED,
					ULTRA_EXPANDED,
					INHERIT_STRETCH
				};
				enum Style {
					NORMAL_STYLE,
					ITALIC,
					OBLIQUE,
					BACKSLANT,
					INHERIT_STYLE
				};
				enum Orientation {
					HORIZONTAL,
					VERTICAL,
					INHERIT_ORIENTATION
				};
			public:
				/**
				 * Constructor.
				 * @param weight
				 * @param stretch
				 * @param style
				 * @param orientation
				 * @param size
				 */
				explicit FontProperties(
					Weight weight = INHERIT_WEIGHT, Stretch stretch = INHERIT_STRETCH,
					Style style = INHERIT_STYLE, Orientation orientation = INHERIT_ORIENTATION, double size = 0.0)
					: weight_(weight), stretch_(stretch), style_(style), orientation_(orientation), size_(size) {}
				/// Equality operator.
				bool operator==(const FontProperties& other) const /*throw()*/ {
					return weight_ == other.weight_ && stretch_ == other.stretch_
						&& style_ == other.style_ && equals(size_, other.size_);
				}
				/// Inequality operator.
				bool operator!=(const FontProperties& other) const /*throw()*/ {
					return !(*this == other);
				}
				/// Returns the hash value for this object.
				std::size_t hash() const /*throw()*/ {
					const std::collate<char>& coll(std::use_facet<std::collate<char> >(std::locale::classic()));
					// bad idea :(
					const char* temp = reinterpret_cast<const char*>(&size_);
					return coll.hash(temp, temp + sizeof(size_) / sizeof(char))
						+ (orientation() << 2) + (stretch() << 4) + (style() << 6) + (weight() << 8);
				}
				/// Returns the orientation.
				Orientation orientation() const /*throw()*/ {return orientation_;}
				/// Returns the size in pixels. Zero means that inherit the parent.
				double size() const /*throw()*/ {return size_;}
				/// Returns the stretch.
				Stretch stretch() const /*throw()*/ {return stretch_;}
				/// Returns the style.
				Style style() const /*throw()*/ {return style_;}
				/// Returns the weight.
				Weight weight() const /*throw()*/ {return weight_;}
			private:
				Weight weight_;
				Stretch stretch_;
				Style style_;
				Orientation orientation_;
				double size_;
			};

			class FontDescription {
			public:
				/**
				 * Constructor.
				 * @param families The family names
				 * @param properties The properties other than the family names
				 */
				explicit FontDescription(
					std::auto_ptr<FontFamilies> families = std::auto_ptr<FontFamilies>(),
					const FontProperties& properties = FontProperties())
					: families_(families), properties_(properties) {}
				/// Returns the family names or @c null.
				const std::auto_ptr<FontFamilies>& families() const /*throw()*/ {return families_;}
				/// Returns the properties other than the family names.
				const FontProperties& properties() const /* throw() */ {return properties_;}
			private:
				const std::auto_ptr<FontFamilies> families_;
				const FontProperties properties_;
			};

			class Font {
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
					/// Returns the em height.
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
#ifdef ASCENSION_WINDOWS
				/// Returns the Win32 @c HFONT handle object.
				virtual const win32::Handle<HFONT>& nativeHandle() const /*throw()*/ = 0;
#endif // ASCENSION_WINDOWS
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
				virtual std::tr1::shared_ptr<const Font> get(const String& familyName,
					const FontProperties& properties, double sizeAdjust = 0.0) const = 0;
				/**
				 * Returns the font for last resort fallback.
				 * @param properties The font properties
				 * @param sizeAdjust 
				 * @return The font has the requested property
				 */
				virtual std::tr1::shared_ptr<const Font> lastResortFallback(
					const FontProperties& properties, double sizeAdjust = 0.0) const = 0;
			};

			const FontCollection& systemFonts() /*throw()*/;

		}
	}
}

#endif // !ASCENSION_FONT_HPP
