/**
 * @file text-layout-styles.hpp
 * @see computed-text-styles.hpp, text-alignment.hpp, presentation/text-style.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010, 2014
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012 was text-layout.hpp
 * @date 2012-08-17 separated from text-layout.hpp
 */

#ifndef ASCENSION_TEXT_LAYOUT_STYLES_HPP
#define ASCENSION_TEXT_LAYOUT_STYLES_HPP

#include <ascension/graphics/font/computed-text-styles.hpp>
#include <vector>

namespace ascension {
	namespace graphics {

		class Paint;

		namespace font {
			/**
			 * @see TextLayout#TextLayout
			 * @note This interface is designed based on @c TabExpander interface of Java.
			 */
			class TabExpander {
			public:
				/// Destructor.
				virtual ~TabExpander() BOOST_NOEXCEPT {}
				/**
				 * Returns the next tab stop position given a reference position.
				 * @param ipd The position in pixels
				 * @param tabOffset The position within the underlying text that the tab occured
				 * @return The next tab stop. Should be greater than @a ipd
				 */
				virtual Scalar nextTabStop(Scalar ipd, Index tabOffset) const = 0;
			};

			/// Standard implementation of @c TabExpander with fixed width tabulations.
			class FixedWidthTabExpander : public TabExpander {
			public:
				explicit FixedWidthTabExpander(Scalar width) BOOST_NOEXCEPT;
				Scalar nextTabStop(Scalar ipd, Index tabOffset) const;
			private:
				const Scalar width_;
			};

			class TextPaintOverride {
			public:
				struct Segment {
					/// The length of this segment.
					Index length;
					/// The overridden foreground or @c null if does not override.
					std::shared_ptr<const Paint> foreground;
					/// The transparency of the overridden foreground. This value should be in the
					/// range from 0.0 (fully transparent) to 1.0 (no additional transparency).
					double foregroundAlpha;
					/// The overridden background or @c null if does not override.
					std::shared_ptr<const Paint> background;
					/// The transparency of the overridden background. This value should be in the
					/// range from 0.0 (fully transparent) to 1.0 (no additional transparency).
					double backgroundAlpha;
					/// Set @c false to paint only the glyphs' bounds with @c #background.
					/// Otherwise the logical highlight bounds of characters are painted as
					/// background.
					bool usesLogicalHighlightBounds;
				};
			public:
				/// Destructor.
				virtual ~TextPaintOverride() BOOST_NOEXCEPT {}
				/**
				 * Returns a vector of segments which describe override the paints of the specified
				 * character range in the line.
				 * @param range The character range in the line
				 * @return A vector of @c #Segment
				 */
				virtual std::vector<const Segment>&&
					queryTextPaintOverride(const boost::integer_range<Index>& range) const = 0;
			};

			/// Computed values of core properties of @c presentation#TextRunStyle.
			struct ComputedTextRunStyleCore : private boost::equality_comparable<ComputedTextRunStyleCore> {
				/// Computed value of @c TextRunStyle#color property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextRunStyle().color)
				>::type::value_type color;
				/// Computed value of @c TextRunStyle#background property.
				std::shared_ptr<const Paint> background;
				/// Computed value of @c TextRunStyle#border property.
				presentation::FlowRelativeFourSides<ComputedBorderSide> border;
				///Computed value of @c TextRunStyle#padding property.
				presentation::FlowRelativeFourSides<Scalar> padding;
				///Computed value of @c TextRunStyle#margin property.
				presentation::FlowRelativeFourSides<Scalar> margin;
				/// Computed value of @c TextRunStyle#textDecoration property.
				ComputedTextDecoration textDecoration;
				/// Computed value of @c TextRunStyle#textEmphasis property.
				ComputedTextEmphasis textEmphasis;
//				TextShadow textShadow;

				/// Default constructor initializes the all properties with their default values.
				ComputedTextRunStyleCore() : color(0, 0, 0), background(nullptr), border() {}
				/// Equality operator.
				bool operator==(const ComputedTextRunStyleCore& other) const BOOST_NOEXCEPT {
					return color == other.color && background == other.background
						&& border == other.border && padding == other.padding && margin == other.margin
						&& textDecoration == other.textDecoration && textEmphasis == other.textEmphasis;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ComputedTextRunStyleCore.
			inline std::size_t hash_value(const ComputedTextRunStyleCore& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.color);
				boost::hash_combine(seed, object.background);
				boost::hash_combine(seed, object.border);
				boost::hash_combine(seed, object.padding);
				boost::hash_combine(seed, object.margin);
				boost::hash_combine(seed, object.textDecoration);
				boost::hash_combine(seed, object.textEmphasis);
				return seed;
			}

			/// Computed values of @c presentation#TextRunStyle.
			struct ComputedTextRunStyle : public ComputedTextRunStyleCore, private boost::equality_comparable<ComputedTextRunStyle> {
				/// Computed values of font specification of @c TextRunStyle.
				ComputedFontSpecification font;

