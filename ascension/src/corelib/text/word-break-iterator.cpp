/**
 * @file word-break-iterator.cpp
 * Implements @c WordBreakIterator class.
 * @author exeal
 * @date 2006-2007 was iterator.cpp
 * @date 2007-2014
 * @date-2016-07-24 Separated from break-iterator.hpp.
 */

#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/word-break-iterator.hpp>
#include <ascension/corelib/text/detail/break-iterator-scan-base.hpp>
#include <boost/core/ignore_unused.hpp>

#if ASCENSION_UAX29_REVISION_NUMBER > 11
#	error "These codes are based on old version of UAX #29"
#endif

namespace ascension {
	namespace text {
		namespace {
			/**
			 * Returns true if the scripts of the two code points are same. This method assumes that the two code
			 * points are alphabetical and treats all ASCII characters as Latin scripts.
			 * @param preceding The code point of the character
			 * @param following The code point of the character
			 * @param lc The locale to detect script of a character
			 * @return true if @a preceding and @a following have same script
			 */
			bool compareScripts(CodePoint preceding, CodePoint following, const std::locale& lc) {
				using ucd::Script;
				const int s1 = Script::of(preceding), s2 = Script::of(following);
				if(s1 == s2 || s1 == Script::COMMON || s2 == Script::COMMON || s1 == Script::INHERITED || s2 == Script::INHERITED)
					return true;
#if 0
				// 日本語の送り仮名?
				// 左側が漢字か片仮名、右側が平仮名であれば単語境界ではない
				if(locales.isJapanese(lc) && s2 == Script::HIRAGANA && (s1 == Script::KATAKANA || s1 == Script::HAN))
					return true;
#else
				boost::ignore_unused(lc);
#endif
				// <平仮名> + 'ー'
				if(s1 == Script::HIRAGANA && following == 0x30fc)
					return true;
				return false;
			}
		}

