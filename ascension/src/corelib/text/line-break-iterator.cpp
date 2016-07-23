/**
 * @file line-break-iterator.cpp
 * Implements @c LineBreakIterator class.
 * @author exeal
 * @date 2006-2007 was iterator.cpp
 * @date 2007-2014
 * @date-2016-07-24 Separated from break-iterator.hpp.
 */

#include <ascension/corelib/text/line-break-iterator.hpp>

#if ASCENSION_UAX29_REVISION_NUMBER > 11
#	error "These codes are based on old version of UAX #29"
#endif

namespace ascension {
	namespace text {
		/// @see BreakIterator#isBoundary
		bool LineBreakIteratorBase::isBoundary(const detail::CharacterIterator& at) const {
			// TODO: not implemented.
			return true;
		}
	}
}
