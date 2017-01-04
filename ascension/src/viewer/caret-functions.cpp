/**
 * @file caret-functions.cpp
 * Implements free functions related to @c Caret class.
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2015-03-25 Separated from caret.cpp
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/grapheme-break-iterator.hpp>
#include <ascension/corelib/text/word-break-iterator.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/virtual-box.hpp>
#include <boost/range/algorithm/find.hpp>

namespace ascension {
	namespace viewer {
		/**
		 * Breaks the line at the caret position and moves the caret to the end of the inserted string.
		 * @param caret The caret
		 * @param inheritIndent Set @c true to inherit the indent of the line @c at.line
		 * @param newlines The number of newlines to insert
		 * @throw DocumentDisposedException The document @a caret connecting to has been disposed
		 * @throw ... Any exceptions @c Document#insert throws
		 */
		void breakLine(Caret& caret, bool inheritIndent, std::size_t newlines /* = 1 */) {
			if(newlines == 0)
				return;

			std::shared_ptr<const kernel::DocumentInput> documentInput(caret.document().input().lock());
			text::Newline newline((documentInput.get() != nullptr) ? documentInput->newline() : ASCENSION_DEFAULT_NEWLINE);
			documentInput.reset();
			String s(newline.asString());

			if(inheritIndent) {	// simple auto-indent
				const auto ip(insertionPosition(caret));
				const String& currentLine = caret.document().lineString(kernel::line(ip));
				const Index len = kernel::detail::identifierSyntax(caret).eatWhiteSpaces(
					currentLine.data(), currentLine.data() + kernel::offsetInLine(ip), true) - currentLine.data();
				s += currentLine.substr(0, len);
			}

			if(newlines > 1) {
				std::basic_stringbuf<Char> sb;
				for(std::size_t i = 0; i < newlines; ++i)
					sb.sputn(s.data(), static_cast<std::streamsize>(s.length()));
				s.assign(sb.str());
			}
			return caret.replaceSelection(s);
		}

		/**
		 * Deletes the selected region.
		 * If @a caret is nowhere, this function does nothing.
		 * @param caret The caret provides a selection
		 * @throw ... Any exceptions @c Document#insert and @c Document#erase throw
		 */
		void eraseSelection(Caret& caret) {
			return caret.replaceSelection(String());
		}

		namespace {
			/**
			 * @internal Indents the region selected by the caret.
			 * @param caret The caret gives the region to indent
			 * @param character A character to make indents
			 * @param rectangle Set @c true for rectangular indents (will be ignored level is negative)
			 * @param level The level of the indentation
			 * @deprecated 0.8
			 */
			void indent(Caret& caret, Char character, bool rectangle, long level) {
				// TODO: this code is not exception-safe.
				if(level == 0)
					return;
				const String indent(abs(level), character);
				const SelectedRegion region(caret.selectedRegion());

				if(boost::size(region.lines()) == 1) {
					// number of selected lines is one -> just insert tab character(s)
					caret.replaceSelection(indent);
					return;
				}

				kernel::Position otherResult(region.anchor());
				Index line = kernel::line(*boost::const_begin(region));

				struct AdvanceOffsetInLine {
					explicit AdvanceOffsetInLine(SignedIndex offset) BOOST_NOEXCEPT : offset(offset) {}
					kernel::Position operator()(const kernel::Position& p) const BOOST_NOEXCEPT {
						return kernel::Position(kernel::line(p), kernel::offsetInLine(p) + offset);
					}
					SignedIndex offset;
				};

				// indent/unindent the first line
				kernel::Document& document = caret.document();
				if(level > 0) {
					insert(document, kernel::Position(line, rectangle ? kernel::offsetInLine(*boost::const_begin(region)) : 0), indent);
					if(line == kernel::line(otherResult) && kernel::offsetInLine(otherResult) != 0)
						otherResult.offsetInLine += level;
					const auto ip(insertionPosition(caret));
					if(line == kernel::line(ip) && kernel::offsetInLine(ip) != 0)
						caret.moveTo(caret.hit().offsetHit(AdvanceOffsetInLine(level)));
				} else {
					const String& s = document.lineString(line);
					Index indentLength;
					for(indentLength = 0; indentLength < s.length(); ++indentLength) {
						// this assumes that all white space characters belong to BMP
						if(s[indentLength] == '\t' && text::ucd::GeneralCategory::of(s[indentLength]) != text::ucd::GeneralCategory::SPACE_SEPARATOR)
							break;
					}
					if(indentLength > 0) {
						const Index deleteLength = std::min<Index>(-level, indentLength);
						kernel::erase(document, kernel::Region(kernel::Position::bol(line), kernel::Position(line, deleteLength)));
						if(line == kernel::line(otherResult) && kernel::offsetInLine(otherResult) != 0)
							otherResult.offsetInLine -= deleteLength;
						const auto ip(insertionPosition(caret));
						if(line == kernel::line(ip) && kernel::offsetInLine(ip) != 0)
							caret.moveTo(caret.hit().offsetHit(AdvanceOffsetInLine(deleteLength)));
					}
				}

				// indent/unindent the following selected lines
				if(level > 0) {
					for(++line; line <= kernel::line(*boost::const_end(region)); ++line) {
						if(document.lineLength(line) != 0 && (line != kernel::line(*boost::const_end(region)) || kernel::offsetInLine(*boost::const_end(region)) > 0)) {
							boost::optional<Index> insertPosition(0);	// zero is suitable for linear selection
							if(rectangle) {
								// TODO: recognize wrap (second parameter).
								const boost::optional<boost::integer_range<Index>> temp(caret.boxForRectangleSelection().characterRangeInVisualLine(graphics::font::VisualLine(line, 0)));
								if(temp)
									insertPosition = temp->front();
								else
									insertPosition = boost::none;
							}
							if(insertPosition)
								insert(document, kernel::Position(line, *insertPosition), indent);
							if(line == kernel::line(otherResult) && kernel::offsetInLine(otherResult) != 0)
								otherResult.offsetInLine += level;
							const auto ip(insertionPosition(caret));
							if(line == kernel::line(ip) && kernel::offsetInLine(ip) != 0)
								caret.moveTo(caret.hit().offsetHit(AdvanceOffsetInLine(level)));
						}
					}
				} else {
					for(++line; line <= kernel::line(*boost::const_end(region)); ++line) {
						const String& s = document.lineString(line);
						Index indentLength;
						for(indentLength = 0; indentLength < s.length(); ++indentLength) {
						// this assumes that all white space characters belong to BMP
							if(s[indentLength] == '\t' && text::ucd::GeneralCategory::of(s[indentLength]) != text::ucd::GeneralCategory::SPACE_SEPARATOR)
								break;
						}
						if(indentLength > 0) {
							const Index deleteLength = std::min<Index>(-level, indentLength);
							kernel::erase(document, kernel::Region(kernel::Position::bol(line), kernel::Position(line, deleteLength)));
							if(line == kernel::line(otherResult) && kernel::offsetInLine(otherResult) != 0)
								otherResult.offsetInLine -= deleteLength;
							const auto ip(insertionPosition(caret));
							if(line == kernel::line(ip) && kernel::offsetInLine(ip) != 0)
								caret.moveTo(caret.hit().offsetHit(AdvanceOffsetInLine(deleteLength)));
						}
					}
				}
			}
		} // namespace @0

		/**
		 * Indents the region selected by the caret by using spaces.
		 * @param caret The caret gives the region to indent
		 * @param rectangle Set @c true for rectangular indents (will be ignored level is negative)
		 * @param level The level of the indentation
		 * @throw ...
		 * @deprecated 0.8
		 */
		void indentBySpaces(Caret& caret, bool rectangle, long level /* = 1 */) {
			return indent(caret, ' ', rectangle, level);
		}

		/**
		 * Indents the region selected by the caret by using horizontal tabs.
		 * @param caret The caret gives the region to indent
		 * @param rectangle Set @c true for rectangular indents (will be ignored level is negative)
		 * @param level The level of the indentation
		 * @throw ...
		 * @deprecated 0.8
		 */
		void indentByTabs(Caret& caret, bool rectangle, long level /* = 1 */) {
			return indent(caret, '\t', rectangle, level);
		}

		/**
		 * Returns @c true if the specified point is over the selection.
		 * @param p The client coordinates of the point
		 * @return true if the point is over the selection
		 * @throw kernel#DocumentDisposedException The document of @a caret connecting to has been disposed
		 * @throw TextViewerDisposedException The text viewer of @a caret connecting to has been disposed
		 */
		bool isPointOverSelection(const Caret& caret, const graphics::Point& p) {
			if(!isSelectionEmpty(caret)) {
				if(caret.isSelectionRectangle())
					return caret.boxForRectangleSelection().includes(p);
				const TextArea& textArea = caret.textArea();
				const TextViewer& textViewer = textArea.textViewer();
				if(textViewer.hitTest(p) == &textArea) {	// ignore if on the margin
					const graphics::Rectangle viewerBounds(widgetapi::bounds(textViewer, false));
					if(graphics::geometry::x(p) <= graphics::geometry::right(viewerBounds) && graphics::geometry::y(p) <= graphics::geometry::bottom(viewerBounds)) {
						const auto viewport(textArea.viewport());
						const auto hit(viewToModelInBounds(textViewer, p));
						const auto selection(caret.selectedRegion());
						return hit != boost::none && hit->characterIndex() >= *boost::const_begin(selection) && hit->characterIndex() <= *boost::const_end(selection);
					}
				}
			}
			return false;
		}

		/**
		 * Returns the selected range on the specified logical line.
		 * This function returns a logical range, and does not support rectangular selection.
		 * @param caret The caret gives a selection
		 * @param line The logical line
		 * @return The selected range. If the selection continued to the next line, @c range.end() returns the position
		 *         of the end of line + 1. Otherwise if there is not selected range on the line, @c boost#none
		 * @throw kernel#DocumentDisposedException The document @a caret connecting to has been disposed
		 * @throw kernel#BadPositionException @a line is outside of the document
		 * @see selectedRangeOnVisualLine, VirtualBox#characterRangeInVisualLine
		 */
		boost::optional<boost::integer_range<Index>> viewer::selectedRangeOnLine(const Caret& caret, Index line) {
			const kernel::Position bos(*boost::const_begin(caret.selectedRegion()));
			if(kernel::line(bos) > line)
				return boost::none;
			const kernel::Position eos(*boost::const_end(caret.selectedRegion()));
			if(kernel::line(eos) < line)
				return boost::none;
			return boost::irange(
				(line == kernel::line(bos)) ? kernel::offsetInLine(bos) : 0,
				(line == kernel::line(eos)) ? kernel::offsetInLine(eos) : caret.document().lineLength(line) + 1);
		}

		namespace {
			boost::optional<boost::integer_range<Index>> selectedRangeOnVisualLine(const Caret& caret, const graphics::font::VisualLine& line, bool useCalculatedLayout) {
				if(!caret.isSelectionRectangle()) {
					auto range(selectedRangeOnLine(caret, line.line));
					if(range == boost::none)
						return boost::none;
					const graphics::font::TextLayout* const layout = useCalculatedLayout ?
						&const_cast<Caret&>(caret).textArea().textRenderer()->layouts().at(line.line, graphics::font::LineLayoutVector::USE_CALCULATED_LAYOUT)
						: caret.textArea().textRenderer()->layouts().at(line.line);
					const Index sublineOffset = (layout != nullptr) ? layout->lineOffset(line.subline) : 0;
					Index maximumEnd;
					if(layout != nullptr)
						maximumEnd = sublineOffset + layout->lineLength(line.subline) + ((line.subline < layout->numberOfLines() - 1) ? 0 : 1);
					else
						maximumEnd = caret.document().lineLength(line.line);
					range = boost::irange(std::max(*boost::const_begin(boost::get(range)), sublineOffset), std::min(*boost::const_end(boost::get(range)), maximumEnd));
					if(boost::get(range).empty())
						range = boost::none;
					return range;
				} else
					return caret.boxForRectangleSelection().characterRangeInVisualLine(line);
			}
		}

		/**
		 * Returns the selected range on the specified visual line.
		 * @param caret The caret gives a selection
		 * @param line The visual line
		 * @return The selected range. If the selection continued to the next logical line, @c range.end() returns the
		 *         position of the end of line + 1. Otherwise if there is not selected range on the line, @c boost#none
		 * @throw kernel#DocumentDisposedException The document @a caret connecting to has been disposed
		 * @throw TextViewerDisposedException The text viewer @a caret connecting to has been disposed
		 * @throw kernel#BadPositionException @a line or @a subline is outside of the document
		 * @see selectedRangeOnLine, VirtualBox#characterRangeInVisualLine
		 */
		boost::optional<boost::integer_range<Index>> selectedRangeOnVisualLine(const Caret& caret, const graphics::font::VisualLine& line) {
			return selectedRangeOnVisualLine(caret, line, false);
		}

		/**
		 * Returns the selected range on the specified visual line.
		 * @param caret The caret gives a selection
		 * @param line The visual line
		 * @return The selected range. If the selection continued to the next logical line, @c range.end() returns the
		 *         position of the end of line + 1. Otherwise if there is not selected range on the line, @c boost#none
		 * @throw kernel#DocumentDisposedException The document @a caret connecting to has been disposed
		 * @throw TextViewerDisposedException The text viewer @a caret connecting to has been disposed
		 * @throw kernel#BadPositionException @a line or @a subline is outside of the document
		 * @note This function may change the layout.
		 * @see selectedRangeOnLine, VirtualBox#characterRangeInVisualLine
		 */
		boost::optional<boost::integer_range<Index>> selectedRangeOnVisualLine(Caret& caret,
				const graphics::font::VisualLine& line, const graphics::font::UseCalculatedLayoutTag&) {
			return selectedRangeOnVisualLine(caret, line, true);
		}

		/**
		 * Writes the selected string into the specified output stream.
		 * @param caret The caret gives a selection
		 * @param[out] out The output stream
		 * @param newline The newline representation for multiline selection. If the selection is rectangular, this
		 *                value is ignored and the document's newline is used instead
		 * @return @a out
		 */
		std::basic_ostream<Char>& selectedString(const Caret& caret, std::basic_ostream<Char>& out, const text::Newline& newline /* = text::Newline::USE_INTRINSIC_VALUE */) {
			if(!isSelectionEmpty(caret)) {
				if(!caret.isSelectionRectangle())
					writeDocumentToStream(out, caret.document(), caret.selectedRegion(), newline);
				else {
					const kernel::Document& document = caret.document();
					const Index lastLine = kernel::line(*boost::const_end(caret.selectedRegion()));
					for(Index line = kernel::line(*boost::const_begin(caret.selectedRegion())); line <= lastLine; ++line) {
						const kernel::Document::Line& ln = document.lineContent(line);
						// TODO: Recognize wrap (second parameter).
						const boost::optional<boost::integer_range<Index>> selection(caret.boxForRectangleSelection().characterRangeInVisualLine(graphics::font::VisualLine(line, 0)));
						if(selection != boost::none)
							out.write(ln.text().data() + selection->front(), static_cast<std::streamsize>(selection->size()));
						const String newlineString(ln.newline().asString());
						out.write(newlineString.data(), static_cast<std::streamsize>(newlineString.length()));
					}
				}
			}
			return out;
		}

		/**
		 * Selects the word at the caret position. This creates a linear selection.
		 * If the caret is nowhere, this function does nothing.
		 */
		void selectWord(Caret& caret) {
			text::WordBreakIterator<kernel::DocumentCharacterIterator> i(
				kernel::DocumentCharacterIterator(caret.document(), insertionPosition(caret)),
				text::WordBreakIteratorBase::BOUNDARY_OF_SEGMENT, kernel::detail::identifierSyntax(caret));
			caret.endRectangleSelection();
			if(kernel::locations::isEndOfLine(caret)) {
				if(kernel::locations::isBeginningOfLine(caret))	// an empty line
					caret.moveTo(caret.hit());
				else	// eol
					caret.select(_anchor = (--i).base().tell(), _caret = caret.hit());
			} else if(kernel::locations::isBeginningOfLine(caret))	// bol
				caret.select(_anchor = caret.hit().characterIndex(), _caret = TextHit::leading((++i).base().tell()));
			else {
				const auto ip(viewer::insertionPosition(caret));
				const kernel::Position p((++i).base().tell());
				i.base().seek(kernel::Position(kernel::line(ip), kernel::offsetInLine(ip) + 1));
				caret.select(_anchor = (--i).base().tell(), _caret = TextHit::leading(p));
			}
		}

		/**
		 * Transposes the character (grapheme cluster) addressed by the caret and the previous character, and moves the
		 * caret to the end of them. If the characters to transpose are not inside of the accessible region, this
		 * method fails and returns @c false
		 * @param caret The caret
		 * @return false if there is not a character to transpose in the line, or the point is not the beginning of a
		 *         grapheme
		 * @throw DocumentDisposedException The document the caret connecting to has been disposed
		 * @throw ... Any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
		 */
		bool transposeCharacters(Caret& caret) {
			// TODO: handle the case when the caret intervened a grapheme cluster.

			// As transposing characters in string "ab":
			//
			//  a b -- transposing clusters 'a' and 'b'. result is "ba"
			// ^ ^ ^
			// | | next-cluster (named pos[2])
			// | middle-cluster (named pos[1]; usually current-position)
			// previous-cluster (named pos[0])

			kernel::Position pos[3];
			const kernel::Region region(caret.document().accessibleRegion());

			if(text::ucd::BinaryProperty::is<text::ucd::BinaryProperty::GRAPHEME_EXTEND>(kernel::locations::characterAt(caret)))	// not the start of a grapheme
				return false;
			const auto ip(insertionPosition(caret));
			if(!encompasses(region, ip))	// inaccessible
				return false;

			if(kernel::offsetInLine(ip) == 0 || ip == *boost::const_begin(region)) {
				text::GraphemeBreakIterator<kernel::DocumentCharacterIterator> i(kernel::DocumentCharacterIterator(caret.document(), pos[0] = ip));
				pos[1] = (++i).base().tell();
				if(kernel::line(pos[1]) != kernel::line(pos[0]) || pos[1] == pos[0] || !encompasses(region, pos[1]))
					return false;
				pos[2] = (++i).base().tell();
				if(kernel::line(pos[2]) != kernel::line(pos[1]) || pos[2] == pos[1] || !encompasses(region, pos[2]))
					return false;
			} else if(kernel::offsetInLine(ip) == caret.document().lineLength(kernel::line(ip)) || ip == *boost::const_end(region)) {
				text::GraphemeBreakIterator<kernel::DocumentCharacterIterator> i(kernel::DocumentCharacterIterator(caret.document(), pos[2] = ip));
				pos[1] = (--i).base().tell();
				if(kernel::line(pos[1]) != kernel::line(pos[2]) || pos[1] == pos[2] || !encompasses(region, pos[1]))
					return false;
				pos[0] = (--i).base().tell();
				if(kernel::line(pos[0]) != kernel::line(pos[1]) || pos[0] == pos[1] || !encompasses(region, pos[0]))
					return false;
			} else {
				text::GraphemeBreakIterator<kernel::DocumentCharacterIterator> i(kernel::DocumentCharacterIterator(caret.document(), pos[1] = ip));
				pos[2] = (++i).base().tell();
				if(kernel::line(pos[2]) != kernel::line(pos[1]) || pos[2] == pos[1] || !encompasses(region, pos[2]))
					return false;
				i.base().seek(pos[1]);
				pos[0] = (--i).base().tell();
				if(kernel::line(pos[0]) != kernel::line(pos[1]) || pos[0] == pos[1] || !encompasses(region, pos[0]))
					return false;
			}

			std::basic_ostringstream<Char> ss;
			writeDocumentToStream(ss, caret.document(), kernel::Region(pos[1], pos[2]), text::Newline::LINE_SEPARATOR);
			writeDocumentToStream(ss, caret.document(), kernel::Region(pos[0], pos[1]), text::Newline::LINE_SEPARATOR);
			try {
				caret.document().replace(kernel::Region(pos[0], pos[2]), ss.str());
			} catch(kernel::DocumentAccessViolationException&) {
				return false;
			}
			assert(insertionPosition(caret) == pos[2]);
			return true;
		}

		/**
		 * Transposes the line addressed by the caret and the next line, and moves the caret to the same offset in the
		 * next line. If the caret is the last line in the document, transposes with the previous line. The intervening
		 * newline character is not moved. If the lines to transpose are not inside of the accessible region, this
		 * method fails and returns @c false
		 * @param caret The caret
		 * @return false if there is not a line to transpose
		 * @throw DocumentDisposedException The document the caret connecting to has been disposed
		 * @throw ... Any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
		 */
		bool transposeLines(Caret& caret) {
			if(caret.document().numberOfLines() == 1)	// there is just one line
				return false;

			kernel::Document& document = caret.document();
			const auto old(caret.hit());
			Index firstLine = kernel::line(old.characterIndex());
			const bool caretWasLastLine = firstLine == document.numberOfLines() - 1;
			if(caretWasLastLine)
				--firstLine;
			String s(document.lineString(firstLine + 1));
			s += document.lineContent(firstLine).newline().asString();
			s += document.lineString(firstLine);

			try {
				document.replace(kernel::Region(
					kernel::Position::bol(firstLine), kernel::Position(firstLine + 1, document.lineLength(firstLine + 1))), s);
				const kernel::Position newCaret(!caretWasLastLine ? firstLine + 1 : firstLine, kernel::offsetInLine(old.characterIndex()));
				caret.moveTo(old.isLeadingEdge() ? TextHit::leading(newCaret) : TextHit::trailing(newCaret));
			} catch(const kernel::DocumentAccessViolationException&) {
				return false;
			}
			return true;
		}

		/**
		 * Transposes the word addressed by the caret and the next word, and moves the caret to the end of
		 * them.
		 * If the words to transpose are not inside of the accessible region, this method fails and returns
		 * @c false
		 * @param caret The caret
		 * @return false if there is not a word to transpose
		 * @throw DocumentDisposedException The document the caret connecting to has been disposed
		 * @throw ... Any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
		 */
		bool transposeWords(Caret& caret) {
			// As transposing words in string "(\w+)[^\w*](\w+)":
			//
			//  abc += xyz -- transposing words "abc" and "xyz". result is "xyz+=abc"
			// ^   ^  ^   ^
			// |   |  |   2nd-word-end (named pos[3])
			// |   |  2nd-word-start (named pos[2])
			// |   1st-word-end (named pos[1])
			// 1st-word-start (named pos[0])

			auto& document = caret.document();
			const auto ip(insertionPosition(caret));
			text::WordBreakIterator<kernel::DocumentCharacterIterator> i(
				kernel::DocumentCharacterIterator(document, ip),
				text::WordBreakIteratorBase::START_OF_ALPHANUMERICS, kernel::detail::identifierSyntax(caret));
			kernel::Position pos[4];

			// find the backward word (1st-word-*)...
			pos[0] = (--i).base().tell();
			i.setComponent(text::WordBreakIteratorBase::END_OF_ALPHANUMERICS);
			pos[1] = (++i).base().tell();
			if(pos[1] == pos[0])	// the word is empty
				return false;

			// ...and then backward one (2nd-word-*)
			i.base().seek(ip);
			i.setComponent(text::WordBreakIteratorBase::START_OF_ALPHANUMERICS);
			pos[2] = (++i).base().tell();
			if(pos[2] == ip)
				return false;
			pos[3] = (++i).base().tell();
			if(pos[2] == pos[3])	// the word is empty
				return false;

			// replace
			std::basic_ostringstream<Char> ss;
			writeDocumentToStream(ss, document, kernel::Region(pos[2], pos[3]), text::Newline::USE_INTRINSIC_VALUE);
			writeDocumentToStream(ss, document, kernel::Region(pos[1], pos[2]), text::Newline::USE_INTRINSIC_VALUE);
			writeDocumentToStream(ss, document, kernel::Region(pos[0], pos[1]), text::Newline::USE_INTRINSIC_VALUE);
			kernel::Position e;
			try {
				e = document.replace(kernel::Region(pos[0], pos[3]), ss.str());
			} catch(const kernel::DocumentAccessViolationException&) {
				return false;
			}
			return caret.moveTo(TextHit::leading(e)), true;
		}

		namespace utils {
			/**
			 * Creates an @c InterprocessData object represents the selected content.
			 * @param caret The caret gives the selection
			 * @param rtf Set @c true if the content is available as Rich Text Format. This feature is not implemented
			 *            yet and the parameter is ignored
			 * @return The @c InterprocessData object
			 */
			InterprocessData createInterprocessDataForSelectedString(const Caret& caret, bool rtf) {
				InterprocessData data;
				data.setText(selectedString(caret, text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED));
				if(caret.isSelectionRectangle())
					data.setData(rectangleTextMimeDataFormat(), boost::make_iterator_range<const unsigned char*>(nullptr, nullptr));
				return data;
#if 0
				if(rtf) {
					const CLIPFORMAT rtfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(L"Rich Text Format"));	// CF_RTF
					const CLIPFORMAT rtfWithoutObjectsFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(L"Rich Text Format Without Objects"));	// CF_RTFNOOBJS
																																							// TODO: implement the follow...
				}
#endif
			}

			/// Returns interprocess data format for rectangle text.
			InterprocessDataFormats::Format rectangleTextMimeDataFormat() {
#ifndef ASCENSION_RECTANGLE_TEXT_MIME_FORMAT
#	if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#		define ASCENSION_RECTANGLE_TEXT_MIME_FORMAT "text/x-ascension-rectangle"
#	elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#		define ASCENSION_RECTANGLE_TEXT_MIME_FORMAT "text/x-ascension-rectangle"
#	elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#		error Not implemented.
#	elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#		define ASCENSION_RECTANGLE_TEXT_MIME_FORMAT L"MSDEVColumnSelect"
#	endif
#endif // !ASCENSION_RECTANGLE_TEXT_MIME_FORMAT
				
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				return std::string(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				return QString::fromLatin1(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#		error Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				static boost::optional<CLIPFORMAT> registered;
				if(registered == boost::none) {
					registered = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT));
					if(boost::get(registered) == 0)
						throw makePlatformError();
				}
				return boost::get(registered);
#endif
			}
		}
	}
}
