/**
 * @file break-iterator-scan-base.hpp
 * @author exeal
 * @date 2006-2007 was iterator.cpp
 * @date 2007-2014
 * @date-2016-07-24 Separated from break-iterator.cpp.
 */

#include <ascension/corelib/text/character-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>

#if ASCENSION_UAX29_REVISION_NUMBER > 11
#	error "These codes are based on old version of UAX #29"
#endif

namespace ascension {
	namespace text {
		namespace detail {
			/// @internal Advances @a i to the next character neither Extend nor Format.
			inline int nextBase(detail::CharacterIterator& i) {
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
			inline int previousBase(detail::CharacterIterator& i) {
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
		}
	}
}
