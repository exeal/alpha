/**
 * @file line-number-ruler.cpp
 * Implements @c LineNumberRuler class.
 * @author exeal
 * @date 2015-01-13 Created.
 */

#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/font-render-context.hpp>
#include <ascension/graphics/font/glyph-metrics.hpp>
#include <ascension/graphics/font/glyph-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <ascension/viewer/source/line-number-ruler.hpp>
#include <ascension/viewer/source/ruler-allocation-width-sink.hpp>
#include <ascension/viewer/source/source-viewer.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			/// Creates @c LineNumberRuler object.
			LineNumberRuler::LineNumberRuler() :
					alignment_(graphics::font::TextAlignment::END), justification_(graphics::font::TextJustification::AUTO),
					color_(graphics::Color::OPAQUE_BLACK), direction_(ASCENSION_DEFAULT_TEXT_READING_DIRECTION),
					paddingStart_(6), paddingEnd_(1), startValue_(1) {
			}

			/// @internal
			inline std::uint8_t LineNumberRuler::computeNumberOfDigits() const BOOST_NOEXCEPT {
				assert(viewer() != nullptr);
				std::uint8_t n = 1;
				Index lines = viewer()->document().numberOfLines() + startValue_ - 1;
				while(lines >= 10) {
					lines /= 10;
					++n;
				}
				return n;
			}

			/// @internal
			void LineNumberRuler::continueLineSelection(const kernel::Position& to) {
				if(viewer() != nullptr && lineSelectionAnchorLine_ != boost::none) {
					const kernel::Document& document = viewer()->document();
					const Index nlines = document.numberOfLines();
					kernel::Position anchor;
					anchor.line = (kernel::line(to) >= boost::get(lineSelectionAnchorLine_)) ? boost::get(lineSelectionAnchorLine_) : boost::get(lineSelectionAnchorLine_) + 1;
					anchor.offsetInLine = (kernel::line(anchor) > nlines - 1) ? document.lineLength(--anchor.line) : 0;
					kernel::Position caret;
					caret.line = (kernel::line(to) >= boost::get(lineSelectionAnchorLine_)) ? kernel::line(to) + 1 : kernel::line(to);
					caret.offsetInLine = (kernel::line(caret) > nlines - 1) ? document.lineLength(--caret.line) : 0;
					viewer()->textArea().caret().select((_anchor = anchor, _caret = caret));
				}
			}

			/// @internal
			void LineNumberRuler::endLineSelection() {
				if(isTrackingLocation()) {
					endLocationTracking();
					lineSelectionAnchorLine_ = boost::none;
				}
			}

			/// @see AbstractRuler#install
			void LineNumberRuler::install(SourceViewer& viewer, const Locator& locator, RulerAllocationWidthSink& allocationWidthSink) {
				AbstractRuler::install(viewer, locator, allocationWidthSink);
				if(this->viewer() == &viewer) {
					const auto viewport(viewer.textArea().textRenderer().viewport());
					viewportResizedConnection_ = viewport->resizedSignal().connect([this](const graphics::Dimension&) {
						// TODO: This implementation is adhoc.
						invalidate();
					});
					viewportScrolledConnection_ = viewport->scrolledSignal().connect(
						[this](const presentation::FlowRelativeTwoAxes<graphics::font::TextViewport::ScrollOffset>&, const graphics::font::VisualLine&) {
							if(SourceViewer* const sourceViewer = this->viewer()) { 
//								widgetapi::scrollPixels(*sourceViewer);
								widgetapi::redrawScheduledRegion(*sourceViewer);
							}
						}
					);
					viewportScrollPropertiesChangedConnection_ = viewport->scrollPropertiesChangedSignal().connect([this](const presentation::FlowRelativeTwoAxes<bool>&) {
						// TODO: This implementation is adhoc.
						invalidate();
					});
				}
			}

			/// @see MouseInputStrategy#interruptMouseReaction
			void LineNumberRuler::interruptMouseReaction(bool forKeyboardInput) {
				endLineSelection();
			}

			inline void LineNumberRuler::invalidate() {
				if(const TextViewerComponent::Locator* const componentLocator = locator())
					widgetapi::scheduleRedraw(*viewer(), componentLocator->locateComponent(*this), false);
			}

			/// @see MouseInputStrategy#mouseButtonInput
			void LineNumberRuler::mouseButtonInput(Action action, widgetapi::event::MouseButtonInput& input, TargetLocker& targetLocker) {
				if(viewer() != nullptr) {
					if(input.button() == widgetapi::event::LocatedUserInput::BUTTON1_DOWN) {
						if(action == PRESSED) {
							// select line(s)
							Caret& caret = viewer()->textArea().caret();
							const kernel::Position to(viewToModel(*viewer(), input.location()).insertionIndex());
							const bool extend = input.hasModifier(widgetapi::event::UserInput::SHIFT_DOWN) && kernel::line(to) != kernel::line(caret.anchor());
							lineSelectionAnchorLine_ = extend ? kernel::line(caret.anchor()) : kernel::line(to);
							caret.endRectangleSelection();
							continueLineSelection(to);
							beginLocationTracking(*viewer(), &targetLocker, true, true);
							return input.consume();
						} else if(action == RELEASED) {
							endLineSelection();
							return input.consume();
						}
					}
				}
				return AbstractMouseInputStrategy::mouseButtonInput(action, input, targetLocker);
			}

			/// @see MouseInputStrategy#mouseInputTargetUnlocked
			void LineNumberRuler::mouseInputTargetUnlocked() {
				endLineSelection();
			}

			/// @see Ruler#paint
			void LineNumberRuler::paint(graphics::PaintContext& context) {
				// TODO: Not implemented.
			}

			/**
			 * Sets the alignment and justification settings.
			 * @param alignment The text alignment
			 * @param justification The text justification setting
			 * @throw UnknownValueException @a alignment or @a justification is invalid
			 */
			void LineNumberRuler::setAlignment(graphics::font::TextAlignment alignment,
					graphics::font::TextJustification justification /* = graphics::font::TextJustification::AUTO */) {
				if(alignment < graphics::font::TextAlignment::START || alignment > graphics::font::TextAlignment::START_END)
					throw UnknownValueException("alignment");
				if(justification < graphics::font::TextJustification::AUTO || justification > graphics::font::TextJustification::DISTRIBUTE)
					throw UnknownValueException("justification");
				alignment_ = alignment;
				justification_ = justification;
//				updateWidth();
			}

			/**
			 * Sets the 'color' style.
			 * @param color The "Actual Value" of the 'color' style
			 */
			void LineNumberRuler::setColor(const graphics::Color& color) {
				color_ = color;
			}

			/**
			 * Sets the reading direction.
			 * @param direction The reading direction
			 * @throw UnknownValueException @a direction is invalid
			 * @see presentation#styles#Direction
			 */
			void LineNumberRuler::setDirection(presentation::ReadingDirection direction) {
				if(direction < presentation::LEFT_TO_RIGHT || direction > presentation::RIGHT_TO_LEFT)
					throw UnknownValueException("direction");
				direction_ = direction;
//				updateWidth();
			}

			/**
			 * Sets the paddings of the line-start-edge and line-end-edge.
			 * @param paddingStart The padding of the line-start-edge in user units
			 * @param paddingEnd The padding of the line-end-edge in user units
			 */
			void LineNumberRuler::setPaddings(graphics::Scalar paddingStart, graphics::Scalar paddingEnd) {
				if(std::is_signed<decltype(paddingStart)>::value && paddingStart < 0)
					throw std::underflow_error("paddingStart");
				if(std::is_signed<decltype(paddingEnd)>::value && paddingEnd < 0)
					throw std::underflow_error("paddingEnd");
				paddingStart_ = paddingStart;
				paddingEnd_ = paddingEnd;
				updateWidth();
			}

			/**
			 * Sets the start value of the line numbers. This is the number used to display the zero line number.
			 * @param The start value of the line numbers
			 */
			void LineNumberRuler::setStartValue(Index startValue) {
				startValue_ = startValue;
				numberOfDigits_ = boost::none;
				updateWidth();
			}

			/// @see MouseInputStrategy#showCursor
			bool LineNumberRuler::showCursor(const graphics::Point& position) {
				return (viewer() != nullptr) ? showArrowCursor(*viewer()) : AbstractMouseInputStrategy::showCursor(position);
			}

			/// @see AbstractMouseInputStrategy#trackedLocationChanged
			void LineNumberRuler::trackedLocationChanged(const kernel::Position& position) {
				continueLineSelection(position);
			}

			/// @see AbstractRuler#uninstall
			void LineNumberRuler::uninstall(SourceViewer& viewer) {
				SourceViewer* const oldViewer = this->viewer();
				AbstractRuler::uninstall(viewer);
				if(this->viewer() == nullptr) {
					viewportResizedConnection_.disconnect();
					viewportScrolledConnection_.disconnect();
					viewportScrollPropertiesChangedConnection_.disconnect();
				}
			}

			/// @internal
			inline bool LineNumberRuler::updateNumberOfDigits() BOOST_NOEXCEPT {
				if(viewer() != nullptr) {
					const std::uint8_t newNumberOfDigits = computeNumberOfDigits();
					if(numberOfDigits_ == boost::none || newNumberOfDigits != boost::get(numberOfDigits_)) {
						numberOfDigits_ = newNumberOfDigits;
						return true;
					}
				}
				return false;
			}

			/// @internal
			inline void LineNumberRuler::updateWidth() {
				width_ = boost::none;
				if(RulerAllocationWidthSink* const sink = allocationWidthSink())
					sink->updateRulerAllocationWidth(*this);
			}

			namespace {
				graphics::Scalar computeContentWidth(graphics::RenderingContext2D& context, std::shared_ptr<const graphics::font::Font> font,
						std::uint8_t digits, presentation::BlockFlowDirection writingMode, const graphics::font::NumberSubstitution& numberSubstitution) {
					std::shared_ptr<const graphics::font::Font> oldFont(context.font());
					context.setFont(font);
/*
#if ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE)
					SCRIPT_STRING_ANALYSIS ssa;
					win32::AutoZero<SCRIPT_CONTROL> sc;
					win32::AutoZero<SCRIPT_STATE> ss;
					HRESULT hr;
//					switch(configuration_.lineNumbers.digitSubstitution) {
//						case DST_CONTEXTUAL:
//						case DST_NOMINAL:
//							break;
//						case DST_NATIONAL:
//							ss.fDigitSubstitute = 1;
//							break;
//						case DST_USER_DEFAULT:
							hr = ::ScriptApplyDigitSubstitution(&numberSubstitution.asUniscribe(), &sc, &ss);
//							break;
//					}
					::SetTextCharacterExtra(dc.get(), 0);
					hr = ::ScriptStringAnalyse(dc.get(), L"0123456789", 10,
						estimateNumberOfGlyphs(10), -1, SSA_FALLBACK | SSA_GLYPHS | SSA_LINK, 0, &sc, &ss, 0, 0, 0, &ssa);
					int glyphWidths[10];
					hr = ::ScriptStringGetLogicalWidths(ssa, glyphWidths);
					int maxGlyphWidth = *max_element(glyphWidths, ASCENSION_ENDOF(glyphWidths));
#else
#endif
*/
					const graphics::font::FontRenderContext frc(context.fontRenderContext());
					Char maximumExtentCharacter;
					graphics::Scalar maximumAdvance = 0;
					for(Char c = '0'; c <= '9'; ++c) {
						std::unique_ptr<const graphics::font::GlyphVector> glyphs(font->createGlyphVector(frc, StringPiece(&c, 1)));
						const graphics::font::GlyphMetrics gm(glyphs->glyphMetrics(0));
						const graphics::Scalar advance = isHorizontal(writingMode) ? gm.advanceX() : gm.advanceY();
						if(advance > maximumAdvance) {
							maximumExtentCharacter = c;
							maximumAdvance = advance;
						}
					}
					const graphics::Dimension stringExtent(context.measureText(String(digits, maximumExtentCharacter)));

					context.setFont(oldFont);
					return presentation::isHorizontal(writingMode) ? graphics::geometry::dx(stringExtent) : graphics::geometry::dy(stringExtent);
				}
			}

			/// @see graphics#font#VisualLinesListener#visualLinesDeleted
			void LineNumberRuler::visualLinesDeleted(const boost::integer_range<Index>& lines,
					Index sublines, bool longestLineChanged) BOOST_NOEXCEPT {
				if(const SourceViewer* const sourceViewer = viewer()) {
					if(*lines.end() < sourceViewer->textArea().textRenderer().viewport()->firstVisibleLine().line)	// deleted before visible area
						invalidate();
				}
			}

			/// @see graphics#font#VisualLinesListener#visualLinesInserted
			void LineNumberRuler::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
				if(const SourceViewer* const sourceViewer = viewer()) {
					if(*lines.end() < sourceViewer->textArea().textRenderer().viewport()->firstVisibleLine().line)	// inserted before visible area
						invalidate();
				}
			}

			/// @see graphics#font#VisualLinesListener#visualLinesModified
			void LineNumberRuler::visualLinesModified(const boost::integer_range<Index>& lines,
					SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT {
				if(const SourceViewer* const sourceViewer = viewer()) {
					if(sublinesDifference != 0) {	// number of visual lines was changed
						if(*lines.end() < sourceViewer->textArea().textRenderer().viewport()->firstVisibleLine().line)	// changed before visible area
							invalidate();
					}
				}
			}

			/// @see Ruler#width
			graphics::Scalar LineNumberRuler::width() const BOOST_NOEXCEPT {
				if(viewer() == nullptr)
					return 0;
				if(width_ == boost::none) {
					const graphics::Scalar interiorWidth = paddingStart_ + paddingEnd_;

					const std::shared_ptr<const graphics::font::Font> font((font_.get() != nullptr) ? font_ : viewer()->textArea().textRenderer().defaultFont());
					std::unique_ptr<graphics::RenderingContext2D> context(widgetapi::createRenderingContext(*viewer()));
					LineNumberRuler& self = *const_cast<LineNumberRuler*>(this);
					self.updateNumberOfDigits();
					graphics::Scalar contentWidth = computeContentWidth(*context, font, boost::get(numberOfDigits_),
						boost::fusion::at_key<presentation::styles::WritingMode>(
							static_cast<const presentation::styles::ComputedValue<presentation::TextToplevelStyle>::type&>(
								viewer()->presentation().computedTextToplevelStyle())), numberSubstitution_);
					const graphics::Scalar minimumContentWidth = context->fontMetrics(font)->averageCharacterWidth() * boost::get(numberOfDigits_);
					contentWidth = std::max(contentWidth, minimumContentWidth);

					self.width_ = interiorWidth + contentWidth;
				}
				return boost::get(width_);
			}
		}
	}
}
