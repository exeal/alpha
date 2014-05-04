/**
 * @file ruler.cpp
 * @author exeal
 * @date 2010-10-27 created (separated code from layout.cpp)
 * @date 2010-2014
 */

#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/glyph-metrics.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>
#ifdef _DEBUG
#	include <boost/log/trivial.hpp>
#endif

namespace ascension {
	extern const bool DIAGNOSE_INHERENT_DRAWING;

	namespace viewers {
		// RulerStyles.LineNumbers ////////////////////////////////////////////////////////////////////////////////////

		/// Constructor initializes the all members to their default values.
		RulerStyles::LineNumbers::LineNumbers() BOOST_NOEXCEPT : visible(false) {
		}

		// RulerStyles.IndicatorMargin ////////////////////////////////////////////////////////////////////////////////

		/// Constructor initializes the all members to their default values.
		RulerStyles::IndicatorMargin::IndicatorMargin() BOOST_NOEXCEPT : visible(false), width(presentation::Length(0)) {
		}

		namespace detail {
			// detail.RulerPainter ////////////////////////////////////////////////////////////////////////////////////////

			// TODO: support locale-dependent number format.

			void drawLineNumber(graphics::PaintContext& context, const graphics::Point& origin, Index lineNumber, const presentation::NumberSubstitution& ns) {
				// format number string
				std::array<wchar_t, 128> s;	// TODO: Oops, is this sufficient?
				// TODO: std.swprintf may be slow.
				// TODO: use 'ns' parameter.
#if defined(_MSC_VER) && (_MSC_VER < 1400)
				const int length = std::swprintf(s.data(), L"%lu", lineNumber);
#else
				const int length = std::swprintf(s.data(), s.size(), L"%lu", lineNumber);
#endif // _MSC_VER < 1400

				context.fillText(s.data(), origin);
			}

			/**
			 * Constructor.
			 * @param viewer The text viewer
			 * @throw std#bad_alloc
			 */
			RulerPainter::RulerPainter(viewers::TextViewer& viewer, std::shared_ptr<const viewers::RulerStyles> initialStyles /* = nullptr */) : viewer_(viewer), declaredStyles_(initialStyles) {
				if(declaredStyles_.get() == nullptr)
					declaredStyles_ = std::make_shared<viewers::RulerStyles>();
				computeAllocationWidth();
			}

