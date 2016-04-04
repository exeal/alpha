/**
 * @file text-area.cpp
 * Implements @c TextArea class.
 * @author exeal
 * @date 2015-03-18 Created.
 */

#include <ascension/corelib/numeric-range-algorithm/hull.hpp>
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/graphics/font/baseline-iterator.hpp>
#include <ascension/graphics/font/font-collection.hpp>
#include <ascension/graphics/font/font-render-context.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-corners.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/geometry/algorithms/normalize.hpp>
#include <ascension/graphics/geometry/algorithms/size.hpp>
#include <ascension/graphics/native-conversion.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/log.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-text-area-mouse-input-strategy.hpp>
#include <ascension/viewer/standard-caret-painter.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/virtual-box.hpp>
#include <boost/foreach.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#ifdef _DEBUG
#	include <ascension/log.hpp>
#	include <boost/geometry/io/dsv/write.hpp>
#endif

#ifdef _DEBUG
#	define ASCENSION_REDRAW_TEXT_AREA_LINE(lineNumber)											\
		ASCENSION_LOG_TRIVIAL(debug) << "Requested redraw line: " << lineNumber << std::endl;	\
		redrawLine(lineNumber, false)
#	define ASCENSION_REDRAW_TEXT_AREA_LINES(lineNumbersRange)												\
		ASCENSION_LOG_TRIVIAL(debug)																		\
			<< "Requested redraw lines: ["																	\
			<< *boost::const_begin(lineNumbersRange) << "," << *boost::const_end(lineNumbersRange) << ")"	\
			<< std::endl;																					\
		redrawLines(lineNumbersRange)
#	define ASCENSION_REDRAW_TEXT_AREA_LINE_AND_FOLLOWINGS(firstLineNumber)	\
		ASCENSION_LOG_TRIVIAL(debug)										\
			<< "Requested redraw line: [" << firstLineNumber << ",..)"		\
			<< std::endl;													\
		redrawLine(firstLineNumber, true)
#else
#	define ASCENSION_REDRAW_TEXT_AREA_LINE(lineNumber)	\
		redrawLine(lineNumber, false)
#	define ASCENSION_REDRAW_TEXT_AREA_LINES(lineNumbersRange)	\
		redrawLines(lineNumbersRange)
#	define ASCENSION_REDRAW_TEXT_AREA_LINE_AND_FOLLOWINGS(firstLineNumber)	\
		redrawLine(firstLineNumber, true)
#endif

namespace ascension {
	namespace viewer {
		namespace {
			inline void redrawIfNotFrozen(TextArea& textArea) {
				if(!textArea.textViewer().isFrozen())
					widgetapi::redrawScheduledRegion(textArea.textViewer());
			}
		}

		/**
		 * @typedef ascension::viewer::TextArea::GeometryChangedSignal
		 * The signal which gets emitted when the allocation- or content-rectangle of the @c TextArea was changed.
		 * @param textArea The @c TextArea
		 * @see #allocationRectangle, #allocationRectangleChangedSignal, #contentRectangle,
		 *      #contentRectangleChangedSignal
		 */

		/// Default constructor.
		TextArea::TextArea() : viewer_(nullptr), locator_(nullptr),
				linesToRedraw_(boost::irange<Index>(0, 0)), mouseInputStrategyIsInstalled_(false) {
		}

		/// Destructor.
		TextArea::~TextArea() BOOST_NOEXCEPT {
			if(viewer_ != nullptr)
				uninstall(*viewer_);
		}

		/**
		 * Returns the 'allocation-rectangle' of the text area, in viewer-coordinates.
		 * @see #contentRectangle, #allocationRectangleChangedSignal
		 */
		graphics::Rectangle TextArea::allocationRectangle() const BOOST_NOEXCEPT {
			if(viewer_ == nullptr || locator_ == nullptr)
				return boost::geometry::make_zero<graphics::Rectangle>();
			graphics::Rectangle requested(locator_->locateComponent(*this)), temp;
			const bool b = boost::geometry::intersection(graphics::geometry::normalize(requested), widgetapi::bounds(*viewer_, false), temp);
			return temp;
		}

		/**
		 * Returns the @c GeometryChangedSignal signal connector for the 'allocation-rectangle'.
		 * @see #allocationRectangle
		 */
		SignalConnector<TextArea::GeometryChangedSignal> TextArea::allocationRectangleChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(allocationRectangleChangedSignal_);
		}