		/// @internal Implements @c WordBreakIterator#next.
		void WordBreakIteratorBase::next(std::size_t n) {
			assert(n > 0);
#define ASCENSION_TRY_RETURN() {if(--n == 0) return;}

			using ucd::BinaryProperty;
			using ucd::GraphemeClusterBreak;
			using ucd::WordBreak;

			// A B | C D -> iteration-direction
			// ^ ^ ^ ^ ^
			// | | | | next-next
			// | | | next (i)
			// | | current-boundary-candidate
			// | prev
			// prev-prev
			if(!characterIterator_.hasNext())	// (WB2)
				return;
			nextBase(characterIterator_);
			if(!characterIterator_.hasNext())	// (WB2)
				return;
			boost::optional<detail::CharacterIterator> prevPrev, prev, nextNext;
			CodePoint nextCP = *characterIterator_, prevCP = INVALID_CODE_POINT;
			int nextClass = WordBreak::of(nextCP, syntax_, locale()),
				prevClass = ucd::NOT_PROPERTY, nextNextClass = ucd::NOT_PROPERTY, prevPrevClass = ucd::NOT_PROPERTY;
			while(true) {
				// 1 つ前 (B) を調べる
				assert(characterIterator_.hasPrevious());
				if(prev == boost::none) {
					prev = characterIterator_;
					previousBase(boost::get(prev));
				}
				if(prevCP == INVALID_CODE_POINT)
					prevCP = *boost::get(prev);
				if(prevClass == ucd::NOT_PROPERTY)
					prevClass = WordBreak::of(prevCP, syntax_, locale());
				if(prevClass == GraphemeClusterBreak::CR && nextClass == GraphemeClusterBreak::LF)	// (WB3)
					/* do nothing */;
				else if(nextClass == WordBreak::A_LETTER && prevClass == WordBreak::A_LETTER) {	// (WB5+, !WB13)
					if(!compareScripts(prevCP, nextCP, locale()))
						ASCENSION_TRY_RETURN()
				} else if((nextClass == WordBreak::A_LETTER || nextClass == WordBreak::NUMERIC || nextClass == WordBreak::EXTEND_NUM_LET)
						&& (prevClass == WordBreak::A_LETTER || prevClass == WordBreak::NUMERIC || prevClass == WordBreak::EXTEND_NUM_LET))	// (WB8, WB9, WB10, WB13a+, WB13b+)
					/* do nothing */;
				else if((prevClass == WordBreak::A_LETTER && nextClass == WordBreak::MID_LETTER)
						|| (prevClass == WordBreak::NUMERIC && nextClass == WordBreak::MID_NUM)) {	// (WB6, WB12)?
					// 2 つ次 (D) を調べる
					nextNext = characterIterator_;
					nextBase(boost::get(nextNext));
					nextNextClass = WordBreak::of(*boost::get(nextNext), syntax_, locale());
					if(!nextNext->hasNext())	// (WB14)
						ASCENSION_TRY_RETURN()
					if(nextNextClass != prevClass	// (WB6, WB12)
							&& ((component_ & ALPHA_NUMERIC) == 0
							|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))
						ASCENSION_TRY_RETURN()
				} else if((prevClass == WordBreak::MID_LETTER && nextClass == WordBreak::A_LETTER)
						|| (prevClass == WordBreak::MID_NUM && nextClass == WordBreak::NUMERIC)) {	// (WB7, WB11)?
					// 2 つ前 (A) を調べる
					if(!prev->hasPrevious()) {	// (WB14)
						ASCENSION_TRY_RETURN()
						break;
					}
					if(prevPrevClass == ucd::NOT_PROPERTY) {
						if(prevPrev == boost::none) {
							prevPrev = boost::get(prev);
							previousBase(boost::get(prevPrev));
						}
						prevPrevClass = WordBreak::of(*boost::get(prevPrev), syntax_, locale());
					}
					if(prevPrevClass != nextClass
							&& ((component_ & ALPHA_NUMERIC) == 0
							|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))	// (WB7, WB11)
						ASCENSION_TRY_RETURN()
				} else if(((component_ & END_OF_SEGMENT) == 0 && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(nextCP))
						|| ((component_ & START_OF_SEGMENT) == 0 && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(prevCP)))	// (+)
					/* do nothing */;
				else if((component_ & ALPHA_NUMERIC) != 0	// (0)
						&& ((component_ & START_OF_SEGMENT) == 0 || !syntax_.isIdentifierContinueCharacter(nextCP))
						&& ((component_ & END_OF_SEGMENT) == 0 || !syntax_.isIdentifierContinueCharacter(prevCP)))	// (+)
					/* do nothing */;
				else
					ASCENSION_TRY_RETURN()

				// 次に進む
				prevPrev = move(prev);
				prev = characterIterator_;
				nextBase(characterIterator_);
				nextNext = boost::none;
				if(!characterIterator_.hasNext())	// (WB2)
					return;
				prevCP = nextCP;
				nextCP = *characterIterator_;
				prevPrevClass = prevClass;
				prevClass = nextClass;
				if(nextNextClass != ucd::NOT_PROPERTY) {
					nextClass = nextNextClass;
					nextNextClass = ucd::NOT_PROPERTY;
				} else
					nextClass = WordBreak::of(nextCP, syntax_, locale());
			}
#undef ASCENSION_TRY_RETURN
		}

