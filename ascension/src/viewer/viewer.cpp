/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2014
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/rules.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-caret-shaper.hpp>
#include <ascension/viewer/default-mouse-input-strategy.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <limits>	// std.numeric_limit
#include <boost/foreach.hpp>
#ifdef _DEBUG
#	include <boost/log/trivial.hpp>
//#	define ASCENSIOB_DIAGNOSE_INHERENT_DRAWING
//#	define ASCENSION_TRACE_DRAWING_STRING
#endif // _DEBUG

namespace ascension {
	namespace viewers {
		namespace {
			/// @internal Maps the given point in viewer-local coordinates into a point in text-area coordinates.
			inline graphics::Point mapLocalToTextArea(const TextViewer& viewer, const graphics::Point& p) {
				const graphics::Rectangle textArea(viewer.textAreaAllocationRectangle());
				graphics::Point temp(p);
				return graphics::geometry::translate(temp,
					graphics::Dimension(
						graphics::geometry::_dx = -graphics::geometry::left(textArea),
						graphics::geometry::_dy = -graphics::geometry::top(textArea)));
			}
			/// @internal Maps the given point in text-area coordinates into a point in viewer-local coordinates.
			inline graphics::Point mapTextAreaToLocal(const TextViewer& viewer, const graphics::Point& p) {
				const graphics::Rectangle textArea(viewer.textAreaAllocationRectangle());
				graphics::Point temp(p);
				return graphics::geometry::translate(temp,
					graphics::Dimension(
						graphics::geometry::_dx = +graphics::geometry::left(textArea),
						graphics::geometry::_dy = +graphics::geometry::top(textArea)));
			}
		}	// namespace @0


		// TextViewer /////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::viewers::TextViewer
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
		 * Constructor.
		 * @param presentation The presentation object
		 */
		TextViewer::TextViewer(presentation::Presentation& presentation) :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::ObjectBase("ascension.viewers.TextViewer"),
#endif
				presentation_(presentation), mouseInputDisabledCount_(0) {
			initialize(nullptr);

			// initializations of renderer_ and mouseInputStrategy_ are in initializeWindow()
		}

		/**
		 * Copy-constructor. Unlike @c win32#Object class, this does not copy the window handle. For
		 * more details, see the description of @c TextViewer.
		 */
		TextViewer::TextViewer(const TextViewer& other) :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::ObjectBase("ascension.viewers.TextViewer"),
#endif
				presentation_(other.presentation_), mouseInputDisabledCount_(0) {
			initialize(&other);
			modeState_ = other.modeState_;
		}

		/// Destructor.
		TextViewer::~TextViewer() {
			document().removeListener(*this);
			document().removeRollbackListener(*this);
			textRenderer().removeComputedBlockFlowDirectionListener(*this);
			textRenderer().removeDefaultFontListener(*this);
			textRenderer().layouts().removeVisualLinesListener(*this);
			BOOST_FOREACH(VisualPoint* p, points_)
				p->viewerDisposed();

			// 非共有データ
		//	delete selection_;
		}

