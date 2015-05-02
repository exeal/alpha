/**
 * @file default-text-area-mouse-input-strategy.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-10-04 separated from viewer.cpp
 * @date 2015-03-16 Renamed from default-mouse-input-strategy.cpp
 * @date 2015-03-19 Renamed from default-text-viewer-mouse-input-strategy.cpp
 */

// TODO: This code does not support platforms other than Win32.

#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/image.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/text-editor/session.hpp>	// texteditor.xxxIncrementalSearch
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-text-area-mouse-input-strategy.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <boost/foreach.hpp>
#include <limits>	// std.numeric_limit

#include "auto-scroll-origin-mark.cpp"

namespace ascension {
	namespace viewer {
		/// @internal
		struct DefaultTextAreaMouseInputStrategy::SelectionExtender {
			virtual ~SelectionExtender() BOOST_NOEXCEPT {}
			virtual void continueSelection(Caret& caret, const kernel::Position& destination) = 0;
		};

		namespace {
			class CharacterSelectionExtender : public DefaultTextAreaMouseInputStrategy::SelectionExtender {
			public:
				CharacterSelectionExtender(Caret& caret, const kernel::Position& initialPosition) {
					caret.moveTo(initialPosition);
				}
				void continueSelection(Caret& caret, const kernel::Position& destination) override {
					caret.extendSelectionTo(destination);
				}
			};

			class WordSelectionExtender : public DefaultTextAreaMouseInputStrategy::SelectionExtender {
			public:
				WordSelectionExtender(Caret& caret, const kernel::Position* initialPosition = nullptr) : anchorOffsetsInLine_(0, 0) {
					if(initialPosition != nullptr)
						caret.moveTo(*initialPosition);
					selectWord(caret);
					anchorLine_ = kernel::line(caret);
					anchorOffsetsInLine_ = boost::irange(kernel::offsetInLine(caret.beginning()), kernel::offsetInLine(caret.end()));
				}
				void continueSelection(Caret& caret, const kernel::Position& destination) override {
					const kernel::Document& document = caret.document();
					const text::IdentifierSyntax& id = document.contentTypeInformation().getIdentifierSyntax(kernel::contentType(caret));
					if(destination.line < anchorLine_
							|| (destination.line == anchorLine_
								&& destination.offsetInLine < *boost::const_begin(anchorOffsetsInLine_))) {
						text::WordBreakIterator<kernel::DocumentCharacterIterator> i(
							kernel::DocumentCharacterIterator(document, destination), text::WordBreakIteratorBase::BOUNDARY_OF_SEGMENT, id);
						--i;
						caret.select(kernel::Position(anchorLine_, *boost::const_end(anchorOffsetsInLine_)),
							(i.base().tell().line == destination.line) ? i.base().tell() : kernel::Position(destination.line, 0));
					} else if(destination.line > anchorLine_
							|| (destination.line == anchorLine_
								&& destination.offsetInLine > *boost::const_end(anchorOffsetsInLine_))) {
						text::WordBreakIterator<kernel::DocumentCharacterIterator> i(
							kernel::DocumentCharacterIterator(document, destination), text::WordBreakIteratorBase::BOUNDARY_OF_SEGMENT, id);
						++i;
						caret.select(kernel::Position(anchorLine_, *boost::const_begin(anchorOffsetsInLine_)),
							(i.base().tell().line == destination.line) ?
								i.base().tell() : kernel::Position(destination.line, document.lineLength(destination.line)));
					} else
						caret.select(kernel::Region(anchorLine_, anchorOffsetsInLine_));
				}
			private:
				Index anchorLine_;
				boost::integer_range<Index> anchorOffsetsInLine_;
			};
		}

		/**
		 * @class ascension#viewer#DefaultTextAreaMouseInputStrategy
		 * Standard implementation of @c MouseOperationStrategy interface.
		 *
		 * This class implements the standard behavior for the user's mouse input.
		 *
		 * - Begins drag-and-drop operation when the mouse moved with the left button down.
		 * - Enters line-selection mode if the mouse left button pressed when the cursor is over the vertical ruler.
		 * - When the cursor is over an invokable link, pressing the left button to open that link.
		 * - Otherwise when the left button pressed, moves the caret to that position. Modifier keys change this
		 *   behavior as follows: [Shift] The anchor will not move. [Control] Enters word-selection mode. [Alt] Enters
		 *   rectangle-selection mode. These modifications can be combined.
		 * - Double click of the left button selects the word under the cursor. And enters word-selection mode.
		 * - Click the middle button to enter auto-scroll mode.
		 * - If the mouse moved with the middle pressed, enters temporary auto-scroll mode. This mode automatically
		 *   ends when the button was released.
		 * - Changes the mouse cursor according to the position of the cursor (Arrow, I-beam and hand).
		 */

		/// Default constructor.
		DefaultTextAreaMouseInputStrategy::DefaultTextAreaMouseInputStrategy() : viewer_(nullptr), lastHoveredHyperlink_(nullptr) {
		}