				/// Computed value of @c TextRunStyle#textHeight property.
				Scalar textHeight;
				/// Computed value of @c TextRunStyle#lineHeight property.
				Scalar lineHeight;
				/// Computed value of @c TextRunStyle#dominantBaseline property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextRunStyle().dominantBaseline)
				>::type dominantBaseline;
				/// Computed value of @c TextRunStyle#alignmentBaseline property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextRunStyle().alignmentBaseline)
				>::type alignmentBaseline;
				/// Computed value of @c TextRunStyle#alignmentAdjustment property.
				Scalar alignmentAdjustment;
				/// Computed value of @c TextRunStyle#baselineShift property.
				Scalar baselineShift;
				
				/// Computed value of @c TextRunStyle#textTransform property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextRunStyle().textTransform)
				>::type textTransform;
				/// Computed value of @c TextRunStyle#hyphens property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextRunStyle().hyphens)
				>::type hyphens;
				/// Computed value of @c TextRunStyle#wordSpacing property.
				presentation::styles::SpacingLimit<Scalar> wordSpacing;
				/// Computed value of @c TextRunStyle#letterSpacing property.
				presentation::styles::SpacingLimit<Scalar> letterSpacing;

//				RubyProperties rubyProperties;

//				Effects effects;
				/// Computed value of @c TextRunStyle#shapingEnabled.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextRunStyle().shapingEnabled)
				>::type shapingEnabled;

				/// Default constructor initializes nothing.
				ComputedTextRunStyle() {}
				/// Equality operator.
				bool operator==(const ComputedTextRunStyle& other) const BOOST_NOEXCEPT {
					return font == other.font
						&& textHeight == other.textHeight && lineHeight == other.lineHeight
						&& dominantBaseline == other.dominantBaseline && alignmentBaseline == other.alignmentBaseline
						&& alignmentAdjustment == other.alignmentAdjustment && baselineShift == other.baselineShift
						&& textTransform == other.textTransform && hyphens == other.hyphens
						&& wordSpacing == other.wordSpacing && letterSpacing == other.letterSpacing;
				}
			};

			/**
			 * @see TextLayout#TextLayout, presentation#StyledTextRunIterator
			 */
			class ComputedStyledTextRunIterator {
			public:
				/// Destructor.
				virtual ~ComputedStyledTextRunIterator() BOOST_NOEXCEPT {}
				/**
				 */
				virtual boost::integer_range<Index> currentRange() const = 0;
				/**
				 */
				virtual void currentStyle(ComputedTextRunStyle& style) const = 0;
				virtual bool isDone() const BOOST_NOEXCEPT = 0;
				virtual void next() = 0;
			};

			/// Computed values of @c presentation#TextLineStyle.
			struct ComputedTextLineStyle : private boost::equality_comparable<ComputedTextLineStyle> {
				/// Computed value of writing modes properties of @c TextToplevelStyle.
				presentation::WritingMode writingMode;

				/// Computed value of @c TextLineStyle#defaultRunStyle#background property.
				std::shared_ptr<const Paint> background;

				/// Computed value of @c TextLineStyle#defaultRunStyle#fontXxxx properties.
				ComputedFontSpecification nominalFont;

				/// Computed value of @c TextLineStyle#lineBoxContain property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().lineBoxContain)
				>::type lineBoxContain;
				/// Computed value of @c TextLineStyle#whiteSpace property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().whiteSpace)
				>::type whiteSpace;
				/// Computed value of @c TextLineStyle#tabSize property.
				std::shared_ptr<TabExpander> tabExpander;
				/// Computed value of @c TextLineStyle#lineBreak property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().lineBreak)
				>::type lineBreak;
				/// Computed value of @c TextLineStyle#wordBreak property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().wordBreak)
				>::type wordBreak;
				/// Computed value of @c TextLineStyle#overflowWrap property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().overflowWrap)
				>::type overflowWrap;
				/// Computed value of @c TextLineStyle#textAlignment property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().textAlignment)
				>::type alignment;
				/// Computed value of @c TextLineStyle#textAlignmentLast property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().textAlignmentLast)
				>::type alignmentLast;
				/// Computed value of @c TextLineStyle#textJustification property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().textJustification)
				>::type justification;
				/// Computed value of @c TextLineStyle#textIndent property.
				presentation::styles::BasicTextIndent<Scalar, bool> indent;
				/// Computed value of @c TextLineStyle#hangingPunctuation property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().hangingPunctuation)
				>::type hangingPunctuation;
				/// Computed value of @c TextLineStyle#dominantBaseline property.
				presentation::styles::ComputedValueType<
					decltype(presentation::TextLineStyle().dominantBaseline)
				>::type dominantBaseline;
				/// Computed value of @c TextLineStyle#lineHeight property.
				Scalar lineHeight;
				/// Computed value of @c TextLineStyle#measure property.
				Scalar measure;
				/// Computed value of @c TextLineStyle#numberSubstitution property.
				presentation::styles::NumberSubstitution numberSubstitution;

				/**
				 * Set @c true to shape zero width control characters as representative glyphs.
				 * The default value is @c false.
				 */
				bool displayShapingControls;
				/**
				 * Set @c true to make the deprecated format characters (NADS, NODS, ASS and ISS)
				 * not effective. The default value is @c false.
				 */
				bool disableDeprecatedFormatCharacters;
				/**
				 * Set @c true to inhibit from generating mirrored glyphs. The default value is
				 * @c false.
				 */
				bool inhibitSymmetricSwapping;

