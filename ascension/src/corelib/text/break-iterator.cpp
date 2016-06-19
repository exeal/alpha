/**
 * @file break-iterator.cpp
 * @date 2006-2007 was iterator.cpp
 * @date 2007-2014
 * @author exeal
 */

#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <boost/core/ignore_unused.hpp>
#include <memory>	// std.unique_ptr

#if ASCENSION_UAX29_REVISION_NUMBER > 11
#	error "These codes are based on old version of UAX #29"
#endif


namespace ascension {
	namespace text {
		// GraphemeBreakIteratorBase //////////////////////////////////////////////////////////////////////////////////

		void GraphemeBreakIteratorBase::next(std::size_t n) {
			using ucd::GraphemeClusterBreak;
			if(!characterIterator_.hasNext())	// (GB2)
				return;
			CodePoint prevCP, cp = *characterIterator_;
			int prev, current = GraphemeClusterBreak::of(cp);
			while(n > 0 && (++characterIterator_).hasNext()) {	// (GB2)
				prevCP = cp;
				prev = current;
				current = GraphemeClusterBreak::of(cp = *characterIterator_);
				if(prev == GraphemeClusterBreak::CR) {	// (GB3, GB4)
					if(current != GraphemeClusterBreak::LF)
						--n;
				} else if(prev == GraphemeClusterBreak::CONTROL || prev == GraphemeClusterBreak::LF	// (GB4)
						|| current == GraphemeClusterBreak::CONTROL || current == GraphemeClusterBreak::CR || current == GraphemeClusterBreak::LF)	// (GB5)
					--n;
				else if(prev == GraphemeClusterBreak::L) {
					if(current != GraphemeClusterBreak::L && current != GraphemeClusterBreak::V && current != GraphemeClusterBreak::LV
							&& current != GraphemeClusterBreak::LVT && current != GraphemeClusterBreak::EXTEND)	// (GB6, GB9)
						--n;
				} else if(prev == GraphemeClusterBreak::LV || prev == GraphemeClusterBreak::V) {
					if(current != GraphemeClusterBreak::V && current != GraphemeClusterBreak::T && current != GraphemeClusterBreak::EXTEND)	// (GB7, GB9)
						--n;
				} else if(prev == GraphemeClusterBreak::LVT || prev == GraphemeClusterBreak::T) {
					if(current != GraphemeClusterBreak::T && current != GraphemeClusterBreak::EXTEND)	// (GB8, GB9)
						--n;
				} else if(current != GraphemeClusterBreak::EXTEND)	// (GB9)
					--n;
			}
		}

		void GraphemeBreakIteratorBase::previous(std::size_t n) {
			using ucd::GraphemeClusterBreak;
			if(!characterIterator_.hasPrevious() || !(--characterIterator_).hasPrevious())	// (GB1)
				return;
			CodePoint prevCP, cp = *characterIterator_;
			int prev, current = GraphemeClusterBreak::of(cp);
			do {
				prevCP = cp;
				prev = current;
				current = GraphemeClusterBreak::of(cp = *--characterIterator_);
				if(prev == GraphemeClusterBreak::LF) {	// (GB3, GB5)
					if(current != GraphemeClusterBreak::CR)
						--n;
				} else if(current == GraphemeClusterBreak::CONTROL || current == GraphemeClusterBreak::CR || current == GraphemeClusterBreak::LF	// (GB4)
						|| prev == GraphemeClusterBreak::CONTROL || prev == GraphemeClusterBreak::CR)	// (GB5)
					--n;
				else if(current == GraphemeClusterBreak::L) {
					if(prev != GraphemeClusterBreak::L && prev != GraphemeClusterBreak::V && prev != GraphemeClusterBreak::LV
							&& prev != GraphemeClusterBreak::LVT && prev != GraphemeClusterBreak::EXTEND)	// (GB6, GB9)
						--n;
				} else if(current == GraphemeClusterBreak::LV || current == GraphemeClusterBreak::V) {
					if(prev != GraphemeClusterBreak::V && prev != GraphemeClusterBreak::T && prev != GraphemeClusterBreak::EXTEND)	// (GB7, GB9)
						--n;
				} else if(current == GraphemeClusterBreak::LVT || current == GraphemeClusterBreak::T) {
					if(prev != GraphemeClusterBreak::T && prev != GraphemeClusterBreak::EXTEND)	// (GB8, GB9)
						--n;
				} else if(prev != GraphemeClusterBreak::EXTEND)	// (GB9)
					--n;
				if(n == 0) {
					++characterIterator_;
					return;
				}
			} while(characterIterator_.hasPrevious());	// (GB1)
		}

