/**
 * @file text-area.cpp
 * Implements @c TextArea class.
 * @author exeal
 * @date 2015-03-18 Created.
 */

#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/graphics/font/font-collection.hpp>
#include <ascension/graphics/font/font-render-context.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/native-conversion.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/log.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <ascension/viewer/default-text-area-mouse-input-strategy.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <boost/foreach.hpp>
#include <boost/geometry/algorithms/equals.hpp>


namespace ascension {
	namespace viewer {
		/// Default constructor.
		TextArea::TextArea() : viewer_(nullptr), locator_(nullptr),
				linesToRedraw_(boost::irange<Index>(0, 0)), mouseInputStrategyIsInstalled_(false) {
		}

		/// Destructor.
		TextArea::~TextArea() BOOST_NOEXCEPT {
			if(viewer_ != nullptr)
				uninstall(*viewer_);
		}

		/// @see Caret#MotionSignal
		void TextArea::caretMoved(const Caret& self, const kernel::Region& oldRegion) {
			if(viewer_ == nullptr || !widgetapi::isVisible(textViewer()))
				return;
			const kernel::Region newRegion(self.selectedRegion());
			bool changed = false;

			// redraw the selected region
			if(self.isSelectionRectangle()) {	// rectangle
				if(!oldRegion.isEmpty())
					redrawLines(boost::irange(oldRegion.beginning().line, oldRegion.end().line + 1));
				if(!newRegion.isEmpty())
					redrawLines(boost::irange(newRegion.beginning().line, newRegion.end().line + 1));
			} else if(newRegion != oldRegion) {	// the selection actually changed
				if(oldRegion.isEmpty()) {	// the selection was empty...
					if(!newRegion.isEmpty())	// the selection is not empty now
						redrawLines(boost::irange(newRegion.beginning().line, newRegion.end().line + 1));
				} else {	// ...if the there is selection
					if(newRegion.isEmpty()) {	// the selection became empty
						redrawLines(boost::irange(oldRegion.beginning().line, oldRegion.end().line + 1));
						if(!textViewer().isFrozen())
							widgetapi::redrawScheduledRegion(textViewer());
					} else if(oldRegion.beginning() == newRegion.beginning()) {	// the beginning point didn't change
						const Index i[2] = {oldRegion.end().line, newRegion.end().line};
						redrawLines(boost::irange(std::min(i[0], i[1]), std::max(i[0], i[1]) + 1));
					} else if(oldRegion.end() == newRegion.end()) {	// the end point didn't change
						const Index i[2] = {oldRegion.beginning().line, newRegion.beginning().line};
						redrawLines(boost::irange(std::min(i[0], i[1]), std::max(i[0], i[1]) + 1));
					} else {	// the both points changed
						if((oldRegion.beginning().line >= newRegion.beginning().line && oldRegion.beginning().line <= newRegion.end().line)
								|| (oldRegion.end().line >= newRegion.beginning().line && oldRegion.end().line <= newRegion.end().line)) {
							const Index i[2] = {
								std::min(oldRegion.beginning().line, newRegion.beginning().line), std::max(oldRegion.end().line, newRegion.end().line)
							};
							redrawLines(boost::irange(std::min(i[0], i[1]), std::max(i[0], i[1]) + 1));
						} else {
							redrawLines(boost::irange(oldRegion.beginning().line, oldRegion.end().line + 1));
							if(!textViewer().isFrozen())
								widgetapi::redrawScheduledRegion(textViewer());
							redrawLines(boost::irange(newRegion.beginning().line, newRegion.end().line + 1));
						}
					}
				}
				changed = true;
			}

			if(changed && !textViewer().isFrozen())
				widgetapi::redrawScheduledRegion(textViewer());
		}

