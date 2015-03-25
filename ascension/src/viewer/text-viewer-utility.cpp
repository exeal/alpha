/**
 * @file text-viewer-utility.cpp
 * Implements free functions in text-viewer-utility.hpp.
 * @author exeal
 * @date 2015-02-28 Separated from text-viewer.cpp.
 * @date 2015-03-26 Renamed from source/utility.cpp.
 */

#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/partition.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/source/utility.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>

namespace ascension {
	namespace viewer {
		namespace utils {
			/**
			 * Returns the identifier near the specified position in the document.
			 * @param document The document
			 * @param position The position
			 * @param[out] startOffsetInLine The start offset in the line, of the identifier. Can be @c nullptr if not
			 *                               needed
			 * @param[out] endOffsetInLine The end offset in the line, of the identifier. Can be @c nullptr if not
			 *                             needed
			 * @return @c true if an identifier was found. @c false if not found and output paramter values are not
			 *         defined in this case
			 * @see #getPointedIdentifier
			 */
			bool getNearestIdentifier(const kernel::Document& document,
					const kernel::Position& position, Index* startOffsetInLine, Index* endOffsetInLine) {
				static const Index MAXIMUM_IDENTIFIER_HALF_LENGTH = 100;

				kernel::DocumentPartition partition;
				document.partitioner().partition(position, partition);
				const text::IdentifierSyntax& syntax = document.contentTypeInformation().getIdentifierSyntax(partition.contentType);
				Index start = position.offsetInLine, end = position.offsetInLine;

				// find the start of the identifier
				if(startOffsetInLine != nullptr) {
					kernel::DocumentCharacterIterator i(document,
						kernel::Region(std::max(partition.region.beginning(), kernel::Position(position.line, 0)), position), position);
					do {
						--i;
						if(!syntax.isIdentifierContinueCharacter(*i)) {
							++i;
							start = i.tell().offsetInLine;
							break;
						} else if(position.offsetInLine - i.tell().offsetInLine > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
							return false;
					} while(i.hasPrevious());
					if(!i.hasPrevious())
						start = i.tell().offsetInLine;
					*startOffsetInLine = start;
				}

				// find the end of the identifier
				if(endOffsetInLine != nullptr) {
					kernel::DocumentCharacterIterator i(document, kernel::Region(position,
						std::min(partition.region.end(), kernel::Position(position.line, document.lineLength(position.line)))), position);
					while(i.hasNext()) {
						if(!syntax.isIdentifierContinueCharacter(*i)) {
							end = i.tell().offsetInLine;
							break;
						}
						++i;
						if(i.tell().offsetInLine - position.offsetInLine > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
							return false;
					}
					if(!i.hasNext())
						end = i.tell().offsetInLine;
					*endOffsetInLine = end;
				}

				return true;
			}

			/**
			 * Returns the identifier near the specified position in the document.
			 * @param document The document
			 * @param position The position
			 * @return The found identifier or @c boost#none if not found
			 * @see #getPointedIdentifier
			 */
			boost::optional<kernel::Region> getNearestIdentifier(const kernel::Document& document, const kernel::Position& position) {
				std::pair<Index, Index> offsetsInLine;
				if(getNearestIdentifier(document, position, &offsetsInLine.first, &offsetsInLine.second))
					return boost::make_optional(kernel::Region(position.line, boost::irange(offsetsInLine.first, offsetsInLine.second)));
				else
					return boost::none;
			}

			const presentation::hyperlink::Hyperlink* getPointedHyperlink(const TextViewer& viewer, const kernel::Position& at) {
				std::size_t numberOfHyperlinks;
				if(const presentation::hyperlink::Hyperlink* const* hyperlinks = viewer.presentation().getHyperlinks(at.line, numberOfHyperlinks)) {
					for(std::size_t i = 0; i < numberOfHyperlinks; ++i) {
						if(at.offsetInLine >= *hyperlinks[i]->region().begin() && at.offsetInLine <= *hyperlinks[i]->region().end())
							return hyperlinks[i];
					}
				}
				return nullptr;
			}

			/**
			 * Returns the identifier near the cursor.
			 * @param viewer The text viewer
			 * @return The found identifier or @c boost#none if not found
			 * @see #getNearestIdentifier
			 */
			boost::optional<kernel::Region> getPointedIdentifier(const TextViewer& viewer) {
//				if(viewer.isWindow()) {
					return getNearestIdentifier(
						viewer.document(), viewToModel(*viewer.textArea().textRenderer().viewport(),
						widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position())).characterIndex());
//				}
				return boost::none;
			}

			/**
			 * Toggles the inline flow direction of the text viewer.
			 * @param viewer The text viewer
			 */
			void toggleOrientation(TextViewer& viewer) BOOST_NOEXCEPT {
				viewer.presentation().setDefaultDirection(!viewer.presentation().defaultDirection());
//				viewer.synchronizeWritingModeUI();
//				if(config.lineWrap.wrapsAtWindowEdge()) {
//					win32::AutoZeroSize<SCROLLINFO> scroll;
//					viewer.getScrollInformation(SB_HORZ, scroll);
//					viewer.setScrollInformation(SB_HORZ, scroll);
//				}
			}
		}
	}
}
