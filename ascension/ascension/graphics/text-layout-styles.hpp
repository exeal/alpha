/**
 * @file text-layout-styles.hpp
 * @see text-alignment.hpp, presentation/text-style.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012 was text-layout.hpp
 * @date 2012-08-17 separated from text-layout.hpp
 */

#ifndef ASCENSION_TEXT_LAYOUT_STYLES_HPP
#define ASCENSION_TEXT_LAYOUT_STYLES_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/corelib/utility.hpp>	// detail.searchBound
#include <ascension/graphics/color.hpp>
#include <ascension/presentation/text-style.hpp>
#include <memory>	// std.shared_ptr, std.unique_ptr

namespace ascension {

	namespace presentation {class Presentation;}
	namespace viewers {class Caret;}

	namespace graphics {

		class Paint;
		class PaintContext;

		namespace font {

			/**
			 * @see TextLayout#TextLayout
			 * @note This interface is designed based on @c TabExpander interface of Java.
			 */
			class TabExpander {
			public:
				/// Destructor.
				virtual ~TabExpander() /*noexcept*/ {}
				/**
				 * Returns the next tab stop position given a reference position.
				 * @param x The position in pixels
				 * @param tabOffset The position within the underlying text that the tab occured
				 * @return The next tab stop. Should be greater than @a x
				 */
				virtual Scalar nextTabStop(Scalar x, Index tabOffset) const = 0;
			};

			/// Standard implementation of @c TabExpander with fixed width tabulations.
			class FixedWidthTabExpander : public TabExpander {
			public:
				explicit FixedWidthTabExpander(Scalar width) /*noexcept*/;
				Scalar nextTabStop(Scalar x, Index tabOffset) const;
			private:
				const Scalar width_;
			};

			class TextPaintOverride {
			public:
				class Iterator {
				public:
					/// Destructor.
					virtual ~Iterator() /*noexcept*/ {}

					/**
					 * Returns the overridden foreground of the current position.
					 * @return The overridden foreground or @c null if does not override
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual const Paint* foreground() const = 0;
					/**
					 * Returns the overridden transparency of the foreground of the current
					 * position.
					 * @return The transparency. This value should be in the range from 0.0 (fully
					 *         transparent) to 1.0 (no additional transparency)
					 */
					virtual double foregroundAlpha() const = 0;
					/**
					 * Returns the overridden background of the current position.
					 * @return The overridden background or @c null if does not override
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual const Paint* background() const = 0;
					/**
					 * Returns the transparency of the overridden background of the current
					 * position.
					 * @return The transparency. This value should be in the range from 0.0 (fully
					 *         transparent) to 1.0 (no additional transparency)
					 */
					virtual double backgroundAlpha() const = 0;
					/**
					 * Returns @c false to paint only the glyphs' bounds with @c #background.
					 * Otherwise the logical highlight bounds of characters are painted as
					 * background.
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual bool usesLogicalHighlightBounds() const = 0;

					/// Returns the length of the current text segment.
					virtual Index length() const = 0;
					/// Returns @c true if the iterator has no more elements.
					virtual bool isDone() const /*noexcept*/ = 0;
					/**
					 * Moves the iterator to the next overriden text segment.
					 * @throw NoSuchElementException The iterator is end
					 */
					virtual void next() = 0;
					/// Moves the iterator to the beginning.
					virtual void reset() /*noexcept*/ = 0;
				};
			public:
				/// Destructor.
				virtual ~TextPaintOverride() /*noexcept*/ {}
				/**
				 * Returns the iterator which overrides the paints of the specified character
				 * range in the line.
				 * @param range The character range in the line
				 * @return The iterator which generates the overridden paints
				 */
				virtual std::unique_ptr<Iterator>
					queryTextPaintOverride(const Range<Index>& range) const = 0;
			};

			struct ComputedBorderSide {
				presentation::sp::IntrinsicType<
					decltype(presentation::Border().sides.start().color)
				>::Type::value_type color;
				presentation::sp::IntrinsicType<
					decltype(presentation::Border().sides.start().style)
				>::Type style;
				Scalar width;