		namespace {
			std::unique_ptr<graphics::Image> createSelectionImage(TextViewer& viewer,
					const graphics::Point& cursorPosition, bool highlightSelection, graphics::geometry::BasicRectangle<std::int32_t>& dimensions) {
				// determine the range to draw
				const kernel::Region selectedRegion(viewer.caret());
//				const shared_ptr<const TextViewport> viewport(viewer.textRenderer().viewport());
//				const Index firstLine = viewport->firstVisibleLineInLogicalNumber();
//				const Index firstSubline = viewport->firstVisibleSublineInLogicalLine();

				// calculate the size of the image
				namespace geometry = graphics::geometry;
				using graphics::Scalar;
				const graphics::Rectangle clientBounds(widgetapi::bounds(viewer, false));
				graphics::font::TextRenderer& renderer = viewer.textArea().textRenderer();
				std::shared_ptr<const graphics::font::TextViewport> viewport(renderer.viewport());
//				const graphics::font::TextViewportNotificationLocker lock(viewport.get());
				graphics::Rectangle selectionBounds(
					graphics::Point(geometry::_x = std::numeric_limits<Scalar>::max(), geometry::_y = 0.0f),
					graphics::Dimension(geometry::_dx = std::numeric_limits<Scalar>::min(), geometry::_dy = 0.0f));
				for(Index line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
					NumericRange<Scalar> yrange(geometry::range<1>(selectionBounds) | adaptors::ordered());
//					yrange.advance_end(widgetapi::createRenderingContext(viewer)->fontMetrics(renderer.defaultFont())->linePitch() * renderer.layouts()[line].numberOfLines());
					yrange = boost::irange(*yrange.begin(), *yrange.end() + widgetapi::createRenderingContext(viewer)->fontMetrics(renderer.defaultFont())->linePitch() * renderer.layouts()[line].numberOfLines());
					geometry::range<1>(selectionBounds) = yrange;
					if(geometry::dy(selectionBounds) > geometry::dy(clientBounds))
						return std::unique_ptr<graphics::Image>();	// overflow
					const graphics::font::TextLayout& layout = renderer.layouts()[line];
					const presentation::WritingMode writingMode(graphics::font::writingMode(layout));
					const Scalar indent = graphics::font::lineIndent(layout, viewport->contentMeasure());
					for(Index subline = 0, sublines = layout.numberOfLines(); subline < sublines; ++subline) {
						boost::optional<boost::integer_range<Index>> range(selectedRangeOnVisualLine(viewer.caret(), graphics::font::VisualLine(line, subline)));
						if(range) {
							range = boost::irange(*range->begin(), std::min(viewer.document().lineLength(line), *range->end()));
							const graphics::Rectangle sublineBounds(geometry::make<graphics::Rectangle>(mapFlowRelativeToPhysical(writingMode, layout.bounds(*range))));
							geometry::range<0>(selectionBounds) = boost::irange(
								std::min(geometry::left(sublineBounds) + indent, geometry::left(selectionBounds)),
								std::max(geometry::right(sublineBounds) + indent, geometry::right(selectionBounds)));
							if(geometry::dx(selectionBounds) > geometry::dx(clientBounds))
								return std::unique_ptr<graphics::Image>();	// overflow
						}
					}
				}
				const geometry::BasicDimension<std::uint32_t> size(
					geometry::_dx = static_cast<std::uint32_t>(geometry::dx(selectionBounds)),
					geometry::_dy = static_cast<std::uint32_t>(geometry::dy(selectionBounds)));

				// create a mask
				graphics::Image mask(size, graphics::Image::A1);	// monochrome. This may occur ERROR_INVALID_BITMAP on Win32
				{
					std::unique_ptr<graphics::RenderingContext2D> context(mask.createRenderingContext());

					// fill with transparent bits
					context->setFillStyle(std::make_shared<graphics::SolidColor>(graphics::Color::OPAQUE_BLACK));
					context->fillRectangle(graphics::Rectangle(boost::geometry::make_zero<graphics::Point>(), geometry::size(selectionBounds)));

					// render mask pattern
					Scalar y = 0;
					for(Index line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
						// render each lines
						const graphics::font::TextLayout& layout = renderer.layouts()[line];
						const Scalar indent = graphics::font::lineIndent(layout, viewport->contentMeasure());
						for(Index subline = 0, sublines = layout.numberOfLines(); subline < sublines; ++subline) {
							boost::optional<boost::integer_range<Index>> range(selectedRangeOnVisualLine(viewer.caret(), graphics::font::VisualLine(line, subline)));
							if(range) {
								range = boost::irange(*range->begin(), std::min(viewer.document().lineLength(line), *range->end()));
								boost::geometry::model::multi_polygon<boost::geometry::model::polygon<graphics::Point>> region;
								layout.blackBoxBounds(*range, region);
								geometry::translate(region,
									graphics::Dimension(geometry::_dx = indent - geometry::left(selectionBounds), geometry::_dy = y - geometry::top(selectionBounds)));
								context->setFillStyle(std::make_shared<graphics::SolidColor>(graphics::Color::OPAQUE_WHITE));
								BOOST_FOREACH(const auto& polygon, region) {
									context->beginPath();
									bool firstPoint = true;
									boost::geometry::for_each_point(polygon, [&context, &firstPoint](const graphics::Point& p) {
										if(firstPoint) {
											context->moveTo(p);
											firstPoint = false;
										} else
											context->lineTo(p);
									});
//									context->endPath();
							context->fill();
								}
							}
							y += widgetapi::createRenderingContext(viewer)->fontMetrics(renderer.defaultFont())->linePitch();
						}
					}
				}

				// create the result image
				std::unique_ptr<graphics::Image> image(new graphics::Image(size, graphics::Image::ARGB32));
				{
					// render the lines
					graphics::Rectangle selectionExtent(selectionBounds);
					geometry::translate(selectionExtent,
						geometry::negate(graphics::Dimension(geometry::_dx = geometry::left(selectionExtent), geometry::_dy = geometry::top(selectionExtent))));
					graphics::PaintContext context(move(image->createRenderingContext()), selectionExtent);
					Scalar y = geometry::top(selectionBounds);
					for(Index line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
						renderer.paint(line, context,
							graphics::Point(
								geometry::_x = graphics::font::lineIndent(renderer.layouts()[line], viewport->contentMeasure()) - geometry::left(selectionBounds),
								geometry::_y = y));
						y += widgetapi::createRenderingContext(viewer)->fontMetrics(renderer.defaultFont())->linePitch() * renderer.layouts().numberOfSublinesOfLine(line);
					}

					// set alpha values
					const std::array<std::uint8_t, 2> alphaChunnels = {
#if !ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
						0x00,
#else
						0x01,
#endif
						0xff
					};
					boost::iterator_range<const std::uint8_t*>::const_iterator maskByte(std::begin(mask.pixels()));
					boost::iterator_range<std::uint8_t*> imageBits(image->pixels());
					for(std::uint16_t y = 0; y < geometry::dy(size); ++y) {
						for(std::uint16_t x = 0; ; ) {
							std::uint8_t* const pixel = std::begin(imageBits) + x + geometry::dx(size) * y;	// points alpha value
							*pixel = alphaChunnels[(*maskByte & (1 << ((8 - x % 8) - 1))) ? 1 : 0];
							if(x % 8 == 7)
								++maskByte;
							if(++x == geometry::dx(size)) {
								if(x % 8 != 0)
									++maskByte;
								break;
							}
						}
						if(reinterpret_cast<std::uintptr_t>(maskByte) % sizeof(std::uint32_t) != 0)
							maskByte += sizeof(std::uint32_t) - reinterpret_cast<std::uintptr_t>(maskByte) % sizeof(std::uint32_t);
					}
				}

				// locate the hotspot of the image based on the cursor position
				// TODO: This code can't handle vertical writing mode.
				graphics::Point hotspot(cursorPosition);
				geometry::x(hotspot) -= geometry::left(viewer.textAreaContentRectangle()) - viewport->scrollPositions().ipd() + geometry::left(selectionBounds);
				geometry::y(hotspot) -= geometry::y(graphics::font::modelToView(*viewport,
					graphics::font::TextHit<kernel::Position>::leading(kernel::Position(selectedRegion.beginning().line, 0))));

				// calculate 'dimensions'
				dimensions = geometry::BasicRectangle<std::uint16_t>(
					static_cast<geometry::BasicPoint<std::uint16_t>>(geometry::negate(hotspot)), static_cast<geometry::BasicDimension<std::uint16_t>>(size));

				return image;
			}
		}

