/**
 * @file utility.hpp
 * @author exeal
 * @date 2015-02-28 Created.
 */

#ifndef ASCENSION_SOURCE_UTILITY_HPP
#define ASCENSION_SOURCE_UTILITY_HPP
#include <ascension/corelib/basic-types.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace kernel {
		class Document;
		class Position;
		class Region;
	}

	namespace viewer {
		class TextViewer;

		namespace source {
			boost::optional<kernel::Region> getPointedIdentifier(const TextViewer& viewer);
			boost::optional<kernel::Region> getNearestIdentifier(
				const kernel::Document& document, const kernel::Position& position);
			bool getNearestIdentifier(const kernel::Document& document,
				const kernel::Position& position, Index* startOffsetInLine, Index* endOffsetInLine);
		}
	}
}

#endif // !ASCENSION_SOURCE_UTILITY_HPP