		/// @see Caret#MotionSignal
		void TextArea::caretMoved(const Caret& self, const SelectedRegion& regionBeforeMotion) {
			if(viewer_ == nullptr || !widgetapi::isVisible(textViewer()))
				return;

			// redraw the selected region
			const kernel::Region newRegion(self.selectedRegion());
			boost::optional<boost::integer_range<Index>> linesToRedraw;
			if(self.isSelectionRectangle()) {	// rectangle
				if(!boost::empty(regionBeforeMotion))
					linesToRedraw = regionBeforeMotion.lines();
				if(!boost::empty(newRegion))
					linesToRedraw = newRegion.lines();
			} else if(newRegion != regionBeforeMotion) {	// the selection actually changed
				if(boost::empty(regionBeforeMotion)) {	// the selection was empty...
					if(!boost::empty(newRegion))	// the selection is not empty now
						linesToRedraw = newRegion.lines();
				} else {	// ...if the there is selection
					if(boost::empty(newRegion))	// the selection became empty
						linesToRedraw = regionBeforeMotion.lines();
					else if(*boost::const_begin(regionBeforeMotion) == *boost::const_begin(newRegion)) {	// the beginning point didn't change
						const Index i[2] = {kernel::line(*boost::const_end(regionBeforeMotion)), kernel::line(*boost::const_end(newRegion))};
						linesToRedraw = boost::irange(std::min(i[0], i[1]), std::max(i[0], i[1]) + 1);
					} else if(*boost::const_end(regionBeforeMotion) == *boost::const_end(newRegion)) {	// the end point didn't change
						const Index i[2] = {kernel::line(*boost::const_begin(regionBeforeMotion)), kernel::line(*boost::const_begin(newRegion))};
						linesToRedraw = boost::irange(std::min(i[0], i[1]), std::max(i[0], i[1]) + 1);
					} else {	// the both points changed
						if((kernel::line(*boost::const_begin(regionBeforeMotion)) >= kernel::line(*boost::const_begin(newRegion))
								&& kernel::line(*boost::const_begin(regionBeforeMotion)) <= kernel::line(*boost::const_end(newRegion)))
								|| (kernel::line(*boost::const_end(regionBeforeMotion)) >= kernel::line(*boost::const_begin(newRegion))
								&& kernel::line(*boost::const_end(regionBeforeMotion)) <= kernel::line(*boost::const_end(newRegion)))) {
							const Index i[2] = {
								std::min(kernel::line(*boost::const_begin(regionBeforeMotion)), kernel::line(*boost::const_begin(newRegion))),
								std::max(kernel::line(*boost::const_end(regionBeforeMotion)), kernel::line(*boost::const_end(newRegion)))
							};
							linesToRedraw = boost::irange(std::min(i[0], i[1]), std::max(i[0], i[1]) + 1);
						} else {
							ASCENSION_REDRAW_TEXT_AREA_LINES(regionBeforeMotion.lines());
							redrawIfNotFrozen(*this);
							linesToRedraw = newRegion.lines();
						}
					}
				}
			}

			if(linesToRedraw != boost::none) {
				ASCENSION_REDRAW_TEXT_AREA_LINES(boost::get(linesToRedraw));
				redrawIfNotFrozen(*this);
			}
		}

		/**
		 * Returns the 'content-rectangle' of the text area, in viewer-coordinates.
		 * @see #allocationRectangle, #contentRectangleChangedSignal
		 */
		graphics::Rectangle TextArea::contentRectangle() const BOOST_NOEXCEPT {
			// TODO: Consider 'padding-start' setting.
			return allocationRectangle();
		}