		void DefaultTextAreaMouseInputStrategy::beginDragAndDrop(const widgetapi::event::LocatedUserInput& input) {
			assert(isStateNeutral());
			dragAndDrop_ = DragAndDrop();
			const Caret& caret = viewer_->caret();
			if(!caret.isSelectionRectangle())
				dragAndDrop_->numberOfRectangleLines = 0;
			else {
				const kernel::Region selection(caret.selectedRegion());
				dragAndDrop_->numberOfRectangleLines = selection.end().line - selection.beginning().line + 1;
			}

			// setup drag-image and begin operation
			widgetapi::DragContext d(*viewer_);

			const std::shared_ptr<const widgetapi::MimeData> draggingContent(utils::createMimeDataForSelectedString(viewer_->caret(), true));
			d.setMimeData(draggingContent);

			graphics::geometry::BasicRectangle<std::int32_t> imageDimensions;
			std::unique_ptr<graphics::Image> image(createSelectionImage(*viewer_, dragAndDrop_->approachedPosition, true, imageDimensions));
			d.setImage(*image, graphics::geometry::negate(graphics::geometry::topLeft(imageDimensions)));

			widgetapi::DropAction possibleActions = widgetapi::DROP_ACTION_COPY;
			if(!viewer_->document().isReadOnly())
				possibleActions |= widgetapi::DROP_ACTION_MOVE;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			d.execute(possibleActions, input.modifiers(), nullptr);
#else
			d.execute(possibleActions);
#endif

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && 0
			if(dnd_.dragSourceHelper.get() != nullptr) {
				unique_ptr<Image> image(createSelectionImage(*viewer_, dragApproachedPosition_, true, imageDimensions));
				if(image.get() != nullptr) {
					SHDRAGIMAGE temp;
					temp.sizeDragImage.cx = geometry::dx(imageDimensions);
					temp.sizeDragImage.cy = geometry::dy(imageDimensions);
					temp.ptOffset.x = -geometry::left(imageDimensions);
					temp.ptOffset.y = -geometry::top(imageDimensions);
					temp.hbmpDragImage = image->asNativeObject;
					temp.crColorKey = CLR_NONE;
					const HRESULT hr = dnd_.dragSourceHelper->InitializeFromBitmap(&image.get(), draggingContent.get());
				}
			}
			dragAndDrop_.state = DragAndDrop::PROCESSING_AS_SOURCE;
			DWORD possibleEffects = DROPEFFECT_COPY | DROPEFFECT_SCROLL, resultEffect;
			if(!viewer_->document().isReadOnly())
				possibleEffects |= DROPEFFECT_MOVE;
			hr = ::DoDragDrop(draggingContent.get(), this, possibleEffects, &resultEffect);
#endif

			dragAndDrop_ = boost::none;
			if(widgetapi::isVisible(*viewer_))
				widgetapi::setFocus(*viewer_);
		}

