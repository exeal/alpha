/**
 * @file text-viewer.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2016
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/algorithms/within.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/mouse-input-strategy.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#ifndef ASCENSION_PIXELFUL_SCROLL_IN_BPD
#	include <boost/math/special_functions/trunc.hpp>
#endif
#ifdef _DEBUG
#	include <ascension/log.hpp>
//#	define ASCENSIOB_DIAGNOSE_INHERENT_DRAWING
//#	define ASCENSION_TRACE_DRAWING_STRING
#endif // _DEBUG
#include <limits>	// std.numeric_limits

namespace ascension {
	namespace viewer {
		// TextViewer /////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::viewer::TextViewer
		 *
		 * The view of Ascension framework. @c TextViewer displays the content of the document, manipulates
		 * the document with the caret and selection, and provides other visual presentations.
		 *
		 * @c TextViewer provides two methods #freeze and #unfreeze to freeze of the screen of the window.
		 * While the viewer is frozen, the window does not redraw the content.
		 *
		 * <h3>Duplication</h3>
		 *
		 * Unlike @c manah#win32#ui#Window class, the copy-constructor does not copy the window handle of
		 * the original (attachement is not performed). This semantics is derived from
		 * @c manah#win32#ui#CustomControl super class.
		 *
		 * So an object just created by copy-constructor does not have valid window handle. Call @c #create
		 * method to construct as a window.
		 *
		 * <h3>Window styles related to bidirectional</h3>
		 *
		 * テキストを右寄せで表示するための @c WS_EX_RIGHT 、右から左に表示するための @c WS_EX_RTLREADING
		 * はいずれも無視される。これらの設定には代わりに @c LayoutSettings の該当メンバを使用しなければならない
		 *
		 * @c WS_EX_LAYOUTRTL is also not supported. The result is undefined if you used.
		 *
		 * 垂直スクロールバーを左側に表示するにはクライアントが @c WS_EX_LEFTSCROLLBAR を設定しなければならない
		 *
		 * これらの設定を一括して変更する場合 @c #setTextDirection を使うことができる
		 *
		 * 垂直ルーラ (インジケータマージンと行番号) の位置はテキストが左寄せであれば左端、
		 * 右寄せであれば右端になる
		 *
		 * <h3>Subclassing</h3>
		 *
		 * @c TextViewer and @c SourceViewer are intended to be subclassed. You can override the virtual
		 * member functions in your derived class. Note that @c TextViewer implements interfaces defined
		 * in Ascension by virtual functions. These are also overridable but you must call base
		 * implementation. For example, you are overriding @c documentChanged:
		 *
		 * @code
		 * void YourViewer::documentChanged(const Document& document, const DocumentChange& change) {
		 *   // ...your own code
		 *   TextViewer::documentChanged(document, change);
		 * }
		 * @endcode
		 *
		 * <h3>The Text Context Area</h3>
		 *
		 * (This concept is based on "Area Model" of XSL 1.1/2.0.)
		 *
		 * The text area of a @c TextViewer is a rectangular portion which shows the text content. This area has a
		 * "content-rectangle", "border-rectangle", "padding-rectangle", "spaces" and "allocation-rectangle" defined in
		 * XSL 1.1/2.0.
		 *
		 * The "allocation-rectangle" is defined by @c #textAreaAllocationRectangle method. This is overridable by
		 * @c #locateComponent virtual method (of @c TextViewerComponent interface) with @c TextArea instance. A
		 * @c TextViewer paints only inside of the "allocation-rectangle". As default, @c #textAreaAllocationRectangle
		 * returns "local-bounds" and all portion is painted. But if the derived class specified the smaller rectangle
		 * for "allocation-rectangle", the outside should be painted by the derived class (also should override
		 * @c #paint method).
		 *
		 * <h3>Windows specific features</h3>
		 *
		 * @c TextViewer supports drag-and-drop. If you want to enable this feature, call Win32
		 * @c OleInitialize in your thread.
		 *
		 * If you want to enable tooltips, call Win32 @c InitCommonControlsEx.
		 *
		 * @see presentation#Presentation, Caret
		 */

		/**
		 * @typedef ascension::viewer::TextViewer::FocusChangedSignal
		 * The signal which gets emitted when the text viewer was about to be lost or gained the focus.
		 * @param viewer The text viewer
		 * @see #focusAboutToBeLost, #focusChangedSignal, #focusGained, widgetapi#hasFocus, widgetapi#setFocus,
		 *      widgetapi#unsetFocus
		 */

		/**
		 * @typedef ascension::viewer::TextViewer::FrozenStateChangedSignal
		 * The signal which gets emitted when the text viewer was frozen or unfrozen.
		 * @param viewer The text viewer
		 * @see #freeze, #frozenStateChangedSignal, #unfreeze, #unfrozen
		 */

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK) && defined(ASCENSION_TEXT_VIEWER_IS_GTK_SCROLLABLE)
		namespace {
			static const char GLIBMM_CUSTOM_TYPE_NAME[] = "ascension.viewer.TextViewer";
		}