		/// @internal Implements @c WordBreakIterator#previous.
		void WordBreakIteratorBase::previous(std::size_t n) {
			assert(n > 0);
#define ASCENSION_TRY_RETURN() {if(--n == 0) return;}

			using ucd::BinaryProperty;
			using ucd::GraphemeClusterBreak;
			using ucd::WordBreak;

			// iteration-direction <- A B | C D
			//                        ^ ^ ^ ^ ^
			//                next-next | | | |
			//                       next | | |
			//   current-boundary-candidate | |
			//                       prev (i) |
			//                        prev-prev
			if(!characterIterator_.hasPrevious())	// (WB1)
				return;
			previousBase(characterIterator_);
			if(!characterIterator_.hasPrevious())	// (WB1)
				return;
			boost::optional<detail::CharacterIterator> next, nextNext, prevPrev;
			CodePoint prevCP = *characterIterator_, nextCP = INVALID_CODE_POINT, nextNextCP = INVALID_CODE_POINT;
			int prevClass = WordBreak::of(prevCP, syntax_, locale()),
				nextClass = ucd::NOT_PROPERTY, nextNextClass = ucd::NOT_PROPERTY, prevPrevClass = ucd::NOT_PROPERTY;
			while(true) {
				// 1 つ次 (B) を調べる
				assert(characterIterator_.hasPrevious());
				if(next == boost::none) {
					next = characterIterator_;
					previousBase(boost::get(next));
				}
				if(nextCP == INVALID_CODE_POINT)
					nextCP = *boost::get(next);
				if(nextClass == ucd::NOT_PROPERTY)
					nextClass = WordBreak::of(nextCP, syntax_, locale());
				if(prevClass == GraphemeClusterBreak::LF && nextClass == GraphemeClusterBreak::CR)	// (WB3)
					/* do nothing */;
				else if(prevClass == WordBreak::A_LETTER && nextClass == WordBreak::A_LETTER) {	// (WB5+, !WB13)
					if(!compareScripts(nextCP, prevCP, locale()))
						ASCENSION_TRY_RETURN()
				} else if((prevClass == WordBreak::A_LETTER || prevClass == WordBreak::NUMERIC || prevClass == WordBreak::EXTEND_NUM_LET)
						&& (nextClass == WordBreak::A_LETTER || nextClass == WordBreak::NUMERIC || nextClass == WordBreak::EXTEND_NUM_LET))	// (WB8, WB9, WB10, WB13a+, WB13b+)
					/* do nothing */;
				else if((nextClass == WordBreak::A_LETTER && prevClass == WordBreak::MID_LETTER)
						|| (nextClass == WordBreak::NUMERIC && prevClass == WordBreak::MID_NUM)) {	// (WB6, WB12)?
					// 2 つ前 (D) を調べる
					if(prevPrevClass == ucd::NOT_PROPERTY) {
						if(prevPrev == boost::none) {
							prevPrev = characterIterator_;
							nextBase(boost::get(prevPrev));
						}
						if(!prevPrev->hasNext()) {	// (WB14)
							ASCENSION_TRY_RETURN()
							break;
						}
						prevPrevClass = WordBreak::of(*boost::get(prevPrev), syntax_, locale());
					}
					if(prevPrevClass != nextClass
							&& ((component_ & ALPHA_NUMERIC) == 0
							|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))	// (WB6, WB12)
						ASCENSION_TRY_RETURN()
				} else if((nextClass == WordBreak::MID_LETTER && prevClass == WordBreak::A_LETTER)
						|| (nextClass == WordBreak::MID_NUM && prevClass == WordBreak::NUMERIC)) {	// (WB7, WB11)?
					// 2 つ次 (A) を調べる
					if(!next->hasPrevious()) {	// (WB14)
						ASCENSION_TRY_RETURN()
						break;
					}
					nextNext = next;
					previousBase(boost::get(nextNext));
					nextNextClass = WordBreak::of(nextNextCP = *boost::get(nextNext), syntax_, locale());
					if(nextNextClass != prevClass
							&& ((component_ & ALPHA_NUMERIC) == 0
							|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))	// (WB7, WB11)
						ASCENSION_TRY_RETURN()
				} else if(((component_ & END_OF_SEGMENT) == 0 && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(prevCP))
						|| ((component_ & START_OF_SEGMENT) == 0 && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(nextCP)))	// (+)
					/* do nothing */;
				else if((component_ & ALPHA_NUMERIC) != 0	// (0)
						&& ((component_ & START_OF_SEGMENT) == 0 || !syntax_.isIdentifierContinueCharacter(prevCP))
						&& ((component_ & END_OF_SEGMENT) == 0 || !syntax_.isIdentifierContinueCharacter(nextCP)))	// (+)
					/* do nothing */;
				else
					ASCENSION_TRY_RETURN()

				// 次に進む
				prevPrev = characterIterator_;
				previousBase(characterIterator_);
				if(!characterIterator_.hasPrevious())	// (WB1)
					ASCENSION_TRY_RETURN()
				next = std::move(nextNext);
				nextNext = boost::none;	// ...is need?
				prevCP = *characterIterator_;
				nextCP = nextNextCP;
				nextNextCP = INVALID_CODE_POINT;
				prevPrevClass = prevClass;
				prevClass = nextClass;
				nextClass = nextNextClass;
				nextNextClass = ucd::NOT_PROPERTY;
			}
		}

