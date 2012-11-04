/**
 * @file text-hit-information.hpp
 * Defines @c ascension#graphics#font#TextHitInformation class.
 * @author exeal
 * @date 2012-11-04 created
 */

#ifndef ASCENSION_TEXT_HIT_INFORMATION_HPP
#define ASCENSION_TEXT_HIT_INFORMATION_HPP
#include <ascension/corelib/basic-types.hpp>	// Index, SignedIndex, BOOST_NOEXCEPT
#include <numeric>		// std.numeric_limits
#include <stdexcept>	// std.overflow_error, std.underflow_error

namespace ascension {
	namespace graphics {
		namespace font {
			class TextHitInformation {
			public:
				// factories
				/**
				 * Creates a @c TextHitInformation at the specified offset, associated with the
				 * character after the offset.
				 * @param offset An offset associated with the character after the offset
				 * @return A @c TextHitInformation at the specified offset
				 * @see #beforeOffset
				 */
				static TextHitInformation afterOffset(Index offset) BOOST_NOEXCEPT {
					return TextHitInformation(offset, true);
				}
				/**
				 * Creates a @c TextHitInformation at the specified offset, associated with the
				 * character before the offset.
				 * @param offset An offset associated with the character before the offset
				 * @return A @c TextHitInformation at the specified offset
				 * @see #afterOffset
				 */
				static TextHitInformation beforeOffset(Index offset) BOOST_NOEXCEPT {
					return TextHitInformation(offset - 1, false);
				}
				/**
				 * Creates a @c TextHitInformation on the leading edge of the character at the
				 * specified @a characterIndex.
				 * @param characterIndex The index of the character hit
				 * @return A @c TextHitInformation on the leading edge of the character at the
				 *         specified @a characterIndex
				 */
				static TextHitInformation leading(Index characterIndex) BOOST_NOEXCEPT {
					return TextHitInformation(characterIndex, true);
				}
				/**
				 * Creates a @c TextHitInformation on the trailing edge of the character at the
				 * specified @a characterIndex.
				 * @param characterIndex The index of the character hit
				 * @return A @c TextHitInformation on the trailing edge of the character at the
				 *         specified @a characterIndex
				 */
				static TextHitInformation trailing(Index characterIndex) BOOST_NOEXCEPT {
					return TextHitInformation(characterIndex, false);
				}
				// 
				/**
				 * Returns the index of the character hit.
				 * @see #insertionIndex
				 */
				Index characterIndex() const BOOST_NOEXCEPT {return characterIndex_;}
				/**
				 * Returns the insertion index. This is the character index if the leading edge of
				 * the character was hit, and one greater than the character index if the trailing
				 * edge was hit.
				 * @see #characterIndex
				 */
				Index insertionIndex() const BOOST_NOEXCEPT {
					return isLeadingEdge() ? characterIndex() : characterIndex() + 1;
				}
				/// Returns @c true if the leading edge of the character was hit.
				bool isLeadingEdge() const BOOST_NOEXCEPT {return isLeadingEdge_;}
				//
				/**
				 * Creates a @c TextHitInformation whose character index is offset by @a delta from
				 * the @c #characterIndex of this @c TextHitInformation. This @c TextHitInformation
				 * remains unchanged.
				 * @param delta The value to offset this @c #characterIndex
				 * @return A @c TextHitInformation whose @c #characterIndex is offset by @a delta
				 *         from the @c #characterIndex of this @c TextHitInformation
				 * @throw std#overflow_error
				 * @throw std#underflow_error
				 * @see #otherHit
				 */
				TextHitInformation offsetHit(SignedIndex delta) const {
					if(delta > 0 && static_cast<Index>(delta) > std::numeric_limits<Index>::max() - characterIndex())
						throw std::overflow_error("delta");
					else if(delta < 0 && static_cast<Index>(-delta) > characterIndex())
						throw std::underflow_error("delta");
					return TextHitInformation(characterIndex() + delta, isLeadingEdge());
				}
				/**
				 * Creates a @c TextHitInformation on the other side of the insertion point. This
				 * @c TextHitInformation remains unchanged.
				 * @return A @c TextHitInformation on the other side of the insertion point
				 * @see #offsetHit
				 */
				TextHitInformation otherHit() const BOOST_NOEXCEPT {
					return isLeadingEdge() ?
						trailing(characterIndex() - 1) : leading(characterIndex() + 1);
				}
			private:
				TextHitInformation(Index characterIndex, bool isLeadingEdge) BOOST_NOEXCEPT
					: characterIndex_(characterIndex), isLeadingEdge_(isLeadingEdge) {}
				Index characterIndex_;
				bool isLeadingEdge_;
			};
		}
	}
}

#endif // !ASCENSION_TEXT_HIT_INFORMATION_HPP
