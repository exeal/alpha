/**
 * @file font/text-hit.hpp
 * Defines @c ascension#graphics#font#TextHit class.
 * @author exeal
 * @date 2012-11-04 created
 * @date 2013-01-27 renamed from text-hit-information.hpp
 * @date 2012-2013
 */

#ifndef ASCENSION_GRAPHICS_FONT_TEXT_HIT_HPP
#define ASCENSION_GRAPHICS_FONT_TEXT_HIT_HPP
#include <ascension/corelib/basic-types.hpp>	// Index, SignedIndex, BOOST_NOEXCEPT
#include <boost/operators.hpp>
#include <numeric>		// std.numeric_limits
#include <stdexcept>	// std.overflow_error, std.underflow_error

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Represents a character position in a text layout or a text viewport, and a <strong>bias</strong> or side
			 * of the character. Biases are either <em>leading</em> (the left edge, for a left-to-right character) or
			 * <em>trailing</em> (the right edge, for a left-to-right character). Instances of @c TextHit are used to
			 * specify caret and insertion positions within text.
			 * @tparam Position A type represents a character position. Typically either @c Index or @c kernel#Position
			 * @see TextLayout, TextViewport
			 * @note This class is designed based on @c java.awt.font.TextHitInfo class in Java.
			 */
			template<typename Position = Index>
			class TextHit : private boost::totally_ordered<TextHit<Position>> {
			public:
				typedef Position value_type;	///< Represents a character position.

				/// @name Factories
				/// @{
				/**
				 * Creates a @c TextHit at the specified offset, associated with the character after the offset.
				 * @param offset An offset associated with the character after the offset
				 * @return A @c TextHit at the specified offset
				 * @see #beforeOffset
				 */
				static TextHit afterOffset(const value_type& offset) BOOST_NOEXCEPT {
					return TextHit(offset, true);
				}
				/**
				 * Creates a @c TextHit at the specified offset, associated with the character before the offset.
				 * @param offset An offset associated with the character before the offset
				 * @return A @c TextHit at the specified offset
				 * @see #afterOffset
				 */
				static TextHit beforeOffset(const value_type& offset) BOOST_NOEXCEPT;
				/**
				 * Creates a @c TextHit on the leading edge of the character at the specified @a characterIndex.
				 * @param characterIndex The index of the character hit
				 * @return A @c TextHit on the leading edge of the character at the specified @a characterIndex
				 */
				static TextHit leading(const value_type& characterIndex) BOOST_NOEXCEPT {
					return TextHit(characterIndex, true);
				}
				/**
				 * Creates a @c TextHit on the trailing edge of the character at the specified @a characterIndex.
				 * @param characterIndex The index of the character hit
				 * @return A @c TextHit on the trailing edge of the character at the specified @a characterIndex
				 */
				static TextHit trailing(const value_type& characterIndex) BOOST_NOEXCEPT {
					return TextHit(characterIndex, false);
				}
				/// @}

				/// @name Relational Operators
				/// Relational operators of @c TextHit compare the @c #characterIndex.
				/// @{
				/** Equality operator. */
				bool operator==(const TextHit& other) const BOOST_NOEXCEPT {
					return characterIndex() == other.characterIndex() && isLeadingEdge() == other.isLeadingEdge();
				}
				/// Less-than operator.
				bool operator<(const TextHit& other) const BOOST_NOEXCEPT {
					return characterIndex() < other.characterIndex()
						|| (characterIndex() == other.characterIndex() && isLeadingEdge() && !other.isLeadingEdge());
				}
				/// @}

				/// @name Attributes
				/// @{
				/**
				 * Returns the index of the character hit.
				 * @see #insertionIndex
				 */
				const value_type& characterIndex() const BOOST_NOEXCEPT {return characterIndex_;}
				/**
				 * Returns the insertion index. This is the character index if the leading edge of the character was
				 * hit, and one greater than the character index if the trailing edge was hit.
				 * @see #characterIndex
				 */
				value_type insertionIndex() const BOOST_NOEXCEPT;
				/// Returns @c true if the leading edge of the character was hit.
				bool isLeadingEdge() const BOOST_NOEXCEPT {return isLeadingEdge_;}
				/// @}

				/// @name Other Factories
				/// @{
				/**
				 * Creates a @c TextHit whose character index is offset by @a delta from the @c #characterIndex of this
				 * @c TextHit. This @c TextHit remains unchanged.
				 * @param delta The value to offset this @c #characterIndex
				 * @return A @c TextHit whose @c #characterIndex is offset by @a delta from the @c #characterIndex of
				 *         this @c TextHit
				 * @throw std#overflow_error
				 * @throw std#underflow_error
				 * @see #otherHit
				 */
				TextHit offsetHit(SignedIndex delta) const;
				/**
				 * Creates a @c TextHit whose character index is offset by the given function object. This @c TextHit
				 * remains unchanged.
				 * @tparam Function The type of @a f
				 * @param f The function object takes a value returned by @c #characterIndex() and returns a value
				 *          which can be passed to the constructors
				 * @return A @c TextHit whose @c #characterIndex is offset by @a f
				 */
				template<typename Function>
				TextHit offsetHit(Function f) const {
					return TextHit(f(characterIndex()), isLeadingEdge());
				}
				/**
				 * Creates a @c TextHit on the other side of the insertion point. This @c TextHit remains unchanged.
				 * @return A @c TextHit on the other side of the insertion point
				 * @see #offsetHit
				 */
				TextHit otherHit() const BOOST_NOEXCEPT;
				/// @}

			private:
				TextHit(const value_type& characterIndex, bool isLeadingEdge) BOOST_NOEXCEPT
					: characterIndex_(characterIndex), isLeadingEdge_(isLeadingEdge) {}
				value_type characterIndex_;
				bool isLeadingEdge_;
			};

			template<typename Character, typename Traits, typename Position>
			inline std::basic_ostream<Character, Traits>& operator<<(std::basic_ostream<Character, Traits>& out, const TextHit<Position>& v) {
				std::ostringstream ss;
				ss << "TextHit[" << v.characterIndex() << (v.isLeadingEdge() ? "L]" : "T]");
				const std::string s(ss.str());
				return out << std::basic_string<Character, Traits>(std::begin(s), std::end(s));
			}

			template<> inline TextHit<Index> TextHit<Index>::beforeOffset(const Index& offset) BOOST_NOEXCEPT {
				return TextHit<Index>(offset - 1, false);
			}

			template<> inline Index TextHit<Index>::insertionIndex() const BOOST_NOEXCEPT {
				return isLeadingEdge() ? characterIndex() : characterIndex() + 1;
			}

			template<> inline TextHit<Index> TextHit<Index>::offsetHit(SignedIndex delta) const {
				if(delta > 0 && static_cast<Index>(delta) > std::numeric_limits<Index>::max() - characterIndex())
					throw std::overflow_error("delta");
				else if(delta < 0 && static_cast<Index>(-delta) > characterIndex())
					throw std::underflow_error("delta");
				return TextHit(characterIndex() + delta, isLeadingEdge());
			}

			template<> inline TextHit<Index> TextHit<Index>::otherHit() const BOOST_NOEXCEPT {
				return isLeadingEdge() ? trailing(characterIndex() - 1) : leading(characterIndex() + 1);
			}

			/// @defgroup texthit_other_factories Free Functions to Make @c TextHit&lt;&gt; Objects
			/// @{

			/// Returns a @c TextHit&lt;Position&gt; by using @c TextHit#afterOffset method.
			template<typename Position>
			inline TextHit<typename std::remove_cv<Position>::type> makeTextHitAfterOffset(const Position& offset) BOOST_NOEXCEPT {
				return TextHit<typename std::remove_cv<Position>::type>::afterOffset(offset);
			}

			/// Returns a @c TextHit&lt;Position&gt; by using @c TextHit#beforeOffset method.
			template<typename Position>
			inline TextHit<typename std::remove_cv<Position>::type> makeTextHitBeforeOffset(const Position& offset) BOOST_NOEXCEPT {
				return TextHit<typename std::remove_cv<Position>::type>::beforeOffset(offset);
			}

			/// Returns a @c TextHit&lt;Position&gt; by using @c TextHit#leading method.
			template<typename Position>
			inline TextHit<typename std::remove_cv<Position>::type> makeLeadingTextHit(const Position& characterIndex) BOOST_NOEXCEPT {
				return TextHit<typename std::remove_cv<Position>::type>::leading(characterIndex);
			}

			/// Returns a @c TextHit&lt;Position&gt; by using @c TextHit#trailing method.
			template<typename Position>
			inline TextHit<typename std::remove_cv<Position>::type> makeTrailingTextHit(const Position& characterIndex) BOOST_NOEXCEPT {
				return TextHit<typename std::remove_cv<Position>::type>::trailing(characterIndex);
			}

			/**
			 * Transforms the given @c TextHit object with the specified functor.
			 * @tparam Position The template parameter type of @a source
			 * @tparam Transformer The type of @a transformer
			 * @param source The object to transform
			 * @param transformer The functor takes a @c Position parameter and returns the new character index
			 * @return The transformed @c TextHit object
			 */
			template<typename Position, typename Transformer>
			inline auto transformTextHit(const TextHit<Position>& source, Transformer transformer) -> TextHit<decltype(transformer(source.characterIndex()))> {
				typedef TextHit<decltype(transformer(source.characterIndex()))> Transformed;
				return source.isLeadingEdge() ? Transformed::leading(transformer(source.characterIndex())) : Transformed::trailing(transformer(source.characterIndex()));
			}

			/**
			 * Transforms the given @c TextHit&lt;From&gt; object to @c TextHit&lt;To&gt; by using @c static_cast.
			 * @tparam To The template parameter type of the return value
			 * @tparam From The template parameter type of @a source
			 * @param source The object to transform
			 * @return  The transformed @c TextHit object
			 */
			template<typename To, typename From>
			inline TextHit<To> castTextHit(const TextHit<From>& source) {
				return transformTextHit(source, [](const From& from) {
					return static_cast<To>(from);
				});
			}
			/// @}
		}
	}
}

#endif // !ASCENSION_GRAPHICS_FONT_TEXT_HIT_HPP