		/// @see CaretListener#caretMoved
		void TextViewer::caretMoved(const Caret& self, const kernel::Region& oldRegion) {
			if(!widgetapi::isVisible(*this))
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
						if(!isFrozen())
							widgetapi::redrawScheduledRegion(*this);
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
							if(!isFrozen())
								widgetapi::redrawScheduledRegion(*this);
							redrawLines(boost::irange(newRegion.beginning().line, newRegion.end().line + 1));
						}
					}
				}
				changed = true;
			}

			if(changed && !isFrozen())
				widgetapi::redrawScheduledRegion(*this);
		}

		/// @see ComputedWritingModeListener#computedBlockFlowDirectionChanged
		void TextViewer::computedBlockFlowDirectionChanged(presentation::BlockFlowDirection used) {
			updateScrollBars(presentation::AbstractTwoAxes<bool>(true, true), presentation::AbstractTwoAxes<bool>(true, true));
		}

		/// @see DefaultFontListener#defaultFontChanged
		void TextViewer::defaultFontChanged() BOOST_NOEXCEPT {
			rulerPainter_->update();
#ifdef ASCENSION_USE_SYSTEM_CARET
			caret().resetVisualization();
#endif
			redrawLine(0, true);
		}

		/// @see kernel#DocumentListener#documentAboutToBeChanged
		void TextViewer::documentAboutToBeChanged(const kernel::Document&) {
			// do nothing
		}

		/// @see kernel#DocumentListener#documentChanged
		void TextViewer::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
			// cancel the active incremental search
			texteditor::abortIncrementalSearch(*this);	// TODO: should TextViewer handle this? (I.S. would...)

			// slide the frozen lines to be drawn
			if(isFrozen() && !freezeRegister_.linesToRedraw().empty()) {
				Index b = *freezeRegister_.linesToRedraw().begin();
				Index e = *freezeRegister_.linesToRedraw().end();
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
				freezeRegister_.resetLinesToRedraw(boost::irange(b, e));
			}
		//	invalidateLines(region.beginning().line, !multiLine ? region.end().line : INVALID_INDEX);
			if(!isFrozen())
				rulerPainter_->update();
		}

		/// @see kernel#DocumentRollbackListener#documentUndoSequenceStarted
		void TextViewer::documentUndoSequenceStarted(const kernel::Document&) {
			freeze();	// TODO: replace with AutoFreeze.
		}

		/// @see kernel#DocumentRollbackListener#documentUndoSequenceStopped
		void TextViewer::documentUndoSequenceStopped(const kernel::Document&, const kernel::Position& resultPosition) {
			unfreeze();	// TODO: replace with AutoFreeze.
			if(/*resultPosition != kernel::Position() &&*/ widgetapi::hasFocus(*this)) {
				utils::closeCompletionProposalsPopup(*this);
				caret_->moveTo(resultPosition);
			}
		}

		void TextViewer::doShowContextMenu(void* nativeEvent) {
			namespace geom = graphics::geometry;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			const Gdk::Event abstractEvent(Glib::wrap(static_cast<GdkEvent*>(nativeEvent)));
			bool byKeyboard;
			switch(abstractEvent.gobj()->type) {
				case Gdk::BUTTON_RELEASE:
					byKeyboard = false;
					break;
				case Gdk::KEY_RELEASE:
					byKeyboard = true;
					break;
				default:
					return;
			}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			const MSG& message = *static_cast<const MSG*>(nativeEvent);
			const auto globalLocation(win32::makeMouseLocation<geom::BasicPoint<WORD>>(message.lParam));
			const bool byKeyboard = geom::x(globalLocation) == 0xffffu && geom::y(globalLocation) == 0xffffu;
#endif

			if(!allowsMouseInput() && !byKeyboard)	// however, may be invoked by other than the mouse...
				return;
			utils::closeCompletionProposalsPopup(*this);
			texteditor::abortIncrementalSearch(*this);

			graphics::Point location;
			widgetapi::LocatedUserInput::MouseButton buttons;
			widgetapi::UserInput::KeyboardModifier modifiers;

			// invoked by the keyboard
			if(byKeyboard) {
				// MSDN says "the application should display the context menu at the location of the current selection."
				location = graphics::font::modelToView(*textRenderer().viewport(), graphics::font::TextHit<kernel::Position>::leading(caret()));
				// TODO: Support RTL and vertical window layout.
				graphics::geometry::y(location) += widgetapi::createRenderingContext(*this)->fontMetrics(textRenderer().defaultFont())->cellHeight() + 1;
				if(!boost::geometry::within(location, textAreaContentRectangle()))
					location = graphics::Point(geom::_x = 1.0f, geom::_y = 1.0f);
			} else {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				double x, y;
				if(!abstractEvent.get_coords(x, y))
					return;	// hmm...
				Gdk::ModifierType state;
				if(!abstractEvent.get_state(state))
					return;
				location = graphics::Point(geom::_x = x, geom::_y = y);
				static const Gdk::ModifierType NATIVE_BUTTON_MASK = Gdk::BUTTON1_MASK | Gdk::BUTTON2_MASK | Gdk::BUTTON3_MASK | Gdk::BUTTON4_MASK | Gdk::BUTTON5_MASK;
				buttons = !byKeyboard ? (state & NATIVE_BUTTON_MASK) : widgetapi::LocatedUserInput::NO_BUTTON;
				modifiers = state & ~NATIVE_BUTTON_MASK;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				location = graphics::Point(geom::_x = geom::x(globalLocation), geom::_y = geom::y(globalLocation));
				widgetapi::mapFromGlobal(*this, location);
				buttons = widgetapi::LocatedUserInput::NO_BUTTON;
				modifiers = win32::makeModifiers();
#endif
			}

			// ignore if the point is over the scroll bars
			const graphics::Rectangle localBounds(widgetapi::bounds(*this, false));
			if(!boost::geometry::within(location, localBounds))
				return;

			return showContextMenu(widgetapi::LocatedUserInput(location, buttons, modifiers), nativeEvent);
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
		void TextViewer::focusAboutToBeLost(widgetapi::Event& event) {
			cursorVanisher_.restore();
			if(mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->interruptMouseReaction(false);
/*			if(caret_->getMatchBracketsTrackingMode() != Caret::DONT_TRACK
					&& getCaret().getMatchBrackets().first != Position::INVALID_POSITION) {	// 対括弧の通知を終了
				FOR_EACH_LISTENERS()
					(*it)->onMatchBracketFoundOutOfView(Position::INVALID_POSITION);
			}
			if(completionWindow_->isWindow() && newWindow != completionWindow_->getSafeHwnd())
				closeCompletionProposalsPopup(*this);
*/			texteditor::abortIncrementalSearch(*this);
			static_cast<detail::InputEventHandler&>(caret()).abortInput();
//			if(currentWin32WindowMessage().wParam != get()) {
//				hideCaret();
//				::DestroyCaret();
//			}
			redrawLines(boost::irange(kernel::line(caret().beginning()), kernel::line(caret().end()) + 1));
			widgetapi::redrawScheduledRegion(*this);

			return event.consume();
		}

		/// Invoked when the widget gained the keyboard focus.
		void TextViewer::focusGained(widgetapi::Event& event) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// restore the scroll positions
			const auto scrollPositions(physicalScrollPosition(*this));
			configureScrollBar(*this, 0, boost::geometry::get<0>(scrollPositions), boost::none, boost::none);
			configureScrollBar(*this, 1, boost::geometry::get<1>(scrollPositions), boost::none, boost::none);
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

			// hmm...
//			if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
				redrawLines(boost::irange(kernel::line(caret().beginning()), kernel::line(caret().end()) + 1));
				widgetapi::redrawScheduledRegion(*this);
//			}

//			if(currentWin32WindowMessage().wParam != get()) {
//				// resurrect the caret
//				recreateCaret();
//				updateCaretPosition();
//				if(texteditor::Session* const session = document().session()) {
//					if(texteditor::InputSequenceCheckers* const isc = session->inputSequenceCheckers())
//						isc->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
//				}
//			}

			return event.consume();
		}

		/**
		 * Freezes the drawing of the viewer.
		 * @throw WindowNotInitialized The window is not initialized
		 * @note This method freezes also viewer's @c TextViewport.
		 * @see #isFrozen, #unfreeze, #AutoFreeze
		 */
		void TextViewer::freeze() {
//			checkInitialization();
			const std::shared_ptr<graphics::font::TextViewport> viewport(textRenderer().viewport());
			if(viewport.get() == nullptr)
				throw IllegalStateException("There is no viewport in this text viewer.");
			freezeRegister_.freeze();	// this may throw std.overflow_error
			try {
				viewport->freezeNotification();
			} catch(const std::overflow_error&) {
				freezeRegister_.thaw();
				throw;
			}
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

			if(pos.offsetInLine == document.getLineLength(pos.line))	// 指定位置に文字が無い
				return false;

			const LineLayout& layout = renderer_->getLineLayout(pos.line);
			const Index subline = layout.getSubline(pos.offsetInLine);
			const Char* const line = document.getLine(pos.line).data();
			const Char* const first = line + layout.getSublineOffset(subline);
			const Char* const last =
				line + ((subline < layout.getNumberOfSublines() - 1) ? layout.getSublineOffset(subline + 1) : document.getLineLength(pos.line));
			Index linkLength;	// URIDetector の eatMailAddress 、eatUrlString で見つけたリンクテキストの長さ

			for(const Char* p = (pos.offsetInLine > 200) ? first + pos.offsetInLine - 200 : first; p <= first + pos.offsetInLine; ) {
				if(p != first) {
					if((p[-1] >= L'A' && p[-1] <= L'Z')
							|| (p[-1] >= L'a' && p[-1] <= L'z')
							|| p[-1] == L'_') {
						++p;
						continue;
					}
				}
				if(0 != (linkLength = rules::URIDetector::eatURL(p, last, true) - p)) {
					if(p - first + linkLength > pos.offsetInLine) {	// カーソル位置を越えた
						region.first.line = region.second.line = pos.line;
						region.first.offsetInLine = p - line;
						region.second.offsetInLine = region.first.offsetInLine + linkLength;
						text.reset(new Char[linkLength + 1]);
						wmemcpy(text.get(), p, linkLength);
						text[linkLength] = 0;
						return true;
					}
					p += linkLength;	// 届かない場合は続行
				} else if(0 != (linkLength = rules::URIDetector::eatMailAddress(p, last, true) - p)) {
					if(p - first + linkLength > pos.offsetInLine) {	// カーソル位置を越えた
						static const wchar_t MAILTO_PREFIX[] = L"mailto:";
						region.first.line = region.second.line = pos.line;
						region.first.offsetInLine = p - line;
						region.second.offsetInLine = region.first.offsetInLine + linkLength;
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

		namespace {
			template<typename Coordinate>
			inline widgetapi::NativeScrollPosition reverseScrollPosition(const TextViewer& textViewer, widgetapi::NativeScrollPosition position) {
				const graphics::font::TextRenderer& textRenderer = textViewer.textRenderer();
//				return static_cast<widgetapi::NativeScrollPosition>(textRenderer.layouts().maximumMeasure()
//					/ widgetapi::createRenderingContext(textViewer)->fontMetrics(textRenderer.defaultFont())->averageCharacterWidth())
//					- position
//					- static_cast<widgetapi::NativeScrollPosition>(textRenderer.viewport()->numberOfVisibleCharactersInLine());
				const presentation::BlockFlowDirection blockFlowDirection = textRenderer.computedBlockFlowDirection();
				const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer.viewport());
				return *graphics::font::scrollableRange<Coordinate>(*viewport).end() - position - graphics::font::pageSize<Coordinate>(*viewport);
			}
			graphics::PhysicalTwoAxes<widgetapi::NativeScrollPosition>&& physicalScrollPosition(const TextViewer& textViewer) {
				const graphics::font::TextRenderer& textRenderer = textViewer.textRenderer();
				const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer.viewport());
				const presentation::WritingMode writingMode(textViewer.presentation().computeWritingMode(&textRenderer));
				const presentation::AbstractTwoAxes<graphics::font::TextViewportScrollOffset> scrollPositions(viewport->scrollPositions());
				graphics::PhysicalTwoAxes<widgetapi::NativeScrollPosition> result;
				switch(writingMode.blockFlowDirection) {
					case presentation::HORIZONTAL_TB:
						result.x() = (writingMode.inlineFlowDirection == presentation::LEFT_TO_RIGHT) ?
							scrollPositions.ipd() : reverseScrollPosition<presentation::ReadingDirection>(textViewer, static_cast<widgetapi::NativeScrollPosition>(scrollPositions.ipd()));
						result.y() = scrollPositions.bpd();
						break;
					case presentation::VERTICAL_RL:
						result.x() = reverseScrollPosition<presentation::BlockFlowDirection>(textViewer, scrollPositions.bpd());
						result.y() = scrollPositions.ipd();
						break;
					case presentation::VERTICAL_LR:
						result.x() = scrollPositions.bpd();
						result.y() = scrollPositions.ipd();
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
//				result.x() /= xScrollRate;
//				result.y() /= yScrollRate;
				return std::move(result);
			}
		}

		namespace {
			template<typename AbstractCoordinate>
			inline widgetapi::NativeScrollPosition calculateScrollStepSize(const TextViewer& viewer) {return 1;}	// TODO: Not implemented.
			template<std::size_t physicalCoordinate>
			inline widgetapi::NativeScrollPosition calculateScrollStepSize(const TextViewer& viewer) {return 1;}	// TODO: Not implemented.
			void configureScrollBar(TextViewer& viewer, std::size_t coordinate, const boost::optional<widgetapi::NativeScrollPosition>& position,
					const boost::optional<boost::integer_range<widgetapi::NativeScrollPosition>>& range, const boost::optional<widgetapi::NativeScrollPosition>& pageSize) {
				assert(coordinate <= 1);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gtk::Adjustment> adjustment((coordinate == 0) ? viewer.get_hadjustment() : viewer.get_vadjustment());
				if(range) {
					adjustment->set_lower(*range->begin());
					adjustment->set_upper(*range->end());
				}
				adjustment->set_step_increment((coordinate == 0) ? calculateScrollStepSize<0>(viewer) : calculateScrollStepSize<1>(viewer));
				if(pageSize != boost::none) {
					adjustment->set_page_increment(*pageSize);
					adjustment->set_page_size(*pageSize);
				}
				if(position != boost::none)
					adjustment->set_value(*position);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QScrollBar* const scrollBar = (coordinate == 0) ? viewer.horizontalScrollBar() : viewer.verticalScrollBar();
				if(range != boost::none)
					scrollBar->setRange(range->beginning(), range->end());
				scrollBar->setSingleStep((coordinate == 0) ? calculateScrollStepSize<0>(viewer) : calculateScrollStepSize<1>(viewer));
				if(pageSize != boost::none)
					scrollBar->setPageStep(*pageSize);
				if(position != boost::none)
					scrollBar->setSliderPosition(*position);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::AutoZeroSize<SCROLLINFO> si;
				if(range/* != boost::none*/) {
					si.fMask |= SIF_RANGE;
					si.nMin = *range->begin();
					si.nMax = *range->end();
				}
				if(pageSize != boost::none) {
					si.fMask |= SIF_PAGE;
					si.nPage = *pageSize;
				}
				if(position != boost::none) {
					si.fMask |= SIF_POS;
					si.nPos = *position;
				}
				::SetScrollInfo(viewer.handle().get(), (coordinate == 0) ? SB_HORZ : SB_VERT, &si, true);
#endif
			}
		}

		/**
		 * Hides the caret.
		 * @see #hidesCaret, #showCaret
		 */
		void TextViewer::hideCaret() BOOST_NOEXCEPT {
			if(!hidesCaret()) {
				caretBlinker_.reset();
				redrawLine(kernel::line(caret()));
			}
		}

		/**
		 * Determines which part is at the specified position.
		 * @param p The position to hit test, in the viewer-local coordinates
		 * @return The result
		 * @see TextViewer#HitTestResult
		 */
		TextViewer::HitTestResult TextViewer::hitTest(const graphics::Point& p) const {
//			checkInitialization();
			const graphics::Rectangle localBounds(widgetapi::bounds(*this, false));
			if(!boost::geometry::within(p, localBounds))
				return OUT_OF_VIEWER;

			const RulerStyles& rulerStyles = rulerPainter_->declaredStyles();
			if(indicatorMargin(rulerStyles)->visible && boost::geometry::within(p, rulerPainter_->indicatorMarginAllocationRectangle()))
				return INDICATOR_MARGIN;
			else if(lineNumbers(rulerStyles)->visible && boost::geometry::within(p, rulerPainter_->lineNumbersAllocationRectangle()))
				return LINE_NUMBERS;
			else if(boost::geometry::within(p, textAreaContentRectangle()))
				return TEXT_AREA_CONTENT_RECTANGLE;
			else {
				assert(boost::geometry::within(p, textAreaAllocationRectangle()));
				return TEXT_AREA_PADDING_START;
			}
		}

		/// @internal Called by constructors.
		void TextViewer::initialize(const TextViewer* other) {
			document().addListener(*this);
			document().addRollbackListener(*this);

//			updateScrollBars(AbstractTwoAxes<bool>(true, true), AbstractTwoAxes<bool>(true, true));
			setMouseInputStrategy(std::shared_ptr<MouseInputStrategy>());

#ifdef ASCENSION_TEST_TEXT_STYLES
			RulerConfiguration rc;
			rc.lineNumbers.visible = true;
			rc.indicatorMargin.visible = true;
			rc.lineNumbers.foreground = Paint(Color(0x00, 0x80, 0x80));
			rc.lineNumbers.background = Paint(Color(0xff, 0xff, 0xff));
			rc.lineNumbers.borderEnd.color = Color(0x00, 0x80, 0x80);
			rc.lineNumbers.borderEnd.style = Border::DOTTED;
			rc.lineNumbers.borderEnd.width = Length(1);
			setConfiguration(nullptr, &rc, false);

#if 0
			// this is JavaScript partitioning and lexing settings for test
			using namespace contentassist;
			using namespace rules;
			using namespace text;

			static const ContentType JS_MULTILINE_DOC_COMMENT = 140,
				JS_MULTILINE_COMMENT = 142, JS_SINGLELINE_COMMENT = 143, JS_DQ_STRING = 144, JS_SQ_STRING = 145;

			class JSContentTypeInformation : public IContentTypeInformationProvider {
			public:
				JSContentTypeInformation()  {
					jsIDs_.overrideIdentifierStartCharacters(L"_", L""); jsdocIDs_.overrideIdentifierStartCharacters(L"$@", L"");}
				const IdentifierSyntax& getIdentifierSyntax(ContentType contentType) const {
					return (contentType != JS_MULTILINE_DOC_COMMENT) ? jsIDs_ : jsdocIDs_;}
			private:
				IdentifierSyntax jsIDs_, jsdocIDs_;
			};
			JSContentTypeInformation* cti = new JSContentTypeInformation;

			TransitionRule* rules[12];
			rules[0] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_MULTILINE_DOC_COMMENT, L"/**");
			rules[1] = new LiteralTransitionRule(JS_MULTILINE_DOC_COMMENT, DEFAULT_CONTENT_TYPE, L"*/");
			rules[2] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_MULTILINE_COMMENT, L"/*");
			rules[3] = new LiteralTransitionRule(JS_MULTILINE_COMMENT, DEFAULT_CONTENT_TYPE, L"*/");
			rules[4] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_SINGLELINE_COMMENT, L"//");
			rules[5] = new LiteralTransitionRule(JS_SINGLELINE_COMMENT, DEFAULT_CONTENT_TYPE, L"", L'\\');
			rules[6] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_DQ_STRING, L"\"");
			rules[7] = new LiteralTransitionRule(JS_DQ_STRING, DEFAULT_CONTENT_TYPE, L"\"", L'\\');
			rules[8] = new LiteralTransitionRule(JS_DQ_STRING, DEFAULT_CONTENT_TYPE, L"");
			rules[9] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_SQ_STRING, L"\'");
			rules[10] = new LiteralTransitionRule(JS_SQ_STRING, DEFAULT_CONTENT_TYPE, L"\'", L'\\');
			rules[11] = new LiteralTransitionRule(JS_SQ_STRING, DEFAULT_CONTENT_TYPE, L"");
			LexicalPartitioner* p = new LexicalPartitioner();
			p->setRules(rules, ASCENSION_ENDOF(rules));
			for(std::size_t i = 0; i < ASCENSION_COUNTOF(rules); ++i)
				delete rules[i];
			document().setPartitioner(unique_ptr<DocumentPartitioner>(p));

			PresentationReconstructor* pr = new PresentationReconstructor(presentation());

			// JSDoc syntax highlight test
			static const Char JSDOC_ATTRIBUTES[] = L"@addon @argument @author @base @class @constructor @deprecated @exception @exec @extends"
				L" @fileoverview @final @ignore @link @member @param @private @requires @return @returns @see @throws @type @version";
			{
				unique_ptr<const WordRule> jsdocAttributes(new WordRule(220, JSDOC_ATTRIBUTES, ASCENSION_ENDOF(JSDOC_ATTRIBUTES) - 1, L' ', true));
				unique_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(JS_MULTILINE_DOC_COMMENT));
				scanner->addWordRule(jsdocAttributes);
				scanner->addRule(unique_ptr<Rule>(new URIRule(219, URIDetector::defaultIANAURIInstance(), false)));
				map<Token::ID, const TextStyle> jsdocStyles;
				jsdocStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle(Colors(Color(0x00, 0x80, 0x00)))));
				jsdocStyles.insert(make_pair(219, TextStyle(Colors(Color(0x00, 0x80, 0x00)), false, false, false, SOLID_UNDERLINE)));
				jsdocStyles.insert(make_pair(220, TextStyle(Colors(Color(0x00, 0x80, 0x00)), true)));
				unique_ptr<IPartitionPresentationReconstructor> ppr(
					new LexicalPartitionPresentationReconstructor(document(), unique_ptr<ITokenScanner>(scanner.release()), jsdocStyles));
				pr->setPartitionReconstructor(JS_MULTILINE_DOC_COMMENT, ppr);
			}

			// JavaScript syntax highlight test
			static const Char JS_KEYWORDS[] = L"Infinity break case catch continue default delete do else false finally for function"
				L" if in instanceof new null return switch this throw true try typeof undefined var void while with";
			static const Char JS_FUTURE_KEYWORDS[] = L"abstract boolean byte char class double enum extends final float goto"
				L" implements int interface long native package private protected public short static super synchronized throws transient volatile";
			{
				unique_ptr<const WordRule> jsKeywords(new WordRule(221, JS_KEYWORDS, ASCENSION_ENDOF(JS_KEYWORDS) - 1, L' ', true));
				unique_ptr<const WordRule> jsFutureKeywords(new WordRule(222, JS_FUTURE_KEYWORDS, ASCENSION_ENDOF(JS_FUTURE_KEYWORDS) - 1, L' ', true));
				unique_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(DEFAULT_CONTENT_TYPE));
				scanner->addWordRule(jsKeywords);
				scanner->addWordRule(jsFutureKeywords);
				scanner->addRule(unique_ptr<const Rule>(new NumberRule(223)));
				map<Token::ID, const TextStyle> jsStyles;
				jsStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle()));
				jsStyles.insert(make_pair(221, TextStyle(Colors(Color(0x00, 0x00, 0xff)))));
				jsStyles.insert(make_pair(222, TextStyle(Colors(Color(0x00, 0x00, 0xff)), false, false, false, DASHED_UNDERLINE)));
				jsStyles.insert(make_pair(223, TextStyle(Colors(Color(0x80, 0x00, 0x00)))));
				pr->setPartitionReconstructor(DEFAULT_CONTENT_TYPE,
					unique_ptr<IPartitionPresentationReconstructor>(new LexicalPartitionPresentationReconstructor(document(),
						unique_ptr<ITokenScanner>(scanner.release()), jsStyles)));
			}

			// other contents
			pr->setPartitionReconstructor(JS_MULTILINE_COMMENT, unique_ptr<IPartitionPresentationReconstructor>(
				new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
			pr->setPartitionReconstructor(JS_SINGLELINE_COMMENT, unique_ptr<IPartitionPresentationReconstructor>(
				new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
			pr->setPartitionReconstructor(JS_DQ_STRING, unique_ptr<IPartitionPresentationReconstructor>(
				new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
			pr->setPartitionReconstructor(JS_SQ_STRING, unique_ptr<IPartitionPresentationReconstructor>(
				new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
			new CurrentLineHighlighter(*caret_, Colors(Color(), Color::fromCOLORREF(::GetSysColor(COLOR_INFOBK))));

			// URL hyperlinks test
			unique_ptr<hyperlinkernel::CompositeHyperlinkDetector> hld(new hyperlinkernel::CompositeHyperlinkDetector);
			hld->setDetector(JS_MULTILINE_DOC_COMMENT, unique_ptr<hyperlinkernel::IHyperlinkDetector>(
				new hyperlinkernel::URIHyperlinkDetector(URIDetector::defaultIANAURIInstance(), false)));
			presentation().setHyperlinkDetector(hld.release(), true);

			// content assist test
			class JSDocProposals : public IdentifiersProposalProcessor {
			public:
				JSDocProposals(const IdentifierSyntax& ids) : IdentifiersProposalProcessor(JS_MULTILINE_DOC_COMMENT, ids) {}
				void computeCompletionProposals(const Caret& caret, bool& incremental,
						Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
					basic_istringstream<Char> s(JSDOC_ATTRIBUTES);
					String p;
					while(s >> p)
						proposals.insert(new CompletionProposal(p));
					IdentifiersProposalProcessor::computeCompletionProposals(caret, incremental = true, replacementRegion, proposals);
				}
				bool isCompletionProposalAutoActivationCharacter(CodePoint c) const BOOST_NOEXCEPT {return c == L'@';}
			};
			class JSProposals : public IdentifiersProposalProcessor {
			public:
				JSProposals(const IdentifierSyntax& ids) : IdentifiersProposalProcessor(DEFAULT_CONTENT_TYPE, ids) {}
				void computeCompletionProposals(const Caret& caret, bool& incremental,
						Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
					basic_istringstream<Char> s(JS_KEYWORDS);
					String p;
					while(s >> p)
						proposals.insert(new CompletionProposal(p));
					IdentifiersProposalProcessor::computeCompletionProposals(caret, incremental = true, replacementRegion, proposals);
				}
				bool isCompletionProposalAutoActivationCharacter(CodePoint c) const BOOST_NOEXCEPT {return c == L'.';}
			};
			unique_ptr<contentassist::ContentAssistant> ca(new contentassist::ContentAssistant());
			ca->setContentAssistProcessor(JS_MULTILINE_DOC_COMMENT, unique_ptr<contentassist::IContentAssistProcessor>(new JSDocProposals(cti->getIdentifierSyntax(JS_MULTILINE_DOC_COMMENT))));
			ca->setContentAssistProcessor(DEFAULT_CONTENT_TYPE, unique_ptr<contentassist::IContentAssistProcessor>(new JSProposals(cti->getIdentifierSyntax(DEFAULT_CONTENT_TYPE))));
			setContentAssistant(unique_ptr<contentassist::IContentAssistant>(ca));
			document().setContentTypeInformation(unique_ptr<IContentTypeInformationProvider>(cti));
#endif // 1

			class ZebraTextRunStyleTest : public TextRunStyleDirector {
			private:
				class Iterator : public StyledTextRunIterator {
				public:
					Iterator(Index lineLength, bool beginningIsBlackBack) : length_(lineLength), beginningIsBlackBack_(beginningIsBlackBack) {
						current_ = StyledTextRun(0, current_.style());
						update();
					}
					StyledTextRun current() const {
						if(!hasNext())
							throw IllegalStateException("");
						return current_;
					}
					bool hasNext() const {
						return current_.position() != length_;
					}
					void next() {
						if(!hasNext())
							throw IllegalStateException("");
						current_ = StyledTextRun(current_.position() + 1, current_.style());
						update();
					}
				private:
					void update() {
						int temp = beginningIsBlackBack_ ? 0 : 1;
						temp += (current_.position() % 2 == 0) ? 0 : 1;
						shared_ptr<TextRunStyle> style(make_shared<TextRunStyle>());
						style->foreground = Paint((temp % 2 == 0) ?
							Color(0xff, 0x00, 0x00) : SystemColors::get(SystemColors::WINDOW_TEXT));
						style->background = Paint((temp % 2 == 0) ?
							Color(0xff, 0xcc, 0xcc) : SystemColors::get(SystemColors::WINDOW));
						current_ = StyledTextRun(current_.position(), style);
					}
				private:
					const Index length_;
					const bool beginningIsBlackBack_;
					StyledTextRun current_;
				};
			public:
				ZebraTextRunStyleTest(const kernel::Document& document) : document_(document) {
				}
				unique_ptr<StyledTextRunIterator> queryTextRunStyle(Index line) const {
					return unique_ptr<StyledTextRunIterator>(new Iterator(document_.lineLength(line), line % 2 == 0));
				}
			private:
				const kernel::Document& document_;
			};
			presentation().setTextRunStyleDirector(
				shared_ptr<TextRunStyleDirector>(new ZebraTextRunStyleTest(document())));
#endif // ASCENSION_TEST_TEXT_STYLES
	
			renderer_->addDefaultFontListener(*this);
			renderer_->layouts().addVisualLinesListener(*this);

			initializeNativeObjects(other);
		}

		/// @internal
		void TextViewer::initializeGraphics() {
			renderer_.reset(/*(other != nullptr) ? new Renderer(*other->renderer_, *this) : */new Renderer(*this));
			textRenderer().addComputedBlockFlowDirectionListener(*this);
//			renderer_->addFontListener(*this);
//			renderer_->addVisualLinesListener(*this);
			rulerPainter_.reset(new detail::RulerPainter(*this));

			caret_.reset(new Caret(*this));
			caretMotionConnection_ = caret().motionSignal().connect(
				std::bind(&TextViewer::caretMoved, this, std::placeholders::_1, std::placeholders::_2));
			matchBracketsChangedConnection_ = caret().matchBracketsChangedSignal().connect(
				std::bind(&TextViewer::matchBracketsChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			selectionShapeChangedConnection_ = caret().selectionShapeChangedSignal().connect(
				std::bind(&TextViewer::selectionShapeChanged, this, std::placeholders::_1));
		}

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
			if((horizontal && rulerPainter_->alignment() == graphics::PhysicalDirection::LEFT)
					|| (!horizontal && rulerPainter_->alignment() == graphics::PhysicalDirection::TOP))
				offset += rulerPainter_->allocationWidth();

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

		namespace {
			void handleDirectionalKey(TextViewer& viewer, graphics::PhysicalDirection direction, widgetapi::UserInput::KeyboardModifier modifiers) {
				using namespace ascension::texteditor::commands;
				using presentation::FlowRelativeDirection;
				using widgetapi::UserInput;
				static kernel::Position(*const nextCharacterLocation)(const kernel::Point&, Direction, kernel::locations::CharacterUnit, Index) = kernel::locations::nextCharacter;

				const presentation::WritingMode writingMode(viewer.presentation().computeWritingMode(&viewer.textRenderer()));
				const FlowRelativeDirection abstractDirection = mapPhysicalToFlowRelative(writingMode, direction);
				const Direction logicalDirection = (abstractDirection == FlowRelativeDirection::AFTER || abstractDirection == FlowRelativeDirection::END) ? Direction::FORWARD : Direction::BACKWARD;
				switch(abstractDirection) {
					case FlowRelativeDirection::BEFORE:
					case FlowRelativeDirection::AFTER:
						if((modifiers & ~(UserInput::SHIFT_DOWN | UserInput::ALT_DOWN)) == 0) {
							if((modifiers & UserInput::ALT_DOWN) == 0)
								makeCaretMovementCommand(viewer, &kernel::locations::nextVisualLine,
									logicalDirection, (modifiers & UserInput::SHIFT_DOWN) != 0)();
							else
								makeRowSelectionExtensionCommand(viewer, &kernel::locations::nextVisualLine, logicalDirection)();
						}
						break;
					case FlowRelativeDirection::START:
					case FlowRelativeDirection::END:
						if((modifiers & ~(UserInput::CONTROL_DOWN | UserInput::SHIFT_DOWN | UserInput::ALT_DOWN)) == 0) {
							if((modifiers & UserInput::ALT_DOWN) == 0) {
								if((modifiers & UserInput::CONTROL_DOWN) != 0)
									makeCaretMovementCommand(viewer, &kernel::locations::nextWord,
										logicalDirection, (modifiers & UserInput::SHIFT_DOWN) != 0)();
								else
									makeCaretMovementCommand(viewer, nextCharacterLocation,
										logicalDirection, (modifiers & UserInput::SHIFT_DOWN) != 0)();
							} else if((modifiers & UserInput::SHIFT_DOWN) != 0) {
								if((modifiers & UserInput::CONTROL_DOWN) != 0)
									makeRowSelectionExtensionCommand(viewer, &kernel::locations::nextWord, logicalDirection)();
								else
									makeRowSelectionExtensionCommand(viewer, nextCharacterLocation, logicalDirection)();
							}
						}
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}
		}

		/// Invoked when a key has been pressed.
		void TextViewer::keyPressed(widgetapi::KeyInput& input) {
			if(mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->interruptMouseReaction(true);

			// TODO: This code is temporary. The following code provides a default implementation of
			// TODO: "key combination to command" map.
			using namespace ascension::viewers::widgetapi;
			using namespace ascension::texteditor::commands;
			static kernel::Position(*const nextCharacterLocation)(const kernel::Point&, Direction, kernel::locations::CharacterUnit, Index) = kernel::locations::nextCharacter;
//			if(hasModifier<UserInput::ALT_DOWN>(input)) {
//				if(!hasModifier<UserInput::SHIFT_DOWN>(input)
//						|| (input.keyboardCode() != VK_LEFT && input.keyboardCode() != VK_UP
//						&& input.keyboardCode() != VK_RIGHT && input.keyboardCode() != VK_DOWN))
//					return false;
//			}
			switch(input.keyboardCode()) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_BackSpace:
				case GDK_KEY_F16:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Backspace:
				case Qt::Key_F16:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_BACK:
				case VK_F16:
#endif
				switch(input.modifiers()) {
					case 0:
					case UserInput::SHIFT_DOWN:
						CharacterDeletionCommand(*this, Direction::BACKWARD)();
						break;
					case UserInput::CONTROL_DOWN:
						WordDeletionCommand(*this, Direction::BACKWARD)();
						break;
					case UserInput::ALT_DOWN:
					case UserInput::SHIFT_DOWN | UserInput::ALT_DOWN:
						UndoCommand(*this, input.hasModifier(UserInput::SHIFT_DOWN))();
						break;
				}
				break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Clear:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Clear:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_CLEAR:
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						EntireDocumentSelectionCreationCommand(*this)();
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Return:
				case GDK_KEY_KP_Enter:
				case GDK_KEY_ISO_Enter:
				case GDK_KEY_3270_Enter:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Enter:
				case Qt::Key_Return:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_RETURN:
#endif
					switch(input.modifiers()) {
						case 0:
						case UserInput::SHIFT_DOWN:
							NewlineCommand(*this)();
							break;
						case UserInput::CONTROL_DOWN:
							NewlineCommand(*this, Direction::BACKWARD)();
							break;
						case UserInput::CONTROL_DOWN | UserInput::SHIFT_DOWN:
							NewlineCommand(*this, Direction::FORWARD)();
							break;
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Escape:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Escape:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_ESCAPE:
#endif
					if(input.modifiers() == 0)
						CancelCommand(*this)();
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Page_Up:	// 'GDK_KEY_Prior' has same value
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_PageUp:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_PRIOR:
#endif
					if(!input.hasModifierOtherThan(UserInput::SHIFT_DOWN))
						makeCaretMovementCommand(*this, &kernel::locations::nextPage, Direction::BACKWARD, input.hasModifier(UserInput::SHIFT_DOWN))();
					else if(input.modifiers() == UserInput::CONTROL_DOWN) {
						if(std::shared_ptr<graphics::font::TextViewport> viewport = textRenderer().viewport())
							viewport->scrollBlockFlowPage(+1);
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Page_Down:	// 'GDK_KEY_Next' has same value
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_PageDown:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_NEXT:
#endif
					if(!input.hasModifierOtherThan(UserInput::SHIFT_DOWN))
						makeCaretMovementCommand(*this, &kernel::locations::nextPage, Direction::FORWARD, input.hasModifier(UserInput::SHIFT_DOWN))();
					else if(input.modifiers() == UserInput::CONTROL_DOWN) {
						if(std::shared_ptr<graphics::font::TextViewport> viewport = textRenderer().viewport())
							viewport->scrollBlockFlowPage(-1);
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Home:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Home:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_HOME:
#endif
					if(!input.hasModifierOtherThan(UserInput::SHIFT_DOWN | UserInput::CONTROL_DOWN)) {
						if(input.hasModifier(UserInput::CONTROL_DOWN))
							makeCaretMovementCommand(*this, &kernel::locations::beginningOfDocument, input.hasModifier(UserInput::SHIFT_DOWN))();
						else
							makeCaretMovementCommand(*this, &kernel::locations::beginningOfVisualLine, input.hasModifier(UserInput::SHIFT_DOWN))();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_End:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_End:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_END:
#endif
					if(!input.hasModifierOtherThan(UserInput::SHIFT_DOWN | UserInput::CONTROL_DOWN)) {
						if(input.hasModifier(UserInput::CONTROL_DOWN))
							makeCaretMovementCommand(*this, &kernel::locations::endOfDocument, input.hasModifier(UserInput::SHIFT_DOWN))();
						else
							makeCaretMovementCommand(*this, &kernel::locations::endOfVisualLine, input.hasModifier(UserInput::SHIFT_DOWN))();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Left:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Left:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_LEFT:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::LEFT, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Up:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Up:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_UP:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::TOP, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Right:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Right:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_RIGHT:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::RIGHT, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Down:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Down:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_DOWN:
#endif
					handleDirectionalKey(*this, graphics::PhysicalDirection::BOTTOM, input.modifiers());
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Insert:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Insert:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_INSERT:
#endif
					if(!input.hasModifierOtherThan(UserInput::SHIFT_DOWN | UserInput::CONTROL_DOWN)) {
						if(input.hasModifier(UserInput::SHIFT_DOWN))
							PasteCommand(*this, input.hasModifier(UserInput::CONTROL_DOWN))();
						else if(input.hasModifier(UserInput::CONTROL_DOWN))
							copySelection(caret(), true);
						else
							OvertypeModeToggleCommand(*this)();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Delete:
				case GDK_KEY_KP_Delete:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Delete:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_DELETE:
#endif
					switch(input.modifiers()) {
						case 0:
							CharacterDeletionCommand(*this, Direction::FORWARD)();
							break;
						case UserInput::SHIFT_DOWN:
							cutSelection(caret(), true);
							break;
						case UserInput::CONTROL_DOWN:
							WordDeletionCommand(*this, Direction::FORWARD)();
							break;
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_A:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_A:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'A':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						EntireDocumentSelectionCreationCommand(*this)();	// ^A -> Select All
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_C:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_C:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'C':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						copySelection(caret(), true);	// ^C -> Copy
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_H:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_H:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'H':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						CharacterDeletionCommand(*this, Direction::BACKWARD)(), true;	// ^H -> Backspace
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_I:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_I:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'I':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						CharacterInputCommand(*this, 0x0009u)();	// ^I -> Tab
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_J:
				case GDK_KEY_M:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_J:
				case Qt::Key_M:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'J':
				case 'M':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						NewlineCommand(*this, false)();	// ^J or ^M -> New Line
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_V:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_V:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'V':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						PasteCommand(*this, false)();	// ^V -> Paste
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_X:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_X:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'X':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						cutSelection(caret(), true);	// ^X -> Cut
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Y:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Y:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'Y':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						UndoCommand(*this, true)();	// ^Y -> Redo
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Z:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Z:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case 'Z':
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN)
						UndoCommand(*this, false)();	// ^Z -> Undo
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_KP_5:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_5:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_NUMPAD5:
#endif
					if(input.modifiers() == UserInput::CONTROL_DOWN) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
						if(hasModifier<Qt::KeypadModifier>(input))
#endif
						EntireDocumentSelectionCreationCommand(*this)();
					}
					break;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_F12:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_F12:
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_F12:
#endif
					if(input.modifiers() == (UserInput::CONTROL_DOWN | UserInput::SHIFT_DOWN))
						CodePointToCharacterConversionCommand(*this)();
					break;

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				case GDK_KEY_Undo:
					UndoCommand(*this, false)();
					break;
				case GDK_KEY_Redo:
					UndoCommand(*this, true)();
					break;
				case GDK_KEY_Shift_L:
					if(input.hasModifier(UserInput::CONTROL_DOWN) && configuration_.readingDirection == presentation::RIGHT_TO_LEFT)
						textRenderer().setDirection(presentation::LEFT_TO_RIGHT);
					break;
				case GDK_KEY_Shift_R:
					if(input.hasModifier(UserInput::CONTROL_DOWN) && configuration_.readingDirection == presentation::LEFT_TO_RIGHT)
						textRenderer().setDirection(presentation::RIGHT_TO_LEFT);
					break;
				case GDK_KEY_Copy:
					copySelection(caret(), true);
					break;
				case GDK_KEY_Cut:
					cutSelection(caret(), true);
					break;
				case GDK_KEY_Paste:
					PasteCommand(*this, false)();
					break;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				case Qt::Key_Copy:
					copySelection(caret(), true);
					break;
				case Qt::Key_Cut:
					cutSelection(caret(), true);
					break;
				case Qt::Key_Paste:
					PasteCommand(*this, false)();
					break;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				case VK_SHIFT:
					if(input.hasModifier(UserInput::CONTROL_DOWN)) {
						if(::GetKeyState(VK_LSHIFT) < 0 && configuration_.readingDirection == RIGHT_TO_LEFT)
							textRenderer().setDirection(LEFT_TO_RIGHT);
						else if(::GetKeyState(VK_RSHIFT) < 0 && configuration_.readingDirection == LEFT_TO_RIGHT)
							textRenderer().setDirection(RIGHT_TO_LEFT);
						}
					break;
#endif
			}
			return input.ignore();
		}

		/// Invoked when a key has been released.
		void TextViewer::keyReleased(widgetapi::KeyInput& input) {
			if(input.hasModifier(widgetapi::UserInput::ALT_DOWN)) {
				cursorVanisher_.restore();
				if(mouseInputStrategy_.get() != nullptr)
					mouseInputStrategy_->interruptMouseReaction(true);
			}
			return input.ignore();
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

		/// @see Caret#MatchBracketsChangedSignal
		void TextViewer::matchBracketsChanged(const Caret& caret, const boost::optional<std::pair<kernel::Position, kernel::Position>>& previouslyMatchedBrackets, bool outsideOfView) {
			const boost::optional<std::pair<kernel::Position, kernel::Position>>& newPair = caret.matchBrackets();
			if(newPair) {
				redrawLine(newPair->first.line);
				if(!isFrozen())
					widgetapi::redrawScheduledRegion(*this);
				if(newPair->second.line != newPair->first.line) {
					redrawLine(newPair->second.line);
					if(!isFrozen())
						widgetapi::redrawScheduledRegion(*this);
				}
				if(previouslyMatchedBrackets	// clear the previous highlight
						&& previouslyMatchedBrackets->first.line != newPair->first.line && previouslyMatchedBrackets->first.line != newPair->second.line) {
					redrawLine(previouslyMatchedBrackets->first.line);
					if(!isFrozen())
						widgetapi::redrawScheduledRegion(*this);
				}
				if(previouslyMatchedBrackets && previouslyMatchedBrackets->second.line != newPair->first.line
						&& previouslyMatchedBrackets->second.line != newPair->second.line && previouslyMatchedBrackets->second.line != previouslyMatchedBrackets->first.line)
					redrawLine(previouslyMatchedBrackets->second.line);
			} else {
				if(previouslyMatchedBrackets) {	// clear the previous highlight
					redrawLine(previouslyMatchedBrackets->first.line);
					if(!isFrozen())
						widgetapi::redrawScheduledRegion(*this);
					if(previouslyMatchedBrackets->second.line != previouslyMatchedBrackets->first.line)
						redrawLine(previouslyMatchedBrackets->second.line);
				}
			}
		}

		/// Invoked when the mouse button has been double-clicked.
		void TextViewer::mouseDoubleClicked(widgetapi::MouseButtonInput& input) {
			if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::DOUBLE_CLICKED, input);
		}

		/// Invoked when the mouse cursor has been moved onto a widget.
		void TextViewer::mouseMoved(widgetapi::LocatedUserInput& input) {
			cursorVanisher_.restore();
			if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->mouseMoved(input);
		}

		/// Invoked when a mouse button has been pressed on a widget.
		void TextViewer::mousePressed(widgetapi::MouseButtonInput& input) {
			cursorVanisher_.restore();
			if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::PRESSED, input);
		}

		/// Invoked when a mouse button has been released on a widget.
		void TextViewer::mouseReleased(widgetapi::MouseButtonInput& input) {
			if(allowsMouseInput() || input.button() == widgetapi::LocatedUserInput::BUTTON3_DOWN)
				cursorVanisher_.restore();
			if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::RELEASED, input);
		}

		/// Invoked when the mouse button has been triple-clicked. 
		void TextViewer::mouseTripleClicked(widgetapi::MouseButtonInput& input) {
			if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::DOUBLE_CLICKED, input);
		}

		/// Invoked when the mouse wheel is rotated.
		void TextViewer::mouseWheelChanged(widgetapi::MouseWheelInput& input) {
			cursorVanisher_.restore();
			if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
				mouseInputStrategy_->mouseWheelRotated(input);
		}

		/// @see Widget#paint
		void TextViewer::paint(graphics::PaintContext& context) {
			if(isFrozen())	// skip if frozen
				return;
			graphics::Rectangle scheduledBounds(context.boundsToPaint());
			if(graphics::geometry::isEmpty(graphics::geometry::normalize(scheduledBounds)))	// skip if the region to paint is empty
				return;

//			Timer tm(L"TextViewer.paint");

			// paint the ruler
			rulerPainter_->paint(context);

			// paint the text area
			textRenderer().paint(context);

			// paint the caret(s)
			paintCaret(context);
		}

		/**
		 * @internal Paints the caret(s).
		 * @param context The graphic context
		 */
		void TextViewer::paintCaret(graphics::PaintContext& context) {
		}

		/**
		 * Redraws the specified line on the view.
		 * If the viewer is frozen, redraws after unfrozen.
		 * @param line The line to be redrawn
		 * @param following Set @c true to redraw also the all lines follow to @a line
		 */
		void TextViewer::redrawLine(Index line, bool following) {
			redrawLines(boost::irange(line, following ? std::numeric_limits<Index>::max() : line + 1));
		}

		/**
		 * Redraws the specified lines on the view. If the viewer is frozen, redraws after unfrozen.
		 * @param lines The lines to be redrawn. The last line (@a lines.end()) is exclusive and this line will not be
		 *              redrawn. If this value is @c std#numeric_limits<Index>#max(), this method redraws the first
		 *              line (@a lines.beginning()) and the following all lines
		 * @throw IndexOutOfBoundsException @a lines intersects outside of the document
		 */
		void TextViewer::redrawLines(const boost::integer_range<Index>& lines) {
		//	checkInitialization();

			if(lines.empty())
				return;

			const boost::integer_range<Index> orderedLines(ordered(lines));
			if(*orderedLines.end() != std::numeric_limits<Index>::max() && *orderedLines.end() >= document().numberOfLines())
				throw IndexOutOfBoundsException("lines");

			if(isFrozen()) {
				freezeRegister_.addLinesToRedraw(orderedLines);
				return;
			}

			if(orderedLines.back() < textRenderer().viewport()->firstVisibleLine().line)
				return;

#if defined(_DEBUG) && defined(ASCENSION_DIAGNOSE_INHERENT_DRAWING)
			BOOST_LOG_TRIVIAL(debug)
				<< L"@TextViewer.redrawLines invalidates lines ["
				<< static_cast<unsigned long>(*lines.begin())
				<< L".." << static_cast<unsigned long>(*lines.end()) << L"]\n";
#endif

			using graphics::Scalar;
			const presentation::WritingMode writingMode(presentation().computeWritingMode(&textRenderer()));
			std::array<Scalar, 2> beforeAndAfter;	// in viewport (distances from before-edge of the viewport)
			graphics::font::BaselineIterator baseline(*textRenderer().viewport(), graphics::font::VisualLine(*lines.begin(), 0));
			std::get<0>(beforeAndAfter) = *baseline;
			if(std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::min() && std::get<0>(beforeAndAfter) != std::numeric_limits<Scalar>::max())
				std::get<0>(beforeAndAfter) -= *textRenderer().layouts().at(baseline.line()->line)->lineMetrics(0).extent().begin();
			baseline = graphics::font::BaselineIterator(*textRenderer().viewport(), graphics::font::VisualLine(*lines.end(), 0));
			std::get<1>(beforeAndAfter) = *baseline;
			if(std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::min() && std::get<1>(beforeAndAfter) != std::numeric_limits<Scalar>::max())
				std::get<1>(beforeAndAfter) += *textRenderer().layouts().at(baseline.line()->line)->lineMetrics(0).extent().end();
			assert(std::get<0>(beforeAndAfter) <= std::get<1>(beforeAndAfter));

			namespace geometry = graphics::geometry;
			const graphics::Rectangle viewerBounds(widgetapi::bounds(*this, false));
			graphics::Rectangle boundsToRedraw(textRenderer().viewport()->boundsInView());
			geometry::translate(boundsToRedraw, graphics::Dimension(geometry::_dx = geometry::left(viewerBounds), geometry::_dy = geometry::top(viewerBounds)));
			assert(boost::geometry::equals(boundsToRedraw, textAreaAllocationRectangle()));

			BOOST_FOREACH(Scalar& edge, beforeAndAfter) {
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

			widgetapi::scheduleRedraw(*this, boundsToRedraw, false);
		}

		/// Redraws the ruler.
		void TextViewer::repaintRuler() {
			widgetapi::scheduleRedraw(*this, graphics::geometry::joined(
				rulerPainter_->indicatorMarginAllocationRectangle(), rulerPainter_->lineNumbersAllocationRectangle()), false);
		}

		/// @see Widget#resized
		void TextViewer::resized(const graphics::Dimension&) {
			utils::closeCompletionProposalsPopup(*this);
			if(widgetapi::Proxy<widgetapi::Window> window = widgetapi::window(*this)) {
				if(widgetapi::isMinimized(window))
					return;
			}
			if(renderer_.get() == nullptr)
				return;
			textRenderer().viewport()->setBoundsInView(textAreaContentRectangle());
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// notify the tooltip
			win32::AutoZeroSize<TOOLINFOW> ti;
			const graphics::Rectangle viewerBounds(widgetapi::bounds(*this, false));
			ti.hwnd = handle().get();
			ti.uId = 1;
			ti.rect = viewerBounds;
			::SendMessageW(toolTip_.get(), TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			rulerPainter_->update();
			if(rulerPainter_->alignment() != graphics::PhysicalDirection::LEFT && rulerPainter_->alignment() != graphics::PhysicalDirection::TOP) {
//				recreateCaret();
//				redrawVerticalRuler();
				widgetapi::scheduleRedraw(*this, false);	// hmm...
			}
			if(contentAssistant() != 0)
				contentAssistant()->viewerBoundsChanged();
		}

		/// @see Caret#SelectionShapeChangedSignal
		void TextViewer::selectionShapeChanged(const Caret& caret) {
			if(!isFrozen() && !isSelectionEmpty(caret))
				redrawLines(boost::irange(kernel::line(caret.beginning()), kernel::line(caret.end()) + 1));
		}

		/**
		 * Sets the caret shaper.
		 * @param shaper The new caret shaper to set
		 */
		void TextViewer::setCaretShaper(std::shared_ptr<CaretShaper> shaper) {
			if(shaper == caretShaper_)
				return;
			if(caretShaper_.get() != nullptr)
				caretShaper_->uninstall(caret());	// TODO: Support multiple carets.
			if(shaper.get() == nullptr)
				shaper = std::make_shared<DefaultCaretShaper>();
			(caretShaper_ = shaper)->install(caret());	// TODO: Support multiple carets.
#ifdef ASCENSION_USE_SYSTEM_CARET
			caretStaticShapeChanged(caret());	// update caret shapes immediately
#endif
		}

		/**
		 * Updates the configurations.
		 * @param general The general configurations. @c null to unchange
		 * @param ruler The configurations about the ruler. @c null to unchange
		 * @param synchronizeUI Set @c true to change the window style according to the new style. This sets
		 *                      @c WS_EX_LEFTSCROLLBAR, @c WS_EX_RIGHTSCROLLBAR, @c WS_EX_LTRREADING and
		 *                      @c WS_EX_RTLREADING styles
		 * @throw UnknownValueException The content of @a verticalRuler is invalid
		 */
		void TextViewer::setConfiguration(const Configuration* general, std::shared_ptr<const RulerStyles> ruler, bool synchronizeUI) {
			if(ruler != nullptr)
				rulerPainter_->setStyles(ruler);
			if(general != nullptr) {
//				const Inheritable<ReadingDirection> oldReadingDirection(configuration_.readingDirection);
//				assert(!oldReadingDirection.inherits());
				configuration_ = *general;
				textRenderer().viewport()->setBoundsInView(textAreaContentRectangle());	// TODO: Should we call resized() instead?
				renderer_->layouts().invalidate();

//				if((oldReadingDirection == LEFT_TO_RIGHT && configuration_.readingDirection == RIGHT_TO_LEFT)
//						|| (oldReadingDirection == RIGHT_TO_LEFT && configuration_.readingDirection == LEFT_TO_RIGHT))
//					scrolls_.horizontal.position = scrolls_.horizontal.maximum
//						- scrolls_.horizontal.pageSize - scrolls_.horizontal.position + 1;
//				scrolls_.resetBars(*this, 'a', false);
//				updateScrollBars(AbstractTwoAxes<bool>(true, true), AbstractTwoAxes<bool>(true, true));

#ifdef ASCENSION_USE_SYSTEM_CARET
				if(!isFrozen() && (widgetapi::hasFocus(*this) /*|| handle() == Viewer::completionWindow_->getSafeHwnd()*/)) {
					caret().resetVisualization();
					caret().updateLocation();
				}
#endif
				if(synchronizeUI) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					if(get_direction() != Gtk::TEXT_DIR_NONE)
						set_direction((configuration_.readingDirection == presentation::LEFT_TO_RIGHT) ? Gtk::TEXT_DIR_LTR : Gtk::TEXT_DIR_RTL);
//					set_placement();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					LONG style = ::GetWindowLongW(handle().get(), GWL_EXSTYLE);
					if(configuration_.readingDirection == LEFT_TO_RIGHT) {
						style &= ~(WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
						style |= WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
					} else {
						style &= ~(WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
						style |= WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR;
					}
					::SetWindowLongW(handle().get(), GWL_EXSTYLE, style);
#endif
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
		 * Sets the mouse input strategy. An instance of @c TextViewer has the default strategy implemented
		 * by @c DefaultMouseInputStrategy class as the construction.
		 * @param newStrategy The new strategy or @c null
		 * @param delegateOwnership Set @c true to transfer the ownership into the callee
		 * @throw IllegalStateException The window is not created yet
		 */
		void TextViewer::setMouseInputStrategy(std::shared_ptr<MouseInputStrategy> newStrategy) {
//			checkInitialization();
			if(mouseInputStrategy_.get() != nullptr) {
				mouseInputStrategy_->interruptMouseReaction(false);
				mouseInputStrategy_->uninstall();
				dropTargetHandler_.reset();
			}
			if(newStrategy.get() != nullptr)
				mouseInputStrategy_ = newStrategy;
			else
				mouseInputStrategy_.reset(new DefaultMouseInputStrategy());	// TODO: the two parameters don't have rationales.
			mouseInputStrategy_->install(*this);
			dropTargetHandler_ = mouseInputStrategy_->handleDropTarget();
		}

		/**
		 * Shows the hidden caret.
		 * @see #hideCaret, #hidesCaret
		 */
		void TextViewer::showCaret() BOOST_NOEXCEPT {
			if(hidesCaret())
				caretBlinker_.reset(new CaretBlinker(*this));
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
			tipText_.assign(text);
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
		 * Returns the 'allocation-rectangle' of the text area, in local-coordinates.
		 * @see #bounds, #textAreaContentRectangle
		 */
		graphics::Rectangle TextViewer::textAreaAllocationRectangle() const BOOST_NOEXCEPT {
			using graphics::PhysicalDirection;
			const graphics::Rectangle window(widgetapi::bounds(*this, false));
			graphics::PhysicalFourSides<graphics::Scalar> result(window);
			switch(rulerPainter_->alignment()) {
				case PhysicalDirection::LEFT:
					result.left() += rulerPainter_->allocationWidth();
					break;
				case PhysicalDirection::TOP:
					result.top() += rulerPainter_->allocationWidth();
					break;
				case PhysicalDirection::RIGHT:
					result.right() -= rulerPainter_->allocationWidth();
					break;
				case PhysicalDirection::BOTTOM:
					result.bottom() -= rulerPainter_->allocationWidth();
					break;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}
			return graphics::geometry::make<graphics::Rectangle>(result);
		}

		/**
		 * Returns the 'content-rectangle' of the text area, in local-coordinates.
		 * @see #bounds, #textAreaAllocationRectangle
		 */
		graphics::Rectangle TextViewer::textAreaContentRectangle() const BOOST_NOEXCEPT {
			// TODO: Consider 'padding-start' setting.
			return textAreaAllocationRectangle();
		}

		/**
		 * Revokes the frozen state of the viewer.
		 * @throw WindowNotInitialized The window is not initialized
		 * @note This method thaws also viewer's @c TextViewport.
		 * @see #freeze, #isFrozen
		 */
		void TextViewer::unfreeze() {
//			checkInitialization();
			const std::shared_ptr<graphics::font::TextViewport> viewport(textRenderer().viewport());
			if(viewport.get() == nullptr)
				throw IllegalStateException("There is no viewport in this text viewer.");
			if(freezeRegister_.isFrozen()) {
				const boost::integer_range<Index> linesToRedraw(freezeRegister_.thaw());
				try {
					viewport->thawNotification();
				} catch(const std::underflow_error&) {
					// ignore
				}
				if(!freezeRegister_.isFrozen()) {
/*					if(scrolls_.changed) {
						updateScrollBars();
						widgetapi::scheduleRedraw(*this, false);
					} else*/ if(!linesToRedraw.empty())
						redrawLines(linesToRedraw);

					rulerPainter_->update();

					caretMoved(caret(), caret().selectedRegion());
					widgetapi::redrawScheduledRegion(*this);
				}
			}
		}

		namespace {
			template<typename T>
			inline T calculateMaximumScrollPosition(T last, T pageSize) {
//				return last /* * rate */ - pageSize + 1;
				return last;
			}
		}

		/**
		 * @internal Updates the scroll information.
		 * @param positions Describes which position(s) to update
		 * @param properties Describes which property(ies) to update
		 */
		void TextViewer::updateScrollBars(const presentation::AbstractTwoAxes<bool>& positions, const presentation::AbstractTwoAxes<bool>& properties) {
//			checkInitialization();
			assert(!isFrozen());
			const auto needsUpdate = [](bool v) {return v;};
			if(renderer_.get() == nullptr
					|| (std::none_of(std::begin(positions), std::end(positions), needsUpdate) && std::none_of(std::begin(properties), std::end(properties), needsUpdate)))
				return;
			const std::shared_ptr<graphics::font::TextViewport> viewport(textRenderer().viewport());
			if(viewport.get() == nullptr)
				return;

#if 1
			const presentation::WritingMode writingMode(presentation().computeWritingMode(&textRenderer()));
			assert(!isFrozen());

			// update the scroll bar in inline-progression-dimension
			if(positions.ipd() || properties.ipd()) {
				const auto viewportRange(graphics::font::scrollableRange<presentation::ReadingDirection>(*viewport));
				boost::optional<widgetapi::NativeScrollPosition> position, size;
				boost::optional<boost::integer_range<widgetapi::NativeScrollPosition>> range;
				if(positions.ipd())
					// TODO: Use reverseScrollPosition().
					position = (writingMode.inlineFlowDirection == presentation::LEFT_TO_RIGHT) ?
						viewport->scrollPositions().ipd() : (*viewportRange.end() - viewport->scrollPositions().ipd() - 1);
				if(properties.ipd()) {
					range = boost::irange<widgetapi::NativeScrollPosition>(*viewportRange.begin(), *viewportRange.end());
					size = graphics::font::pageSize<presentation::ReadingDirection>(*viewport);
				}
				configureScrollBar(*this, presentation::isHorizontal(writingMode.blockFlowDirection) ? 0 : 1, position, range, size);
			}

			// update the scroll bar in block-progression-dimension
			if(positions.bpd() || properties.bpd()) {
				const auto viewportRange(graphics::font::scrollableRange<presentation::BlockFlowDirection>(*viewport));
				boost::optional<widgetapi::NativeScrollPosition> position, size;
				boost::optional<boost::integer_range<widgetapi::NativeScrollPosition>> range;
				if(positions.bpd())
					// TODO: Use reverseScrollPosition().
					position = (writingMode.blockFlowDirection != presentation::VERTICAL_RL) ?
						viewport->scrollPositions().bpd() : (*viewportRange.end() - viewport->scrollPositions().bpd() - 1);
				if(properties.bpd()) {
					range = boost::irange<widgetapi::NativeScrollPosition>(*viewportRange.begin(), *viewportRange.end());
					size = graphics::font::pageSize<presentation::BlockFlowDirection>(*viewport);
				}
				configureScrollBar(*this, presentation::isHorizontal(writingMode.blockFlowDirection) ? 1 : 0, position, range, size);
			}

#else
			const LineLayoutVector& layouts = textRenderer().layouts();
			typedef PhysicalTwoAxes<widgetapi::NativeScrollPosition> ScrollPositions;
			const ScrollPositions positions(physicalScrollPosition(*this));
			ScrollPositions endPositions(
#if 0
				_x = static_cast<widgetapi::NativeScrollPosition>(layouts.maximumMeasure()),
				_y = static_cast<widgetapi::NativeScrollPosition>(layouts.numberOfVisualLines()));
#else
				_x = static_cast<widgetapi::NativeScrollPosition>(*scrollableRange<0>(*viewport).end()),
				_y = static_cast<widgetapi::NativeScrollPosition>(*scrollableRange<1>(*viewport).end()));
#endif
			ScrollPositions pageSizes(
				_x = static_cast<widgetapi::NativeScrollPosition>(pageSize<0>(*viewport)),
				_y = static_cast<widgetapi::NativeScrollPosition>(pageSize<1>(*viewport)));
			if(isVertical(writingMode.blockFlowDirection)) {
#if 0
				geometry::transpose(endPositions);
#endif
				geometry::transpose(pageSizes);
			}

			// about horizontal scroll bar
			bool wasNeededScrollbar = calculateMaximumScrollPosition(geometry::x(endPositions), geometry::x(pageSizes)) > 0;
			// scroll to leftmost/rightmost before the scroll bar vanishes
			widgetapi::NativeScrollPosition minimum = calculateMaximumScrollPosition(geometry::x(endPositions), geometry::x(pageSizes));
			if(wasNeededScrollbar && minimum <= 0) {
//				scrolls_.horizontal.position = 0;
				if(!isFrozen()) {
					widgetapi::scheduleRedraw(*this, false);
					caret().updateLocation();
				}
			} else if(geometry::x(positions) > minimum)
				viewport->scrollTo(PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>(minimum, boost::none));
			assert(calculateMaximumScrollPosition(geometry::x(endPositions), geometry::x(pageSizes)) > 0 || geometry::x(positions) == 0);
			if(!isFrozen())
				configureScrollBar(*this, 0, geometry::x(positions),
					boost::irange<widgetapi::NativeScrollPosition>(0, geometry::x(endPositions)),
					boost::make_optional<widgetapi::NativeScrollPosition>(geometry::x(pageSizes)));

			// about vertical scroll bar
			wasNeededScrollbar = calculateMaximumScrollPosition(geometry::y(endPositions), geometry::y(pageSizes)) > 0;
			minimum = calculateMaximumScrollPosition(geometry::y(endPositions), geometry::y(pageSizes));
			// validate scroll position
			if(minimum <= 0) {
//				scrolls_.vertical.position = 0;
//				scrolls_.firstVisibleLine = VisualLine(0, 0);
				if(!isFrozen()) {
					widgetapi::scheduleRedraw(*this, false);
					caret().updateLocation();
				}
			} else if(static_cast<widgetapi::NativeScrollPosition>(geometry::y(positions)) > minimum)
				viewport->scrollTo(PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>(boost::none, minimum));
			assert(calculateMaximumScrollPosition(geometry::y(endPositions), geometry::y(pageSteps)) > 0 || geometry::y(positions) == 0);
			if(!isFrozen())
				configureScrollBar(*this, 1, geometry::y(positions),
					boost::irange<widgetapi::NativeScrollPosition>(0, geometry::y(endPositions)),
					static_cast<widgetapi::NativeScrollPosition>(geometry::y(pageSizes)));

			scrolls_.changed = isFrozen();
#endif
		}

		/// @see TextViewport#viewportBoundsInViewChanged
		void TextViewer::viewportBoundsInViewChanged(const graphics::Rectangle& oldBounds) BOOST_NOEXCEPT {
//			textRenderer().setTextWrapping(textRenderer().textWrapping(), widgetapi::createRenderingContext(*this).get());
			updateScrollBars(presentation::AbstractTwoAxes<bool>(true, true), presentation::AbstractTwoAxes<bool>(true, true));
		}

		namespace {
			void scrollBarParameters(const TextViewer& viewer,
					std::size_t coordinate, widgetapi::NativeScrollPosition* position,
					boost::integer_range<widgetapi::NativeScrollPosition>* range, widgetapi::NativeScrollPosition* pageSize) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				const Glib::RefPtr<const Gtk::Adjustment> adjustment((coordinate == 0) ? viewer.get_hadjustment() : viewer.get_vadjustment());
				if(range != nullptr)
					*range = boost::irange(adjustment->get_lower(), adjustment->get_upper());
				if(pageSize != nullptr)
					*pageSize = adjustment->get_page_increment();
//					*pageSize = adjustment->get_page_size();
				if(position != nullptr)
					*position = adjustment->get_value();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				const QScrollBar* const scrollBar = (coordinate == geometry::X_COORDINATE) ? viewer.horizontalScrollBar() : viewer.verticalScrollBar();
				if(range != nullptr)
					*range = boost::irange(scrollBar->minimum(), scrollBar->maximum());
				if(pageSize != nullptr)
					*pageSize = scrollBar->pageStep();
				if(position != nullptr)
					*position = scrollBar->sliderPosition();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::AutoZeroSize<SCROLLINFO> si;
				si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
				if(win32::boole(::GetScrollInfo(viewer.handle().get(), (coordinate == 0) ? SB_HORZ : SB_VERT, &si)))
					throw makePlatformError();
				if(range != nullptr)
					*range = boost::irange<widgetapi::NativeScrollPosition>(si.nMin, si.nMax);
				if(pageSize != nullptr)
					*pageSize = si.nPage;
				if(position != nullptr)
					*position = si.nPos;
#endif
			}
		}

		/// @see TextViewportListener#viewportScrollPositionChanged
		void TextViewer::viewportScrollPositionChanged(
				const presentation::AbstractTwoAxes<graphics::font::TextViewportScrollOffset>& positionsBeforeScroll,
				const graphics::font::VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT {
			assert(!isFrozen());

			// 1. update the scroll positions
			updateScrollBars(presentation::AbstractTwoAxes<bool>(true, true), presentation::AbstractTwoAxes<bool>(false, false));
//			closeCompletionProposalsPopup(*this);
			hideToolTip();

			// 2. calculate pixels to scroll
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			presentation::AbstractTwoAxes<std::int32_t> abstractScrollOffsetInPixels;
			// 2-1. block dimension
			{
				graphics::font::VisualLine p(viewport->firstVisibleLine());
				const graphics::font::TextLayout* layout = textRenderer().layouts().at(p.line);
				abstractScrollOffsetInPixels.bpd() = 0;
				while(layout != nullptr && p < firstVisibleLineBeforeScroll) {
					abstractScrollOffsetInPixels.bpd() -= static_cast<std::uint32_t>(layout->lineMetrics(p.subline).height());
					if(p.subline < layout->numberOfLines() - 1)
						++p.subline;
					else if(p.line < document().numberOfLines() - 1) {
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
					abstractScrollOffsetInPixels.bpd() = std::numeric_limits<graphics::font::TextViewportSignedScrollOffset>::max();
			}
			// 2-2. inline dimension
			abstractScrollOffsetInPixels.ipd() = (abstractScrollOffsetInPixels.bpd() != std::numeric_limits<std::int32_t>::max()) ?
				static_cast<graphics::font::TextViewportSignedScrollOffset>(
					inlineProgressionOffsetInViewerGeometry(*viewport, positionsBeforeScroll.ipd())
						- inlineProgressionOffsetInViewerGeometry(*viewport, viewport->scrollPositions().ipd()))
				: std::numeric_limits<std::int32_t>::max();
			// 2-3. calculate physical offsets
			const auto scrollOffsetsInPixels(presentation::mapAbstractToPhysical(
				presentation().computeWritingMode(&textRenderer()), abstractScrollOffsetInPixels));

			// 3. scroll the graphics device
			namespace geometry = graphics::geometry;
			const graphics::Rectangle& boundsToScroll = viewport->boundsInView();
			if(std::abs(geometry::x(scrollOffsetsInPixels)) >= geometry::dx(boundsToScroll)
					|| std::abs(geometry::y(scrollOffsetsInPixels)) >= geometry::dy(boundsToScroll))
				widgetapi::scheduleRedraw(*this, boundsToScroll, false);	// repaint all if the amount of the scroll is over a page
			else {
				// scroll image by BLIT
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				textAreaWindow_->scroll(geometry::x(scrollOffsetsInPixels), geometry::y(scrollOffsetsInPixels));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				::ScrollWindowEx(handle().get(),
					geometry::x(scrollOffsetsInPixels), geometry::y(scrollOffsetsInPixels), nullptr, &static_cast<RECT>(boundsToScroll), nullptr, nullptr, SW_INVALIDATE);
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				// invalidate bounds newly entered into the viewport
				using graphics::Dimension;
				using graphics::Scalar;
				if(graphics::geometry::x(scrollOffsetsInPixels) > 0)
					widgetapi::scheduleRedraw(*this, graphics::Rectangle(
						geometry::topLeft(boundsToScroll),
						Dimension(geometry::_dx = static_cast<Scalar>(geometry::x(scrollOffsetsInPixels)), geometry::_dy = geometry::dy(boundsToScroll))), false);
				else if(geometry::x(scrollOffsetsInPixels) < 0)
					widgetapi::scheduleRedraw(*this, graphics::Rectangle(
						geometry::topRight(boundsToScroll),
						Dimension(geometry::_dx = static_cast<Scalar>(geometry::x(scrollOffsetsInPixels)), geometry::_dy = geometry::dy(boundsToScroll))), false);
				if(geometry::y(scrollOffsetsInPixels) > 0)
					widgetapi::scheduleRedraw(*this, graphics::Rectangle(
						geometry::topLeft(boundsToScroll),
						Dimension(geometry::_dx = geometry::dx(boundsToScroll), geometry::_dy = static_cast<Scalar>(geometry::y(scrollOffsetsInPixels)))), false);
				else if(geometry::y(scrollOffsetsInPixels) < 0)
					widgetapi::scheduleRedraw(*this, graphics::Rectangle(
						geometry::bottomLeft(boundsToScroll),
						Dimension(geometry::_dx = geometry::dx(boundsToScroll), geometry::_dy = static_cast<Scalar>(geometry::y(scrollOffsetsInPixels)))), false);
			}

			// 4. scroll the ruler
			rulerPainter_->scroll(firstVisibleLineBeforeScroll);

			// TODO: Step 3 and step 4 should reverse?

			// 5. repaint
			widgetapi::redrawScheduledRegion(*this);
		}

		/// @see TextViewportListener#viewportScrollPropertiesChanged
		void TextViewer::viewportScrollPropertiesChanged(const presentation::AbstractTwoAxes<bool>& changedDimensions) BOOST_NOEXCEPT {
			updateScrollBars(presentation::AbstractTwoAxes<bool>(true, true), presentation::AbstractTwoAxes<bool>(true, true));
		}

		/// @see VisualLinesListener#visualLinesDeleted
		void TextViewer::visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) BOOST_NOEXCEPT {
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			if(*lines.end() < viewport->firstVisibleLine().line) {	// deleted before visible area
//				scrolls_.firstVisibleLine.line -= length(lines);
//				scrolls_.vertical.position -= static_cast<int>(sublines);
//				scrolls_.vertical.maximum -= static_cast<int>(sublines);
				repaintRuler();
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
		void TextViewer::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
			const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
			if(*lines.end() < viewport->firstVisibleLine().line) {	// inserted before visible area
//				scrolls_.firstVisibleLine.line += length(lines);
//				scrolls_.vertical.position += static_cast<int>(length(lines));
//				scrolls_.vertical.maximum += static_cast<int>(length(lines));
				repaintRuler();
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
		void TextViewer::visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT {
			if(sublinesDifference == 0)	// number of visual lines was not changed
				redrawLines(lines);
			else {
				const std::shared_ptr<const graphics::font::TextViewport> viewport(textRenderer().viewport());
				if(*lines.end() < viewport->firstVisibleLine().line) {	// changed before visible area
//					scrolls_.vertical.position += sublinesDifference;
//					scrolls_.vertical.maximum += sublinesDifference;
					repaintRuler();
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


		// AutoFreeze /////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::viewers::AutoFreeze
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


		// TextViewer.CursorVanisher //////////////////////////////////////////////////////////////////////////////////

		TextViewer::CursorVanisher::CursorVanisher() BOOST_NOEXCEPT : viewer_(nullptr) {
		}

		TextViewer::CursorVanisher::~CursorVanisher() BOOST_NOEXCEPT {
			restore();
		}

		void TextViewer::CursorVanisher::install(TextViewer& viewer) {
			assert(viewer_ == nullptr);
			viewer_ = &viewer;
		}

		void TextViewer::CursorVanisher::restore() {
			if(vanished_) {
				widgetapi::Cursor::show();
				widgetapi::releaseInput(*viewer_);
				vanished_ = false;
			}
		}

		void TextViewer::CursorVanisher::vanish() {
			if(!vanished_ && viewer_->configuration().vanishesCursor && widgetapi::hasFocus(*viewer_)) {
				vanished_ = true;
				widgetapi::Cursor::hide();
				widgetapi::grabInput(*viewer_);
			}
		}

		bool TextViewer::CursorVanisher::vanished() const {
			return vanished_;
		}


		// TextViewer.FreezeRegister //////////////////////////////////////////////////////////////////////////////////

		TextViewer::FreezeRegister::FreezeRegister() BOOST_NOEXCEPT : linesToRedraw_(boost::irange<Index>(0, 0)) {
			freeze();
			thaw();
		}

		inline void TextViewer::FreezeRegister::addLinesToRedraw(const boost::integer_range<Index>& lines) {
			assert(isFrozen());
#if 0
			linesToRedraw_ = merged(linesToRedraw_, lines);
#else
			const boost::integer_range<Index> linesToAdd(ordered(lines));
			linesToRedraw_ = boost::irange(std::min(*linesToAdd.begin(), *linesToRedraw_.begin()), std::max(*linesToAdd.end(), *linesToRedraw_.end()));
#endif
		}

		inline void TextViewer::FreezeRegister::freeze() {
			if(count_ == std::numeric_limits<std::decay<decltype(count_.data())>::type>::max())
				throw std::overflow_error("");
			++count_;
		}

		inline void TextViewer::FreezeRegister::resetLinesToRedraw(const boost::integer_range<Index>& lines) {
			assert(isFrozen());
			linesToRedraw_ = ordered(lines);
		}

		inline boost::integer_range<Index> TextViewer::FreezeRegister::thaw() {
			if(count_ == 0)
				throw std::underflow_error("");
			const boost::integer_range<Index> temp(linesToRedraw());
			--count_;
			linesToRedraw_ = boost::irange<Index>(0, 0);
			return temp;
		}


		// TextViewer.Renderer ////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param viewer The text viewer
		 */
		TextViewer::Renderer::Renderer(TextViewer& viewer) :
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
		TextViewer::Renderer::Renderer(const Renderer& other, TextViewer& viewer) : TextRenderer(other), viewer_(viewer) {
		}

		/// @see graphics#font#TextRenderer#createLineLayout
		std::unique_ptr<const graphics::font::TextLayout> TextViewer::Renderer::createLineLayout(Index line) const {
			const std::unique_ptr<graphics::RenderingContext2D> renderingContext(widgetapi::createRenderingContext(viewer_));
			graphics::font::ComputedTextLineStyle lineStyle;
			std::unique_ptr<graphics::font::ComputedStyledTextRunIterator> runStyles;
			buildLineLayoutConstructionParameters(line, *renderingContext, lineStyle, runStyles);
			return std::unique_ptr<const graphics::font::TextLayout>(new graphics::font::TextLayout(
				viewer_.document().line(line), lineStyle, std::move(runStyles), fontCollection(),
				renderingContext->fontRenderContext()));
		}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
		/// Rewraps the visual lines at the window's edge.
		void TextViewer::Renderer::rewrapAtWindowEdge() {
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
		Scalar TextViewer::Renderer::width() const BOOST_NOEXCEPT {
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


		// TextViewer.Configuration ///////////////////////////////////////////////////////////////////////////////////

		/// Default constructor.
		TextViewer::Configuration::Configuration() BOOST_NOEXCEPT :
				readingDirection(presentation::LEFT_TO_RIGHT), usesRichTextClipboardFormat(false) {
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

		namespace utils {
			// ascension.viewers.utils free functions /////////////////////////////////////////////////////////////////

			/// Closes the opened completion proposals popup immediately.
			void closeCompletionProposalsPopup(TextViewer& viewer) BOOST_NOEXCEPT {
				if(contentassist::ContentAssistant* ca = viewer.contentAssistant()) {
					if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->completionProposalsUI())
						cpui->close();
				}
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
			 * Toggles the inline flow direction of the text viewer.
			 * @param viewer The text viewer
			 */
			void toggleOrientation(TextViewer& viewer) BOOST_NOEXCEPT {
				viewer.textRenderer().setDirection(!viewer.presentation().computeWritingMode(&viewer.textRenderer()).inlineFlowDirection);
//				viewer.synchronizeWritingModeUI();
//				if(config.lineWrap.wrapsAtWindowEdge()) {
//					win32::AutoZeroSize<SCROLLINFO> scroll;
//					viewer.getScrollInformation(SB_HORZ, scroll);
//					viewer.setScrollInformation(SB_HORZ, scroll);
//				}
			}
		}

		namespace source {
			// ascension.viewers.source free functions ////////////////////////////////////////////////////////////////

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
				using namespace text;
				static const Index MAXIMUM_IDENTIFIER_HALF_LENGTH = 100;

				kernel::DocumentPartition partition;
				document.partitioner().partition(position, partition);
				const IdentifierSyntax& syntax = document.contentTypeInformation().getIdentifierSyntax(partition.contentType);
				Index start = position.offsetInLine, end = position.offsetInLine;

				// find the start of the identifier
				if(startOffsetInLine != nullptr) {
					kernel::DocumentCharacterIterator i(document,
						kernel::Region(std::max(partition.region.beginning(), kernel::Position(position.line, 0)), position), position);
					do {
						i.previous();
						if(!syntax.isIdentifierContinueCharacter(i.current())) {
							i.next();
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
						if(!syntax.isIdentifierContinueCharacter(i.current())) {
							end = i.tell().offsetInLine;
							break;
						}
						i.next();
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

			/**
			 * Returns the identifier near the cursor.
			 * @param viewer The text viewer
			 * @return The found identifier or @c boost#none if not found
			 * @see #getNearestIdentifier
			 */
			boost::optional<kernel::Region> getPointedIdentifier(const TextViewer& viewer) {
//				if(viewer.isWindow()) {
					return getNearestIdentifier(
						viewer.document(), viewToModel(*viewer.textRenderer().viewport(),
						widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position())).characterIndex());
//				}
				return boost::none;
			}
		}
	}

	namespace texteditor {
		/**
		 * Calls @c IncrementalSearcher#abort from @a viewer.
		 * @return true if the incremental search was running
		 */
		bool abortIncrementalSearch(viewers::TextViewer& viewer) BOOST_NOEXCEPT {
			if(Session* session = viewer.document().session()) {
				if(session->incrementalSearcher().isRunning())
					return session->incrementalSearcher().abort(), true;
			}
			return false;
		}

		/**
		 * Calls @c IncrementalSearcher#end from @a viewer.
		 * @return true if the incremental search was running
		 */
		bool endIncrementalSearch(viewers::TextViewer& viewer) BOOST_NOEXCEPT {
			if(Session* session = viewer.document().session()) {
				if(session->incrementalSearcher().isRunning())
					return session->incrementalSearcher().end(), true;
			}
			return false;
		}
	}
}