			/**
			 * Computes the snap alignment of the ruler of the text viewer.
			 * @param viewer The text viewer
			 * @return The snap alignment of the ruler in the text viewer
			 */
			graphics::PhysicalDirection RulerPainter::alignment() const {
				presentation::TextAlignment computedAlignment;
				if(!declaredStyles().alignment.inherits())
					computedAlignment = declaredStyles().alignment.get();
				else {
					std::shared_ptr<const presentation::TextLineStyle> defaultLineStyle(defaultTextLineStyle(viewer_.presentation().textToplevelStyle()));
					assert(defaultLineStyle.get() != nullptr);
					computedAlignment = !defaultLineStyle->textAlignment.inherits() ?
						defaultLineStyle->textAlignment.get() : declaredStyles().alignment.initialValue();
				}

				using presentation::detail::PhysicalTextAnchor;
				using presentation::detail::computePhysicalTextAnchor;
				const presentation::WritingMode writingMode(viewer_.presentation().computeWritingMode(&viewer_.textRenderer()));
				PhysicalTextAnchor anchor;
				switch(declaredStyles().alignment.getOrInitial()) {
					case presentation::TextAlignment::START:
						anchor = computePhysicalTextAnchor(presentation::TextAnchor::START, writingMode.inlineFlowDirection);
						break;
					case presentation::TextAlignment::END:
						anchor = computePhysicalTextAnchor(presentation::TextAnchor::END, writingMode.inlineFlowDirection);
						break;
					case presentation::TextAlignment::LEFT:
						anchor = PhysicalTextAnchor::LEFT;
						break;
					case presentation::TextAlignment::RIGHT:
						anchor = PhysicalTextAnchor::RIGHT;
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
				switch(anchor) {
					// TODO: 'text-orientation' is ignored.
					case PhysicalTextAnchor::LEFT:
						return presentation::isHorizontal(writingMode.blockFlowDirection) ? graphics::PhysicalDirection::LEFT : graphics::PhysicalDirection::TOP;
					case PhysicalTextAnchor::RIGHT:
						return presentation::isHorizontal(writingMode.blockFlowDirection) ? graphics::PhysicalDirection::RIGHT : graphics::PhysicalDirection::BOTTOM;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}
		} // namespace detail

		namespace {
			graphics::Scalar computeMaximumNumberGlyphsExtent(graphics::RenderingContext2D& context, std::shared_ptr<const graphics::font::Font> font,
					std::uint8_t digits, const presentation::WritingMode& writingMode, const presentation::NumberSubstitution& numberSubstitution) {
				std::shared_ptr<const graphics::font::Font> oldFont(context.font());
				context.setFont(font);
/*
#if ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE)
				SCRIPT_STRING_ANALYSIS ssa;
				win32::AutoZero<SCRIPT_CONTROL> sc;
				win32::AutoZero<SCRIPT_STATE> ss;
				HRESULT hr;
//				switch(configuration_.lineNumbers.digitSubstitution) {
//					case DST_CONTEXTUAL:
//					case DST_NOMINAL:
//						break;
//					case DST_NATIONAL:
//						ss.fDigitSubstitute = 1;
//						break;
//					case DST_USER_DEFAULT:
						hr = ::ScriptApplyDigitSubstitution(&numberSubstitution.asUniscribe(), &sc, &ss);
//						break;
//				}
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
					const graphics::Scalar advance = isHorizontal(writingMode.blockFlowDirection) ? gm.advanceX() : gm.advanceY();
					if(advance > maximumAdvance) {
						maximumExtentCharacter = c;
						maximumAdvance = advance;
					}
				}
				const graphics::Dimension stringExtent(context.measureText(String(digits, maximumExtentCharacter)));

				context.setFont(oldFont);
				return presentation::isHorizontal(writingMode.blockFlowDirection) ? graphics::geometry::dx(stringExtent) : graphics::geometry::dy(stringExtent);
			}
	
			inline std::uint16_t platformIndicatorMarginWidthInPixels(bool horizontalLayout) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				return 15;	// TODO: Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#if 1
				const int width = ::GetSystemMetrics(horizontalLayout ? SM_CYHSCROLL : SM_CXVSCROLL);
				return static_cast<std::uint16_t>(width != 0) ? width : 15;
#else
				// TODO: This code is not suitable when the indicator margin top or bottom of the viewer?
				win32::AutoZeroSize<NONCLIENTMETRICSW> ncm;
				if(win32::boole(::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0)))
					width = Length(ncm.iScrollWidth, Length::PIXELS);
				else
					width = Length(15, Length::PIXELS);
#endif
#endif
			}
		}

		namespace detail {
			/// Recomputes the total width of the ruler.
			void RulerPainter::computeAllocationWidth() BOOST_NOEXCEPT {
				// (ruler-total-width) = (line-numbers-width) + (indicator-margin-width)
				//   (indicator-margin-width) = (indicator-margin-border-width) + (indicator-margin-content-width)
				//   (line-numbers-width) = (line-numbers-exterior-width) + (line-numbers-interior-width) + (line-numbers-content-width)
				//     (line-numbers-exterior-width) = (line-numbers-border-width) + (line-numbers-space-width)
				//     (line-numbers-interior-width) = (line-numbers-padding-start) + (line-numbers-padding-end)
				//     (line-numbers-content-width) = max((glyphs-extent), (average-glyph-extent) * (minimum-digits-setting))

				std::unique_ptr<graphics::RenderingContext2D> context(viewers::widgetapi::createRenderingContext(viewer_));

				// compute the width of the line numbers
				decltype(computedLineNumbersBorderEnd_) computedLineNumbersBorderEnd;
				decltype(computedLineNumbersContentWidth_) computedLineNumbersContentWidth;
				decltype(computedLineNumbersPaddingStart_) computedLineNumbersPaddingStart;
				decltype(computedLineNumbersPaddingEnd_) computedLineNumbersPaddingEnd;
				decltype(computedLineNumberDigits_) computedLineNumberDigits;
				if(lineNumbers(declaredStyles())->visible) {
					std::shared_ptr<const viewers::RulerStyles::LineNumbers> declaredStyle(lineNumbers(declaredStyles()));
					boost::get(computedLineNumberDigits) = computeMaximumDigitsForLineNumbers();
					const graphics::Scalar glyphsExtent = computeMaximumNumberGlyphsExtent(
						*context, viewer_.textRenderer().defaultFont(), computedLineNumberDigits,
						viewer_.presentation().computeWritingMode(&viewer_.textRenderer()),
						declaredStyle->numberSubstitution.getOrInitial());
					const graphics::Scalar minimumExtent = context->fontMetrics(viewer_.textRenderer().defaultFont())->averageCharacterWidth() * computedLineNumberDigits;
					boost::get(computedLineNumbersContentWidth) = std::max(glyphsExtent, minimumExtent);

					// 'padding-start' and 'padding-end'
					const graphics::Dimension referenceBox(
						graphics::geometry::_dx = computedLineNumbersContentWidth, graphics::geometry::_dy = computedLineNumbersContentWidth);
					const presentation::Length::Context lengthContext(context.get(), &referenceBox);
					boost::get(computedLineNumbersPaddingStart) = static_cast<graphics::Scalar>(declaredStyle->paddingStart.getOrInitial().value(lengthContext));
					boost::get(computedLineNumbersPaddingEnd) = static_cast<graphics::Scalar>(declaredStyle->paddingEnd.getOrInitial().value(lengthContext));

					// 'border-end'
					computedLineNumbersBorderEnd.color =
						computeColor(&declaredStyle->borderEnd.color, &declaredStyles_->color, viewer_.presentation().textToplevelStyle());
					declaredStyle->borderEnd.color;
					computedLineNumbersBorderEnd.style = declaredStyle->borderEnd.style.getOrInitial();
					computedLineNumbersBorderEnd.width = declaredStyle->borderEnd.width.getOrInitial().value(lengthContext);
//					const Scalar spaceWidth = 0;
//					const Scalar exteriorWidth = borderWidth + spaceWidth;
				}

				// compute the width of the indicator margin
				decltype(computedIndicatorMarginBorderEnd_) computedIndicatorMarginBorderEnd;
				decltype(computedIndicatorMarginContentWidth_) computedIndicatorMarginContentWidth;
				if(indicatorMargin(declaredStyles())->visible) {
					std::shared_ptr<const viewers::RulerStyles::IndicatorMargin> declaredStyle(indicatorMargin(declaredStyles()));

					// 'width'
					boost::optional<presentation::Length> contentWidth(declaredStyle->width.getOrInitial());
					boost::get(computedIndicatorMarginContentWidth) = (contentWidth != boost::none) ?
						static_cast<graphics::Scalar>(contentWidth->value(presentation::Length::Context(context.get(), nullptr)))
						: platformIndicatorMarginWidthInPixels(isHorizontal(viewer_.textRenderer().computedBlockFlowDirection()));

					// 'border-end'
					computedIndicatorMarginBorderEnd.color =
						computeColor(&declaredStyle->borderEnd.color, &declaredStyles_->color, viewer_.presentation().textToplevelStyle());
					computedIndicatorMarginBorderEnd.style = declaredStyle->borderEnd.style.getOrInitial();
					computedIndicatorMarginBorderEnd.width = declaredStyle->borderEnd.width.getOrInitial().value(presentation::Length::Context(
						context.get(), &graphics::Dimension(
							graphics::geometry::_dx = computedIndicatorMarginContentWidth,
							graphics::geometry::_dy = computedIndicatorMarginContentWidth)));
				}

				// commit
				const graphics::Scalar oldAllocationWidth = allocationWidth();
				computedIndicatorMarginBorderEnd_ = computedIndicatorMarginBorderEnd;
				computedLineNumbersBorderEnd_ = computedLineNumbersBorderEnd;
				computedIndicatorMarginContentWidth_ = computedIndicatorMarginContentWidth;
				computedLineNumbersContentWidth_ = computedLineNumbersContentWidth;
				computedLineNumbersPaddingStart_ = computedLineNumbersPaddingStart;
				computedLineNumbersPaddingEnd_ = computedLineNumbersPaddingEnd;
				if(allocationWidth() != oldAllocationWidth) {
					viewers::widgetapi::scheduleRedraw(viewer_, false);
#ifdef ASCENSION_USE_SYSTEM_CARET
					viewer_.caret().updateLocation();
#endif
				}
			}

			/// Computes the maximum number of digits of line numbers.
			std::uint8_t RulerPainter::computeMaximumDigitsForLineNumbers() const BOOST_NOEXCEPT {
				std::uint8_t n = 1;
				const Index startValue = lineNumbers(declaredStyles())->startValue.getOrInitial();
				Index lines = viewer_.document().numberOfLines() + startValue - 1;
				while(lines >= 10) {
					lines /= 10;
					++n;
				}
				return static_cast<std::uint8_t>(n);	// hmm...
			}

			/// Returns the 'allocation-rectangle' of the indicator margin in the viewer-local coordinates.
			graphics::Rectangle RulerPainter::indicatorMarginAllocationRectangle() const BOOST_NOEXCEPT {
				namespace geometry = graphics::geometry;
				using graphics::Dimension;
				using graphics::PhysicalDirection;
				const graphics::Rectangle localBounds(viewers::widgetapi::bounds(viewer_, false));
				switch(alignment()) {
					case PhysicalDirection::LEFT:
						return graphics::Rectangle(
							geometry::topLeft(localBounds),
							Dimension(geometry::_dx = indicatorMarginAllocationWidth(), geometry::_dy = geometry::dy(localBounds)));
					case PhysicalDirection::TOP:
						return graphics::Rectangle(
							geometry::topLeft(localBounds),
							Dimension(geometry::_dx = geometry::dx(localBounds), geometry::_dy = indicatorMarginAllocationWidth()));
					case PhysicalDirection::RIGHT:
						return geometry::normalize(
							graphics::Rectangle(
								geometry::topRight(localBounds),
								Dimension(geometry::_dx = -indicatorMarginAllocationWidth(), geometry::_dy = geometry::dy(localBounds))));
					case PhysicalDirection::BOTTOM:
						return geometry::normalize(
							graphics::Rectangle(
								geometry::bottomLeft(localBounds),
								Dimension(geometry::_dx = geometry::dx(localBounds), geometry::_dy = -indicatorMarginAllocationWidth())));
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/// Returns the 'allocation-rectangle' of the line numbers in the viewer-local coordinates.
			graphics::Rectangle RulerPainter::lineNumbersAllocationRectangle() const BOOST_NOEXCEPT {
				namespace geometry = graphics::geometry;
				using graphics::Dimension;
				using graphics::PhysicalDirection;
				const graphics::Rectangle localBounds(viewers::widgetapi::bounds(viewer_, false));
				switch(alignment()) {
					case PhysicalDirection::LEFT:
						return graphics::Rectangle(
							geometry::translate(
								geometry::topLeft(localBounds), Dimension(geometry::_dx = indicatorMarginAllocationWidth(), geometry::_dy = 0.0f)),
							Dimension(geometry::_dx = lineNumbersAllocationWidth(), geometry::_dy = geometry::dy(localBounds)));
					case PhysicalDirection::TOP:
						return graphics::Rectangle(
							geometry::translate(
								geometry::topLeft(localBounds), Dimension(geometry::_dx = 0.0f, geometry::_dy = indicatorMarginAllocationWidth())),
							Dimension(geometry::_dx = geometry::dx(localBounds), geometry::_dy = lineNumbersAllocationWidth()));
					case PhysicalDirection::RIGHT:
						return geometry::normalize(
							graphics::Rectangle(
								geometry::translate(
									geometry::topRight(localBounds), Dimension(geometry::_dx = -indicatorMarginAllocationWidth(), geometry::_dy = 0.0f)),
								Dimension(geometry::_dx = -lineNumbersAllocationWidth(), geometry::_dy = geometry::dy(localBounds))));
					case PhysicalDirection::BOTTOM:
						return geometry::normalize(
							graphics::Rectangle(
								geometry::translate(
									geometry::bottomLeft(localBounds), Dimension(geometry::_dx = 0.0f, geometry::_dy = -indicatorMarginAllocationWidth())),				
								Dimension(geometry::_dx = geometry::dx(localBounds), geometry::_dy = -lineNumbersAllocationWidth())));
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			/**
			 * Paints the ruler.
			 * @param context The graphics context
			 */
			void RulerPainter::paint(graphics::PaintContext& context) {
				if(allocationWidth() == 0)
					return;

				const graphics::Rectangle paintBounds(context.boundsToPaint());
				const graphics::font::TextRenderer& renderer = viewer_.textRenderer();

				const graphics::Rectangle indicatorMarginRectangle(indicatorMarginAllocationRectangle());
				const graphics::Rectangle lineNumbersRectangle(lineNumbersAllocationRectangle());

				const bool indicatorMarginToPaint = indicatorMargin(declaredStyles())->visible
					&& !graphics::geometry::isEmpty(indicatorMarginRectangle) && boost::geometry::intersects(indicatorMarginRectangle, paintBounds);
				const bool lineNumbersToPaint = lineNumbers(declaredStyles())->visible
					&& !graphics::geometry::isEmpty(lineNumbersRectangle) && boost::geometry::intersects(lineNumbersRectangle, paintBounds);
				if(!indicatorMarginToPaint && !lineNumbersToPaint)
					return;

#ifdef _DEBUG
				if(DIAGNOSE_INHERENT_DRAWING)
					BOOST_LOG_TRIVIAL(debug) << L"@RulerPainter.paint draws y = "
						<< graphics::geometry::top(paintBounds) << L" ~ " << graphics::geometry::bottom(paintBounds) << L"\n";
#endif // _DEBUG

				context.save();

				// which side of border to paint?
				const graphics::PhysicalDirection borderSide(!alignment());

				// paint the indicator margin
				const presentation::WritingMode writingMode(viewer_.presentation().computeWritingMode(&renderer));
				if(indicatorMarginToPaint) {
					// background
					std::shared_ptr<const graphics::Paint> background(computeBackground(
						&indicatorMargin(declaredStyles())->background, &declaredStyles().background, viewer_.presentation().textToplevelStyle()));
//					const shared_ptr<Paint> paint(indicatorMargin(declaredStyles())->paint.getOr(
//						shared_ptr<Paint>(new SolidColor(SystemColors::get(SystemColors::THREE_D_FACE)))));
					context.setFillStyle(background);
					context.fillRectangle(indicatorMarginRectangle);

					// border
					graphics::PhysicalFourSides<graphics::font::ComputedBorderSide> borders;
					borders[borderSide] = computedIndicatorMarginBorderEnd_;
					graphics::font::detail::paintBorder(context, indicatorMarginRectangle, borders, writingMode);
				}

				// paint the line numbers
				if(lineNumbersToPaint) {
					const presentation::TextToplevelStyle& toplevelStyle = viewer_.presentation().textToplevelStyle();

					// background
					std::shared_ptr<graphics::Paint> background(computeBackground(&lineNumbers(declaredStyles())->background, &declaredStyles().background, toplevelStyle));
					context.setFillStyle(background);
					context.fillRectangle(lineNumbersRectangle);
		
					// border
					graphics::PhysicalFourSides<graphics::font::ComputedBorderSide> borders;
					borders[borderSide] = computedLineNumbersBorderEnd_;
					graphics::font::detail::paintBorder(context, lineNumbersRectangle, borders, writingMode);

					// text
					context.setFillStyle(std::make_shared<const graphics::SolidColor>(computeColor(&lineNumbers(declaredStyles())->color, &declaredStyles().color, toplevelStyle)));
					context.setFont(viewer_.textRenderer().defaultFont());
//					context.setTextAlign();
//					context.setTextBaseline();

					// TODO: paint glyphs.
				}

#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && 0
				graphics::Scalar left;
				HDC dcex;
				if(enablesDoubleBuffering_) {
					if(memoryDC_.get() == nullptr)
						memoryDC_.reset(::CreateCompatibleDC(win32::ClientAreaGraphicsContext(viewer_.identifier()).nativeHandle().get()), &::DeleteDC);
					if(memoryBitmap_.get() == nullptr)
						memoryBitmap_.reset(::CreateCompatibleBitmap(context.nativeHandle().get(),
							width(), geometry::dy(clientBounds) + ::GetSystemMetrics(SM_CYHSCROLL)), &::DeleteObject);
					::SelectObject(memoryDC_.get(), memoryBitmap_.get());
					dcex = memoryDC_.get();
					left = 0;
				} else {
					dcex = context.nativeHandle().get();
					left = leftAligned ? geometry::left(clientBounds) : geometry::right(clientBounds) - width();
				}
				const graphics::Scalar right = left + width();

				// first of all, paint the drawing area
				if(configuration_.indicatorMargin.visible) {
					// border and inside of the indicator margin
					const Scalar borderX = leftAligned ? left + imWidth - 1 : right - imWidth;
					HPEN oldPen = static_cast<HPEN>(::SelectObject(dcex, indicatorMarginPen_.get()));
					HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(dcex, indicatorMarginBrush_.get()));
					::PatBlt(dcex, leftAligned ? left : borderX + 1, geometry::top(paintBounds), imWidth, geometry::dy(paintBounds), PATCOPY);
					::MoveToEx(dcex, borderX, geometry::top(paintBounds), nullptr);
					::LineTo(dcex, borderX, geometry::bottom(paintBounds));
					::SelectObject(dcex, oldPen);
					::SelectObject(dcex, oldBrush);
				}
				if(configuration_.lineNumbers.visible) {
					// background of the line numbers
					HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(dcex, lineNumbersBrush_.get()));
					::PatBlt(dcex, leftAligned ? left + imWidth : left, geometry::top(paintBounds), right - imWidth, geometry::bottom(paintBounds), PATCOPY);
					// border of the line numbers
					if(configuration_.lineNumbers.borderStyle != RulerConfiguration::LineNumbers::NONE) {
						HPEN oldPen = static_cast<HPEN>(::SelectObject(dcex, lineNumbersPen_.get()));
						const Scalar x = (leftAligned ? right : left + 1) - configuration_.lineNumbers.borderWidth;
						::MoveToEx(dcex, x, 0/*paintRect.top*/, nullptr);
						::LineTo(dcex, x, gewometry::bottom(paintBounds));
						::SelectObject(dcex, oldPen);
					}
					::SelectObject(dcex, oldBrush);

					// for next...
					::SetBkMode(dcex, TRANSPARENT);
					::SetTextColor(dcex, systemColors.serve(configuration_.lineNumbers.textColor.foreground, COLOR_WINDOWTEXT));
					::SetTextCharacterExtra(dcex, 0);	// line numbers ignore character extra
					::SelectObject(dcex, renderer.defaultFont()->nativeHandle().get());
				}

				// prepare to draw the line numbers
				presentation::Inheritable<presentation::ReadingDirection> lineNumbersReadingDirection;
				presentation::TextAnchor lineNumbersAlignment;
				graphics::Scalar lineNumbersX;
				if(configuration_.lineNumbers.visible) {
					// compute reading direction of the line numbers from 'configuration_.lineNumbers.readingDirection'
					if(configuration_.lineNumbers.readingDirection.inherits()) {
						const shared_ptr<const TextLineStyle> defaultLineStyle(viewer_.presentation().globalTextStyle()->defaultLineStyle);
						if(defaultLineStyle.get() != nullptr)
							lineNumbersReadingDirection = defaultLineStyle->readingDirection;
						if(lineNumbersReadingDirection.inherits())
							lineNumbersReadingDirection = renderer.defaultUIWritingMode().inlineFlowDirection;
						if(lineNumbersReadingDirection.inherits())
							lineNumbersReadingDirection = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
						assert(lineNumbersReadingDirection == LEFT_TO_RIGHT || lineNumbersReadingDirection == RIGHT_TO_LEFT);
					} else if(configuration_.lineNumbers.readingDirection == LEFT_TO_RIGHT
							|| configuration_.lineNumbers.readingDirection == RIGHT_TO_LEFT)
						lineNumbersReadingDirection = configuration_.lineNumbers.readingDirection;
					else
						throw runtime_error("can't resolve reading direction of line numbers in vertical ruler.");

					// compute alignment of the line numbers from 'configuration_.lineNumbers.anchor'
					switch(detail::computePhysicalTextAnchor(configuration_.lineNumbers.anchor, lineNumbersReadingDirection)) {
					case detail::LEFT:
						lineNumbersX = leftAligned ?
							left + imWidth + configuration_.lineNumbers.leadingMargin : left + configuration_.lineNumbers.trailingMargin + 1;
						::SetTextAlign(dcex, TA_LEFT | TA_TOP | TA_NOUPDATECP);
						break;
					case detail::RIGHT:
						lineNumbersX = leftAligned ?
							right - configuration_.lineNumbers.trailingMargin - 1 : right - imWidth - configuration_.lineNumbers.leadingMargin;
						::SetTextAlign(dcex, TA_RIGHT | TA_TOP | TA_NOUPDATECP);
						break;
					case detail::MIDDLE:
						lineNumbersX = leftAligned ?
							left + (imWidth + configuration_.lineNumbers.leadingMargin + width() - configuration_.lineNumbers.trailingMargin) / 2
							: right - (width() - configuration_.lineNumbers.trailingMargin + imWidth + configuration_.lineNumbers.leadingMargin) / 2;
						::SetTextAlign(dcex, TA_CENTER | TA_TOP | TA_NOUPDATECP);
						break;
					}
				}

				// 1 行ずつ細かい描画
				Index line, visualSublineOffset;
				const Index lines = viewer_.document().numberOfLines();
				viewer_.mapClientYToLine(geometry::top(paintBounds), &line, &visualSublineOffset);	// $friendly-access
				if(visualSublineOffset > 0)	// 描画開始は次の論理行から...
					++line;
				int y = viewer_.mapLineToClientY(line, false);
				if(y != 32767 && y != -32768) {
					const int dy = renderer.defaultFont()->metrics().linePitch();
					while(y < geometry::bottom(paintBounds) && line < lines) {
						const TextLayout& layout = renderer.layouts().at(line);
						const int nextY = y + static_cast<int>(layout.numberOfLines() * dy);
						if(nextY >= geometry::top(paintBounds)) {
							// 派生クラスにインジケータマージンの描画機会を与える
							if(configuration_.indicatorMargin.visible) {
								NativeRectangle rect;
								geometry::range<geometry::X_COORDINATE>(rect) = makeRange(
									leftAligned ? left : right - configuration_.indicatorMargin.width,
									leftAligned ? left + configuration_.indicatorMargin.width : right);
								geometry::range<geometry::Y_COORDINATE>(rect) = makeRange(y, y + dy);
								viewer_.drawIndicatorMargin(line, *dcex, rect);
							}

							// draw line number digits
							if(configuration_.lineNumbers.visible)
								drawLineNumber(*dcex, lineNumbersX, y, line + configuration_.lineNumbers.startValue);
						}
						++line;
						y = nextY;
					}
				}

				if(enablesDoubleBuffering_)
					::BitBlt(context.engine()->nativeHandle().get(),
						leftAligned ? geometry::left(clientBounds) : geometry::right(clientBounds) - width(), geometry::top(paintBounds),
						right - left, geometry::dy(paintBounds), memoryDC_.get(), 0, geometry::top(paintBounds), SRCCOPY);
#endif

				context.restore();
			}

			void RulerPainter::scroll(const graphics::font::VisualLine& from) {
			}

			void RulerPainter::setStyles(std::shared_ptr<const viewers::RulerStyles> styles) {
				if(styles.get() != nullptr) {
					if(!styles->alignment.inherits()) {
						const presentation::TextAlignment alignment = styles->alignment.get();
						if(alignment != presentation::TextAlignment::START
								&& alignment != presentation::TextAlignment::END
								&& alignment != presentation::TextAlignment::LEFT
								&& alignment != presentation::TextAlignment::RIGHT)
							throw UnknownValueException("styles->alignment");
					}
				} else
					declaredStyles_.reset(new viewers::RulerStyles);
				update();
			}

			void RulerPainter::update() BOOST_NOEXCEPT {
				// TODO: Is this method need?
				computedLineNumberDigits_ = boost::value_initialized<std::uint8_t>();
				computeAllocationWidth();
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && 0
				updateGDIObjects();
				if(enablesDoubleBuffering_ && memoryBitmap_.get() != nullptr)
					memoryBitmap_.reset();
#endif
			}

#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && 0
			///
			void RulerPainter::updateGDIObjects() BOOST_NOEXCEPT {
				indicatorMarginPen_.reset();
				indicatorMarginBrush_.reset();
				if(configuration_.indicatorMargin.visible) {
					indicatorMarginPen_.reset(::CreatePen(PS_SOLID, 1,
						systemColors.serve(configuration_.indicatorMargin.borderColor, COLOR_3DSHADOW)), &::DeleteObject);
					indicatorMarginBrush_.reset(::CreateSolidBrush(
						systemColors.serve(configuration_.indicatorMargin.color, COLOR_3DFACE)), &::DeleteObject);
				}

				lineNumbersPen_.reset();
				lineNumbersBrush_.reset();
				if(configuration_.lineNumbers.visible) {
					if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::SOLID)	// 実線
						lineNumbersPen_.reset(::CreatePen(PS_SOLID, configuration_.lineNumbers.borderWidth,
							systemColors.serve(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT)), &::DeleteObject);
					else if(configuration_.lineNumbers.borderStyle != RulerConfiguration::LineNumbers::NONE) {
						LOGBRUSH brush;
						brush.lbColor = systemColors.serve(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT);
						brush.lbStyle = BS_SOLID;
						if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::DASHED)	// 破線
							lineNumbersPen_.reset(::ExtCreatePen(
								PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, configuration_.lineNumbers.borderWidth, &brush, 0, 0), &::DeleteObject);
						else if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::DASHED_ROUNDED)	// 丸破線
							lineNumbersPen_.reset(::ExtCreatePen(
								PS_GEOMETRIC | PS_DASH | PS_ENDCAP_ROUND, configuration_.lineNumbers.borderWidth, &brush, 0, 0), &::DeleteObject);
						else if(configuration_.lineNumbers.borderStyle == RulerConfiguration::LineNumbers::DOTTED)	// 点線
							lineNumbersPen_.reset(::ExtCreatePen(
								PS_GEOMETRIC | PS_DOT, configuration_.lineNumbers.borderWidth, &brush, 0, 0), &::DeleteObject);
					}
					lineNumbersBrush_.reset(::CreateSolidBrush(
						systemColors.serve(configuration_.lineNumbers.textColor.background, COLOR_WINDOW)), &::DeleteObject);
				}
			}
#endif
		}
	}
}