		/// @internal Implements @c GraphemeBreakIterator#isBoundary
		bool GraphemeBreakIteratorBase::isBoundary(const detail::CharacterIterator& at) const {
			using ucd::GraphemeClusterBreak;
			if(!at.hasNext() || !at.hasPrevious())	// (GB1, GB2)
				return true;
			const int p = GraphemeClusterBreak::of(*at);
			if(p == GraphemeClusterBreak::CR || p == GraphemeClusterBreak::CONTROL)	// (GB5)
				return true;
			detail::CharacterIterator i(at);
			--i;
			const int prev = GraphemeClusterBreak::of(*i);
			if(prev == GraphemeClusterBreak::CR)
				return p != GraphemeClusterBreak::LF;	// (GB3, GB4)
			else if(prev == GraphemeClusterBreak::LF || prev == GraphemeClusterBreak::CONTROL || p == GraphemeClusterBreak::LF)	// (GB4, GB5)
				return true;
			else if(prev == GraphemeClusterBreak::L)	// (GB6)
				return p != GraphemeClusterBreak::L && p != GraphemeClusterBreak::V && p != GraphemeClusterBreak::LV && p != GraphemeClusterBreak::LVT;
			else if(prev == GraphemeClusterBreak::LV || prev == GraphemeClusterBreak::V)	// (GB7)
				return p != GraphemeClusterBreak::V && p != GraphemeClusterBreak::T;
			else if(prev == GraphemeClusterBreak::LVT || prev == GraphemeClusterBreak::T)	// (GB8)
				return p != GraphemeClusterBreak::T;
			return p != GraphemeClusterBreak::EXTEND;	// (GB9, 10)
		}


		// WordBreakIteratorBase //////////////////////////////////////////////////////////////////////////////////////

		namespace {
			/// @internal Advances @a i to the next character neither Extend nor Format.
			int nextBase(detail::CharacterIterator& i) {
				if(!i.hasNext())
					return ucd::GeneralCategory::LAST_VALUE;
				CodePoint cp = *i;
				if(cp == LINE_FEED || cp == CARRIAGE_RETURN || cp == NEXT_LINE || cp == LINE_SEPARATOR || cp == PARAGRAPH_SEPARATOR) {	// !Sep
					++i;
					return ucd::GeneralCategory::LAST_VALUE;
				}
				int gc = ucd::GeneralCategory::LAST_VALUE;
				while((++i).hasNext()) {
					gc = ucd::GeneralCategory::of(cp = *i);
					if(gc != ucd::GeneralCategory::FORMAT && !ucd::BinaryProperty::is<ucd::BinaryProperty::GRAPHEME_EXTEND>(cp))
						break;
				}
				return gc;
			}

			/// @internal Advances @a i to the previous character neither Extend nor Format.
			int previousBase(detail::CharacterIterator& i) {
				if(!i.hasPrevious())
					return ucd::GeneralCategory::of(*i);
				int gc = ucd::GeneralCategory::LAST_VALUE;
				CodePoint cp;
				do {
					cp = *--i;
					if(gc != ucd::GeneralCategory::LAST_VALUE
							&& (cp == LINE_FEED || cp == CARRIAGE_RETURN || cp == NEXT_LINE || cp == LINE_SEPARATOR || cp == PARAGRAPH_SEPARATOR)) {	// !Sep
						++i;
						break;
					}
							gc = ucd::GeneralCategory::of(cp);
					if(gc != ucd::GeneralCategory::FORMAT && !ucd::BinaryProperty::is<ucd::BinaryProperty::GRAPHEME_EXTEND>(cp))
						break;
				} while(i.hasPrevious());
				return gc;
			}

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
				else if(locales.isJapanese(lc) && s2 == Script::HIRAGANA && (s1 == Script::KATAKANA || s1 == Script::HAN))
					return true;
#else
				boost::ignore_unused(lc);
#endif
				// <平仮名> + 'ー'
				else if(s1 == Script::HIRAGANA && following == 0x30fc)
					return true;
				return false;
			}
		}

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


		// SentenceBreakIteratorBase //////////////////////////////////////////////////////////////////////////////////

		namespace {
			/// Tries SB8 rule.
			bool trySB8(detail::CharacterIterator& i) {
				detail::CharacterIterator j(i);
				for(; j.hasNext(); nextBase(j)) {
					switch(ucd::SentenceBreak::of(*j)) {
						case ucd::SentenceBreak::O_LETTER:
						case ucd::SentenceBreak::UPPER:
						case ucd::SentenceBreak::SEP:
							break;
						case ucd::SentenceBreak::LOWER:
							while(i.offset() < j.offset())
								nextBase(i);
							return false;	// (SB8)
						default:
							previousBase(i);
							return true;	// (SB12)
					}
				}
				previousBase(i);
				return true;	// (SB12)
			}