		/// @see TextRenderer#DefaultFontChangedSignal
		void TextArea::defaultFontChanged(const graphics::font::TextRenderer& textRenderer) {
			if(&textRenderer == renderer_.get()) {
#ifdef ASCENSION_USE_SYSTEM_CARET
				caret().resetVisualization();
#endif
				redrawLine(0, true);
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
					Index b = *linesToRedraw_.begin();
					Index e = *linesToRedraw_.end();
					if(change.erasedRegion().first.line != change.erasedRegion().second.line) {
						const Index first = change.erasedRegion().first.line + 1, last = change.erasedRegion().second.line;
						if(b > last)
							b -= last - first + 1;
						else if(b > first)
							b = first;
						if(e != std::numeric_limits<Index>::max()) {
							if(e > last)
								e -= last - first + 1;
							else if(e > first)
								e = first;
						}
					}
					if(change.insertedRegion().first.line != change.insertedRegion().second.line) {
						const Index first = change.insertedRegion().first.line + 1, last = change.insertedRegion().second.line;
						if(b >= first)
							b += last - first + 1;
						if(e >= first && e != std::numeric_limits<Index>::max())
							e += last - first + 1;
					}
					linesToRedraw_ = boost::irange(b, e);
				}
//				redrawLines(region.beginning().line, !multiLine ? region.end().line : INVALID_INDEX);
			}
		}

