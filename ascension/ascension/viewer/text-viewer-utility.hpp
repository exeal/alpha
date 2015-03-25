/**
 * @file text-viewer-utility.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2015-02-28 Separated from text-viewer.hpp.
 * @date 2015-03-26 Renamed from source/utility.hpp.
 */

#ifndef ASCENSION_TEXT_VIEWER_UTILITY_HPP
#define ASCENSION_TEXT_VIEWER_UTILITY_HPP
#include <ascension/corelib/basic-types.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace kernel {
		class Document;
		class Position;
		class Region;
	}

	namespace presentation {
		namespace hyperlink {
			class Hyperlink;
		}
	}

	namespace viewer {
		class TextViewer;

		/// Provides the utility stuffs for viewers.
		namespace utils {
			void closeCompletionProposalsPopup(TextViewer& viewer) BOOST_NOEXCEPT;
			const presentation::hyperlink::Hyperlink* getPointedHyperlink(const TextViewer& viewer, const kernel::Position& at);
			boost::optional<kernel::Region> getPointedIdentifier(const TextViewer& viewer);
			boost::optional<kernel::Region> getNearestIdentifier(
				const kernel::Document& document, const kernel::Position& position);
			bool getNearestIdentifier(const kernel::Document& document,
				const kernel::Position& position, Index* startOffsetInLine, Index* endOffsetInLine);
			void toggleOrientation(TextViewer& viewer) BOOST_NOEXCEPT;
		}
	}
}

#endif // !ASCENSION_TEXT_VIEWER_UTILITY_HPP