#endif

		/// Destructor.
		TextViewer::~TextViewer() {
		}

		/**
		 * Additionally draws the indicator margin on the vertical ruler.
		 * @param line The line number
		 * @param context The graphics context
		 * @param rect The rectangle to draw
		 */
		void TextViewer::drawIndicatorMargin(Index /* line */, graphics::PaintContext& /* context */, const graphics::Rectangle& /* rect */) {
		}

		/// Invoked when the widget is about to lose the keyboard focus.
		void TextViewer::focusAboutToBeLost(widgetapi::event::Event& event) {
			mouseVanisher_.restoreHiddenCursor();
			if(const auto mouseInputStrategy = textArea_->mouseInputStrategy().lock())
				mouseInputStrategy->interruptMouseReaction(false);
/*			if(caret_->getMatchBracketsTrackingMode() != Caret::DONT_TRACK
					&& getCaret().getMatchBrackets().first != Position::INVALID_POSITION) {	// 対括弧の通知を終了
				FOR_EACH_LISTENERS()
					(*it)->onMatchBracketFoundOutOfView(Position::INVALID_POSITION);
			}
			if(completionWindow_->isWindow() && newWindow != completionWindow_->getSafeHwnd())
				closeCompletionProposalsPopup(*this);
*/			detail::resetInputMethod(*this);
//			if(currentWin32WindowMessage().wParam != get()) {
//				hideCaret();
//				::DestroyCaret();
//			}

			focusChangedSignal_(*this);

			return event.consume();
		}

		/// Returns the @c FocusChangedSignal signal connector.
		SignalConnector<TextViewer::FocusChangedSignal> TextViewer::focusChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(focusChangedSignal_);
		}

		namespace {
			template<typename Coordinate>
			inline widgetapi::NativeScrollPosition reverseScrollPosition(const TextViewer& textViewer, widgetapi::NativeScrollPosition position) {
				const auto textRenderer(textViewer.textArea()->textRenderer());
				//				return static_cast<widgetapi::NativeScrollPosition>(textRenderer->layouts().maximumMeasure()
				//					/ widgetapi::createRenderingContext(textViewer)->fontMetrics(textRenderer->defaultFont())->averageCharacterWidth())
				//					- position
				//					- static_cast<widgetapi::NativeScrollPosition>(textRenderer->viewport()->numberOfVisibleCharactersInLine());
				const presentation::BlockFlowDirection blockFlowDirection = textRenderer->blockFlowDirection();
				const auto viewport(textViewer.textArea()->viewport());
				return static_cast<widgetapi::NativeScrollPosition>(*graphics::font::scrollableRange<Coordinate>(*viewport).end() - position - graphics::font::pageSize<Coordinate>(*viewport));
			}

			graphics::PhysicalTwoAxes<widgetapi::NativeScrollPosition> physicalScrollPosition(const TextViewer& textViewer) {
				const auto textRenderer(textViewer.textArea()->textRenderer());
				const auto viewport(textViewer.textArea()->viewport());
				const presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::ScrollOffset> scrollPositions(viewport->scrollPositions());
				widgetapi::NativeScrollPosition x, y;
				switch(textRenderer->blockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						x = (textRenderer->inlineFlowDirection() == presentation::LEFT_TO_RIGHT) ?
							scrollPositions.ipd() : reverseScrollPosition<presentation::ReadingDirection>(textViewer, static_cast<widgetapi::NativeScrollPosition>(scrollPositions.ipd()));
						y = scrollPositions.bpd();
						break;
					case presentation::VERTICAL_RL:
						x = reverseScrollPosition<presentation::BlockFlowDirection>(textViewer, scrollPositions.bpd());
						y = scrollPositions.ipd();
						break;
					case presentation::VERTICAL_LR:
						x = scrollPositions.bpd();
						y = scrollPositions.ipd();
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
//				x /= xScrollRate;
//				y /= yScrollRate;
				return graphics::makePhysicalTwoAxes((graphics::_x = x, graphics::_y = y));
			}

			template<typename AbstractCoordinate>
			inline widgetapi::NativeScrollPosition calculateScrollStepSize(const TextViewer& viewer) {
				return 1;	// TODO: Not implemented.
			}

			template<std::size_t physicalCoordinate>
			inline widgetapi::NativeScrollPosition calculateScrollStepSize(const TextViewer& viewer) {
				return 1;	// TODO: Not implemented.
			}

			void configureScrollBar(TextViewer& viewer, std::size_t coordinate, const boost::optional<widgetapi::NativeScrollPosition>& position,
				const boost::optional<NumericRange<widgetapi::NativeScrollPosition>>& range, const boost::optional<widgetapi::NativeScrollPosition>& pageSize) {
				assert(coordinate <= 1);
#ifdef _DEBUG
				std::ostringstream parametersMessage;
				if(position != boost::none)
					parametersMessage << "position=" << boost::get(position) << ", ";
				if(range != boost::none)
					parametersMessage << "range=[" << *boost::const_begin(boost::get(range)) << "," << *boost::const_end(boost::get(range)) << "), ";
				if(pageSize != boost::none)
					parametersMessage << "pagesize=" << boost::get(pageSize) << ", ";
				ASCENSION_LOG_TRIVIAL(debug)
					<< "A TextViewer 0x" << std::hex << reinterpret_cast<std::uintptr_t>(&viewer)
					<< " was configured the scroll bar (" << coordinate << ") with: \n\t" << parametersMessage.str() << std::endl;
#endif

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#ifdef ASCENSION_TEXT_VIEWER_IS_GTK_SCROLLABLE
				Glib::RefPtr<Gtk::Adjustment> adjustment((coordinate == 0) ? viewer.get_hadjustment() : viewer.get_vadjustment());
				if(range != boost::none) {
					adjustment->set_lower(*boost::const_begin(boost::get(range)));
					adjustment->set_upper(*boost::const_end(boost::get(range)));
				}
				adjustment->set_step_increment((coordinate == 0) ? calculateScrollStepSize<0>(viewer) : calculateScrollStepSize<1>(viewer));
				if(pageSize != boost::none) {
					adjustment->set_page_increment(boost::get(pageSize));
					adjustment->set_page_size(boost::get(pageSize));
				}
				if(position != boost::none)
					adjustment->set_value(boost::get(position));
#endif
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QScrollBar* const scrollBar = (coordinate == 0) ? viewer.horizontalScrollBar() : viewer.verticalScrollBar();
				if(range != boost::none)
					scrollBar->setRange(*boost::const_begin(boost::get(range)), *boost::const_end(boost::get(range)));
				scrollBar->setSingleStep((coordinate == 0) ? calculateScrollStepSize<0>(viewer) : calculateScrollStepSize<1>(viewer));
				if(pageSize != boost::none)
					scrollBar->setPageStep(boost::get(pageSize));
				if(position != boost::none)
					scrollBar->setSliderPosition(boost::get(position));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				auto si(win32::makeZeroSize<SCROLLINFO>());
				if(range/* != boost::none*/) {
					si.fMask |= SIF_RANGE;
					si.nMin = *boost::const_begin(boost::get(range));
					si.nMax = *boost::const_end(boost::get(range));
				}
				if(pageSize != boost::none) {
					si.fMask |= SIF_PAGE;
					si.nPage = boost::get(pageSize) + 1;
				}
				if(position != boost::none) {
					si.fMask |= SIF_POS;
					si.nPos = boost::get(position);
				}
				::SetScrollInfo(viewer.handle().get(), (coordinate == 0) ? SB_HORZ : SB_VERT, &si, true);
#endif
			}
		}

		/// Invoked when the widget gained the keyboard focus.
		void TextViewer::focusGained(widgetapi::event::Event& event) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// restore the scroll positions
			const auto scrollPositions(physicalScrollPosition(*this));
			configureScrollBar(*this, 0, boost::geometry::get<0>(scrollPositions), boost::none, boost::none);
			configureScrollBar(*this, 1, boost::geometry::get<1>(scrollPositions), boost::none, boost::none);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

//			if(currentWin32WindowMessage().wParam != get()) {
//				// resurrect the caret
//				recreateCaret();
//				updateCaretPosition();
//				if(texteditor::Session* const session = document().session()) {
//					if(texteditor::InputSequenceCheckers* const isc = session->inputSequenceCheckers())
//						isc->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
//				}
//			}

			focusChangedSignal_(*this);

			return event.consume();
		}

		/**
		 * Freezes the drawing of the viewer.
		 * @throw std#overflow_error
		 * @note This method freezes also viewer's @c TextViewport.
		 * @see #isFrozen, #unfreeze, #AutoFreeze
		 */
		void TextViewer::freeze() {
//			checkInitialization();
			if(frozenCount_ == std::numeric_limits<std::decay<decltype(frozenCount_.data())>::type>::max())
				throw std::overflow_error("");
			if(++frozenCount_ == 1)
				frozenStateChangedSignal_(*this);
		}

		/// Returns the @c FrozenStateChangedSignal signal connector.
		SignalConnector<TextViewer::FrozenStateChangedSignal> TextViewer::frozenStateChangedSignal() BOOST_NOEXCEPT {
			return SignalConnector<FrozenStateChangedSignal>(frozenStateChangedSignal_);
		}