		/**
		 * Returns the @c GeometryChangedSignal signal connector for the 'content-rectangle'.
		 * @see #contentRectangle
		 */
		SignalConnector<TextArea::GeometryChangedSignal> TextArea::contentRectangleChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(contentRectangleChangedSignal_);
		}

		/// @see TextRenderer#DefaultFontChangedSignal
		void TextArea::defaultFontChanged(const graphics::font::TextRenderer& textRenderer) {
			if(&textRenderer == renderer_.get()) {
#ifdef ASCENSION_USE_SYSTEM_CARET
				caret().resetVisualization();
#endif
				ASCENSION_REDRAW_TEXT_AREA_LINE_AND_FOLLOWINGS(0);
			}
		}

		/// @see kernel#DocumentListener#documentAboutToBeChanged
		void TextArea::documentAboutToBeChanged(const kernel::Document&) {
			// does nothing
		}

		/// @see kernel#DocumentListener#documentChanged
		void TextArea::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
			if(viewer_ != nullptr) {
				// slide the frozen lines to be drawn
				if(textViewer().isFrozen() && !boost::empty(linesToRedraw_)) {
					Index b = *boost::const_begin(linesToRedraw_);
					Index e = *boost::const_end(linesToRedraw_);
					if(boost::size(change.erasedRegion().lines()) > 1) {
						const auto range(boost::irange(
							kernel::line(*boost::const_begin(change.erasedRegion())) + 1, kernel::line(*boost::const_end(change.erasedRegion()))));
						if(b > *boost::const_end(range))
							b -= boost::size(range) + 1;
						else if(b > *boost::const_begin(range))
							b = *boost::const_begin(range);
						if(e != std::numeric_limits<Index>::max()) {
							if(e > *boost::const_end(range))
								e -= boost::size(range) + 1;
							else if(e > *boost::const_begin(range))
								e = *boost::const_begin(range);
						}
					}
					if(boost::size(change.insertedRegion().lines()) > 1) {
						const auto range(boost::irange(
							kernel::line(*boost::const_begin(change.insertedRegion())) + 1, kernel::line(*boost::const_end(change.insertedRegion()))));
						if(b >= *boost::const_begin(range))
							b += boost::size(range) + 1;
						if(e >= *boost::const_begin(range) && e != std::numeric_limits<Index>::max())
							e += boost::size(range) + 1;
					}
					linesToRedraw_ = boost::irange(b, e);
				}
//				ASCENSION_REDRAW_TEXT_AREA_LINES(kernel::line(*boost::const_begin(region)), !multiLine ? kernel::line(*boost::const_end(region)) : INVALID_INDEX);
			}
		}

		/// @see graphics#font#LineRenderingOptions#endOfLine
		std::unique_ptr<const graphics::font::InlineObject> TextArea::endOfLine(Index line) const BOOST_NOEXCEPT {
			return std::unique_ptr<const graphics::font::InlineObject>();
		}

		/// @see TextViewer#FocusChangedSignal
		void TextArea::focusChanged(const TextViewer& viewer) {
			if(&viewer == &textViewer()) {
				// repaint the lines where the caret places
//				ASCENSION_REDRAW_TEXT_AREA_LINES(boost::irange(kernel::line(caret().beginning()), kernel::line(caret().end()) + 1));
//				redrawIfNotFrozen(*this);
			}
		}

		/// @see TextViewer#FrozenStateChangedSignal
		void TextArea::frozenStateChanged(const TextViewer& viewer) {
			if(&viewer == &textViewer()) {
				if(const std::shared_ptr<graphics::font::TextViewport> viewport = textRenderer().viewport()) {
					if(viewer.isFrozen()) {
						try {
							viewport->freezeNotification();
						} catch(const std::overflow_error&) {
							// ignore
						}
					} else {
						try {
							viewport->thawNotification();
						} catch(const std::underflow_error&) {
							// ignore
						}
						if(!linesToRedraw_.empty()) {
							ASCENSION_REDRAW_TEXT_AREA_LINES(linesToRedraw_);
							linesToRedraw_ = boost::irange<Index>(0, 0);
						}
						caretMoved(caret(), caret().selectedRegion());
						widgetapi::redrawScheduledRegion(textViewer());
					}
				}
			}
		}

		/**
		 * Hides the caret.
		 * @see #hidesCaret, #showCaret
		 */
		void TextArea::hideCaret() BOOST_NOEXCEPT {
			if(caretPainter_.get() != nullptr)
				static_cast<detail::CaretPainterBase&>(*caretPainter_).hide();
		}

		/// @see TextViewerComponent#install
		void TextArea::install(TextViewer& viewer, const Locator& locator) {
			viewer_ = &viewer;
			locator_ = &locator;
			caret_.reset(new Caret(viewer.document()));
			renderer_.reset(new Renderer(viewer));
			caret().install(*this);
			textViewer().document().addListener(*this);
			viewerFocusChangedConnection_ = textViewer().focusChangedSignal().connect(
				std::bind(&TextArea::focusChanged, this, std::placeholders::_1));
			viewerFrozenStateChangedConnection_ = textViewer().frozenStateChangedSignal().connect(
				std::bind(&TextArea::frozenStateChanged, this, std::placeholders::_1));
			caretMotionConnection_ = caret().motionSignal().connect(
				std::bind(&TextArea::caretMoved, this, std::placeholders::_1, std::placeholders::_2));
			selectionShapeChangedConnection_ = caret().selectionShapeChangedSignal().connect(
				std::bind(&TextArea::selectionShapeChanged, this, std::placeholders::_1));
			matchBracketsChangedConnection_ = caret().matchBracketsChangedSignal().connect(
				std::bind(&TextArea::matchBracketsChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			defaultFontChangedConnection_ =
				renderer_->defaultFontChangedSignal().connect(std::bind(&TextArea::defaultFontChanged, this, std::placeholders::_1));
			viewportResizedConnection_ =
				renderer_->viewport()->resizedSignal().connect(std::bind(&TextArea::viewportResized, this, std::placeholders::_1));
			viewportScrolledConnection_ =
				renderer_->viewport()->scrolledSignal().connect(std::bind(&TextArea::viewportScrolled, this, std::placeholders::_1, std::placeholders::_2));
			renderer_->layouts().addVisualLinesListener(*this);
			if(mouseInputStrategy_.get() != nullptr) {
				assert(!mouseInputStrategyIsInstalled_);
				mouseInputStrategy_->install(*this);
			} else
				setMouseInputStrategy(std::unique_ptr<TextAreaMouseInputStrategy>());
			setCaretPainter(std::unique_ptr<CaretPainter>());
			showCaret();
			relocated();
		}

		/// @see Caret#MatchBracketsChangedSignal
		void TextArea::matchBracketsChanged(const Caret& caret,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& previouslyMatchedBrackets, bool outsideOfView) {
			if(viewer_ == nullptr)
				return;
			const boost::optional<std::pair<kernel::Position, kernel::Position>>& newPair = caret.matchBrackets();
			if(newPair != boost::none) {
				ASCENSION_REDRAW_TEXT_AREA_LINE(kernel::line(std::get<0>(boost::get(newPair))));
				redrawIfNotFrozen(*this);
				if(kernel::line(std::get<1>(boost::get(newPair))) != kernel::line(std::get<0>(boost::get(newPair)))) {
					ASCENSION_REDRAW_TEXT_AREA_LINE(kernel::line(std::get<1>(boost::get(newPair))));
					redrawIfNotFrozen(*this);
				}
				if(previouslyMatchedBrackets != boost::none	// clear the previous highlight
						&& kernel::line(std::get<0>(boost::get(previouslyMatchedBrackets))) != kernel::line(std::get<0>(boost::get(newPair)))
						&& kernel::line(std::get<0>(boost::get(previouslyMatchedBrackets))) != kernel::line(std::get<1>(boost::get(newPair)))) {
					ASCENSION_REDRAW_TEXT_AREA_LINE(kernel::line(std::get<0>(boost::get(previouslyMatchedBrackets))));
					redrawIfNotFrozen(*this);
				}
				if(previouslyMatchedBrackets != boost::none
						&& kernel::line(std::get<1>(boost::get(previouslyMatchedBrackets))) != kernel::line(std::get<0>(boost::get(newPair)))
						&& kernel::line(std::get<1>(boost::get(previouslyMatchedBrackets))) != kernel::line(std::get<1>(boost::get(newPair)))
						&& kernel::line(std::get<1>(boost::get(previouslyMatchedBrackets))) != kernel::line(std::get<0>(boost::get(previouslyMatchedBrackets))))
					ASCENSION_REDRAW_TEXT_AREA_LINE(kernel::line(std::get<1>(boost::get(previouslyMatchedBrackets))));
			} else {
				if(previouslyMatchedBrackets != boost::none) {	// clear the previous highlight
					ASCENSION_REDRAW_TEXT_AREA_LINE(kernel::line(std::get<0>(boost::get(previouslyMatchedBrackets))));
					redrawIfNotFrozen(*this);
					if(kernel::line(std::get<1>(boost::get(previouslyMatchedBrackets))) != kernel::line(std::get<0>(boost::get(previouslyMatchedBrackets))))
						ASCENSION_REDRAW_TEXT_AREA_LINE(kernel::line(std::get<1>(boost::get(previouslyMatchedBrackets))));
				}
			}
		}

		/// @see TextViewerComponent#mouseInputStrategy
		std::weak_ptr<MouseInputStrategy> TextArea::mouseInputStrategy() const {
			return std::weak_ptr<MouseInputStrategy>(mouseInputStrategy_);
		}

		namespace {
			std::pair<graphics::Color, graphics::Color> selectionColors(const TextViewer& textViewer) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				const Glib::RefPtr<const Gtk::StyleContext> styleContext(textViewer.get_style_context());
				const Gtk::StateFlags state(textViewer.get_state_flags() | Gtk::STATE_FLAG_SELECTED);
				return std::make_pair(
					graphics::fromNative<graphics::Color>(styleContext->get_color(state)),
					graphics::fromNative<graphics::Color>(styleContext->get_background_color(state)));	// TODO: Gtk.StyleContext.get_background_color is deprecated.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#endif
			}
		}

		/// @see graphics#font#LineRenderingOptions#overrideTextPaint
		void TextArea::overrideTextPaint(Index line, std::vector<const graphics::font::OverriddenSegment>& segments) const BOOST_NOEXCEPT {
			segments.clear();
			if(!isSelectionEmpty(caret())) {
				std::vector<boost::integer_range<Index>> selectedRanges;
				if(!caret().isSelectionRectangle()) {
					const auto range(selectedRangeOnLine(caret(), line));
					if(range != boost::none)
						selectedRanges.push_back(boost::get(range));
				} else {
					const Index nlines = textRenderer().layouts().numberOfSublinesOfLine(line);
					const VirtualBox& selection = caret().boxForRectangleSelection();
					for(graphics::font::VisualLine i(line, 0); i.subline < nlines; ++i.subline) {
						const auto range(selection.characterRangeInVisualLine(i));
						if(range != boost::none)
							selectedRanges.push_back(boost::get(range));
					}
				}

				if(!selectedRanges.empty()) {
					const auto colors(selectionColors(textViewer()));
					graphics::font::OverriddenSegment segment;
#if 0
					segment.foreground = std::make_shared<graphics::SolidColor>(std::get<0>(colors));
#else
					segment.color = boost::make_optional(std::get<0>(colors));
#endif
					segment.background = std::make_shared<graphics::SolidColor>(std::get<1>(colors));
					segment.foregroundAlpha = segment.backgroundAlpha = 1.0;
					segment.usesLogicalHighlightBounds = true;
					BOOST_FOREACH(const auto& selectedRange, selectedRanges) {
						segment.range = selectedRange;
						segments.push_back(segment);
					}
				}
			}

			// TODO: Highlight search results.
		}

		/// @see TextViewerComponent#paint
		void TextArea::paint(graphics::PaintContext& context) {
//			Timer tm(L"TextViewer.paint");

			// paint the text area
			const auto cr(contentRectangle()), ar(allocationRectangle());
			const bool narrowed = !boost::geometry::equals(cr, ar);
			if(narrowed) {
				context.save();
				context.translate(graphics::geometry::left(cr) - graphics::geometry::left(ar), graphics::geometry::top(cr) - graphics::geometry::top(ar));
				context.rectangle(graphics::geometry::make<graphics::Rectangle>(boost::geometry::make_zero<graphics::Point>(), graphics::geometry::size(cr))).clip();
			}
			textRenderer().paint(context, this);
			if(narrowed)
				context.restore();

			// paint the caret(s)
			if(const graphics::font::TextLayout* const layout = textRenderer().layouts().at(kernel::line(caret())))
				static_cast<detail::CaretPainterBase&>(*caretPainter_).paintIfShows(
					context, *layout, modelToView(textViewer(), graphics::font::TextHit<kernel::Position>::leading(kernel::Position::bol(caret().position()))));
		}

		/**
		 * Redraws the specified line on the view.
		 * If the viewer is frozen, redraws after unfrozen.
		 * @param line The line to be redrawn
		 * @param following Set @c true to redraw also the all lines follow to @a line
		 */
		void TextArea::redrawLine(Index line, bool following) {
			redrawLines(boost::irange(line, following ? textViewer().document().numberOfLines() : line + 1));
		}

		/**
		 * Redraws the specified lines on the view. If the viewer is frozen, redraws after unfrozen.
		 * @param lines The lines to be redrawn
		 * @throw IndexOutOfBoundsException @a lines intersects outside of the document
		 * @see This method only schedule redrawing, and does not repaint the canvas.
		 */
		void TextArea::redrawLines(const boost::integer_range<Index>& lines) {
//			checkInitialization();

			if(viewer_ == nullptr || lines.empty())
				return;

			const auto orderedLines(lines | adaptors::ordered());
			if(*boost::const_end(orderedLines) > textViewer().document().numberOfLines())
				throw IndexOutOfBoundsException("lines");

			if(textViewer().isFrozen()) {
				linesToRedraw_ = boost::empty(linesToRedraw_) ? orderedLines : hull(orderedLines, linesToRedraw_);
				return;
			}

			if(orderedLines.back() < textRenderer().viewport()->firstVisibleLine().line)
				return;

			using graphics::Scalar;
			const presentation::WritingMode writingMode(textViewer().presentation().computeWritingMode());
			std::array<Scalar, 2> beforeAndAfter;	// in viewport (distances from before-edge of the viewport)
			{
				graphics::font::BaselineIterator baseline(*textRenderer().viewport(), graphics::font::VisualLine(*boost::const_begin(lines), 0), false);
				std::get<0>(beforeAndAfter) = *baseline;
				if(std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::min() && std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::max()) {
					const graphics::font::TextLayout* const layout = textRenderer().layouts().at(baseline.line()->line);
					// TODO: Handle the case if the layout is null, by using the default line metrics.
					assert(layout != nullptr);
					std::get<0>(beforeAndAfter) += *boost::const_begin(layout->lineMetrics(0).extent());
				}
				baseline = graphics::font::BaselineIterator(*textRenderer().viewport(), graphics::font::VisualLine(*boost::const_end(lines) - 1, 0), false);
				std::get<1>(beforeAndAfter) = *baseline;
				if(std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::min() && std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::max()) {
					const graphics::font::TextLayout* const layout = textRenderer().layouts().at(baseline.line()->line);
					// TODO: Handle the case if the layout is null, by using the default line metrics.
					assert(layout != nullptr);
					std::get<1>(beforeAndAfter) += *boost::const_end(layout->lineMetrics(layout->numberOfLines() - 1).extent());
				}

				assert(std::get<0>(beforeAndAfter) <= std::get<1>(beforeAndAfter));
				if(std::get<0>(beforeAndAfter) == std::numeric_limits<Scalar>::max() || std::get<1>(beforeAndAfter) == std::numeric_limits<Scalar>::min())
					return;
			}

			graphics::Rectangle boundsToRedraw(allocationRectangle());
			graphics::Scalar topLeft, bottomRight;
			switch(textRenderer().computedBlockFlowDirection()) {
				case presentation::HORIZONTAL_TB:
					topLeft = graphics::geometry::top(boundsToRedraw);
					if(std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::min())
						topLeft += std::get<0>(beforeAndAfter);
					bottomRight = (std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::max()) ?
						graphics::geometry::top(boundsToRedraw) + std::get<1>(beforeAndAfter) : graphics::geometry::bottom(boundsToRedraw);
					graphics::geometry::range<1>(boundsToRedraw) = nrange(topLeft, bottomRight);
					break;
				case presentation::VERTICAL_RL:
					bottomRight = graphics::geometry::right(boundsToRedraw);
					if(std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::min())
						bottomRight -= std::get<0>(beforeAndAfter);
					topLeft = (std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::max()) ?
						graphics::geometry::right(boundsToRedraw) - std::get<1>(beforeAndAfter) : graphics::geometry::left(boundsToRedraw);
					graphics::geometry::range<1>(boundsToRedraw) = nrange(topLeft, bottomRight);
					break;
				case presentation::VERTICAL_LR:
					topLeft = graphics::geometry::left(boundsToRedraw);
					if(std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::min())
						topLeft += std::get<0>(beforeAndAfter);
					bottomRight = (std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::max()) ?
						graphics::geometry::left(boundsToRedraw) + std::get<1>(beforeAndAfter) : graphics::geometry::right(boundsToRedraw);
					graphics::geometry::range<1>(boundsToRedraw) = nrange(topLeft, bottomRight);
					break;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}

			widgetapi::scheduleRedraw(textViewer(), boundsToRedraw, false);
#ifdef _DEBUG
			{
				static unsigned long n;
				ASCENSION_LOG_TRIVIAL(debug)
					<< "[#" << std::dec << (n++) << "]" << std::endl
					<< "Invalidated lines: [" << std::dec << *boost::const_begin(lines) << "," << std::dec << *boost::const_end(lines) << ")" << std::endl
					<< "Sheduled redraw: " << boost::geometry::dsv(boundsToRedraw) << std::endl;
			}
#endif
		}

		/// @see TextViewerComponent#relocated
		void TextArea::relocated() {
			allocationRectangleChangedSignal_(*this);
			contentRectangleChangedSignal_(*this);
			if(viewer_ != nullptr)
				textRenderer().viewport()->resize(graphics::geometry::size(allocationRectangle()));	// update the size of 'initial-containing-block'
		}

		/// @see Caret#SelectionShapeChangedSignal
		void TextArea::selectionShapeChanged(const Caret& caret) {
			if(viewer_ != nullptr) {
				if(!textViewer().isFrozen() && !isSelectionEmpty(caret))
					ASCENSION_REDRAW_TEXT_AREA_LINES(caret.selectedRegion().lines());
			}
		}

		/**
		 * Sets the mouse input strategy. An instance of @c TextArea has the default strategy implemented
		 * by @c DefaultTextViewerMouseInputStrategy class as the construction.
		 * @param newStrategy The new strategy or @c null
		 */
		void TextArea::setMouseInputStrategy(std::unique_ptr<TextAreaMouseInputStrategy> newStrategy) {
//			checkInitialization();
			if(mouseInputStrategy_.get() != nullptr) {
				mouseInputStrategy_->interruptMouseReaction(false);
				mouseInputStrategy_->uninstall();
				dropTargetHandler_.reset();
			}
			if(newStrategy.get() != nullptr)
				mouseInputStrategy_ = std::move(newStrategy);
			else
				mouseInputStrategy_ = std::make_shared<DefaultTextAreaMouseInputStrategy>();	// TODO: the two parameters don't have rationales.
			mouseInputStrategy_->install(*this);
			dropTargetHandler_ = mouseInputStrategy_->handleDropTarget();
		}

		/**
		 * Shows (and begins blinking) the hidden caret.
		 * @see #hideCaret, #hidesCaret
		 */
		void TextArea::showCaret() BOOST_NOEXCEPT {
			if(caretPainter_.get() != nullptr)
				static_cast<detail::CaretPainterBase&>(*caretPainter_).show();
		}

		/// @see graphics#font#LineRenderingOptions#textWrappingMark
		std::unique_ptr<const graphics::font::InlineObject> TextArea::textWrappingMark(Index line) const BOOST_NOEXCEPT {
			return std::unique_ptr<const graphics::font::InlineObject>();
		}

		/// @see TextViewerComponent#uninstall
		void TextArea::uninstall(TextViewer& viewer) {
			if(&viewer == &textViewer()) {
				mouseInputStrategy_->uninstall();
				mouseInputStrategyIsInstalled_ = false;
				renderer_->layouts().removeVisualLinesListener(*this);
				viewerFocusChangedConnection_.disconnect();
				viewerFrozenStateChangedConnection_.disconnect();
				viewportResizedConnection_.disconnect();
				viewportScrolledConnection_.disconnect();
				caretMotionConnection_.disconnect();
				selectionShapeChangedConnection_.disconnect();
				matchBracketsChangedConnection_.disconnect();
				defaultFontChangedConnection_.disconnect();
				textViewer().document().removeListener(*this);
				renderer_.reset();
				caretPainter_.reset();
				caret_.reset();
				locator_ = nullptr;
				viewer_ = nullptr;
			}
		}

		/// @see TextViewport#ResizedSignal
		void TextArea::viewportResized(const graphics::Dimension& oldSize) BOOST_NOEXCEPT {
			textRenderer().layouts().invalidate();
//			textRenderer().setTextWrapping(textRenderer().textWrapping(), widgetapi::createRenderingContext(*this).get());
		}

		/// @see TextViewport#ScrolledSignal
		void TextArea::viewportScrolled(
				const presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::ScrollOffset>& positionsBeforeScroll,
				const graphics::font::VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT {
			if(viewer_ == nullptr || textViewer().isFrozen())
				return;

			// 1. calculate pixels to scroll
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			presentation::FlowRelativeTwoAxes<std::int32_t> abstractScrollOffsetInPixels;
			// 1-1. block dimension
			{
				graphics::font::VisualLine p(viewport->firstVisibleLine());
				const graphics::font::TextLayout* layout = textRenderer().layouts().at(p.line);
				abstractScrollOffsetInPixels.bpd() = 0;
				while(layout != nullptr && p < firstVisibleLineBeforeScroll) {
					abstractScrollOffsetInPixels.bpd() -= static_cast<std::uint32_t>(layout->lineMetrics(p.subline).height());
					if(p.subline < layout->numberOfLines() - 1)
						++p.subline;
					else if(p.line < textViewer().document().numberOfLines() - 1) {
						layout = textRenderer().layouts().at(++p.line);
						p.subline = 0;
					} else
						break;
				}
				while(layout != nullptr && p > firstVisibleLineBeforeScroll) {
					if(p.subline > 0)
						--p.subline;
					else if(p.line > 0) {
						layout = textRenderer().layouts().at(--p.line);
						p.subline = layout->numberOfLines() - 1;
					} else
						break;
					abstractScrollOffsetInPixels.bpd() += static_cast<std::uint32_t>(layout->lineMetrics(p.subline).height());
				}
				if(p != firstVisibleLineBeforeScroll)
					layout = nullptr;
				if(layout == nullptr)
					abstractScrollOffsetInPixels.bpd() = std::numeric_limits<graphics::font::TextViewport::SignedScrollOffset>::max();
			}
			// 1-2. inline dimension
			abstractScrollOffsetInPixels.ipd() = (abstractScrollOffsetInPixels.bpd() != std::numeric_limits<std::int32_t>::max()) ?
				static_cast<graphics::font::TextViewport::SignedScrollOffset>(
					inlineProgressionOffsetInViewerGeometry(*viewport, positionsBeforeScroll.ipd())
						- inlineProgressionOffsetInViewerGeometry(*viewport, viewport->scrollPositions().ipd()))
				: std::numeric_limits<std::int32_t>::max();

			if(abstractScrollOffsetInPixels.bpd() != 0 || abstractScrollOffsetInPixels.ipd() != 0) {
				// 1-3. calculate physical offsets
				graphics::PhysicalTwoAxes<std::int32_t> scrollOffsetsInPixels;
				presentation::mapDimensions(textViewer().presentation().computeWritingMode(),
					presentation::_from = abstractScrollOffsetInPixels, presentation::_to = scrollOffsetsInPixels);

				// 2. scroll the graphics device
				const graphics::Rectangle boundsToScroll(contentRectangle());
				if(std::abs(graphics::geometry::x(scrollOffsetsInPixels)) >= graphics::geometry::dx(boundsToScroll)
						|| std::abs(graphics::geometry::y(scrollOffsetsInPixels)) >= graphics::geometry::dy(boundsToScroll))
					widgetapi::scheduleRedraw(textViewer(), boundsToScroll, false);	// repaint all if the amount of the scroll is over a page
				else {
					// scroll image by BLIT
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#if 0
					// TODO: The destructor of Cairo.RefPtr crashes with MSVC heap manager :(
					Cairo::RefPtr<Cairo::Region> regionToScroll(
						Cairo::Region::create(graphics::toNative<Cairo::RectangleInt>(boundsToScroll)));
					textViewer().get_window()->move_region(regionToScroll,
						graphics::geometry::x(scrollOffsetsInPixels), graphics::geometry::y(scrollOffsetsInPixels));
#else
					Cairo::Region regionToScroll(::cairo_region_create_rectangle(&graphics::toNative<Cairo::RectangleInt>(boundsToScroll)), false);
					::gdk_window_move_region(textViewer().get_window()->gobj(), regionToScroll.cobj(),
						static_cast<int>(graphics::geometry::x(scrollOffsetsInPixels)), static_cast<int>(graphics::geometry::y(scrollOffsetsInPixels)));
#endif
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					::ScrollWindowEx(handle().get(),
						graphics::geometry::x(scrollOffsetsInPixels), graphics::geometry::y(scrollOffsetsInPixels),
						nullptr, &static_cast<RECT>(boundsToScroll), nullptr, nullptr, SW_INVALIDATE);
#else
					ASCENSION_CANT_DETECT_PLATFORM();
#endif
					// invalidate bounds newly entered into the viewport
					boost::optional<graphics::Point> originOfBoundsToRedraw;
					if(graphics::geometry::x(scrollOffsetsInPixels) > 0)
						originOfBoundsToRedraw = graphics::geometry::topLeft(boundsToScroll);
					else if(graphics::geometry::x(scrollOffsetsInPixels) < 0)
						originOfBoundsToRedraw = graphics::geometry::topRight(boundsToScroll);
					if(originOfBoundsToRedraw != boost::none)
						widgetapi::scheduleRedraw(
							textViewer(),
							graphics::geometry::make<graphics::Rectangle>(
								boost::get(originOfBoundsToRedraw),
								graphics::Dimension(
									graphics::geometry::_dx = static_cast<graphics::Scalar>(graphics::geometry::x(scrollOffsetsInPixels)),
									graphics::geometry::_dy = graphics::geometry::dy(boundsToScroll))),
							false);

					originOfBoundsToRedraw = boost::none;
					if(graphics::geometry::y(scrollOffsetsInPixels) > 0)
						originOfBoundsToRedraw = graphics::geometry::topLeft(boundsToScroll);
					else if(graphics::geometry::y(scrollOffsetsInPixels) < 0)
						originOfBoundsToRedraw = graphics::geometry::bottomLeft(boundsToScroll);
					if(originOfBoundsToRedraw != boost::none)
						widgetapi::scheduleRedraw(
							textViewer(),
							graphics::geometry::make<graphics::Rectangle>(
								boost::get(originOfBoundsToRedraw),
								graphics::Dimension(
									graphics::geometry::_dx = graphics::geometry::dx(boundsToScroll),
									graphics::geometry::_dy = static_cast<graphics::Scalar>(graphics::geometry::y(scrollOffsetsInPixels)))),
							false);
				}

				// 3. repaint
				widgetapi::redrawScheduledRegion(textViewer());
			}
		}

		/// @see VisualLinesListener#visualLinesDeleted
		void TextArea::visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) BOOST_NOEXCEPT {
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			boost::optional<Index> firstLineToDraw;
			if(*boost::const_end(lines) < viewport->firstVisibleLine().line) {	// deleted before visible area
//				scrolls_.firstVisibleLine.line -= length(lines);
//				scrolls_.vertical.position -= static_cast<int>(sublines);
//				scrolls_.vertical.maximum -= static_cast<int>(sublines);
			} else if(*boost::const_begin(lines) > viewport->firstVisibleLine().line	// deleted the first visible line and/or after it
					|| (*boost::const_begin(lines) == viewport->firstVisibleLine().line && viewport->firstVisibleLine().subline == 0)) {
//				scrolls_.vertical.maximum -= static_cast<int>(sublines);
				firstLineToDraw = *boost::const_begin(lines);
			} else {	// deleted lines contain the first visible line
//				scrolls_.firstVisibleLine.line = lines.beginning();
//				scrolls_.updateVertical(*this);
				firstLineToDraw = *boost::const_begin(lines);
			}
			if(firstLineToDraw != boost::none) {
				ASCENSION_REDRAW_TEXT_AREA_LINE_AND_FOLLOWINGS(boost::get(firstLineToDraw));
				redrawIfNotFrozen(*this);
			}
		}

		/// @see VisualLinesListener#visualLinesInserted
		void TextArea::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			boost::optional<Index> firstLineToDraw;
			if(*boost::const_end(lines) < viewport->firstVisibleLine().line) {	// inserted before visible area
//				scrolls_.firstVisibleLine.line += length(lines);
//				scrolls_.vertical.position += static_cast<int>(length(lines));
//				scrolls_.vertical.maximum += static_cast<int>(length(lines));
			} else if(*boost::const_begin(lines) > viewport->firstVisibleLine().line	// inserted at or after the first visible line
					|| (*boost::const_begin(lines) == viewport->firstVisibleLine().line && viewport->firstVisibleLine().subline == 0)) {
//				scrolls_.vertical.maximum += static_cast<int>(length(lines));
				firstLineToDraw = *boost::const_begin(lines);
			} else {	// inserted around the first visible line
//				scrolls_.firstVisibleLine.line += length(lines);
//				scrolls_.updateVertical(*this);
				firstLineToDraw = *boost::const_begin(lines);
			}
			if(firstLineToDraw != boost::none) {
				ASCENSION_REDRAW_TEXT_AREA_LINE_AND_FOLLOWINGS(boost::get(firstLineToDraw));
				redrawIfNotFrozen(*this);
			}
		}

		/// @see VisualLinesListener#visualLinesModified
		void TextArea::visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT {
			boost::optional<Index> firstLineToDraw;
			if(sublinesDifference == 0) {	// number of visual lines was not changed
				ASCENSION_REDRAW_TEXT_AREA_LINES(lines);
				redrawIfNotFrozen(*this);
			} else {
				const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
				boost::optional<Index> firstLineToDraw;
				if(*boost::const_end(lines) < viewport->firstVisibleLine().line) {	// changed before visible area
//					scrolls_.vertical.position += sublinesDifference;
//					scrolls_.vertical.maximum += sublinesDifference;
				} else if(*boost::const_begin(lines) > viewport->firstVisibleLine().line	// changed at or after the first visible line
						|| (*boost::const_begin(lines) == viewport->firstVisibleLine().line && viewport->firstVisibleLine().subline == 0)) {
//					scrolls_.vertical.maximum += sublinesDifference;
					firstLineToDraw = *boost::const_begin(lines);
				} else {	// changed lines contain the first visible line
//					scrolls_.updateVertical(*this);
					firstLineToDraw = *boost::const_begin(lines);
				}
				if(firstLineToDraw != boost::none) {
					ASCENSION_REDRAW_TEXT_AREA_LINE_AND_FOLLOWINGS(boost::get(firstLineToDraw));
					redrawIfNotFrozen(*this);
				}
			}
		}


		// TextArea.Renderer //////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param viewer The text viewer
		 */
		TextArea::Renderer::Renderer(TextViewer& viewer) :
				TextRenderer(viewer.presentation(), widgetapi::createRenderingContext(viewer)->availableFonts(),
				graphics::geometry::size(viewer.textArea().contentRectangle())), viewer_(viewer) {
			// TODO: other FontCollection object used?
#if 0
			// for test
			setSpecialCharacterRenderer(new DefaultSpecialCharacterRenderer, true);
#endif
		}

		/**
		 * Copy-constructor with a parameter.
		 * @param other The source object
		 * @param viewer The text viewer
		 */
		TextArea::Renderer::Renderer(const Renderer& other, TextViewer& viewer) : TextRenderer(other), viewer_(viewer) {
		}

		/// @see graphics#font#TextRenderer#createLineLayout
		std::unique_ptr<const graphics::font::TextLayout> TextArea::Renderer::createLineLayout(Index line) const {
			const std::unique_ptr<graphics::RenderingContext2D> renderingContext(widgetapi::createRenderingContext(viewer_));
			auto styles(buildLineLayoutConstructionParameters(line, *renderingContext));
			return std::unique_ptr<const graphics::font::TextLayout>(
				new graphics::font::TextLayout(
					viewer_.document().lineString(line),
					presentation().computedTextToplevelStyle(), styles.first, std::move(styles.second),
					presentation().computeTextRunStyleForLine(line),
					presentation::styles::Length::Context(*renderingContext, graphics::geometry::size(viewer_.textArea().allocationRectangle())),
					graphics::geometry::size(viewer_.textArea().contentRectangle()), fontCollection(), renderingContext->fontRenderContext()));
		}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		/// Rewraps the visual lines at the window's edge.
		void TextArea::Renderer::rewrapAtWindowEdge() {
			class Local {
			public:
				explicit Local(Scalar newMeasure) : newMeasure_(newMeasure) {}
				bool operator()(const LineLayoutVector::LineLayout& layout) const {
					return layout.second->numberOfLines() != 1
						|| layout.second->style().justification == NO_JUSTIFICATION
						|| layout.second->measure() > newMeasure_;
				}
			private:
				const Scalar newMeasure_;
			};

			if(viewer_.configuration().lineWrap.wrapsAtWindowEdge()) {
				const graphics::Rectangle clientBounds(viewer_.bounds(false));
				const PhysicalFourSides<Scalar>& spaces(viewer_.spaceWidths());
				if(isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection))
					layouts().invalidateIf(Local(geometry::dx(clientBounds) - spaces.left - spaces.right));
				else
					layouts().invalidateIf(Local(geometry::dy(clientBounds) - spaces.top - spaces.bottom));
			}
		}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		/// @see TextRenderer#width
		graphics::Scalar TextArea::Renderer::width() const BOOST_NOEXCEPT {
			const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
			if(!lwc.wraps())
				return (viewer_.horizontalScrollBar().range().end() + 1) * viewer_.textRenderer().defaultFont()->metrics().averageCharacterWidth();
			else if(lwc.wrapsAtWindowEdge()) {
				const graphics::Rectangle clientBounds(viewer_.bounds(false));
				const PhysicalFourSides<Scalar>& spaces(viewer_.spaceWidths());
				return isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection) ?
					(geometry::dx(clientBounds) - spaces.left - spaces.right) : (geometry::dy(clientBounds) - spaces.top - spaces.bottom);
			} else
				return lwc.width;
		}
#endif // ASCENSION_ABANDONED_AT_VERSION_08
	}
}
