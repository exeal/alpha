/**
 * @file break-iterator.cpp
 * @date 2006-2007 (was iterator.cpp)
 * @date 2007
 * @author exeal
 */

#include "stdafx.h"
#include "break-iterator.hpp"
#include <memory>	// std::auto_ptr
using namespace ascension;
using namespace ascension::unicode;
using namespace ascension::unicode::internal;
using namespace std;

#if ASCENSION_UAX29_REVISION_NUMBER > 11
#error "These codes are based on old version of UAX #29"
#endif


// AbstractGraphemeBreakIterator ////////////////////////////////////////////

/**
 * Protected constructor.
 * @param lc the locale
 */
AbstractGraphemeBreakIterator::AbstractGraphemeBreakIterator(const locale& lc) throw() : BreakIterator(lc) {
}

void AbstractGraphemeBreakIterator::doNext(ptrdiff_t amount) {
	assert(amount > 0);
	CharacterIterator& i = getCharacterIterator();
	if(i.isLast())	// (GB2)
		return;
	CodePoint prevCP, cp = i.current();
	int prev, current = GraphemeClusterBreak::of(cp);
	while(amount > 0 && !i.next().isLast()) {	// (GB2)
		prevCP = cp;
		prev = current;
		current = GraphemeClusterBreak::of(cp = i.current());
		if(prev == GraphemeClusterBreak::CR) {	// (GB3, GB4)
			if(current != GraphemeClusterBreak::LF)
				--amount;
		} else if(prev == GraphemeClusterBreak::CONTROL || prev == GraphemeClusterBreak::LF	// (GB4)
				|| current == GraphemeClusterBreak::CONTROL || current == GraphemeClusterBreak::CR || current == GraphemeClusterBreak::LF)	// (GB5)
			--amount;
		else if(prev == GraphemeClusterBreak::L) {
			if(current != GraphemeClusterBreak::L && current != GraphemeClusterBreak::V && current != GraphemeClusterBreak::LV
					&& current != GraphemeClusterBreak::LVT && current != GraphemeClusterBreak::EXTEND)	// (GB6, GB9)
				--amount;
		} else if(prev == GraphemeClusterBreak::LV || prev == GraphemeClusterBreak::V) {
			if(current != GraphemeClusterBreak::V && current != GraphemeClusterBreak::T && current != GraphemeClusterBreak::EXTEND)	// (GB7, GB9)
				--amount;
		} else if(prev == GraphemeClusterBreak::LVT || prev == GraphemeClusterBreak::T) {
			if(current != GraphemeClusterBreak::T && current != GraphemeClusterBreak::EXTEND)	// (GB8, GB9)
				--amount;
		} else if(current != GraphemeClusterBreak::EXTEND)	// (GB9)
			--amount;
	}
}

void AbstractGraphemeBreakIterator::doPrevious(ptrdiff_t amount) {
	assert(amount > 0);
	CharacterIterator& i = getCharacterIterator();
	if(i.isFirst() || i.previous().isFirst())	// (GB1)
		return;
	CodePoint prevCP, cp = i.current();
	int prev, current = GraphemeClusterBreak::of(cp);
	do {
		prevCP = cp;
		prev = current;
		current = GraphemeClusterBreak::of(cp = i.previous().current());
		if(prev == GraphemeClusterBreak::LF) {	// (GB3, GB5)
			if(current != GraphemeClusterBreak::CR)
				--amount;
		} else if(current == GraphemeClusterBreak::CONTROL || current == GraphemeClusterBreak::CR || current == GraphemeClusterBreak::LF	// (GB4)
				|| prev == GraphemeClusterBreak::CONTROL || prev == GraphemeClusterBreak::CR)	// (GB5)
			--amount;
		else if(current == GraphemeClusterBreak::L) {
			if(prev != GraphemeClusterBreak::L && prev != GraphemeClusterBreak::V && prev != GraphemeClusterBreak::LV
					&& prev != GraphemeClusterBreak::LVT && prev != GraphemeClusterBreak::EXTEND)	// (GB6, GB9)
				--amount;
		} else if(current == GraphemeClusterBreak::LV || current == GraphemeClusterBreak::V) {
			if(prev != GraphemeClusterBreak::V && prev != GraphemeClusterBreak::T && prev != GraphemeClusterBreak::EXTEND)	// (GB7, GB9)
				--amount;
		} else if(current == GraphemeClusterBreak::LVT || current == GraphemeClusterBreak::T) {
			if(prev != GraphemeClusterBreak::T && prev != GraphemeClusterBreak::EXTEND)	// (GB8, GB9)
				--amount;
		} else if(prev != GraphemeClusterBreak::EXTEND)	// (GB9)
			--amount;
		if(amount == 0) {
			i.next();
			return;
		}
	} while(!i.isFirst());	// (GB1)
}