#if 0
		/**
		 * Returns the text and the region of a link near the cursor.
		 * @param[out] region the region of the link
		 * @param[out] text the text of the link. if the link is mail address, "mailto:" will be added to the head
		 * @return @c true if the cursor is on link
		 * @deprecated 0.8
		 */
		bool TextViewer::getPointedLinkText(Region& region, AutoBuffer<Char>& text) const {
			checkInitialization();
			const Document& document = document();
			const Position pos = getCharacterForClientXY(getCursorPosition(), false);	// カーソル位置に最も近い文字位置

			if(kernel::offsetInLine(pos) == document.getLineLength(pos.line))	// 指定位置に文字が無い
				return false;

			const LineLayout& layout = renderer_->getLineLayout(pos.line);
			const Index subline = layout.getSubline(kernel::offsetInLine(pos));
			const Char* const line = document.getLine(pos.line).data();
			const Char* const first = line + layout.getSublineOffset(subline);
			const Char* const last =
				line + ((subline < layout.getNumberOfSublines() - 1) ? layout.getSublineOffset(subline + 1) : document.getLineLength(pos.line));
			Index linkLength;	// URIDetector の eatMailAddress 、eatUrlString で見つけたリンクテキストの長さ

			for(const Char* p = (pos.offsetInLine > 200) ? first + kernel::offsetInLine(pos) - 200 : first; p <= first + kernel::offsetInLine(pos); ) {
				if(p != first) {
					if((p[-1] >= L'A' && p[-1] <= L'Z')
							|| (p[-1] >= L'a' && p[-1] <= L'z')
							|| p[-1] == L'_') {
						++p;
						continue;
					}
				}
				if(0 != (linkLength = rules::URIDetector::eatURL(p, last, true) - p)) {
					if(p - first + linkLength > kernel::offsetInLine(pos)) {	// カーソル位置を越えた
						region.first.line = region.second.line = pos.line;
						region.first.offsetInLine = p - line;
						region.second.offsetInLine = kernel::offsetInLine(region.first) + linkLength;
						text.reset(new Char[linkLength + 1]);
						wmemcpy(text.get(), p, linkLength);
						text[linkLength] = 0;
						return true;
					}
					p += linkLength;	// 届かない場合は続行
				} else if(0 != (linkLength = rules::URIDetector::eatMailAddress(p, last, true) - p)) {
					if(p - first + linkLength > kernel::offsetInLine(pos)) {	// カーソル位置を越えた
						static const wchar_t MAILTO_PREFIX[] = L"mailto:";
						region.first.line = region.second.line = pos.line;
						region.first.offsetInLine = p - line;
						region.second.offsetInLine = kernel::offsetInLine(region.first) + linkLength;
						text.reset(new Char[linkLength + 7 + 1]);
						wmemcpy(text.get(), MAILTO_PREFIX, countof(MAILTO_PREFIX) - 1);
						wmemcpy(text.get() + countof(MAILTO_PREFIX) - 1, p, linkLength);
						text[countof(MAILTO_PREFIX) - 1 + linkLength] = 0;
						return true;
					}
					p += linkLength;	// 届かない場合は続行
				} else
					++p;
			}
			return false;
		}