				/// Equality operator.
				bool operator==(const ComputedTextLineStyle& other) const BOOST_NOEXCEPT {
					return writingMode == other.writingMode
						&& background == other.background && nominalFont == other.nominalFont
						&& lineBoxContain == other.lineBoxContain && whiteSpace == other.whiteSpace
						&& tabExpander == other.tabExpander && lineBreak == other.lineBreak && wordBreak == other.wordBreak
						&& overflowWrap == other.overflowWrap
						&& alignment == other.alignment && alignmentLast == other.alignmentLast
						&& justification == other.justification && indent == other.indent
						&& hangingPunctuation == other.hangingPunctuation
						&& dominantBaseline == other.dominantBaseline
						&& lineHeight == other.lineHeight && measure == other.measure
						&& numberSubstitution == other.numberSubstitution;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c ComputedTextLineStyle.
			inline std::size_t hash_value(const ComputedTextLineStyle& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.writingMode);
				boost::hash_combine(seed, object.background);
				boost::hash_combine(seed, object.nominalFont);
				boost::hash_combine<int>(seed, boost::native_value(object.lineBoxContain));
				boost::hash_combine<int>(seed, boost::native_value(object.whiteSpace));
				boost::hash_combine(seed, object.tabExpander);
				boost::hash_combine<int>(seed, boost::native_value(object.lineBreak));
				boost::hash_combine<int>(seed, boost::native_value(object.wordBreak));
				boost::hash_combine<int>(seed, boost::native_value(object.overflowWrap));
				boost::hash_combine<int>(seed, boost::native_value(object.alignment));
				boost::hash_combine<int>(seed, boost::native_value(object.alignmentLast));
				boost::hash_combine<int>(seed, boost::native_value(object.justification));
				boost::hash_combine(seed, object.indent);
				boost::hash_combine<int>(seed, boost::native_value(object.hangingPunctuation));
				boost::hash_combine<int>(seed, boost::native_value(object.dominantBaseline));
				boost::hash_combine(seed, object.lineHeight);
				boost::hash_combine(seed, object.measure);
				boost::hash_combine(seed, object.numberSubstitution);
				boost::hash_combine(seed, object.displayShapingControls);
				boost::hash_combine(seed, object.disableDeprecatedFormatCharacters);
				boost::hash_combine(seed, object.inhibitSymmetricSwapping);
				return true;
			}

			namespace detail {
				class ComputedStyledTextRunEnumerator {
				public:
					ComputedStyledTextRunEnumerator(const StringPiece& textString,
							std::unique_ptr<graphics::font::ComputedStyledTextRunIterator> source)
							: source_(std::move(source)), textString_(textString), position_(0) {
						if(source_.get() == nullptr)
							throw NullPointerException("source");
					}
					bool isDone() const BOOST_NOEXCEPT {
						return position_ == textString_.length();
					}
					void next() {
						throwIfDone();
						if(source_->isDone())
							position_ = textString_.length();
						else {
							const boost::integer_range<Index> sourceRange(source_->currentRange());
							// sanity checks...
							if(sourceRange.empty())
								throw std::domain_error("ComputedStyledTextRunIterator.currentRange returned an empty range.");
							else if(textString_.begin() + *sourceRange.end() > textString_.end())
								throw std::domain_error("ComputedStyledTextRunIterator.currentRange returned a range intersects outside of the source text string.");
							else if(*sourceRange.begin() <= position_)
								throw std::domain_error("ComputedStyledTextRunIterator.currentRange returned a backward range.");
							if(position_ < *sourceRange.begin())
								position_ = *sourceRange.begin();
							else {
								assert(position_ == *sourceRange.begin());
								source_->next();
								position_ = *sourceRange.end();
							}
						}
					}
					StringPiece::const_iterator position() const {
						throwIfDone();
						return textString_.begin() + position_;
					}
					void style(graphics::font::ComputedTextRunStyle& v) const {
						throwIfDone();
						if(position_ == *source_->currentRange().begin())
							source_->currentStyle(v);
						else
							v = graphics::font::ComputedTextRunStyle();
					}
				private:
					void throwIfDone() const {
						if(isDone())
							throw NoSuchElementException();
					}
					std::unique_ptr<graphics::font::ComputedStyledTextRunIterator> source_;
					const StringPiece& textString_;
					Index position_;	// beginning of current run
				};

				class Font;
				class FontCollection;
	
				std::shared_ptr<const graphics::font::Font> findMatchingFont(
					const StringPiece& textRun, const graphics::font::FontCollection& collection,
					const graphics::font::ComputedFontSpecification& specification);
			}
		}
	}
} // namespace ascension.graphics.font

namespace std {
	/// Specialization of @c std#hash class template for @c ComputedTextRunStyleCore.
	template<>
	class hash<ascension::graphics::font::ComputedTextRunStyleCore> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::ComputedTextRunStyleCore&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_TEXT_LAYOUT_STYLES_HPP