		namespace {
			inline bool isMimeDataAcceptable(const widgetapi::MimeDataFormats& formats, bool onlyRectangle) {
				if(onlyRectangle)
					return formats.hasFormat(utils::rectangleTextMimeDataFormat());
				return formats.hasText();
#if 0
#	if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				return (data.get_target() == ASCENSION_RECTANGLE_TEXT_MIME_FORMAT) || (!onlyRectangle && data.targets_include_text());
#	elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				return data.hasFormat(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT) || (!onlyRectangle && data.hasText());
#	elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				const array<CLIPFORMAT, 3> formats = {
					static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT)), CF_UNICODETEXT, CF_TEXT
				};
				FORMATETC format = {0, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
				for(std::size_t i = 0; i < (onlyRectangle ? 1 : formats.size()); ++i) {
					if((format.cfFormat = formats[i]) != 0) {
						const HRESULT hr = const_cast<widgetapi::NativeMimeData&>(data).QueryGetData(&format);
						if(hr == S_OK)
							return true;
					}
				}
				return false;
#	endif
#endif
			}
		}

		/// @see DropTarget#dragEntered
		void DefaultTextAreaMouseInputStrategy::dragEntered(widgetapi::DragEnterInput& input) {
			input.setDropAction(widgetapi::DROP_ACTION_IGNORE);
			if(/*dnd_.supportLevel == DONT_SUPPORT_DND ||*/ viewer_->document().isReadOnly() || !viewer_->allowsMouseInput())
				return input.ignore(boost::none);

			// validate the dragged data if can drop
			if(!isMimeDataAcceptable(input.mimeDataFormats(), false))
				return input.ignore(boost::none);

			if(dragAndDrop_ == boost::none || dragAndDrop_->state != DragAndDrop::PROCESSING_AS_SOURCE) {
				assert(isStateNeutral());
				dragAndDrop_ = DragAndDrop();
				// retrieve number of lines if text is rectangle
				dragAndDrop_->numberOfRectangleLines = 0;
				if(isMimeDataAcceptable(input.mimeDataFormats(), true)) {
					// TODO: Not implemented.
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					const presentation::TextAnchor anchor = defaultTextAnchor(viewer_->presentation());
					const presentation::ReadingDirection readingDirection = defaultReadingDirection(viewer_->presentation());
					if((anchor == presentation::TextAnchor::START && readingDirection == presentation::RIGHT_TO_LEFT)
							|| (anchor == presentation::TextAnchor::END && readingDirection == presentation::LEFT_TO_RIGHT))
						return input.ignore(boost::none);	// TODO: support alignments other than ALIGN_LEFT.
					try {
						std::pair<String, bool> text(utils::getTextFromMimeData(input.mimeData()));
						dragAndDrop_.numberOfRectangleLines = text::calculateNumberOfLines(text.first) - 1;
					} catch(...) {
						return input.ignore(boost::none);
					}
#endif
				}
				dragAndDrop_->state = DragAndDrop::PROCESSING_AS_TARGET;
			}

			widgetapi::setFocus(*viewer_);
			beginLocationTracking(*viewer_, nullptr, true, false);
			return dragMoved(input);
		}

		/// @see DropTarget#dragLeft
		void DefaultTextAreaMouseInputStrategy::dragLeft(widgetapi::DragLeaveInput& input) {
			widgetapi::unsetFocus(*viewer_);
			endLocationTracking();
//			if(dnd_.supportLevel >= SUPPORT_DND) {
				if(dragAndDrop_ != boost::none && dragAndDrop_->state == DragAndDrop::PROCESSING_AS_TARGET)
					dragAndDrop_ = boost::none;
//			}
			input.consume();
		}

		namespace {
			graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset> calculateDnDScrollOffset(const TextViewer& viewer) {
				const graphics::Point p(widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position()));
				const graphics::Rectangle localBounds(widgetapi::bounds(viewer, false));
				graphics::Rectangle inset(viewer.textAreaContentRectangle());
				std::unique_ptr<const graphics::font::FontMetrics<graphics::Scalar>> fontMetrics(
					widgetapi::createRenderingContext(viewer)->fontMetrics(viewer.textArea().textRenderer().defaultFont()));

				namespace geometry = graphics::geometry;
				geometry::range<0>(inset) = boost::irange(
					geometry::left(inset) + fontMetrics->averageCharacterWidth(), geometry::right(inset) - fontMetrics->averageCharacterWidth());
				geometry::range<1>(inset) = boost::irange(
					geometry::top(inset) + fontMetrics->linePitch() / 2, geometry::bottom(inset) - fontMetrics->linePitch() / 2);

				// On Win32, oleidl.h defines the value named DD_DEFSCROLLINSET, but...

				graphics::font::TextViewportSignedScrollOffset dx = 0, dy = 0;
				if(includes(boost::irange(geometry::top(localBounds), geometry::top(inset)), geometry::y(p)))
					dy = -1;
				else if(includes(boost::irange(geometry::bottom(localBounds), geometry::bottom(inset)), geometry::y(p)))
					dy = +1;
				if(includes(boost::irange(geometry::left(localBounds), geometry::left(inset)), geometry::x(p)))
					dx = -3;
				else if(includes(boost::irange(geometry::right(localBounds), geometry::right(inset)), geometry::y(p)))
					dx = +3;
				return graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset>(dx, dy);
			}
		}

		/// @see DropTarget#dragMoved
		void DefaultTextAreaMouseInputStrategy::dragMoved(widgetapi::DragMoveInput& input) {
			widgetapi::DropAction dropAction = widgetapi::DROP_ACTION_IGNORE;
			bool acceptable = false;
			const graphics::font::TextViewportNotificationLocker lock(viewer_->textArea().textRenderer().viewport().get());

			if((dragAndDrop_ != boost::none)
					&& !viewer_->document().isReadOnly() && viewer_->allowsMouseInput()) {
				const graphics::Point caretPoint(widgetapi::mapFromGlobal(*viewer_, input.location()));
				const kernel::Position p(viewToModel(*viewer_->textArea().textRenderer().viewport(), caretPoint).characterIndex());
//				viewer_->setCaretPosition(viewer_->localPointForCharacter(p, true, TextLayout::LEADING));

				// drop rectangle text into bidirectional line is not supported...
				if(dragAndDrop_->numberOfRectangleLines == 0)
					acceptable = true;
				else {
					const Index lines = std::min(viewer_->document().numberOfLines(), p.line + dragAndDrop_->numberOfRectangleLines);
					bool bidirectional = false;
					for(Index line = p.line; line < lines; ++line) {
						if(viewer_->textArea().textRenderer().layouts()[line].isBidirectional()) {
							bidirectional = true;
							break;
						}
					}
					acceptable = !bidirectional;
				}
			}

			if(acceptable) {
				dropAction = input.hasModifier(widgetapi::event::UserInput::CONTROL_DOWN) ? widgetapi::DROP_ACTION_COPY : widgetapi::DROP_ACTION_MOVE;
				const graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset> scrollOffset(calculateDnDScrollOffset(*viewer_));
				if(scrollOffset.x() != 0 || scrollOffset.y() != 0) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					dropAction |= widgetapi::DROP_ACTION_WIN32_SCROLL;
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					// only one direction to scroll
					if(scrollOffset.x() != 0)
						viewer_->textArea().textRenderer().viewport()->scroll(
							graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset>(0, scrollOffset.y()));
					else
						viewer_->textArea().textRenderer().viewport()->scroll(
							graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset>(scrollOffset.x(), 0));
				}
			}
			input.setDropAction(dropAction);
			input.consume();
		}

		/// @see DropTarget#drop
		void DefaultTextAreaMouseInputStrategy::dropped(widgetapi::DropInput& input) {
			kernel::Document& document = viewer_->document();
			input.setDropAction(widgetapi::DROP_ACTION_IGNORE);
			if(/*dnd_.supportLevel == DONT_SUPPORT_DND ||*/ dragAndDrop_ == boost::none || document.isReadOnly() || !viewer_->allowsMouseInput())
				return input.ignore();

			Caret& caret = viewer_->caret();
			const std::shared_ptr<graphics::font::TextViewport> viewport(viewer_->textArea().textRenderer().viewport());
			const graphics::font::TextViewportNotificationLocker lock(viewport.get());
			const graphics::Point caretPoint(input.location());
			const kernel::Position destination(graphics::font::viewToModel(*viewport, caretPoint).characterIndex());

			if(!document.accessibleRegion().includes(destination))
				return input.ignore();

			if(dragAndDrop_->state == DragAndDrop::PROCESSING_AS_TARGET) {	// dropped from the other widget
				endLocationTracking();
				if((input.possibleActions() & widgetapi::DROP_ACTION_COPY) != 0) {
					caret.moveTo(destination);

					std::pair<String, bool> content;
					bool failed = false;
					try {
						content = utils::getTextFromMimeData(*input.mimeData());
					} catch(...) {
						failed = true;
					}
					if(!failed) {
						AutoFreeze af(viewer_);
						try {
							caret.replaceSelection(content.first, content.second);
						} catch(...) {
							failed = true;
						}
						if(!failed) {
							if(content.second)
								caret.beginRectangleSelection();
							caret.select(destination, caret);
							input.setDropAction(widgetapi::DROP_ACTION_COPY);
						}
					}
				}
				dragAndDrop_ = boost::none;
			} else {	// drop from the same widget
				assert(dragAndDrop_->state == DragAndDrop::PROCESSING_AS_TARGET);
				String text(selectedString(caret, text::Newline::USE_INTRINSIC_VALUE));

				// can't drop into the selection
				if(isPointOverSelection(caret, caretPoint)) {
					caret.moveTo(destination);
					dragAndDrop_ = boost::none;
				} else {
					const bool rectangle = caret.isSelectionRectangle();
					bool failed = false;
					if(input.hasModifier(widgetapi::event::UserInput::CONTROL_DOWN)) {	// copy
						if((input.possibleActions() & widgetapi::DROP_ACTION_COPY) != 0) {
							document.insertUndoBoundary();
							AutoFreeze af(viewer_);
//							viewer_->redrawLines(ca.beginning().line(), ca.end().line());
							caret.enableAutoShow(false);
							caret.moveTo(destination);
							try {
								caret.replaceSelection(text, rectangle);
							} catch(...) {
								failed = true;
							}
							caret.enableAutoShow(true);
							if(!failed) {
								caret.select(destination, caret);
								input.setDropAction(widgetapi::DROP_ACTION_COPY);
							}
							document.insertUndoBoundary();
						}
					} else {	// move as a rectangle or linear
						if((input.possibleActions() & widgetapi::DROP_ACTION_MOVE) != 0) {
							document.insertUndoBoundary();
							AutoFreeze af(viewer_);
							std::pair<kernel::Point, kernel::Point> oldSelection(std::make_pair(kernel::Point(caret.anchor()), kernel::Point(caret)));
							caret.enableAutoShow(false);
							caret.moveTo(destination);
							try {
								caret.replaceSelection(text, rectangle);
							} catch(...) {
								failed = true;
							}
							if(!failed) {
								caret.select(destination, caret);
								if(rectangle)
									caret.beginRectangleSelection();
								try {
									erase(caret.document(), oldSelection.first, oldSelection.second);
								} catch(...) {
									failed = true;
								}
							}
							caret.enableAutoShow(true);
							if(!failed)
								input.setDropAction(widgetapi::DROP_ACTION_MOVE);
							document.insertUndoBoundary();
						}
					}

					// dragAndDrop_ will be cleared by 'beginDragAndDrop' method immediately.
				}
			}
		}

		/**
		 * Ends the auto scroll.
		 * @return true if the auto scroll was active
		 */
		bool DefaultTextAreaMouseInputStrategy::endAutoScroll() {
			if(autoScroll_ != boost::none) {
				if(autoScroll_->state == AutoScroll::SCROLLING_WITH_DRAG || autoScroll_->state == AutoScroll::SCROLLING_WITHOUT_DRAG) {
					timer_.stop();
					autoScroll_ = boost::none;
					widgetapi::hide(*autoScrollOriginMark_);
					widgetapi::releaseInput(*viewer_);
					return true;
				}
			}
			return false;
		}

		/// Extends the selection to the current cursor position.
		void DefaultTextAreaMouseInputStrategy::continueSelectionExtension(const kernel::Position& to) {
			if(selectionExtender_.get() == nullptr)
				throw IllegalStateException("not extending the selection.");
			selectionExtender_->continueSelection(viewer_->caret(), to);
		}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		/// @see IDropSource#GiveFeedback
		STDMETHODIMP DefaultTextAreaMouseInputStrategy::GiveFeedback(DWORD) {
			return DRAGDROP_S_USEDEFAULTCURSORS;	// use the system default cursor
		}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

		/// @see MouseInputStrategy#handleDropTarget
		std::shared_ptr<widgetapi::DropTarget> DefaultTextAreaMouseInputStrategy::handleDropTarget() const {
			const widgetapi::DropTarget* const self = this;
			return std::shared_ptr<widgetapi::DropTarget>(const_cast<widgetapi::DropTarget*>(self), boost::null_deleter());
		}

		/**
		 * Handles double click action of the left button.
		 * @param input The input event. Call @c Event#consume() if processed the input; In this case, the original
		 *              behavior of @c DefaultTextAreaMouseInputStrategy is suppressed. The default implementation ignores this
		 */
		void DefaultTextAreaMouseInputStrategy::handleLeftButtonDoubleClick(widgetapi::event::MouseButtonInput& input) {
			return input.ignore();
		}

		/// @internal
		void DefaultTextAreaMouseInputStrategy::handleLeftButtonPressed(widgetapi::event::MouseButtonInput& input, TargetLocker& targetLocker) {
			Caret& caret = viewer_->caret();
			utils::closeCompletionProposalsPopup(*viewer_);
			utils::endIncrementalSearch(*viewer_);

			if(!isStateNeutral())
				return interruptMouseReaction(false);

			bool boxDragging = false;

			// approach drag-and-drop
			if(/*dnd_.supportLevel >= SUPPORT_DND &&*/ !isSelectionEmpty(caret) && isPointOverSelection(caret, input.location())) {
				dragAndDrop_ = DragAndDrop();
				dragAndDrop_->state = DragAndDrop::APPROACHING;
				dragAndDrop_->approachedPosition = input.location();
				if(caret.isSelectionRectangle())
					boxDragging = true;
			}

			else {
				// try hyperlink
				bool hyperlinkInvoked = false;
				if(input.hasModifier(widgetapi::event::UserInput::CONTROL_DOWN)) {
					if(!isPointOverSelection(caret, input.location())) {
						if(const boost::optional<graphics::font::TextHit<kernel::Position>> p =
								viewToModelInBounds(*viewer_->textArea().textRenderer().viewport(), input.location())) {
							if(const presentation::hyperlink::Hyperlink* link = utils::getPointedHyperlink(*viewer_, p->characterIndex())) {
								link->invoke();
								hyperlinkInvoked = true;
							}
						}
					}
				}

				if(!hyperlinkInvoked) {
					// modification keys and result
					//
					// shift => keep the anchor and move the caret to the cursor position
					// ctrl  => begin word selection
					// alt   => begin rectangle selection
					if(const boost::optional<graphics::font::TextHit<kernel::Position>> to =
							viewToModelInBounds(*viewer_->textArea().textRenderer().viewport(), input.location())) {
						if(input.hasModifier(widgetapi::event::UserInput::CONTROL_DOWN | widgetapi::event::UserInput::SHIFT_DOWN)) {
							const bool shift = input.hasModifier(widgetapi::event::UserInput::SHIFT_DOWN);
							if(input.hasModifier(widgetapi::event::UserInput::CONTROL_DOWN)) {	// begin word selection
								if(shift)
									selectionExtender_.reset(new WordSelectionExtender(caret, &caret.anchor().position()));
								else
									selectionExtender_.reset(new WordSelectionExtender(caret, &to->characterIndex()));
							}
							if(shift)
								selectionExtender_->continueSelection(caret, to->characterIndex());
						}
						if(selectionExtender_.get() == nullptr)
							selectionExtender_.reset(new CharacterSelectionExtender(caret, to->characterIndex()));
						if(input.hasModifier(widgetapi::event::UserInput::ALT_DOWN))	// make the selection reactangle
							caret.beginRectangleSelection();
						else
							caret.endRectangleSelection();
						beginLocationTracking(*viewer_, &targetLocker, true, true);
					}
				}
			}

//			if(!caret.isSelectionRectangle() && !boxDragging)
//				viewer_->redrawLine(caret.line());
			widgetapi::setFocus(*viewer_);

			return input.consume();
		}

		/// @internal
		void DefaultTextAreaMouseInputStrategy::handleLeftButtonReleased(widgetapi::event::MouseButtonInput& input) {
			// cancel if drag-and-drop approaching
			if(/*dnd_.supportLevel >= SUPPORT_DND &&*/ dragAndDrop_ != boost::none
					&& (dragAndDrop_->state == DragAndDrop::APPROACHING || dragAndDrop_->state == DragAndDrop::PROCESSING_AS_SOURCE)) {
				// TODO: Should this handle only case APPROACHING_DND?
				dragAndDrop_ = boost::none;
				viewer_->caret().moveTo(viewToModel(*viewer_->textArea().textRenderer().viewport(), input.location()).characterIndex());
				::SetCursor(::LoadCursor(nullptr, IDC_IBEAM));	// hmm...
			}

			endLocationTracking();
			timer_.stop();
			if(selectionExtender_ != nullptr) {
				selectionExtender_.reset();
				// if released the button when extending the selection, the scroll may not reach the caret position
				utils::show(viewer_->caret());
			}
			widgetapi::releaseInput(*viewer_);

			return input.consume();
		}

		/**
		 * Handles mouse right button input.
		 * @param action Same as @c MouseInputStrategy#mouseButtonInput
		 * @param input The input event. Call @c Event#consume() if processed the input; In this case, the original
		 *              behavior of @c DefaultTextAreaMouseInputStrategy is suppressed. The default implementation
		 *              ignores this
		 */
		void DefaultTextAreaMouseInputStrategy::handleRightButton(Action action, widgetapi::event::MouseButtonInput& input) {
			return input.ignore();
		}

		/**
		 * Handles mouse first X1 button input.
		 * @param action Same as @c MouseInputStrategy#mouseButtonInput
		 * @param input The input event. Call @c Event#consume() if processed the input; In this case, the original
		 *              behavior of @c DefaultTextAreaMouseInputStrategy is suppressed. The default implementation
		 *              ignores this
		 */
		void DefaultTextAreaMouseInputStrategy::handleX1Button(Action action, widgetapi::event::MouseButtonInput& input) {
			return input.ignore();
		}

		/**
		 * Handles mouse first X2 button input.
		 * @param action Same as @c MouseInputStrategy#mouseButtonInput
		 * @param input The input event. Call @c Event#consume() if processed the input; In this case, the original
		 *              behavior of @c DefaultTextAreaMouseInputStrategy is suppressed. The default implementation
		 *              ignores this
		 */
		void DefaultTextAreaMouseInputStrategy::handleX2Button(Action action, widgetapi::event::MouseButtonInput& input) {
			return input.ignore();
		}

		/// @see TextViewerMouseInputStrategy#install
		void DefaultTextAreaMouseInputStrategy::install(TextViewer& viewer) {
			if(viewer_ != nullptr)
				uninstall();
			viewer_ = &viewer;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			if(dnd_.dragSourceHelper.get() == nullptr)
				dnd_.dragSourceHelper = win32::com::SmartPointer<IDragSourceHelper>::create(CLSID_DragDropHelper, IID_IDragSourceHelper, CLSCTX_INPROC_SERVER);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			interruptMouseReaction(false);

			// create the window for the auto scroll origin mark
			autoScrollOriginMark_.reset(new AutoScrollOriginMark(*viewer_));
		}

		/// @see MouseInputStrategy#interruptMouseReaction
		void DefaultTextAreaMouseInputStrategy::interruptMouseReaction(bool forKeyboardInput) {
			timer_.stop();
			endLocationTracking();
			selectionExtender_.reset();
			endAutoScroll();
		}

		/// @internal Returns @c true if this is processing nothing.
		inline bool DefaultTextAreaMouseInputStrategy::isStateNeutral() const BOOST_NOEXCEPT {
			return selectionExtender_.get() == nullptr && autoScroll_ == boost::none && dragAndDrop_ == boost::none;
		}

		/// @see MouseInputStrategy#mouseButtonInput
		void DefaultTextAreaMouseInputStrategy::mouseButtonInput(Action action, widgetapi::event::MouseButtonInput& input, TargetLocker& targetLocker) {
			if(viewer_ == nullptr)
				return input.ignore();
			if(action != RELEASED && endAutoScroll())
				return input.consume();

			switch(input.button()) {
				case widgetapi::event::LocatedUserInput::BUTTON1_DOWN:
					if(action == PRESSED)
						handleLeftButtonPressed(input, targetLocker);
					else if(action == RELEASED)
						handleLeftButtonReleased(input);
					else if(action == DOUBLE_CLICKED) {
						utils::abortIncrementalSearch(*viewer_);
						handleLeftButtonDoubleClick(input);
						if(!input.isConsumed() && isStateNeutral()) {
							// begin word selection
							selectionExtender_.reset(new WordSelectionExtender(viewer_->caret()));
							beginLocationTracking(*viewer_, &targetLocker, true, true);
							input.consume();
						}
					}
					break;
				case widgetapi::event::LocatedUserInput::BUTTON2_DOWN:
					if(action == PRESSED && isStateNeutral()) {
						if(viewer_->document().numberOfLines() > viewer_->textArea().textRenderer().viewport()->numberOfVisibleLines()) {
							autoScroll_ = AutoScroll();
							autoScroll_->state = AutoScroll::APPROACHING;
							autoScroll_->approachedPosition = input.location();
							const graphics::Point p(widgetapi::mapToGlobal(*viewer_, input.location()));
							widgetapi::setFocus(*viewer_);
							// show the indicator margin
							graphics::Rectangle rect(widgetapi::bounds(*autoScrollOriginMark_, true));
							widgetapi::move(
								widgetapi::window(*autoScrollOriginMark_),
								graphics::Point(
									graphics::geometry::_x = graphics::geometry::x(p) - graphics::geometry::dx(rect) / 2,
									graphics::geometry::_y = graphics::geometry::y(p) - graphics::geometry::dy(rect) / 2));
							widgetapi::show(*autoScrollOriginMark_);
							widgetapi::raise(widgetapi::window(*autoScrollOriginMark_));
							beginLocationTracking(*viewer_, &targetLocker, false, false);
							showCursor(input.location());
							input.consume();
						}
					} else if(action == RELEASED) {
						if(autoScroll_ != boost::none) {
							if(autoScroll_->state == AutoScroll::APPROACHING) {
								autoScroll_->state = AutoScroll::SCROLLING_WITHOUT_DRAG;
								timer_.start(boost::chrono::milliseconds::zero(), *this);
							} else if(autoScroll_->state == AutoScroll::SCROLLING_WITH_DRAG)
								endAutoScroll();
						}
					}
					break;
				case widgetapi::event::LocatedUserInput::BUTTON3_DOWN:
					handleRightButton(action, input);
					break;
				case widgetapi::event::LocatedUserInput::BUTTON4_DOWN:
					handleX1Button(action, input);
					break;
				case widgetapi::event::LocatedUserInput::BUTTON5_DOWN:
					handleX2Button(action, input);
					break;
			}
		}

		/// @see MouseInputStrategy#mouseInputTargetUnlocked
		void DefaultTextAreaMouseInputStrategy::mouseInputTargetUnlocked() {
			interruptMouseReaction(false);
		}

		/// @see MouseInputStrategy#mouseMoved
		void DefaultTextAreaMouseInputStrategy::mouseMoved(widgetapi::event::LocatedUserInput& input, TargetLocker&) {
			if((autoScroll_ != boost::none && autoScroll_->state == AutoScroll::APPROACHING)
					|| (/*dnd_.supportLevel >= SUPPORT_DND &&*/ dragAndDrop_ != boost::none && dragAndDrop_->state == DragAndDrop::APPROACHING)) {	// dragging starts?
				if(dragAndDrop_ != boost::none && isSelectionEmpty(viewer_->caret())) {
					dragAndDrop_ = boost::none;	// approaching... => cancel
					input.consume();
				} else {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					// the following code can be replaced with DragDetect in user32.lib
					const int cxDragBox = ::GetSystemMetrics(SM_CXDRAG);
					const int cyDragBox = ::GetSystemMetrics(SM_CYDRAG);
					if((graphics::geometry::x(input.location()) > graphics::geometry::x(dragApproachedPosition_) + cxDragBox / 2)
							|| (graphics::geometry::x(input.location()) < graphics::geometry::x(dragApproachedPosition_) - cxDragBox / 2)
							|| (graphics::geometry::y(input.location()) > graphics::geometry::y(dragApproachedPosition_) + cyDragBox / 2)
							|| (graphics::geometry::y(input.location()) < graphics::geometry::y(dragApproachedPosition_) - cyDragBox / 2)) {
						if(dragAndDrop_ != boost::none)
							beginDragAndDrop(input);
						else {
							autoScroll_->state = AutoScroll::SCROLLING_WITH_DRAG;
							autoScroll_->timer.start(boost::chrono::milliseconds::zero(), *this);
						}
					}
					input.consume();
#endif
				}
			} else if(selectionExtender_.get() != nullptr) {
				assert(isTrackingLocation());
				input.consume();
			}
		}

		/// @see MouseInputStrategy#mouseWheelRotated
		void DefaultTextAreaMouseInputStrategy::mouseWheelRotated(widgetapi::event::MouseWheelInput& input, TargetLocker&) {
			if(!endAutoScroll()) {
				const std::shared_ptr<graphics::font::TextViewport> viewport(viewer_->textArea().textRenderer().viewport());
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && 0
				// use system settings
				UINT lines;	// the number of lines to scroll
				if(!win32::boole(::SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0)))
					lines = 3;
				if(lines == WHEEL_PAGESCROLL) {
					// TODO: calculate precise page size.
					lines = static_cast<UINT>(viewport->numberOfVisibleLines());
				}
				viewport->scroll(graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(
					0, static_cast<graphics::font::TextViewport::SignedScrollOffset>(-graphics::geometry::dy(input.rotation()) * static_cast<short>(lines) / WHEEL_DELTA)));
#else
				if(input.scrollType() == widgetapi::event::MouseWheelInput::WHEEL_UNIT_SCROLL) {
					const auto units(input.unitsToScroll());
					assert(units != boost::none);
					const graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset> offsets(
						graphics::_x = static_cast<graphics::font::TextViewportSignedScrollOffset>(graphics::geometry::dx(*units)),
						graphics::_y = static_cast<graphics::font::TextViewportSignedScrollOffset>(graphics::geometry::dy(*units)));
					viewport->scroll(offsets);
					input.consume();
				} else if(input.scrollType() == widgetapi::event::MouseWheelInput::WHEEL_BLOCK_SCROLL) {
					const graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset> physicalPages(
						graphics::_x = static_cast<graphics::font::TextViewportSignedScrollOffset>(graphics::geometry::dx(input.wheelRotation())),
						graphics::_y = static_cast<graphics::font::TextViewportSignedScrollOffset>(graphics::geometry::dy(input.wheelRotation())));
					presentation::FlowRelativeTwoAxes<graphics::font::TextViewportSignedScrollOffset> flowRelativePages(
						presentation::mapPhysicalToFlowRelative(viewer_->textArea().textRenderer().presentation().computeWritingMode(), physicalPages));
					if(flowRelativePages.bpd() != 0) {
						viewport->scrollBlockFlowPage(flowRelativePages.bpd());
						flowRelativePages.bpd() = 0;
					}
					viewport->scroll(flowRelativePages);
					input.consume();
				}
#endif
			}
		}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		/// Implements @c IDropSource#QueryContinueDrag method.
		STDMETHODIMP DefaultTextAreaMouseInputStrategy::QueryContinueDrag(BOOL escapePressed, DWORD keyState) {
			if(win32::boole(escapePressed) || win32::boole(keyState & MK_RBUTTON))	// cancel
				return DRAGDROP_S_CANCEL;
			if(!win32::boole(keyState & MK_LBUTTON))	// drop
				return DRAGDROP_S_DROP;
			return S_OK;
		}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

		/// @see MouseInputStrategy#showCursor
		bool DefaultTextAreaMouseInputStrategy::showCursor(const graphics::Point& position) {
			boost::optional<widgetapi::Cursor::BuiltinShape> builtinShape;
			const presentation::hyperlink::Hyperlink* newlyHoveredHyperlink = nullptr;

			if(/*dnd_.supportLevel >= SUPPORT_DND &&*/ !isSelectionEmpty(viewer_->caret()) && isPointOverSelection(viewer_->caret(), position))	// on a draggable text selection?
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				builtinShape = Gdk::ARROW;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				builtinShape = Qt::ArrowCursor;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				builtinShape = [NSCursor arrowCursor];
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				builtinShape = IDC_ARROW;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			else {
				// on a hyperlink?
				if(const boost::optional<graphics::font::TextHit<kernel::Position>> p =
						viewToModelInBounds(*viewer_->textArea().textRenderer().viewport(), position, kernel::locations::UTF16_CODE_UNIT))
					newlyHoveredHyperlink = utils::getPointedHyperlink(*viewer_, p->characterIndex());
				if(newlyHoveredHyperlink != nullptr && win32::boole(::GetAsyncKeyState(VK_CONTROL) & 0x8000))
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				builtinShape = Gdk::HAND1;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				builtinShape = Qt::PointingHandCursor;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				builtinShape = [NSCursor pointingHandCursor];
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				builtinShape = IDC_HAND;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			}

			if(builtinShape != boost::none) {
				const widgetapi::Cursor cursor(*builtinShape);
				showCursor(*viewer_, cursor);
				return true;
			}
			if(newlyHoveredHyperlink != nullptr) {
				if(newlyHoveredHyperlink != lastHoveredHyperlink_)
					viewer_->showToolTip(newlyHoveredHyperlink->description(), 1000, 30000);
			} else
				viewer_->hideToolTip();
			lastHoveredHyperlink_ = newlyHoveredHyperlink;
			return false;
		}

		/// @see Timer#timeElapsed
		void DefaultTextAreaMouseInputStrategy::timeElapsed(Timer<DefaultTextAreaMouseInputStrategy>& timer) {
			if(viewer_ != nullptr) {
				if(autoScroll_ != boost::none && autoScroll_->state != AutoScroll::APPROACHING) {
					const std::shared_ptr<graphics::font::TextViewport> viewport(viewer_->textArea().textRenderer().viewport());
					timer.stop();
					const graphics::Point p(widgetapi::mapFromGlobal(*viewer_, widgetapi::Cursor::position()));
					graphics::Dimension scrollUnits(
						graphics::geometry::_dx =
							graphics::font::inlineProgressionOffsetInViewerGeometry(*viewport, 1),
						graphics::geometry::_dy =
							widgetapi::createRenderingContext(*viewer_)->fontMetrics(viewer_->textArea().textRenderer().defaultFont())->linePitch());
					if(presentation::isVertical(viewer_->textArea().textRenderer().computedBlockFlowDirection()))
						graphics::geometry::transpose(scrollUnits);
					const graphics::Dimension scrollOffsets(
						graphics::geometry::_dx =
							(graphics::geometry::x(p) - graphics::geometry::x(dragAndDrop_->approachedPosition)) / graphics::geometry::dx(scrollUnits),
						graphics::geometry::_dy =
							(graphics::geometry::y(p) - graphics::geometry::y(dragAndDrop_->approachedPosition)) / graphics::geometry::dy(scrollUnits));
//					const graphics::Scalar scrollDegree = std::max(std::abs(yScrollDegree), std::abs(xScrollDegree));

					if(graphics::geometry::dy(scrollOffsets) != 0 /*&& abs(geometry::dy(scrollOffsets)) >= abs(geometry::dx(scrollOffsets))*/)
						viewport->scroll(
							graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset>(
								0, (graphics::geometry::dy(scrollOffsets) > 0) ? +1 : -1));
//					else if(graphics::geometry::dx(scrollOffsets) != 0)
//						viewport->scroll(
//							graphics::PhysicalTwoAxes<graphics::font::TextViewport::SignedScrollOffset>(
//								(graphics::geometry::dx(scrollOffsets) > 0) ? +1 : -1, 0));

					if(graphics::geometry::dy(scrollOffsets) != 0) {
						const boost::chrono::milliseconds interval(
							500 / static_cast<unsigned int>((std::pow(2.0f, abs(graphics::geometry::dy(scrollOffsets)) / 2))));
						timer.start(interval, *this);
						const widgetapi::Cursor& cursor = AutoScrollOriginMark::cursorForScrolling(
							(graphics::geometry::dy(scrollOffsets) > 0) ? AutoScrollOriginMark::CURSOR_DOWNWARD : AutoScrollOriginMark::CURSOR_UPWARD);
						showCursor(*viewer_, cursor);
					} else {
						timer.start(boost::chrono::milliseconds(300), *this);
						const widgetapi::Cursor& cursor = AutoScrollOriginMark::cursorForScrolling(AutoScrollOriginMark::CURSOR_NEUTRAL);
						showCursor(*viewer_, cursor);
					}
#if 0
				} else if(self.dnd_.enabled && (self.state_ & DND_MASK) == DND_MASK) {	// scroll automatically during dragging
					const SIZE scrollOffset = calculateDnDScrollOffset(*self.viewer_);
					if(scrollOffset.cy != 0)
						self.viewer_->scroll(0, scrollOffset.cy, true);
					else if(scrollOffset.cx != 0)
						self.viewer_->scroll(scrollOffset.cx, 0, true);
#endif
				}
			}
		}

		/// @see TextViewerMouseInputStrategy#uninstall
		void DefaultTextAreaMouseInputStrategy::uninstall() {
			interruptMouseReaction(false);
			if(autoScrollOriginMark_.get() != nullptr)
				autoScrollOriginMark_.reset();
			viewer_ = nullptr;
		}


		// window system-dependent implementations ////////////////////////////////////////////////////////////////////

#if defined(ASCENSION_WINDOWS_SYSTEM_GTK)
#elif defined(ASCENSION_WINDOWS_SYSTEM_QT)
#elif defined(ASCENSION_WINDOWS_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOWS_SYSTEM_WIN32)
#endif
	}
}