			/// Handles after (STerm|ATerm).
			bool tryToExtendTerm(detail::CharacterIterator& i, bool aTerm) {
				using ucd::SentenceBreak;
				assert(i.hasPrevious());
				bool closeOccured = false;	// true if (STerm|ATerm) Close+
				bool spOccured = false;		// true if (STerm|ATerm) Sp+ or (STerm|ATerm) Close+ Sp+
				while(i.hasNext()) {
					switch(SentenceBreak::of(*i)) {
					case SentenceBreak::SEP:
						nextBase(i);
						return true;	// (SB4)
					case SentenceBreak::FORMAT:
						assert(false);
					case SentenceBreak::SP:
						spOccured = true;	// (SB9)
						break;
					case SentenceBreak::LOWER:
						return !aTerm;	// (SB8, SB11)
					case SentenceBreak::UPPER:	// (SB7, SB12)?
						if(!aTerm || (!closeOccured && !spOccured))
							return false;	// (SB12)
						else {
							detail::CharacterIterator temp(i);
							previousBase(temp);
							if(!temp.hasPrevious())
								return true;	// (SB12)
							previousBase(temp);
							return SentenceBreak::of(*temp) != SentenceBreak::UPPER;
						}
					case SentenceBreak::O_LETTER:
						return true;	// (SB12)
					case SentenceBreak::NUMERIC:
						if(aTerm && !closeOccured && !spOccured)
							return false;	// (SB6)
						nextBase(i);
						return trySB8(i);	// (SB8?)
					case SentenceBreak::A_TERM:
					case SentenceBreak::S_TERM:
						return false;	// (SB8a)
					case SentenceBreak::CLOSE:	// (SB8, SB12)?
						if(!spOccured)
							closeOccured = true;	// (SB9)
						else if(aTerm) {
							nextBase(i);
							return trySB8(i);	// (SB8?)
						} else
							return true;	// (SB12)
					case SentenceBreak::OTHER:
						return true;	// (SB12)
					}
					nextBase(i);	// (SB5)
				}
				return true;	// (SB2)
			}
		}	// namespace @0

		void SentenceBreakIteratorBase::next(std::size_t n) {
			// TODO: not implemented.
			while(characterIterator_.hasNext()) {
				if(*characterIterator_ == CARRIAGE_RETURN) {
					++characterIterator_;
					if(!characterIterator_.hasNext())
						return;	// (SB2)
					if(*characterIterator_ == LINE_FEED)
						++characterIterator_;	// (SB3)
					return;	// (SB4)
				}

				using ucd::SentenceBreak;
				switch(SentenceBreak::of(*characterIterator_)) {
					case SentenceBreak::SEP:
						++characterIterator_;
						return;	// (SB4)
					case SentenceBreak::A_TERM:
						nextBase(characterIterator_);
						if(tryToExtendTerm(characterIterator_, true))
							return;	// (SB11)
						break;
					case SentenceBreak::S_TERM:
						nextBase(characterIterator_);
						if(tryToExtendTerm(characterIterator_, false))
							return;	// (SB11)
						break;
					default:
						break;	// (SB5, SB12)
				}
			}
			// (SB2)
		}

		void SentenceBreakIteratorBase::previous(std::size_t n) {
			// TODO: not implemented.
		}

		/// @internal Implements @c SentenceBreakIterator#isBoundary
		bool SentenceBreakIteratorBase::isBoundary(const detail::CharacterIterator& at) const {
			if(!at.hasNext() || !at.hasPrevious())
				return true;	// (SB1, SB2)
			detail::CharacterIterator i(at);
			if(*at == LINE_FEED) {
				if(*--i == CARRIAGE_RETURN)
					return false;	// (SB3)
				else if(!i.hasPrevious())
					return true;	// (SB12)
				const int p = ucd::SentenceBreak::of(*i);
				if(p == ucd::GraphemeClusterBreak::EXTEND || p == ucd::SentenceBreak::FORMAT)
					previousBase(i);	// (SB5)
			} else
				previousBase(i);	// (SB5)
			do {
				switch(ucd::SentenceBreak::of(*i)) {
					case ucd::SentenceBreak::SEP:
						return at.offset() - i.offset() == 1;	// (SB4)
					case ucd::SentenceBreak::A_TERM:
						nextBase(i);
						return tryToExtendTerm(i, true) && i.offset() == at.offset();
					case ucd::SentenceBreak::S_TERM:
						nextBase(i);
						return tryToExtendTerm(i, false) && i.offset() == at.offset();
					default:
						break;
				}
				previousBase(i);
			} while(i.hasPrevious());
			return false;	// (SB1)
		}

		/**
		 * Sets the word component to search.
		 * @param component The new component to set
		 * @throw UnknownValueException @a component is invalid
		 */
		void SentenceBreakIteratorBase::setComponent(Component component) {
			if((component & ~BOUNDARY_OF_SEGMENT) != 0)
				throw UnknownValueException("component");
			component_ = component;
		}


		// LineBreakIteratorBase //////////////////////////////////////////////////////////////////////////////////////

		/// @see BreakIterator#isBoundary
		bool LineBreakIteratorBase::isBoundary(const detail::CharacterIterator& at) const {
			// TODO: not implemented.
			return true;
		}
	}
}
