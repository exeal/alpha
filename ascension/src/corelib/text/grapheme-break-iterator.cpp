/**
 * @file grapheme-break-iterator.cpp
 * Implements @c GraphemeBreakIterator class.
 * @author exeal
 * @date 2006-2007 was iterator.cpp
 * @date 2007-2014
 * @date-2016-07-24 Separated from break-iterator.hpp.
 */

#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/grapheme-break-iterator.hpp>

#if ASCENSION_UAX29_REVISION_NUMBER > 11
#	error "These codes are based on old version of UAX #29"
#endif

namespace ascension {
	namespace text {
		/// @internal Implements @c GraphemeBreakIterator#next.
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

		/// @internal Implements @c GraphemeBreakIterator#previous.
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

		/// @internal Implements @c GraphemeBreakIterator#isBoundary.
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
	}
}