#endif

		/// @overload
		TextViewerComponent* TextViewer::hitTest(const graphics::Point& location) BOOST_NOEXCEPT {
			const TextViewer& self = *this;
			return const_cast<TextViewerComponent*>(self.hitTest(location));
		}

		/**
		 * Returns the @c TextViewerComponent which contains the specified location.
		 * @param location The position to hit test, in the viewer-local coordinates
		 * @return The @c TextViewerComponent addressed by @a location, or @c nullptr if there is nothing
		 * @see #textAreaAllocationRectangle, TextArea, TextViewerComponent#Locator
		 */
		const TextViewerComponent* TextViewer::hitTest(const graphics::Point& location) const BOOST_NOEXCEPT {
			if(const auto ta = textArea()) {
#if 0
				if(graphics::geometry::within(location, ta->contentRectangle()))
#else
				if(graphics::geometry::within(location, ta->allocationRectangle()))
#endif
					return ta.get();
			}
			return nullptr;
		}
#if 0
		/**
		 * Returns an offset from left/top-edge of local-bounds to one of the content-area in user units.
		 * This algorithm considers the ruler, the scroll position and spaces around the content box.
		 * @return The offset
		 * @see TextLayout#lineStartEdge, TextRenderer#lineStartEdge
		 */
		graphics::Scalar TextViewer::inlineProgressionOffsetInViewport() const {
			const bool horizontal = isHorizontal(textRenderer().computedBlockFlowDirection());
			graphics::Scalar offset = 0;

			// scroll position
			const graphics::PhysicalTwoAxes<widgetapi::NativeScrollPosition> scrollPosition(physicalScrollPosition(*this));
			offset -= graphics::font::inlineProgressionOffsetInViewerGeometry(*textRenderer().viewport(),
				static_cast<graphics::font::TextViewportScrollOffset>(
					horizontal ? boost::geometry::get<0>(scrollPosition) : boost::geometry::get<1>(scrollPosition)));

			// ruler width
			const graphics::Rectangle localBounds(widgetapi::bounds(*this, false));
			const graphics::Rectangle allocationRectangle(textAreaAllocationRectangle());
			if(horizontal)
				offset += graphics::geometry::left(allocationRectangle) - graphics::geometry::left(localBounds);
			else
				offset += graphics::geometry::top(allocationRectangle) - graphics::geometry::top(localBounds);

			return offset;

#if 0
			const detail::PhysicalTextAnchor alignment(
				detail::computePhysicalTextAnchor(layout.anchor(), layout.writingMode().inlineFlowDirection));
			if(alignment == detail::LEFT /*|| ... != NO_JUSTIFICATION*/)	// TODO: this code ignores last visual line with justification.
				return spaces.left - scrolls_.x() * renderer_->defaultFont()->metrics().averageCharacterWidth();

			Scalar indent;
			const graphics::Rectangle clientBounds(bounds(false));
			if(renderer_->layouts().maximumMeasure() + spaces.left + spaces.right > geometry::dx(clientBounds)) {
				indent = renderer_->layouts().maximumMeasure() - layout.measure(0) + spaces.left;
				indent += (geometry::dx(clientBounds) - spaces.left - spaces.right) % renderer_->defaultFont()->metrics().averageCharacterWidth();
			} else
				indent = geometry::dx(clientBounds) - layout.measure(0) - spaces.right;
			if(alignment == detail::MIDDLE)
				indent /= 2;
			else
				assert(alignment == detail::RIGHT);
			return indent - static_cast<Scalar>(scrolls_.x()) * renderer_->defaultFont()->metrics().averageCharacterWidth();
#endif
		}