				/// Default constructor.
				ComputedBorderSide() /*noexcept*/ :
					color(Color::TRANSPARENT_BLACK), style(presentation::Border::NONE), width(0) {}
				/// Returns the computed width in pixels.
				Scalar computedWidth() const /*noexcept*/ {
					return (style != presentation::Border::NONE) ? width : 0;
				}
				/// Returns @c true if this part is invisible (but may be consumes place).
				bool hasVisibleStyle() const /*noexcept*/ {
					return style != presentation::Border::NONE && style != presentation::Border::HIDDEN;
				}
			};

			/**
			 * @see ComputedTextRunStyle
			 */
			struct ComputedFontSpecification {
				const presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().fontFamily)
				>::Type* families;
				double pointSize;
//				presentation::sp::IntrinsicType<
//					decltype(presentation::TextRunStyle().fontSize)
//				> pointSize;
				FontProperties properties;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().fontSizeAdjust)
				>::Type sizeAdjust;
			};

			struct ComputedTextDecoration {
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().lines)
				>::Type lines;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().style)
				>::Type style;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().skip)
				>::Type skip;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextDecoration().underlinePosition)
				>::Type underlinePosition;
			};

			struct ComputedTextEmphasis {
				presentation::sp::IntrinsicType<
					decltype(presentation::TextEmphasis().style)
				>::Type style;
				Color color;
				presentation::sp::IntrinsicType<
					decltype(presentation::TextEmphasis().position)
				>::Type position;

				/// Default constructor initializes the members with initial values.
				ComputedTextEmphasis() /*noexcept*/ :
					style(presentation::TextEmphasis::NONE), color(Color::TRANSPARENT_BLACK),
					position(presentation::TextEmphasis::ABOVE | presentation::TextEmphasis::RIGHT) {}
			};

			/// Computed values of @c presentation#TextRunStyle.
			struct ComputedTextRunStyle {
				/// Computed value of @c TextRunStyle#color property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().color)
				>::Type::value_type color;
				/// Computed value of @c TextRunStyle#background property.
				std::shared_ptr<const Paint> background;
				/// Computed value of @c TextRunStyle#border property.
				PhysicalFourSides<ComputedBorderSide> borders;

				/// Computed values of font specification of @c TextRunStyle.
				ComputedFontSpecification font;

				/// Computed value of @c TextRunStyle#textHeight property.
				Scalar textHeight;
				/// Computed value of @c TextRunStyle#lineHeight property.
				Scalar lineHeight;
				/// Computed value of @c TextRunStyle#dominantBaseline property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().dominantBaseline)
				> dominantBaseline;
				/// Computed value of @c TextRunStyle#alignmentBaseline property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().alignmentBaseline)
				> alignmentBaseline;
				/// Computed value of @c TextRunStyle#alignmentAdjustment property.
				Scalar alignmentAdjustment;
				/// Computed value of @c TextRunStyle#baselineShift property.
				Scalar baselineShift;
				
				/// Computed value of @c TextRunStyle#textTransform property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().textTransform)
				>::Type textTransform;
				/// Computed value of @c TextRunStyle#hyphens property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().hyphens)
				>::Type hyphens;
				/// Computed value of @c TextRunStyle#wordSpacing property.
				presentation::SpacingLimit<Scalar> wordSpacing;
				/// Computed value of @c TextRunStyle#letterSpacing property.
				presentation::SpacingLimit<Scalar> letterSpacing;
				/// Computed value of @c TextRunStyle#textDecoration property.
				ComputedTextDecoration textDecoration;
				/// Computed value of @c TextRunStyle#textEmphasis property.
				ComputedTextEmphasis textEmphasis;
//				TextShadow textShadow;

//				RubyProperties rubyProperties;

//				Effects effects;
				/// Computed value of @c TextRunStyle#shapingEnabled.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextRunStyle().shapingEnabled)
				>::Type shapingEnabled;

				/// Default constructor initializes the all properties with their default values.
				ComputedTextRunStyle() : color(0, 0, 0), background(nullptr), borders(), font() {}
			};

			/**
			 * @see TextLayout#TextLayout, presentation#StyledTextRunIterator
			 */
			class ComputedStyledTextRunIterator {
			public:
				/// Destructor.
				virtual ~ComputedStyledTextRunIterator() /*noexcept*/ {}
				/**
				 */
				virtual Range<Index> currentRange() const = 0;
				/**
				 */
				virtual void currentStyle(ComputedTextRunStyle& style) const = 0;
				virtual bool isDone() const /*noexcept*/ = 0;
				virtual void next() = 0;
			};

			struct ComputedNumberSubstitution {
				presentation::sp::IntrinsicType<
					decltype(presentation::NumberSubstitution().method)
				>::Type method;
				presentation::sp::IntrinsicType<
					decltype(presentation::NumberSubstitution().localeName)
				>::Type localeName;
				presentation::sp::IntrinsicType<
					decltype(presentation::NumberSubstitution().ignoreUserOverride)
				>::Type ignoreUserOverride;
			};

			/// Computed values of @c presentation#TextLineStyle.
			struct ComputedTextLineStyle {
				/// Computed value of writing modes properties of @c TextToplevelStyle.
				presentation::WritingMode writingMode;

				/// Computed value of @c TextLineStyle#lineBoxContain property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().lineBoxContain)
				>::Type lineBoxContain;
				/// Computed value of @c TextLineStyle#whiteSpace property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().whiteSpace)
				>::Type whiteSpace;
				/// Computed value of @c TextLineStyle#tabSize property.
				std::shared_ptr<TabExpander> tabExpander;
				/// Computed value of @c TextLineStyle#lineBreak property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().lineBreak)
				>::Type lineBreak;
				/// Computed value of @c TextLineStyle#wordBreak property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().wordBreak)
				>::Type wordBreak;
				/// Computed value of @c TextLineStyle#overflowWrap property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().overflowWrap)
				>::Type overflowWrap;
				/// Computed value of @c TextLineStyle#textAlignment property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().textAlignment)
				>::Type alignment;
				/// Computed value of @c TextLineStyle#textAlignmentLast property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().textAlignmentLast)
				>::Type alignmentLast;
				/// Computed value of @c TextLineStyle#textJustification property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().textJustification)
				>::Type justification;
				/// Computed value of @c TextLineStyle#textIndent property.
				presentation::TextIndent<Scalar, bool> indent;
				/// Computed value of @c TextLineStyle#hangingPunctuation property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().hangingPunctuation)
				>::Type hangingPunctuation;
				/// Computed value of @c TextLineStyle#dominantBaseline property.
				presentation::sp::IntrinsicType<
					decltype(presentation::TextLineStyle().dominantBaseline)
				>::Type dominantBaseline;
				/// Computed value of @c TextLineStyle#lineHeight property.
				Scalar lineHeight;
				/// Computed value of @c TextLineStyle#measure property.
				Scalar measure;
				/// Computed value of @c TextLineStyle#numberSubstitution property.
				ComputedNumberSubstitution numberSubstitution;

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
			};

		}
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
			bool isDone() const /*noexcept*/ {
				return position_ == length(textString_);
			}
			void next() {
				throwIfDone();
				if(source_->isDone())
					position_ = length(textString_);
				else {
					const Range<Index> sourceRange(source_->currentRange());
					// sanity checks...
					if(isEmpty(sourceRange))
						throw std::domain_error("ComputedStyledTextRunIterator.currentRange returned an empty range.");
					else if(textString_.beginning() + sourceRange.end() > textString_.end())
						throw std::domain_error("ComputedStyledTextRunIterator.currentRange returned a range intersects outside of the source text string.");
					else if(sourceRange.beginning() <= position_)
						throw std::domain_error("ComputedStyledTextRunIterator.currentRange returned a backward range.");
					if(position_ < sourceRange.beginning())
						position_ = sourceRange.beginning();
					else {
						assert(position_ == sourceRange.beginning());
						source_->next();
						position_ = sourceRange.end();
					}
				}
			}
			StringPiece::const_pointer position() const {
				throwIfDone();
				return textString_.beginning() + position_;
			}
			void style(graphics::font::ComputedTextRunStyle& v) const {
				throwIfDone();
				if(position_ == source_->currentRange().beginning())
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
		std::shared_ptr<const graphics::font::Font> findMatchingFont(
			const StringPiece& textRun, const graphics::font::FontCollection& collection,
			const graphics::font::ComputedFontSpecification& specification);
		void paintBorder(graphics::PaintContext& context,
			const graphics::NativeRectangle& rectangle,
			const graphics::PhysicalFourSides<graphics::font::ComputedBorderSide>& style,
			const presentation::WritingMode& writingMode);
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_LAYOUT_STYLES_HPP
