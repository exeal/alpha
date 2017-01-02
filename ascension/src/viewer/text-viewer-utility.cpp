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
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
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
				Index start = kernel::offsetInLine(position), end = kernel::offsetInLine(position);

				// find the start of the identifier
				if(startOffsetInLine != nullptr) {
					kernel::DocumentCharacterIterator i(document,
						kernel::Region(std::max(*boost::const_begin(partition.region), kernel::Position::bol(position)), position), position);
					do {
						--i;
						if(!syntax.isIdentifierContinueCharacter(*i)) {
							++i;
							start = kernel::offsetInLine(i.tell());
							break;
						} else if(kernel::offsetInLine(position) - kernel::offsetInLine(i.tell()) > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
							return false;
					} while(i.hasPrevious());
					if(!i.hasPrevious())
						start = kernel::offsetInLine(i.tell());
					*startOffsetInLine = start;
				}

				// find the end of the identifier
				if(endOffsetInLine != nullptr) {
					kernel::DocumentCharacterIterator i(document, kernel::Region(position,
						std::min(*boost::const_end(partition.region), kernel::Position(kernel::line(position), document.lineLength(kernel::line(position))))), position);
					while(i.hasNext()) {
						if(!syntax.isIdentifierContinueCharacter(*i)) {
							end = kernel::offsetInLine(i.tell());
							break;
						}
						++i;
						if(kernel::offsetInLine(i.tell()) - kernel::offsetInLine(position) > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
							return false;
					}
					if(!i.hasNext())
						end = kernel::offsetInLine(i.tell());
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
				if(getNearestIdentifier(document, position, &std::get<0>(offsetsInLine), &std::get<1>(offsetsInLine)))
					return boost::make_optional(kernel::Region::makeSingleLine(kernel::line(position), boost::irange(std::get<0>(offsetsInLine), std::get<1>(offsetsInLine))));
				else
					return boost::none;
			}

			const presentation::hyperlink::Hyperlink* getPointedHyperlink(const TextViewer& viewer, const kernel::Position& at) {
				// TODO: Not implemented.
#if 0
				std::size_t numberOfHyperlinks;
				if(const presentation::hyperlink::Hyperlink* const* hyperlinks = viewer.presentation().getHyperlinks(kernel::line(at), numberOfHyperlinks)) {
					for(std::size_t i = 0; i < numberOfHyperlinks; ++i) {
						if(kernel::offsetInLine(at) >= *hyperlinks[i]->region().begin() && kernel::offsetInLine(at) <= *hyperlinks[i]->region().end())
							return hyperlinks[i];
					}
				}
#endif
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
					return getNearestIdentifier(*document(viewer),
						viewToModel(viewer, widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position())).characterIndex());
//				}
				return boost::none;
			}
#if 0
			/**
			 * Toggles the inline flow direction of the text viewer.
			 * @param viewer The text viewer
			 */
			void toggleOrientation(TextViewer& viewer) BOOST_NOEXCEPT {
				viewer.writingModeProvider().setDefaultDirection(!viewer.writingModeProvider().defaultDirection());
//				viewer.synchronizeWritingModeUI();
//				if(config.lineWrap.wrapsAtWindowEdge()) {
//					auto scroll(win32::makeZeroSize<SCROLLINFO>());
//					viewer.getScrollInformation(SB_HORZ, scroll);
//					viewer.setScrollInformation(SB_HORZ, scroll);
//				}
			}
#endif
		}
	}
}