		/// @see TextViewer#FocusChangedSignal
		void TextArea::focusChanged(const TextViewer& viewer) {
			if(&viewer == &textViewer()) {
				// repaint the lines where the caret places
				redrawLines(boost::irange(kernel::line(caret().beginning()), kernel::line(caret().end()) + 1));
				widgetapi::redrawScheduledRegion(textViewer());
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
							redrawLines(linesToRedraw_);
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
			if(!hidesCaret()) {
				caretBlinker_.reset();
				redrawLine(kernel::line(caret()));
			}
		}

		/// @see TextViewerComponent#install
		void TextArea::install(TextViewer& viewer, const Locator& locator) {
			viewer_ = &viewer;
			locator_ = &locator;
			caret_.reset(new Caret(viewer.document()));
			renderer_.reset(new Renderer(viewer));
			caret().install(*this);
			textViewer().document().addListener(*this);
			caretMotionConnection_ = caret().motionSignal().connect(
				std::bind(&TextArea::caretMoved, this, std::placeholders::_1, std::placeholders::_2));
			selectionShapeChangedConnection_ = caret().selectionShapeChangedSignal().connect(
				std::bind(&TextArea::selectionShapeChanged, this, std::placeholders::_1));
			matchBracketsChangedConnection_ = caret().matchBracketsChangedSignal().connect(
				std::bind(&TextArea::matchBracketsChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			defaultFontChangedConnection_ =
				renderer_->defaultFontChangedSignal().connect(std::bind(&TextArea::defaultFontChanged, this, std::placeholders::_1));
			renderer_->viewport()->addListener(*this);
			renderer_->layouts().addVisualLinesListener(*this);
			if(mouseInputStrategy_.get() != nullptr) {
				assert(!mouseInputStrategyIsInstalled_);
				mouseInputStrategy_->install(*this);
			} else
				setMouseInputStrategy(std::unique_ptr<TextAreaMouseInputStrategy>());
		}

		/// @see Caret#MatchBracketsChangedSignal
		void TextArea::matchBracketsChanged(const Caret& caret,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& previouslyMatchedBrackets, bool outsideOfView) {
			if(viewer_ == nullptr)
				return;
			const boost::optional<std::pair<kernel::Position, kernel::Position>>& newPair = caret.matchBrackets();
			if(newPair) {
				redrawLine(newPair->first.line);
				if(!textViewer().isFrozen())
					widgetapi::redrawScheduledRegion(textViewer());
				if(newPair->second.line != newPair->first.line) {
					redrawLine(newPair->second.line);
					if(!textViewer().isFrozen())
						widgetapi::redrawScheduledRegion(textViewer());
				}
				if(previouslyMatchedBrackets	// clear the previous highlight
						&& previouslyMatchedBrackets->first.line != newPair->first.line && previouslyMatchedBrackets->first.line != newPair->second.line) {
					redrawLine(previouslyMatchedBrackets->first.line);
					if(!textViewer().isFrozen())
						widgetapi::redrawScheduledRegion(textViewer());
				}
				if(previouslyMatchedBrackets && previouslyMatchedBrackets->second.line != newPair->first.line
						&& previouslyMatchedBrackets->second.line != newPair->second.line && previouslyMatchedBrackets->second.line != previouslyMatchedBrackets->first.line)
					redrawLine(previouslyMatchedBrackets->second.line);
			} else {
				if(previouslyMatchedBrackets) {	// clear the previous highlight
					redrawLine(previouslyMatchedBrackets->first.line);
					if(!textViewer().isFrozen())
						widgetapi::redrawScheduledRegion(textViewer());
					if(previouslyMatchedBrackets->second.line != previouslyMatchedBrackets->first.line)
						redrawLine(previouslyMatchedBrackets->second.line);
				}
			}
		}

		/// @see TextViewerComponent#mouseInputStrategy
		std::weak_ptr<MouseInputStrategy> TextArea::mouseInputStrategy() const {
			return std::weak_ptr<MouseInputStrategy>(mouseInputStrategy_);
		}

		/// @see TextViewerComponent#paint
		void TextArea::paint(graphics::PaintContext& context) {
//			Timer tm(L"TextViewer.paint");

			// paint the text area
			textRenderer().paint(context);

			// paint the caret(s)
			paintCaret(context);
		}

		/**
		 * @internal Paints the caret(s).
		 * @param context The graphic context
		 */
		void TextArea::paintCaret(graphics::PaintContext& context) {
		}

		/**
		 * Redraws the specified line on the view.
		 * If the viewer is frozen, redraws after unfrozen.
		 * @param line The line to be redrawn
		 * @param following Set @c true to redraw also the all lines follow to @a line
		 */
		void TextArea::redrawLine(Index line, bool following) {
			redrawLines(boost::irange(line, following ? std::numeric_limits<Index>::max() : line + 1));
		}

		/**
		 * Redraws the specified lines on the view. If the viewer is frozen, redraws after unfrozen.
		 * @param lines The lines to be redrawn. The last line (@a lines.end()) is exclusive and this line will not be
		 *              redrawn. If this value is @c std#numeric_limits<Index>#max(), this method redraws the first
		 *              line (@a lines.beginning()) and the following all lines
		 * @throw IndexOutOfBoundsException @a lines intersects outside of the document
		 */
		void TextArea::redrawLines(const boost::integer_range<Index>& lines) {
		//	checkInitialization();

			if(viewer_ != nullptr || lines.empty())
				return;

			const boost::integer_range<Index> orderedLines(lines | adaptors::ordered());
			if(*orderedLines.end() != std::numeric_limits<Index>::max() && *orderedLines.end() >= textViewer().document().numberOfLines())
				throw IndexOutOfBoundsException("lines");

			if(textViewer().isFrozen()) {
				if(boost::empty(linesToRedraw_))
					linesToRedraw_ = orderedLines;
				else
					linesToRedraw_ = boost::irange(
						std::min(*orderedLines.begin(), *linesToRedraw_.begin()), std::max(*orderedLines.end(), *linesToRedraw_.end()));
				return;
			}

			if(orderedLines.back() < textRenderer().viewport()->firstVisibleLine().line)
				return;

#if defined(_DEBUG) && defined(ASCENSION_DIAGNOSE_INHERENT_DRAWING)
			ASCENSION_LOG_TRIVIAL(debug)
				<< L"@TextViewer.redrawLines invalidates lines ["
				<< static_cast<unsigned long>(*lines.begin())
				<< L".." << static_cast<unsigned long>(*lines.end()) << L"]\n";
#endif

			using graphics::Scalar;
			const presentation::WritingMode writingMode(textViewer().presentation().computeWritingMode());
			std::array<Scalar, 2> beforeAndAfter;	// in viewport (distances from before-edge of the viewport)
			graphics::font::BaselineIterator baseline(*textRenderer().viewport(), graphics::font::VisualLine(*lines.begin(), 0));
			std::get<0>(beforeAndAfter) = *baseline;
			if(std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::min() && std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::max())
				std::get<0>(beforeAndAfter) -= *graphics::font::TextLayout::LineMetricsIterator(*textRenderer().layouts().at(baseline.line()->line), 0).extent().begin();
			baseline = graphics::font::BaselineIterator(*textRenderer().viewport(), graphics::font::VisualLine(*lines.end(), 0));
			std::get<1>(beforeAndAfter) = *baseline;
			if(std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::min() && std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::max())
				std::get<1>(beforeAndAfter) += *graphics::font::TextLayout::LineMetricsIterator(*textRenderer().layouts().at(baseline.line()->line), 0).extent().end();
			assert(std::get<0>(beforeAndAfter) <= std::get<1>(beforeAndAfter));

			namespace geometry = graphics::geometry;
			const graphics::Rectangle viewerBounds(widgetapi::bounds(textViewer(), false));
			graphics::Rectangle boundsToRedraw(textRenderer().viewport()->boundsInView());
			geometry::translate(boundsToRedraw, graphics::Dimension(geometry::_dx = geometry::left(viewerBounds), geometry::_dy = geometry::top(viewerBounds)));
			assert(boost::geometry::equals(boundsToRedraw, textViewer().textAreaAllocationRectangle()));

			BOOST_FOREACH(graphics::Scalar& edge, beforeAndAfter) {
				switch(textRenderer().computedBlockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						if(edge == std::numeric_limits<Scalar>::min())
							edge = geometry::top(boundsToRedraw);
						else if(edge == std::numeric_limits<Scalar>::max())
							edge = geometry::bottom(boundsToRedraw);
						else
							edge = geometry::top(boundsToRedraw) + edge;
						break;
					case presentation::VERTICAL_RL:
						if(edge == std::numeric_limits<Scalar>::min())
							edge = geometry::right(boundsToRedraw);
						else if(edge == std::numeric_limits<Scalar>::max())
							edge = geometry::left(boundsToRedraw);
						else
							edge = geometry::right(boundsToRedraw) - edge;
						break;
					case presentation::VERTICAL_LR:
						if(edge == std::numeric_limits<Scalar>::min())
							edge = geometry::left(boundsToRedraw);
						else if(edge == std::numeric_limits<Scalar>::max())
							edge = geometry::right(boundsToRedraw);
						else
							edge = geometry::left(boundsToRedraw) + edge;
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			widgetapi::scheduleRedraw(textViewer(), boundsToRedraw, false);
		}

		/// @see TextViewerComponent#relocated
		void TextArea::relocated() {
			if(viewer_ != nullptr)
				textRenderer().viewport()->setBoundsInView(textViewer().textAreaContentRectangle());
		}

		/// @see Caret#SelectionShapeChangedSignal
		void TextArea::selectionShapeChanged(const Caret& caret) {
			if(viewer_ != nullptr) {
				if(!textViewer().isFrozen() && !isSelectionEmpty(caret))
					redrawLines(boost::irange(kernel::line(caret.beginning()), kernel::line(caret.end()) + 1));
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
		 * Shows the hidden caret.
		 * @see #hideCaret, #hidesCaret
		 */
		void TextArea::showCaret() BOOST_NOEXCEPT {
			if(hidesCaret())
				caretBlinker_.reset(new CaretBlinker(caret()));
		}

		/// @see TextViewerComponent#uninstall
		void TextArea::uninstall(TextViewer& viewer) {
			if(&viewer == &textViewer()) {
				mouseInputStrategy_->uninstall();
				mouseInputStrategyIsInstalled_ = false;
				renderer_->layouts().removeVisualLinesListener(*this);
				renderer_->viewport()->removeListener(*this);
				caretMotionConnection_.disconnect();
				selectionShapeChangedConnection_.disconnect();
				matchBracketsChangedConnection_.disconnect();
				defaultFontChangedConnection_.disconnect();
				textViewer().document().removeListener(*this);
				renderer_.reset();
				locator_ = nullptr;
				viewer_ = nullptr;
			}
		}

		/// @see TextViewportListener#viewportBoundsInViewChanged
		void TextArea::viewportBoundsInViewChanged(const graphics::Rectangle& oldBounds) BOOST_NOEXCEPT {
//			textRenderer().setTextWrapping(textRenderer().textWrapping(), widgetapi::createRenderingContext(*this).get());
		}

		/// @see TextViewportListener#viewportScrollPositionChanged
		void TextArea::viewportScrollPositionChanged(
				const presentation::FlowRelativeTwoAxes<graphics::font::TextViewportScrollOffset>& positionsBeforeScroll,
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
					const graphics::font::TextLayout::LineMetrics& lm = layout->lineMetrics(p.subline);
					abstractScrollOffsetInPixels.bpd() -= static_cast<std::uint32_t>(lm.ascent);
					abstractScrollOffsetInPixels.bpd() -= static_cast<std::uint32_t>(lm.descent);
					abstractScrollOffsetInPixels.bpd() -= static_cast<std::uint32_t>(lm.leading);
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
					const graphics::font::TextLayout::LineMetrics& lm = layout->lineMetrics(p.subline);
					abstractScrollOffsetInPixels.bpd() += static_cast<std::uint32_t>(lm.ascent);
					abstractScrollOffsetInPixels.bpd() += static_cast<std::uint32_t>(lm.descent);
					abstractScrollOffsetInPixels.bpd() += static_cast<std::uint32_t>(lm.leading);
				}
				if(p != firstVisibleLineBeforeScroll)
					layout = nullptr;
				if(layout == nullptr)
					abstractScrollOffsetInPixels.bpd() = std::numeric_limits<graphics::font::TextViewportSignedScrollOffset>::max();
			}
			// 1-2. inline dimension
			abstractScrollOffsetInPixels.ipd() = (abstractScrollOffsetInPixels.bpd() != std::numeric_limits<std::int32_t>::max()) ?
				static_cast<graphics::font::TextViewportSignedScrollOffset>(
					inlineProgressionOffsetInViewerGeometry(*viewport, positionsBeforeScroll.ipd())
						- inlineProgressionOffsetInViewerGeometry(*viewport, viewport->scrollPositions().ipd()))
				: std::numeric_limits<std::int32_t>::max();
			// 1-3. calculate physical offsets
			const auto scrollOffsetsInPixels(presentation::mapFlowRelativeToPhysical(
				textViewer().presentation().computeWritingMode(), abstractScrollOffsetInPixels));

			// 2. scroll the graphics device
			const graphics::Rectangle& boundsToScroll = viewport->boundsInView();
			if(std::abs(graphics::geometry::x(scrollOffsetsInPixels)) >= graphics::geometry::dx(boundsToScroll)
					|| std::abs(graphics::geometry::y(scrollOffsetsInPixels)) >= graphics::geometry::dy(boundsToScroll))
				widgetapi::scheduleRedraw(textViewer(), boundsToScroll, false);	// repaint all if the amount of the scroll is over a page
			else {
				// scroll image by BLIT
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				{
#if 0
					// TODO: The destructor of Cairo.RefPtr crashes with MSVC heap manager :(
					Cairo::RefPtr<Cairo::Region> regionToScroll(
						Cairo::Region::create(graphics::toNative<Cairo::RectangleInt>(boundsToScroll)));
					textViewer().get_window()->move_region(regionToScroll,
						graphics::geometry::x(scrollOffsetsInPixels), graphics::geometry::y(scrollOffsetsInPixels));
#else
					Cairo::Region regionToScroll(::cairo_region_create(), false);
					::gdk_window_move_region(textViewer().get_window()->gobj(), regionToScroll.cobj(),
						graphics::geometry::x(scrollOffsetsInPixels), graphics::geometry::y(scrollOffsetsInPixels));
#endif
				}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				::ScrollWindowEx(handle().get(),
					graphics::geometry::x(scrollOffsetsInPixels), graphics::geometry::y(scrollOffsetsInPixels),
					nullptr, &static_cast<RECT>(boundsToScroll), nullptr, nullptr, SW_INVALIDATE);
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				// invalidate bounds newly entered into the viewport
				{
					boost::optional<graphics::Point> origin;
					if(graphics::geometry::x(scrollOffsetsInPixels) > 0)
						origin = graphics::geometry::topLeft(boundsToScroll);
					else if(graphics::geometry::x(scrollOffsetsInPixels) < 0)
						origin = graphics::geometry::topRight(boundsToScroll);
					if(origin != boost::none)
						widgetapi::scheduleRedraw(
							textViewer(),
							graphics::geometry::make<graphics::Rectangle>(
								boost::get(origin),
								graphics::Dimension(
									graphics::geometry::_dx = static_cast<graphics::Scalar>(graphics::geometry::x(scrollOffsetsInPixels)),
									graphics::geometry::_dy = graphics::geometry::dy(boundsToScroll))),
							false);
				}
				{
					boost::optional<graphics::Point> origin;
					if(graphics::geometry::y(scrollOffsetsInPixels) > 0)
						origin = graphics::geometry::topLeft(boundsToScroll);
					else if(graphics::geometry::y(scrollOffsetsInPixels) < 0)
						origin = graphics::geometry::bottomLeft(boundsToScroll);
					if(origin != boost::none)
						widgetapi::scheduleRedraw(
							textViewer(),
							graphics::geometry::make<graphics::Rectangle>(
								boost::get(origin),
								graphics::Dimension(
									graphics::geometry::_dx = graphics::geometry::dx(boundsToScroll),
									graphics::geometry::_dy = static_cast<graphics::Scalar>(graphics::geometry::y(scrollOffsetsInPixels)))),
							false);
				}
			}

			// 3. repaint
			widgetapi::redrawScheduledRegion(textViewer());
		}

		void TextArea::viewportScrollPropertiesChanged(const presentation::FlowRelativeTwoAxes<bool>& changedDimensions) {
		}

		/// @see VisualLinesListener#visualLinesDeleted
		void TextArea::visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) BOOST_NOEXCEPT {
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			if(*lines.end() < viewport->firstVisibleLine().line) {	// deleted before visible area
//				scrolls_.firstVisibleLine.line -= length(lines);
//				scrolls_.vertical.position -= static_cast<int>(sublines);
//				scrolls_.vertical.maximum -= static_cast<int>(sublines);
			} else if(*lines.begin() > viewport->firstVisibleLine().line	// deleted the first visible line and/or after it
					|| (*lines.begin() == viewport->firstVisibleLine().line && viewport->firstVisibleLine().subline == 0)) {
//				scrolls_.vertical.maximum -= static_cast<int>(sublines);
				redrawLine(*lines.begin(), true);
			} else {	// deleted lines contain the first visible line
//				scrolls_.firstVisibleLine.line = lines.beginning();
//				scrolls_.updateVertical(*this);
				redrawLine(*lines.begin(), true);
			}
		}

		/// @see VisualLinesListener#visualLinesInserted
		void TextArea::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			if(*lines.end() < viewport->firstVisibleLine().line) {	// inserted before visible area
//				scrolls_.firstVisibleLine.line += length(lines);
//				scrolls_.vertical.position += static_cast<int>(length(lines));
//				scrolls_.vertical.maximum += static_cast<int>(length(lines));
			} else if(*lines.begin() > viewport->firstVisibleLine().line	// inserted at or after the first visible line
					|| (*lines.begin() == viewport->firstVisibleLine().line && viewport->firstVisibleLine().subline == 0)) {
//				scrolls_.vertical.maximum += static_cast<int>(length(lines));
				redrawLine(*lines.begin(), true);
			} else {	// inserted around the first visible line
//				scrolls_.firstVisibleLine.line += length(lines);
//				scrolls_.updateVertical(*this);
				redrawLine(*lines.begin(), true);
			}
		}

		/// @see VisualLinesListener#visualLinesModified
		void TextArea::visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT {
			if(sublinesDifference == 0)	// number of visual lines was not changed
				redrawLines(lines);
			else {
				const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
				if(*lines.end() < viewport->firstVisibleLine().line) {	// changed before visible area
//					scrolls_.vertical.position += sublinesDifference;
//					scrolls_.vertical.maximum += sublinesDifference;
				} else if(*lines.begin() > viewport->firstVisibleLine().line	// changed at or after the first visible line
						|| (*lines.begin() == viewport->firstVisibleLine().line && viewport->firstVisibleLine().subline == 0)) {
//					scrolls_.vertical.maximum += sublinesDifference;
					redrawLine(*lines.begin(), true);
				} else {	// changed lines contain the first visible line
//					scrolls_.updateVertical(*this);
					redrawLine(*lines.begin(), true);
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
				graphics::geometry::size(viewer.textAreaContentRectangle())), viewer_(viewer) {
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
					viewer_.document().line(line),
					presentation().computedTextToplevelStyle(), styles.first, std::move(styles.second),
					presentation().computeTextRunStyleForLine(line),
					presentation::styles::Length::Context(*renderingContext, graphics::geometry::size(viewer_.textAreaAllocationRectangle())),
					graphics::geometry::size(viewer_.textAreaContentRectangle()), fontCollection(), renderingContext->fontRenderContext()));
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