		/// @internal Implements @c WordBreakIterator#isBoundary
		bool WordBreakIteratorBase::isBoundary(const detail::CharacterIterator& at) const {
			using ucd::GraphemeClusterBreak;
			using ucd::WordBreak;

			if(!at.hasNext() || !at.hasPrevious())	// (WB1, WB2)
				return true;

			const CodePoint nextCP = *at;
			const int nextClass = WordBreak::of(nextCP, syntax_, locale());
			if(nextClass == WordBreak::OTHER)	// (WB14)
				return true;
			detail::CharacterIterator i(at);
			previousBase(i);
			CodePoint prevCP = *i;
			int prevClass = WordBreak::of(prevCP, syntax_, locale());

			if(prevClass == GraphemeClusterBreak::CR && nextClass == GraphemeClusterBreak::LF)	// (WB3)
				return false;
			else if(nextClass == WordBreak::A_LETTER && prevClass == WordBreak::A_LETTER)	// (WB5+, !WB13)
				return !compareScripts(prevCP, nextCP, locale());
			else if((nextClass == WordBreak::A_LETTER || nextClass == WordBreak::NUMERIC || nextClass == WordBreak::EXTEND_NUM_LET)
					&& (prevClass == WordBreak::A_LETTER || prevClass == WordBreak::NUMERIC || prevClass == WordBreak::EXTEND_NUM_LET))	// (WB8, WB9, WB10, WB13a+, WB13b+)
				return false;
			else if((prevClass == WordBreak::A_LETTER && nextClass == WordBreak::MID_LETTER)
					|| (prevClass == WordBreak::NUMERIC && nextClass == WordBreak::MID_NUM)) {	// (WB6, WB12)?
				// 2 つ次を調べる
				int nextNextClass;
				i = at;
				nextBase(i);
				while(true) {
					if(!i.hasNext())	// (WB14)
						return true;
					else if(WordBreak::FORMAT != (nextNextClass = WordBreak::of(*i, syntax_, locale())))	// (WB4)
						break;
					nextBase(i);
				}
				return nextNextClass != prevClass;	// (WB6, WB12)
			} else if(i.hasPrevious()
					&& ((prevClass == WordBreak::MID_LETTER && nextClass == WordBreak::A_LETTER)
					|| (prevClass == WordBreak::MID_NUM && nextClass == WordBreak::NUMERIC))) {	// (WB7, WB11)?
				// 2 つ前を調べる
				int prevPrevClass;
				while(true) {
					previousBase(i);
					if(!i.hasPrevious())	// (WB14)
						return true;
					else if(WordBreak::FORMAT != (prevPrevClass = WordBreak::of(*i, syntax_, locale())))	// (WB4)
						break;
				}
				return prevPrevClass != nextClass;	// (WB7, WB11)
			}
			return true;	// (WB14)
		}

		/**
		 * Sets the word component to search.
		 * @param component The new component to set
		 * @throw UnknownValueException @a component is invalid
		 */
		void WordBreakIteratorBase::setComponent(Component component) {
			if((component & ~BOUNDARY_OF_ALPHANUMERICS) != 0)
				throw UnknownValueException("component");
			component_ = component;
		}
	}
}