#endif
		/// @see TextViewerComponent#Locator#locateComponent
		graphics::Rectangle TextViewer::locateComponent(const TextViewerComponent& component) const {
			if(&component != textArea().get())
				throw std::invalid_argument("component");
			return widgetapi::bounds(*this, false);
		}

		/// @see MouseInputStrategy#TargetLocker#lockMouseInputTarget
		bool TextViewer::lockMouseInputTarget(std::weak_ptr<MouseInputStrategy> strategy) {
			if(strategy.expired())
				throw std::bad_weak_ptr();
			if(lockedMouseInputStrategy_.expired()) {
				lockedMouseInputStrategy_ = strategy;
				return true;
			}
			return false;
		}

#if 0
		/**
		 * @internal
		 * @see #mapViewportIpdToLineLayout
		 */
		inline Scalar TextViewer::mapLineLayoutIpdToViewport(Index line, Scalar ipd) const {
			return ipd + lineStartEdge(textRenderer().layouts().at(line),
				textRenderer().viewport()->contentMeasure()) + inlineProgressionOffsetInViewport();
		}

		/**
		 * @internal
		 * @see #mapLineLayoutIpdToViewport
		 */
		inline Scalar TextViewer::mapViewportIpdToLineLayout(Index line, Scalar ipd) const {
			return ipd - lineStartEdge(textRenderer().layouts().at(line),
				textRenderer().viewport()->contentMeasure()) - inlineProgressionOffsetInViewport();
		}

		/**
		 * Returns the distance from the before-edge of the viewport to the baseline of the specified line.
		 * @param line The logical line number
		 * @param fullSearch @c false to return special value for the line outside of the viewport
		 * @return The distance from the viewport's edge to the line in pixels
		 * @retval std#numeric_limits&lt;Scalar&gt;::max() @a fullSearch is @c false and @a line is outside of the
		 *                                                 after-edge of the viewport
		 * @retval std#numeric_limits&lt;Scalar&gt;::min() @a fullSearch is @c false and @a line is outside of the
		 *                                                 before-edge of the viewport
		 * @throw kernel#BadPositionException @a line is outside of the document
		 * @see #BaseIterator, #mapViewportBpdToLine, TextRenderer#offsetVisualLine
		 */
		Scalar TextViewer::mapLineToViewportBpd(Index line, bool fullSearch) const {
			const PhysicalFourSides<Scalar> spaces(spaceWidths());
			if(line == scrolls_.firstVisibleLine.line) {
				if(scrolls_.firstVisibleLine.subline == 0)
					return spaces.top;
				else
					return fullSearch ? spaces.top
						- static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * scrolls_.firstVisibleLine.subline) : numeric_limits<Scalar>::min();
			} else if(line > scrolls_.firstVisibleLine.line) {
				const Scalar lineSpan = renderer_->defaultFont()->metrics().linePitch();
				const graphics::Rectangle clientBounds(bounds(false));
				Scalar y = spaces.top;
				y += lineSpan * static_cast<Scalar>(
					renderer_->layouts().numberOfSublinesOfLine(scrolls_.firstVisibleLine.line) - scrolls_.firstVisibleLine.subline);
				for(Index i = scrolls_.firstVisibleLine.line + 1; i < line; ++i) {
					y += lineSpan * static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(i));
					if(y >= geometry::dy(clientBounds) && !fullSearch)
						return numeric_limits<Scalar>::max();
				}
				return y;
			} else if(!fullSearch)
				return numeric_limits<Scalar>::min();
			else {
				const Scalar linePitch = renderer_->defaultFont()->metrics().linePitch();
				Scalar y = spaces.top - static_cast<Scalar>(linePitch * scrolls_.firstVisibleLine.subline);
				for(Index i = scrolls_.firstVisibleLine.line - 1; ; --i) {
					y -= static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(i) * linePitch);
					if(i == line)
						break;
				}
				return y;
			}
		}