/// @see BreakIterator#isBoundary
bool AbstractGraphemeBreakIterator::isBoundary(const CharacterIterator& at) const {
	if(at.isFirst() || at.isLast())	// (GB1, GB2)
		return true;
	const int p = GraphemeClusterBreak::of(at.current());
	if(p == GraphemeClusterBreak::CR || p == GraphemeClusterBreak::CONTROL)	// (GB5)
		return true;
	auto_ptr<CharacterIterator> i(at.clone());
	i->previous();
	const int prev = GraphemeClusterBreak::of(i->current());
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

/// @see BreakIterator#next
void AbstractGraphemeBreakIterator::next(ptrdiff_t amount) {
	if(amount > 0)
		doNext(+amount);
	else if(amount < 0)
		doPrevious(-amount);
}


// AbstractWordBreakIterator ////////////////////////////////////////////////

namespace {
	/// Advances @a i to the next character neither Extend nor Format.
	int nextBase(CharacterIterator& i) {
		if(i.isLast())
			return GeneralCategory::COUNT;
		CodePoint cp = i.current();
		if(cp == LINE_FEED || cp == CARRIAGE_RETURN || cp == NEXT_LINE || cp == LINE_SEPARATOR || cp == PARAGRAPH_SEPARATOR) {	// !Sep
			i.next();
			return GeneralCategory::COUNT;
		}
		int gc = GeneralCategory::COUNT;
		while(!i.next().isLast()) {
			gc = GeneralCategory::of(cp = i.current());
			if(gc != GeneralCategory::OTHER_FORMAT && !BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
				break;
		}
		return gc;
	}

	/// Advances @a i to the previous character neither Extend nor Format.
	int previousBase(CharacterIterator& i) {
		if(i.isFirst())
			return GeneralCategory::of(i.current());
		int gc = GeneralCategory::COUNT;
		CodePoint cp;
		do {
			cp = i.previous().current();
			if(gc != GeneralCategory::COUNT
					&& (cp == LINE_FEED || cp == CARRIAGE_RETURN || cp == NEXT_LINE || cp == LINE_SEPARATOR || cp == PARAGRAPH_SEPARATOR)) {	// !Sep
				i.next();
				break;
			}
					gc = GeneralCategory::of(cp);
			if(gc != GeneralCategory::OTHER_FORMAT && !BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
				break;
		} while(!i.isFirst());
		return gc;
	}

	/**
	 * Returns true if the scripts of the two code points are same.
	 * This method assumes that the two code points are alphabetical
	 * and treats all ASCII characters as Latin scripts.
	 * @param preceding the code point of the character
	 * @param following the code point of the character
	 * @param lc the locale to detect script of a character
	 * @return true if @a preceding and @a following have same script
	 */
	bool compareScripts(CodePoint preceding, CodePoint following, const locale& lc) throw() {
		const int s1 = Script::of(preceding), s2 = Script::of(following);
		if(s1 == s2 || s1 == Script::COMMON || s2 == Script::COMMON || s1 == Script::INHERITED || s2 == Script::INHERITED)
			return true;
//		// 日本語の送り仮名?
//		// 左側が漢字か片仮名、右側が平仮名であれば単語境界ではない
//		else if(locales.isJapanese(lc) && s2 == Script::HIRAGANA && (s1 == Script::KATAKANA || s1 == Script::HAN))
//			return true;
		// <平仮名> + 'ー'
		else if(s1 == Script::HIRAGANA && following == 0x30FC)
			return true;
		return false;
	}
}

/**
 * Protected constructor.
 * @param component the word component to search
 * @param syntax the identifier syntax
 * @param lc the locale
 * @see #setComponent
 */
AbstractWordBreakIterator::AbstractWordBreakIterator(Component component,
		const IdentifierSyntax& syntax, const locale& lc) throw() : BreakIterator(lc), component_(component), syntax_(syntax) {
}

void AbstractWordBreakIterator::doNext(ptrdiff_t amount) {
	assert(amount > 0);
#define TRY_RETURN() {if(--amount == 0) return;}
	// A B | C D -> iteration-direction
	// ^ ^ ^ ^ ^
	// | | | | next-next
	// | | | next (i)
	// | | current-boundary-candidate
	// | prev
	// prev-prev
	CharacterIterator& i = getCharacterIterator();
	if(i.isLast())	// (WB2)
		return;
	nextBase(i);
	if(i.isLast())	// (WB2)
		return;
	auto_ptr<CharacterIterator> prevPrev, prev, nextNext;
	CodePoint nextCP = i.current(), prevCP = INVALID_CODE_POINT;
	int nextClass = WordBreak::of(nextCP, syntax_, getLocale()),
		prevClass = NOT_PROPERTY, nextNextClass = NOT_PROPERTY, prevPrevClass = NOT_PROPERTY;
	while(true) {
		// 1 つ前 (B) を調べる
		assert(!i.isFirst());
		if(prev.get() == 0) {prev = i.clone(); previousBase(*prev);}
		if(prevCP == INVALID_CODE_POINT) prevCP = prev->current();
		if(prevClass == NOT_PROPERTY) prevClass = WordBreak::of(prevCP, syntax_, getLocale());
		if(prevClass == GraphemeClusterBreak::CR && nextClass == GraphemeClusterBreak::LF)	// (WB3)
			/* do nothing */;
		else if(nextClass == WordBreak::A_LETTER && prevClass == WordBreak::A_LETTER) {	// (WB5+, !WB13)
			if(!compareScripts(prevCP, nextCP, getLocale()))
				TRY_RETURN()
		} else if((nextClass == WordBreak::A_LETTER || nextClass == WordBreak::NUMERIC || nextClass == WordBreak::EXTEND_NUM_LET)
				&& (prevClass == WordBreak::A_LETTER || prevClass == WordBreak::NUMERIC || prevClass == WordBreak::EXTEND_NUM_LET))	// (WB8, WB9, WB10, WB13a+, WB13b+)
			/* do nothing */;
		else if((prevClass == WordBreak::A_LETTER && nextClass == WordBreak::MID_LETTER)
				|| (prevClass == WordBreak::NUMERIC && nextClass == WordBreak::MID_NUM)) {	// (WB6, WB12)?
			// 2 つ次 (D) を調べる
			nextNext = i.clone();
			nextBase(*nextNext);
			nextNextClass = WordBreak::of(nextNext->current(), syntax_, getLocale());
			if(nextNext->isLast())	// (WB14)
				TRY_RETURN()
			if(nextNextClass != prevClass	// (WB6, WB12)
					&& (!toBoolean(component_ & ALPHA_NUMERIC)
					|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))
				TRY_RETURN()
		} else if((prevClass == WordBreak::MID_LETTER && nextClass == WordBreak::A_LETTER)
				|| (prevClass == WordBreak::MID_NUM && nextClass == WordBreak::NUMERIC)) {	// (WB7, WB11)?
			// 2 つ前 (A) を調べる
			if(prev->isFirst()) {	// (WB14)
				TRY_RETURN()
				break;
			}
			if(prevPrevClass == NOT_PROPERTY) {
				if(prevPrev.get() == 0) {
					prevPrev = prev->clone();
					previousBase(*prevPrev);
				}
				prevPrevClass = WordBreak::of(prevPrev->current(), syntax_, getLocale());
			}
			if(prevPrevClass != nextClass
					&& (!toBoolean(component_ & ALPHA_NUMERIC)
					|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))	// (WB7, WB11)
				TRY_RETURN()
		} else if((!toBoolean(component_ & END_OF_SEGMENT) && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(nextCP))
				|| (!toBoolean(component_ & START_OF_SEGMENT) && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(prevCP)))	// (+)
			/* do nothing */;
		else if(toBoolean(component_ & ALPHA_NUMERIC)	// (0)
				&& (!toBoolean(component_ & START_OF_SEGMENT) || !syntax_.isIdentifierContinueCharacter(nextCP))
				&& (!toBoolean(component_ & END_OF_SEGMENT) || !syntax_.isIdentifierContinueCharacter(prevCP)))	// (+)
			/* do nothing */;
		else
			TRY_RETURN()

		// 次に進む
		prevPrev = prev;
		prev = i.clone();
		nextBase(i);
		nextNext.reset(0);
		if(i.isLast())	// (WB2)
			return;
		prevCP = nextCP;
		nextCP = i.current();
		prevPrevClass = prevClass;
		prevClass = nextClass;
		if(nextNextClass != NOT_PROPERTY) {
			nextClass = nextNextClass;
			nextNextClass = NOT_PROPERTY;
		} else
			nextClass = WordBreak::of(nextCP, syntax_, getLocale());
	}
#undef TRY_RETURN
}

void AbstractWordBreakIterator::doPrevious(ptrdiff_t amount) {
	assert(amount > 0);
#define TRY_RETURN() {if(--amount == 0) return;}
	// iteration-direction <- A B | C D
	//                        ^ ^ ^ ^ ^
	//                next-next | | | |
	//                       next | | |
	//   current-boundary-candidate | |
	//                       prev (i) |
	//                        prev-prev
	CharacterIterator& i = getCharacterIterator();
	if(i.isFirst())	// (WB1)
		return;
	previousBase(i);
	if(i.isFirst())	// (WB1)
		return;
	auto_ptr<CharacterIterator> next, nextNext, prevPrev;
	CodePoint prevCP = i.current(), nextCP = INVALID_CODE_POINT, nextNextCP = INVALID_CODE_POINT;
	int prevClass = WordBreak::of(prevCP, syntax_, getLocale()),
		nextClass = NOT_PROPERTY, nextNextClass = NOT_PROPERTY, prevPrevClass = NOT_PROPERTY;
	while(true) {
		// 1 つ次 (B) を調べる
		assert(!i.isFirst());
		if(next.get() == 0) {
			next = i.clone();
			previousBase(*next);
		}
		if(nextCP == INVALID_CODE_POINT) nextCP = next->current();
		if(nextClass == NOT_PROPERTY) nextClass = WordBreak::of(nextCP, syntax_, getLocale());
		if(prevClass == GraphemeClusterBreak::LF && nextClass == GraphemeClusterBreak::CR)	// (WB3)
			/* do nothing */;
		else if(prevClass == WordBreak::A_LETTER && nextClass == WordBreak::A_LETTER) {	// (WB5+, !WB13)
			if(!compareScripts(nextCP, prevCP, getLocale()))
				TRY_RETURN()
		} else if((prevClass == WordBreak::A_LETTER || prevClass == WordBreak::NUMERIC || prevClass == WordBreak::EXTEND_NUM_LET)
				&& (nextClass == WordBreak::A_LETTER || nextClass == WordBreak::NUMERIC || nextClass == WordBreak::EXTEND_NUM_LET))	// (WB8, WB9, WB10, WB13a+, WB13b+)
			/* do nothing */;
		else if((nextClass == WordBreak::A_LETTER && prevClass == WordBreak::MID_LETTER)
				|| (nextClass == WordBreak::NUMERIC && prevClass == WordBreak::MID_NUM)) {	// (WB6, WB12)?
			// 2 つ前 (D) を調べる
			if(prevPrevClass == NOT_PROPERTY) {
				if(prevPrev.get() == 0) {
					prevPrev = i.clone();
					nextBase(*prevPrev);
				}
				if(prevPrev->isLast()) {	// (WB14)
					TRY_RETURN()
					break;
				}
				prevPrevClass = WordBreak::of(prevPrev->current(), syntax_, getLocale());
			}
			if(prevPrevClass != nextClass
					&& (!toBoolean(component_ & ALPHA_NUMERIC)
					|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))	// (WB6, WB12)
				TRY_RETURN()
		} else if((nextClass == WordBreak::MID_LETTER && prevClass == WordBreak::A_LETTER)
				|| (nextClass == WordBreak::MID_NUM && prevClass == WordBreak::NUMERIC)) {	// (WB7, WB11)?
			// 2 つ次 (A) を調べる
			if(next->isFirst()) {	// (WB14)
				TRY_RETURN()
				break;
			}
			nextNext = next->clone();
			previousBase(*nextNext);
			nextNextClass = WordBreak::of(nextNextCP = nextNext->current(), syntax_, getLocale());
			if(nextNextClass != prevClass
					&& (!toBoolean(component_ & ALPHA_NUMERIC)
					|| syntax_.isIdentifierContinueCharacter(prevCP) || syntax_.isIdentifierContinueCharacter(nextCP)))	// (WB7, WB11)
				TRY_RETURN()
		} else if((!toBoolean(component_ & END_OF_SEGMENT) && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(prevCP))
				|| (!toBoolean(component_ & START_OF_SEGMENT) && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(nextCP)))	// (+)
			/* do nothing */;
		else if(toBoolean(component_ & ALPHA_NUMERIC)	// (0)
				&& (!toBoolean(component_ & START_OF_SEGMENT) || !syntax_.isIdentifierContinueCharacter(prevCP))
				&& (!toBoolean(component_ & END_OF_SEGMENT) || !syntax_.isIdentifierContinueCharacter(nextCP)))	// (+)
			/* do nothing */;
		else
			TRY_RETURN()

		// 次に進む
		prevPrev = i.clone();
		previousBase(i);
		if(i.isFirst())	// (WB1)
			TRY_RETURN()
		next = nextNext;
		nextNext.reset(0);
		prevCP = i.current();
		nextCP = nextNextCP;
		nextNextCP = INVALID_CODE_POINT;
		prevPrevClass = prevClass;
		prevClass = nextClass;
		nextClass = nextNextClass;
		nextNextClass = NOT_PROPERTY;
	}
}

/// @see BreakIterator#isBoundary
bool AbstractWordBreakIterator::isBoundary(const CharacterIterator& at) const {
	if(at.isFirst() || at.isLast())	// (WB1, WB2)
		return true;

	const CodePoint nextCP = at.current();
	const int nextClass = WordBreak::of(nextCP, syntax_, getLocale());
	if(nextClass == WordBreak::OTHER)	// (WB14)
		return true;
	auto_ptr<CharacterIterator> i(at.clone());
	previousBase(*i);
	CodePoint prevCP = i->current();
	int prevClass = WordBreak::of(prevCP, syntax_, getLocale());

	if(prevClass == GraphemeClusterBreak::CR && nextClass == GraphemeClusterBreak::LF)	// (WB3)
		return false;
	else if(nextClass == WordBreak::A_LETTER && prevClass == WordBreak::A_LETTER)	// (WB5+, !WB13)
		return !compareScripts(prevCP, nextCP, getLocale());
	else if((nextClass == WordBreak::A_LETTER || nextClass == WordBreak::NUMERIC || nextClass == WordBreak::EXTEND_NUM_LET)
			&& (prevClass == WordBreak::A_LETTER || prevClass == WordBreak::NUMERIC || prevClass == WordBreak::EXTEND_NUM_LET))	// (WB8, WB9, WB10, WB13a+, WB13b+)
		return false;
	else if((prevClass == WordBreak::A_LETTER && nextClass == WordBreak::MID_LETTER)
			|| (prevClass == WordBreak::NUMERIC && nextClass == WordBreak::MID_NUM)) {	// (WB6, WB12)?
		// 2 つ次を調べる
		int nextNextClass;
		i = at.clone();
		nextBase(*i);
		while(true) {
			if(i->isLast())	// (WB14)
				return true;
			else if(WordBreak::FORMAT != (nextNextClass = WordBreak::of(i->current(), syntax_, getLocale())))	// (WB4)
				break;
			nextBase(*i);
		}
		return nextNextClass != prevClass;	// (WB6, WB12)
	} else if(!i->isFirst()
			&& ((prevClass == WordBreak::MID_LETTER && nextClass == WordBreak::A_LETTER)
			|| (prevClass == WordBreak::MID_NUM && nextClass == WordBreak::NUMERIC))) {	// (WB7, WB11)?
		// 2 つ前を調べる
		int prevPrevClass;
		while(true) {
			previousBase(*i);
			if(i->isFirst())	// (WB14)
				return true;
			else if(WordBreak::FORMAT != (prevPrevClass = WordBreak::of(i->current(), syntax_, getLocale())))	// (WB4)
				break;
		}
		return prevPrevClass != nextClass;	// (WB7, WB11)
	}
	return true;	// (WB14)
}

/// @see BreakIterator#next
void AbstractWordBreakIterator::next(ptrdiff_t amount) {
	if(amount > 0)
		doNext(+amount);
	else if(amount < 0)
		doPrevious(-amount);
}


// AbstractSentenceBreakIterator ////////////////////////////////////////////

namespace {
	/// Tries SB8 rule.
	bool trySB8(CharacterIterator& i) throw() {
		auto_ptr<CharacterIterator> j(i.clone());
		for(; j->isLast(); nextBase(*j)) {
			switch(SentenceBreak::of(j->current())) {
			case SentenceBreak::O_LETTER:
			case SentenceBreak::UPPER:
			case SentenceBreak::SEP:
				break;
			case SentenceBreak::LOWER:
				while(i < *j)
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
	bool tryToExtendTerm(CharacterIterator& i, bool aTerm) throw() {
		assert(!i.isFirst());
		bool closeOccured = false;	// true if (STerm|ATerm) Close+
		bool spOccured = false;		// true if (STerm|ATerm) Sp+ or (STerm|ATerm) Close+ Sp+
		while(!i.isLast()) {
			switch(SentenceBreak::of(i.current())) {
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
					auto_ptr<CharacterIterator> temp(i.clone());
					previousBase(*temp);
					if(temp->isFirst())
						return true;	// (SB12)
					previousBase(*temp);
					return SentenceBreak::of(temp->current()) != SentenceBreak::UPPER;
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
}

/**
 * Protected constructor.
 * @param component the sentence component to search
 * @param ctypes the character detector
 * @param lc the locale
 */
AbstractSentenceBreakIterator::AbstractSentenceBreakIterator(Component component,
		const IdentifierSyntax& syntax, const locale& lc) throw() : BreakIterator(lc), component_(component), syntax_(syntax) {
}

void AbstractSentenceBreakIterator::doNext(ptrdiff_t amount) {
	CharacterIterator& i = getCharacterIterator();
	while(!i.isLast()) {
		if(i.current() == CARRIAGE_RETURN) {
			i.next();
			if(i.isLast())
				return;	// (SB2)
			if(i.current() == LINE_FEED)
				i.next();	// (SB3)
			return;	// (SB4)
		}
		switch(SentenceBreak::of(i.current())) {
		case SentenceBreak::SEP:
			i.next();
			return;	// (SB4)
		case SentenceBreak::A_TERM:
			nextBase(i);
			if(tryToExtendTerm(i, true))
				return;	// (SB11)
			break;
		case SentenceBreak::S_TERM:
			nextBase(i);
			if(tryToExtendTerm(i, false))
				return;	// (SB11)
			break;
		default:
			break;	// (SB5, SB12)
		}
	}
	// (SB2)
}

void AbstractSentenceBreakIterator::doPrevious(ptrdiff_t amount) {
}

/// @see BreakIterator#isBoundary
bool AbstractSentenceBreakIterator::isBoundary(const CharacterIterator& at) const {
	if(at.isFirst() || at.isLast())
		return true;	// (SB1, SB2)
	auto_ptr<CharacterIterator> i(at.clone());
	if(at.current() == LINE_FEED) {
		if(i->previous().current() == CARRIAGE_RETURN)
			return false;	// (SB3)
		else if(i->isFirst())
			return true;	// (SB12)
		const int p = SentenceBreak::of(i->current());
		if(p == GraphemeClusterBreak::EXTEND || p == SentenceBreak::FORMAT)
			previousBase(*i);	// (SB5)
	} else
		previousBase(*i);	// (SB5)
	do {
		switch(SentenceBreak::of(i->current())) {
		case SentenceBreak::SEP:
			return at - *i == 1;	// (SB4)
		case SentenceBreak::A_TERM:
			nextBase(*i);
			return tryToExtendTerm(*i, true) && *i == at;
		case SentenceBreak::S_TERM:
			nextBase(*i);
			return tryToExtendTerm(*i, false) && *i == at;
		default:
			break;
		}
		previousBase(*i);
	} while(!i->isFirst());
	return false;	// (SB1)
}

/// @see BreakIterator#next
void AbstractSentenceBreakIterator::next(ptrdiff_t amount) {
	if(amount > 0)
		doNext(+amount);
	else if(amount < 0)
		doPrevious(-amount);
}
