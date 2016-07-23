/**
 * @file sentence-break-iterator.cpp
 * Implements @c SentenceBreakIterator class.
 * @author exeal
 * @date 2006-2007 was iterator.cpp
 * @date 2007-2014
 * @date-2016-07-24 Separated from break-iterator.hpp.
 */

#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/sentence-break-iterator.hpp>
#include <ascension/corelib/text/detail/break-iterator-scan-base.hpp>

#if ASCENSION_UAX29_REVISION_NUMBER > 11
#	error "These codes are based on old version of UAX #29"
#endif

namespace ascension {
	namespace text {
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

		/// @internal Implements @c SentenceBreakIterator#next.
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

		/// @internal Implements @c SentenceBreakIterator#previous.
		void SentenceBreakIteratorBase::previous(std::size_t n) {
			// TODO: not implemented.
		}

		/// @internal Implements @c SentenceBreakIterator#isBoundary.
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
	}
}
