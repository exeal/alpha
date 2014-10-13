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

namespace ascension {
	namespace graphics {

		class Paint;

		namespace font {
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
/*
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
*/
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