#endif

		/**
		 * Returns @c MouseInputStrategy object to handle the event occurred at the specified location.
		 * @param p The location where the event occurred, in viewer-local coordinates
		 * @return The @c MouseInputStrategy object or @c null
		 */
		std::shared_ptr<MouseInputStrategy> TextViewer::mouseInputStrategy(const graphics::Point& p) {
			if(!lockedMouseInputStrategy_.expired()) {
				if(const std::shared_ptr<MouseInputStrategy> temp = lockedMouseInputStrategy_.lock())
					return temp;
			}
			if(TextViewerComponent* const component = hitTest(p))
				return component->mouseInputStrategy().lock();
			return std::shared_ptr<MouseInputStrategy>();
		}

		/**
		 * Widget realization process completed.
		 */
		void TextViewer::realized() BOOST_NOEXCEPT {
		}

		/// @internal
		void TextViewer::realizeWidget() {
			initializeNativeWidget();

			textArea_.reset(new TextArea());
			static_cast<TextViewerComponent*>(textArea_.get())->install(*this, *this);
			writingModesChangedConnection_ =
				textArea()->textRenderer()->writingModesChangedSignal().connect(
					std::bind(&TextViewer::writingModesChanged, this, std::placeholders::_1));

			const auto viewport(textArea()->viewport());
			//			viewportResizedConnection_ = viewport->resizedSignal().connect([this](const graphics::Dimension&) {
			//				this->updateScrollBars(presentation::FlowRelativeTwoAxes<bool>(true, true), presentation::FlowRelativeTwoAxes<bool>(true, true));
			//			});
			viewportScrolledConnection_ = viewport->scrolledSignal().connect(
				[this](const presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::ScrollOffset>&, const graphics::font::VisualLine&) {
				assert(!this->isFrozen());
				// update the scroll positions
				this->updateScrollBars(presentation::FlowRelativeTwoAxes<bool>(true, true), presentation::FlowRelativeTwoAxes<bool>(false, false));
				//					this->closeCompletionProposalsPopup(*this);
				this->hideToolTip();
			}
			);
			viewportScrollPropertiesChangedConnection_ = viewport->scrollPropertiesChangedSignal().connect([this](const presentation::FlowRelativeTwoAxes<bool>&) {
				this->updateScrollBars(presentation::FlowRelativeTwoAxes<bool>(true, true), presentation::FlowRelativeTwoAxes<bool>(true, true));
			});

			realized();
		}

		/**
		 * Updates the configurations.
		 * @param newConfiguration The new configurations
		 * @param synchronizeUI Set @c true to change the window style according to the new style. This sets
		 *                      @c WS_EX_LEFTSCROLLBAR, @c WS_EX_RIGHTSCROLLBAR, @c WS_EX_LTRREADING and
		 *                      @c WS_EX_RTLREADING styles
		 * @throw UnknownValueException The content of @a verticalRuler is invalid
		 */
		void TextViewer::setConfiguration(const Configuration& newConfiguration, bool synchronizeUI) {
//			const Inheritable<ReadingDirection> oldReadingDirection(configuration_.readingDirection);
//			assert(!oldReadingDirection.inherits());
			configuration_ = newConfiguration;
//			textRenderer().viewport()->setBoundsInView(textAreaContentRectangle());	// TODO: Should we call resized() instead?
//			renderer_->layouts().invalidate();

//			if((oldReadingDirection == LEFT_TO_RIGHT && configuration_.readingDirection == RIGHT_TO_LEFT)
//					|| (oldReadingDirection == RIGHT_TO_LEFT && configuration_.readingDirection == LEFT_TO_RIGHT))
//				scrolls_.horizontal.position = scrolls_.horizontal.maximum
//					- scrolls_.horizontal.pageSize - scrolls_.horizontal.position + 1;
//			scrolls_.resetBars(*this, 'a', false);
//			updateScrollBars(FlowRelativeTwoAxes<bool>(true, true), FlowRelativeTwoAxes<bool>(true, true));

#ifdef ASCENSION_USE_SYSTEM_CARET
			if(!isFrozen() && (widgetapi::hasFocus(*this) /*|| handle() == Viewer::completionWindow_->getSafeHwnd()*/)) {
				caret().resetVisualization();
				caret().updateLocation();
			}
#endif
			if(synchronizeUI) {
				if(const auto ta = textArea()) {
					if(const auto renderer = ta->textRenderer()) {
						const auto direction = renderer->inlineFlowDirection();
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
						if(get_direction() != Gtk::TEXT_DIR_NONE)
							set_direction((direction == presentation::LEFT_TO_RIGHT) ? Gtk::TEXT_DIR_LTR : Gtk::TEXT_DIR_RTL);
//						set_placement();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
						auto style = win32::getWindowLong(handle().get(), GWL_EXSTYLE);
						if(direction == presentation::LEFT_TO_RIGHT) {
							style &= ~(WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
							style |= WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
						} else {
							style &= ~(WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
							style |= WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR;
						}
						win32::setWindowLong(handle().get(), GWL_EXSTYLE, style);
#endif
					}
				}
			}
			widgetapi::scheduleRedraw(*this, false);
		}

		/**
		 * Sets the new content assistant.
		 * @param newContentAssistant the content assistant to set. the ownership will be transferred to the callee.
		 */
		void TextViewer::setContentAssistant(std::unique_ptr<contentassist::ContentAssistant> newContentAssistant) BOOST_NOEXCEPT {
			if(contentAssistant_.get() != nullptr)
				contentAssistant_->uninstall();	// $friendly-access
			(contentAssistant_ = std::move(newContentAssistant))->install(*this);	// $friendly-access
		}

		/**
		 * Shows the tool tip at the cursor position.
		 * @param text the text to be shown. CRLF represents a line break. this can not contain any NUL character
		 * @param timeToWait the time to wait in milliseconds. -1 to use the system default value
		 * @param timeRemainsVisible the time remains visible in milliseconds. -1 to use the system default value
		 * @deprecated 0.8
		 */
		void TextViewer::showToolTip(const String& text, unsigned long timeToWait /* = -1 */, unsigned long timeRemainsVisible /* = -1 */) {
//			checkInitialization();

			hideToolTip();
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			if(timeToWait == -1)
				timeToWait = ::GetDoubleClickTime();
			tipText_.assign(text.cbegin(), text.cend());
			::SetTimer(handle().get(), TIMERID_CALLTIP, timeToWait, nullptr);
#endif
		}

#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
		HRESULT TextViewer::startTextServices() {
			assertValid();
			ComPtr<ITfThreadMgr> threadManager;
			HRESULT hr = threadManager.createInstance(CLSID_TF_ThreadMgr, 0, CLSCTX_INPROC_SERVER);
			if(FAILED(hr))
				return hr;
			ComPtr<ITfDocumentMgr> documentManager;
			hr = threadManager->CreateDocumentMgr(&documentManager);
			if(FAILED(hr))
				return hr;
			ComPtr<ITfContext> context;
			documentManager->CreateContext(...);
			...
		}
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK

		/**
		 * Revokes the frozen state of the viewer.
		 * @throw WindowNotInitialized The window is not initialized
		 * @note This method thaws also viewer's @c TextViewport.
		 * @see #freeze, #isFrozen
		 */
		void TextViewer::unfreeze() {
//			checkInitialization();
			if(isFrozen()) {
				--frozenCount_;
				if(!isFrozen()) {
					unfrozen();
					frozenStateChangedSignal_(*this);
				}
			}
		}

		/**
		 * This method is called when the frozen state of the viewer was revoked.
		 * If the derived class overriden this method, should call @c TextViewer#unfrozen.
		 * @param linesToRedraw The line numbers which were scheduled to redraw
		 * @note @c TextViewer#unfrozen calls @c widgetapi#redrawScheduledRegion.
		 */
		void TextViewer::unfrozen() {
/*			if(scrolls_.changed) {
				updateScrollBars();
				widgetapi::scheduleRedraw(*this, false);
			}*/
		}

		/// @see MouseInputStrategy#TargetLocker#unlockMouseInputTarget
		void TextViewer::unlockMouseInputTarget(MouseInputStrategy& strategy) BOOST_NOEXCEPT {
			if(&strategy == lockedMouseInputStrategy_.lock().get())
				lockedMouseInputStrategy_.reset();
		}

		/**
		 * @internal Updates the scroll information.
		 * @param positions Describes which position(s) to update
		 * @param properties Describes which property(ies) to update
		 */
		void TextViewer::updateScrollBars(const presentation::FlowRelativeTwoAxes<bool>& positions, const presentation::FlowRelativeTwoAxes<bool>& properties) {
//			checkInitialization();
			assert(!isFrozen());
			const auto needsUpdate = [](bool v) {return v;};
			const auto ta(textArea());
			if(ta.get() == nullptr
					|| (std::none_of(std::begin(positions), std::end(positions), needsUpdate) && std::none_of(std::begin(properties), std::end(properties), needsUpdate)))
				return;
			const auto renderer(ta->textRenderer());
			const auto viewport(ta->viewport());
			if(renderer.get() == nullptr || viewport.get() == nullptr)
				return;

			assert(!isFrozen());

			// update the scroll bar in inline-progression-dimension
			if(positions.ipd() || properties.ipd()) {
				const auto viewportRange(graphics::font::scrollableRange<presentation::ReadingDirection>(*viewport));
				boost::optional<widgetapi::NativeScrollPosition> position, size;
				boost::optional<NumericRange<widgetapi::NativeScrollPosition>> range;
				if(positions.ipd())
					// TODO: Use reverseScrollPosition().
					position = (renderer->inlineFlowDirection() == presentation::LEFT_TO_RIGHT) ?
						viewport->scrollPositions().ipd() : (*boost::const_end(viewportRange) - viewport->scrollPositions().ipd() - 1);
				if(properties.ipd()) {
					const float realSize = graphics::font::pageSize<presentation::ReadingDirection>(*viewport);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
#	error Not implemented.
#else
					range = nrange<widgetapi::NativeScrollPosition>(
						*boost::const_begin(viewportRange), *boost::const_end(viewportRange) + static_cast<widgetapi::NativeScrollPosition>(std::ceil(realSize)) - 1);
					size = static_cast<widgetapi::NativeScrollPosition>(boost::math::trunc(realSize));
#endif
				}
				configureScrollBar(*this, presentation::isHorizontal(renderer->blockFlowDirection()) ? 0 : 1, position, range, size);
			}

			// update the scroll bar in block-progression-dimension
			if(positions.bpd() || properties.bpd()) {
				const auto viewportRange(graphics::font::scrollableRange<presentation::BlockFlowDirection>(*viewport));
				boost::optional<widgetapi::NativeScrollPosition> position, size;
				boost::optional<NumericRange<widgetapi::NativeScrollPosition>> range;
				if(positions.bpd())
					// TODO: Use reverseScrollPosition().
					position = (renderer->blockFlowDirection() != presentation::VERTICAL_RL) ?
						viewport->scrollPositions().bpd() : (*boost::const_end(viewportRange) - viewport->scrollPositions().bpd() - 1);
				if(properties.bpd()) {
					const float realSize = graphics::font::pageSize<presentation::BlockFlowDirection>(*viewport);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
#	error Not implemented.
#else
					range = nrange<widgetapi::NativeScrollPosition>(
						*boost::const_begin(viewportRange), *boost::const_end(viewportRange) + static_cast<widgetapi::NativeScrollPosition>(std::ceil(realSize)) - 1);
					size = static_cast<widgetapi::NativeScrollPosition>(boost::math::trunc(realSize));
#endif
				}
				configureScrollBar(*this, presentation::isHorizontal(renderer->blockFlowDirection()) ? 1 : 0, position, range, size);
			}
		}

		/**
		 * Updates the 'allocation-rectangle' of the @c TextArea.
		 * The subclass should call this method when changed the layout of the @c TextArea.
		 */
		void TextViewer::updateTextAreaAllocationRectangle() {
			static_cast<TextViewerComponent&>(*textArea()).relocated();
		}

		/// @see graphics#font#TextRenderer#WritingModesChangedSignal
		void TextViewer::writingModesChanged(const graphics::font::TextRenderer&) {
			updateScrollBars(presentation::FlowRelativeTwoAxes<bool>(true, true), presentation::FlowRelativeTwoAxes<bool>(true, true));
		}


		// AutoFreeze /////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::viewer::AutoFreeze
		 *
		 * Calls automatically @c TextViewer#freeze and @c TextViewer#unfreeze.
		 *
		 * @code
		 * extern TextViewer* target;
		 * AutoFreeze af(target);
		 * target-&gt;mayThrow();
		 * // target-&gt;unfreeze() will be called automatically
		 * @endcode
		 *
		 * @note This class is not intended to be subclassed.
		 */

		/**
		 * Constructor calls @c TextViewer#freeze.
		 * @param textViewer The text viewer this object manages. If this is @c null, the object does nothing at all
		 * @throw ... Any exceptions @c TextViewer#freeze throws
		 */
		AutoFreeze::AutoFreeze(TextViewer* textViewer) : textViewer_(textViewer) {
			if(textViewer_ != nullptr)
				textViewer_->freeze();
		}

		/// Destructor calls @c TextViewer#unfreeze.
		AutoFreeze::~AutoFreeze() BOOST_NOEXCEPT {
			try {
				if(textViewer_ != nullptr)
					textViewer_->unfreeze();
			} catch(...) {
				// ignore
			}
		}


		// TextViewer.Configuration ///////////////////////////////////////////////////////////////////////////////////

		/// Default constructor.
		TextViewer::Configuration::Configuration() BOOST_NOEXCEPT : usesRichTextClipboardFormat(false) {
#if(_WIN32_WINNT >= 0x0501)
			BOOL b;
			if(::SystemParametersInfoW(SPI_GETMOUSEVANISH, 0, &b, 0) != 0)
				vanishesCursor = win32::boole(b);
			else
				vanishesCursor = false;
#else
			vanishesCursor = false;
#endif // _WIN32_WINNT >= 0x0501
		}


		// TextViewer.MouseVanisher ///////////////////////////////////////////////////////////////////////////////////

		TextViewer::MouseVanisher::MouseVanisher(TextViewer& target) BOOST_NOEXCEPT : target_(target), hidden_(false) {
		}

		TextViewer::MouseVanisher::~MouseVanisher() BOOST_NOEXCEPT {
			restoreHiddenCursor();
		}

		void TextViewer::MouseVanisher::restoreHiddenCursor() {
			if(hidesCursor()) {
				widgetapi::Cursor::show();
				widgetapi::releaseInput(target_);
				hidden_ = false;
			}
		}

		void TextViewer::MouseVanisher::hideCursor() {
			if(!hidesCursor() && target_.configuration().vanishesCursor && widgetapi::hasFocus(target_)) {
				hidden_ = true;
				widgetapi::Cursor::hide();
				widgetapi::grabInput(target_);
			}
		}

		bool TextViewer::MouseVanisher::hidesCursor() const BOOST_NOEXCEPT {
			return hidden_;
		}
	}
}
