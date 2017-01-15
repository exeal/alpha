/**
 * @file text-renderer.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2014
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/corelib/numeric-range-algorithm/overlaps.hpp>
#include <ascension/graphics/font/baseline-iterator.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-collection.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/line-rendering-options.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/graphics/paint.hpp>
#include <ascension/graphics/paint-context.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/win32/system-default-font.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/foreach.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#ifdef _DEBUG
#	include <ascension/log.hpp>
#	include <boost/geometry/io/dsv/write.hpp>
#endif

namespace ascension {
	namespace graphics {
		namespace font {
			// FontSelector ///////////////////////////////////////////////////////////////////////////////////////////
#if 0
			namespace {
				AutoBuffer<WCHAR> ASCENSION_FASTCALL mapFontFileNameToTypeface(const WCHAR* fileName) {
					assert(fileName != nullptr);
					static const WCHAR KEY_NAME[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
					HKEY key;
					long e = ::RegOpenKeyExW(HKEY_CURRENT_USER, KEY_NAME, 0, KEY_QUERY_VALUE, &key);
					if(e != ERROR_SUCCESS)
						e = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_NAME, 0, KEY_QUERY_VALUE, &key);
					if(e == ERROR_SUCCESS) {
						const std::size_t fileNameLength = wcslen(fileName);
						DWORD maximumValueNameLength, maximumValueBytes;
						e = ::RegQueryInfoKeyW(key, nullptr, nullptr, nullptr,
							nullptr, nullptr, nullptr, nullptr, &maximumValueNameLength, &maximumValueBytes, nullptr, nullptr);
						if(e == ERROR_SUCCESS && (maximumValueBytes / sizeof(WCHAR)) - 1 >= fileNameLength) {
							const std::size_t fileNameLength = wcslen(fileName);
							AutoBuffer<WCHAR> valueName(new WCHAR[maximumValueNameLength + 1]);
							AutoBuffer<BYTE> value(new BYTE[maximumValueBytes]);
							DWORD valueNameLength = maximumValueNameLength + 1, valueBytes = maximumValueBytes, type;
							for(DWORD index = 0; ; ++index, valueNameLength = maximumValueNameLength + 1, valueBytes = maximumValueBytes) {
								e = ::RegEnumValueW(key, index, valueName.get(), &valueNameLength, 0, &type, value.get(), &valueBytes);
								if(e == ERROR_SUCCESS) {
									if(type == REG_SZ && (valueBytes / sizeof(WCHAR)) - 1 == fileNameLength
											&& wmemcmp(fileName, reinterpret_cast<WCHAR*>(value.get()), fileNameLength) == 0) {
										::RegCloseKey(key);
										std::size_t nameLength = valueNameLength;
										if(valueName[nameLength - 1] == L')') {
											if(const WCHAR* const opening = wcsrchr(valueName.get(), L'(')) {
												nameLength = opening - valueName.get();
												if(nameLength > 1 && valueName[nameLength - 1] == L' ')
													--nameLength;
											}
										}
										if(nameLength > 0) {
											AutoBuffer<WCHAR> temp(new WCHAR[nameLength + 1]);
											wmemcpy(temp.get(), valueName.get(), nameLength);
											temp[nameLength] = 0;
											return temp;
										} else
											return AutoBuffer<WCHAR>();
									}
								} else	// ERROR_NO_MORE_ITEMS
									break;
							}
						}
						::RegCloseKey(key);
					}
					return AutoBuffer<WCHAR>();
				}
			} // namespace @0

			void FontSelector::linkPrimaryFont() BOOST_NOEXCEPT {
				// TODO: this does not support nested font linking.
				assert(linkedFonts_ != nullptr);
				BOOST_FOREACH(Fontset*& fontset, linkedFonts_)
					delete fontset;
				linkedFonts_->clear();

				// read font link settings from registry
				static const WCHAR KEY_NAME[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink";
				HKEY key;
				if(ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_CURRENT_USER, KEY_NAME, 0, KEY_QUERY_VALUE, &key)
						|| ERROR_SUCCESS == ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_NAME, 0, KEY_QUERY_VALUE, &key)) {
					DWORD type, bytes;
					if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, 0, &bytes)) {
						AutoBuffer<BYTE> data(new BYTE[bytes]);
						if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, data.get(), &bytes)) {
							const WCHAR* sz = reinterpret_cast<WCHAR*>(data.get());
							const WCHAR* const e = sz + bytes / sizeof(WCHAR);
							for(; sz < e; sz += wcslen(sz) + 1) {
								const WCHAR* comma = wcschr(sz, L',');
								if(comma != nullptr && comma[1] != 0)	// "<file-name>,<typeface>"
									linkedFonts_->push_back(new Fontset(comma + 1));
								else {	// "<file-name>"
									AutoBuffer<WCHAR> typeface(mapFontFileNameToTypeface(sz));
									if(typeface.get() != nullptr)
										linkedFonts_->push_back(new Fontset(typeface.get()));
								}
							}
						}
					}
					::RegCloseKey(key);
				}
				fireFontChanged();
			}
#endif


			// TextRenderer ///////////////////////////////////////////////////////////////////////////////////////////

#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
			namespace {
				inline int calculateMemoryBitmapSize(int src) BOOST_NOEXCEPT {
					const int UNIT = 32;
					return (src % UNIT != 0) ? src + UNIT - src % UNIT : src;
				}
			} // namespace @0
#endif // ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08

			/**
			 * @class ascension::graphics::font::TextRenderer
			 * @c TextRenderer renders styled text to the display or to a printer. Although this class extends
			 * @c LineLayoutBuffer class and implements @c ILayoutInformationProvider interface,
			 * @c LineLayoutBuffer#deviceContext, @c ILayoutInformationProvider#layoutSettings, and
			 * @c ILayoutInformationProvider#width methods are not defined (An internal extension
			 * @c TextViewer#Renderer class implements these).
			 *
			 * <h3>The Default Font</h3>
			 *
			 * A @c TextRenderer holds an "Actual Value" of the font used to compute default text metrics. This font is
			 * known as "Default Font" and be constructed based on the value of @c Presentation#computedTextRunStyle
			 * method. You can get the default font by calling @c TextRenderer#defaultFont method. See also
			 * @c TextRenderer#defaultFontChangedSignal method.
			 *
			 * @see TextLayout, LineLayoutBuffer, Presentation
			 */

			/**
			 * Constructor.
			 * @param document The document
			 * @param renderingContextProvider The @c RenderingContextProvider strategy object
			 * @param styleProvider The @c StyleProvider strategy object
			 * @param initialSize
			 */
			TextRenderer::TextRenderer(kernel::Document& document, const Dimension& initialSize) /*: spacePainter_(new SpacePainter)*/ {
//				textWrapping_.measure = 0;
				layouts_.reset(new LineLayoutVector(document,
					std::bind(&TextRenderer::generateLineLayout, this, std::placeholders::_1), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
//				viewport_ = detail::createTextViewport(*this);
	//			updateDefaultFont();
	//			assert(defaultFont_.get() != nullptr);
//				presentation::FlowRelativeFourSides<presentation::styles::Length> zeroSpaces;
//				zeroSpaces.fill(presentation::styles::Length(0));
//				spacePainter_->update(*this, initialSize, zeroSpaces);
/*				switch(PRIMARYLANGID(getUserDefaultUILanguage())) {
					case LANG_CHINESE:
					case LANG_JAPANESE:
					case LANG_KOREAN:
						enableFontLinking();
						break;
				}*/
//				updateViewerSize(); ???
			}
#if 0
			/// Copy-constructor.
			TextRenderer::TextRenderer(const TextRenderer& other) : layouts_(), defaultFont_(other.defaultFont_) {
				layouts_.reset(new LineLayoutVector(const_cast<kernel::Document&>(other.layouts_->document()),
					std::bind(&TextRenderer::generateLineLayout, this, std::placeholders::_1), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
//				viewport_ = detail::createTextViewport(*this);
			//	updateViewerSize(); ???
			}
#endif
			/// Destructor.
			TextRenderer::~TextRenderer() BOOST_NOEXCEPT {
//				getTextViewer().removeDisplaySizeListener(*this);
//				layouts_.removeVisualLinesListener(*this);
			}

			/**
			 * @fn ascension::graphics::font::TextRenderer::actualLineBackgroundColor
			 * Returns the "Actual Value" of background color of the specified line.
			 * @param layout The line layout
			 */

			/**
			 * Returns the distance from the baseline of the line @a from to the baseline of the line @a to in
			 * block progression direction in user units.
			 * @param lines The first and second lines
			 * @return The distance between the two baselines in user units
			 * @note This method calls LineLayoutVector#at(Index, const UseCalculatedLayoutTag&amp;) which may change
			 *       the layout
			 */
			Scalar TextRenderer::baselineDistance(const boost::integer_range<VisualLine>& lines) const {
				// TODO: This code does not consider 'line-stacking-strategy'.
				TextRenderer& self = const_cast<TextRenderer&>(*this);
				if(boost::empty(lines)) {
					if(boost::const_begin(lines)->subline == boost::const_end(lines)->subline)
						return 0;
					const TextLayout& layout = self.layouts().at(boost::const_begin(lines)->line, LineLayoutVector::USE_CALCULATED_LAYOUT);
					return TextLayout::LineMetricsIterator(layout, boost::const_end(lines)->subline).baselineOffset()
						- TextLayout::LineMetricsIterator(layout, boost::const_begin(lines)->subline).baselineOffset();
				} else {
					const TextLayout* layout = &self.layouts().at(boost::const_begin(lines)->line, LineLayoutVector::USE_CALCULATED_LAYOUT);
					Scalar bpd = *boost::const_end(layout->extent()) - TextLayout::LineMetricsIterator(*layout, boost::const_begin(lines)->subline).baselineOffset();
					for(Index line = boost::const_begin(lines)->line + 1; line < boost::const_end(lines)->line; ++line) {
//						bpd += layouts().at(line).height();
						const NumericRange<Scalar> lineExtent(self.layouts().at(line, LineLayoutVector::USE_CALCULATED_LAYOUT).extent() | adaptors::ordered());
						bpd += *boost::const_end(lineExtent) - *boost::const_begin(lineExtent);
					}
					layout = &self.layouts().at(boost::const_end(lines)->line, LineLayoutVector::USE_CALCULATED_LAYOUT);
					return bpd += TextLayout::LineMetricsIterator(*layout, boost::const_end(lines)->subline).baselineOffset() - layout->extent().front();
				}
			}

			/**
			 * @fn ascension::graphics::font::TextRenderer::computedBlockFlowDirection
			 * Returns the computed block-flow-direction.
			 */

			/// Returns the @c DefaultFontChangedSignal signal connector.
			SignalConnector<TextRenderer::DefaultFontChangedSignal> TextRenderer::defaultFontChangedSignal() BOOST_NOEXCEPT {
				return makeSignalConnector(defaultFontChangedSignal_);
			}

			/**
			 * @fn ascension::graphics::font::TextRenderer::createLineLayout
			 * Creates and returns the text layout for the specified line.
			 * @param line The line number
			 * @return The generated line layout
			 * @see #buildLineLayoutConstructionParameters
			 */

			/// @internal Calls @c #createLineLayout overridable method.
			std::unique_ptr<const TextLayout> TextRenderer::generateLineLayout(Index line) const {
				return createLineLayout(line);
			}

			/// Returns the line relative alignment.
			TextRenderer::LineRelativeAlignmentAxis TextRenderer::lineRelativeAlignment() const BOOST_NOEXCEPT {
				const auto anchor = textAnchor();
				switch(boost::native_value(anchor)) {
					case TextAnchor::START:
					case TextAnchor::END:
					default: {
						LineRelativeAlignmentAxis start, end;
						if(presentation::isHorizontal(blockFlowDirection())) {
							std::tie(start, end) = std::make_pair(LEFT, RIGHT);
							if(inlineFlowDirection() == presentation::RIGHT_TO_LEFT)
								std::swap(start, end);
						} else {
							assert(presentation::isVertical(blockFlowDirection()));
							std::tie(start, end) = std::make_pair(TOP, BOTTOM);
							if(inlineFlowDirection() == presentation::RIGHT_TO_LEFT)
								std::swap(start, end);
							if(presentation::resolveTextOrientation(writingModes()) == presentation::SIDEWAYS_LEFT)
								std::swap(start, end);
						}
						return (anchor != TextAnchor::END) ? start : end;
					}
					case TextAnchor::MIDDLE:
						return presentation::isHorizontal(blockFlowDirection()) ? HORIZONTAL_CENTER : VERTICAL_CENTER;
				}
			}

			/**
			 * Returns the start-edge of the specified visual line in user units.
			 * @param line The line number
			 * @return The start-edge, which is distance from the start-edge of content-area to the one of the line
			 * @throw IndexOutOfBoundsException @a line is invalid
			 * @see TextLayout#lineStartEdge
			 */
			Scalar TextRenderer::lineStartEdge(const VisualLine& line) const {
				const TextLayout* const layout = layouts().at(line.line);	// this may throw IndexOutOfBoundsException
				const TextAnchor anchor = (layout != nullptr) ? layout->anchor(0) : textAnchor();
				Scalar d;
				switch(boost::native_value(anchor)) {
					case TextAnchor::START:
						d = 0;
						break;
					case TextAnchor::MIDDLE:
						d = (layouts().maximumMeasure() - ((layout != nullptr) ? layout->measure() : 0)) / 2;
						break;
					case TextAnchor::END:
						d = layouts().maximumMeasure() - ((layout != nullptr) ? layout->measure() : 0);
						break;
				}

				if(layout != nullptr)
					return d += layout->lineStartEdge(line.subline);	// this may throw IndexOutOfBoundsException
				else if(line.subline > 0)
					throw IndexOutOfBoundsException("line.subline");
				else
					return d;
			}

			/**
			 * @fn ascension::graphics::font::TextRenderer::newDefaultFont
			 * Returns the default font for text rendering.
			 * @see #defaultFont, #DefaultFontChangedSignal
			 */

			/**
			 * Paints the specified output device with text layout.
			 * @param context The graphics context
			 * @param viewport Text viewport
			 * @param options The optional @c LineRenderingOptions
			 */
			void TextRenderer::paint(PaintContext& context, const TextViewport& viewport, const LineRenderingOptions* options /* = nullptr */) const {
				// scan lines in the text area
				const bool horizontal = presentation::isHorizontal(blockFlowDirection());
				const auto bpdRange(horizontal ? geometry::range<1>(context.boundsToPaint()) : geometry::range<0>(context.boundsToPaint()));
				struct LineToPaint {
					Index lineNumber;
					graphics::Scalar baseline;
					NumericRange<graphics::Scalar> extent;
				};
				std::deque<LineToPaint> linesToPaint;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				static_assert(false, "Not implemented.");
#else
				BaselineIterator baseline(viewport, true);
				const BaselineIterator lastBaseline;
				for(Index logicalLine; baseline != lastBaseline; ++baseline) {
					assert(baseline.line() != boost::none);
					logicalLine = boost::get(baseline.line()).line;
					const TextLayout& layout = const_cast<LineLayoutVector&>(layouts()).at(logicalLine, LineLayoutVector::USE_CALCULATED_LAYOUT);
					if(overlaps(baseline.extentWithHalfLeadings(), bpdRange)) {	// this logical line should be painted
						LineToPaint lineToPaint;
						lineToPaint.lineNumber = logicalLine;
						if(boost::get(baseline.line()).subline == 0)
							lineToPaint.baseline = *baseline;
						else {
							BaselineIterator i(baseline);
							while(boost::get(i.line()).subline > 0)
								--i;
							lineToPaint.baseline = *i;
						}
						lineToPaint.extent = layout.extentWithHalfLeadings();
						lineToPaint.extent.advance_begin(lineToPaint.baseline);
						lineToPaint.extent.advance_end(lineToPaint.baseline);
						linesToPaint.push_back(lineToPaint);
						if(boost::get(baseline.line()).subline < layout.numberOfLines() - 1)
							std::advance(baseline, layout.numberOfLines() - 1 - boost::get(baseline.line()).subline);	// skip to the next logical line
					}
				}

				// paint marked logical lines
				BOOST_FOREACH(const LineToPaint& lineToPaint, linesToPaint) {
					const TextLayout& layout = const_cast<LineLayoutVector&>(layouts()).at(lineToPaint.lineNumber, LineLayoutVector::USE_CALCULATED_LAYOUT);
					// calculate the background area
					const presentation::FlowRelativeFourSides<Scalar> abstractLineArea(
						presentation::_blockStart = *boost::const_begin(lineToPaint.extent), presentation::_blockEnd = *boost::const_end(lineToPaint.extent),
						presentation::_inlineStart = std::numeric_limits<Scalar>::lowest(), presentation::_inlineEnd = std::numeric_limits<Scalar>::max());
					Rectangle physicalLineArea;
					{
						PhysicalFourSides<Scalar> temp;
						presentation::mapDimensions(writingMode(layout), presentation::_from = abstractLineArea, presentation::_to = temp);
						boost::geometry::assign(physicalLineArea, geometry::make<Rectangle>(temp));
					}
					boost::geometry::intersection(physicalLineArea, context.boundsToPaint(), physicalLineArea);

					// paint the background
					const SolidColor background(actualLineBackgroundColor(layout));
					context.setFillStyle(std::shared_ptr<const Paint>(&background, boost::null_deleter()));
					context.fillRectangle(physicalLineArea);

					// paint the text content
					graphics::PhysicalTwoAxes<graphics::Scalar> p;
					presentation::mapDimensions(
						writingMode(layout),
						presentation::_from = presentation::makeFlowRelativeTwoAxes((
							presentation::_bpd = lineToPaint.baseline,
							presentation::_ipd = -inlineProgressionOffsetInViewerGeometry(viewport))),
						presentation::_to = p);
					paint(layout, lineToPaint.lineNumber, context, graphics::geometry::make<graphics::Point>(p), options);
				}
#	ifdef _DEBUG
				if(!boost::empty(linesToPaint)) {
					static unsigned long n;
					ASCENSION_LOG_TRIVIAL(debug)
						<< "Repainted lines [" << linesToPaint.front().lineNumber << "," << linesToPaint.back().lineNumber << "]"
						<< " for area " << boost::geometry::dsv(context.boundsToPaint())
						<< " [#" << (n++) << "]" << std::endl;
				}
#	endif
#endif
			}

			/**
			 * @internal Paints the specified output device with the text layout of the specified line.
			 * @param layout The line layout
			 * @param line The line number
			 * @param context The graphics context
			 * @param alignmentPoint The alignment point of the text layout of the line to draw
			 * @param options The optional @c LineRenderingOptions
			 */
			inline void TextRenderer::paint(const TextLayout& layout, Index line,
					PaintContext& context, const Point& alignmentPoint, const LineRenderingOptions* options) const {
//				if(!enablesDoubleBuffering_) {
					std::vector<OverriddenSegment> overriddenSegments;
					if(options != nullptr)
						options->overrideTextPaint(line, overriddenSegments);
					layout.draw(context, alignmentPoint, overriddenSegments,
						(options != nullptr) ? options->endOfLine(line).get() : nullptr,
						(options != nullptr) ? options->textWrappingMark(line).get() : nullptr);
//				}
			}

			/**
			 * Paints the specified output device with text layout of the specified line.
			 * @param line The line number
			 * @param context The graphics context
			 * @param alignmentPoint The alignment point of the text layout of the line to draw
			 * @param options The optional @c LineRenderingOptions
			 * @note This method calls LineLayoutVector#at(Index, const UseCalculatedLayoutTag&amp;) which may change
			 *       the layout
			 */
			void TextRenderer::paint(Index line, PaintContext& context,
					const Point& alignmentPoint, const LineRenderingOptions* options /* = nullptr */) const {
				return paint(const_cast<TextRenderer*>(this)->layouts().at(line, LineLayoutVector::USE_CALCULATED_LAYOUT), line, context, alignmentPoint, options);

#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
				// TODO: this code uses deprecated terminologies for text coordinates.

				const TextLayout& layout = layouts().at(line);
				const Scalar dy = defaultFont()->metrics().linePitch();

				// skip to the subline needs to draw
				const Scalar top = max(geometry::top(context.boundsToPaint()), geometry::top(clipRect));
				Scalar y = geometry::y(origin);
				Index subline = (y + dy >= top) ? 0 : (top - (y + dy)) / dy;
				if(subline >= layout.numberOfLines())
					return;	// this logical line does not need to draw
				y += static_cast<Scalar>(dy * subline);

				if(memoryDC_.get() == nullptr)		
					memoryDC_.reset(::CreateCompatibleDC(context.nativeObject().get()), &::DeleteDC);
				const int horizontalResolution = calculateMemoryBitmapSize(geometry::dx(context.device()->viewportSize()));
				if(memoryBitmap_.get() != nullptr) {
					BITMAP temp;
					::GetObjectW(memoryBitmap_.get(), sizeof(HBITMAP), &temp);
					if(temp.bmWidth < horizontalResolution)
						memoryBitmap_.reset();
				}
				if(memoryBitmap_.get() == nullptr)
					memoryBitmap_.reset(::CreateCompatibleBitmap(
						context.nativeObject().get(),
						horizontalResolution, calculateMemoryBitmapSize(dy)), &::DeleteObject);
				::SelectObject(memoryDC_.get(), memoryBitmap_.get());

				const int left = max(geometry::left(context.boundsToPaint()), geometry::left(clipRect));
				const int right = min(geometry::right(context.boundsToPaint()), geometry::right(clipRect));
				const Scalar x = geometry::x(origin) - left;
				NativeRectangle offsetedPaintRect, offsetedClipRect;
				geometry::translate((
					geometry::_from = paintRect, geometry::_to = offsetedPaintRect,
					geometry::_dx = -left, geometry::_dy = -y));
				geometry::translate((
					geometry::_from = clipRect, geometry::_to = offsetedClipRect,
					geometry::_dx = -left, geometry::_dy = -y));
				for(; subline < layout.numberOfLines() && geometry::bottom(offsetedPaintRect) >= 0; ++subline, y += dy,
						geometry::translate(offsetedPaintRect, geometry::make<NativeSize>(0, -dy)),
						geometry::translate(offsetedClipRect, geometry::make<NativeSize>(0, -dy))) {
					layout.draw(subline, memoryDC_, geometry::make<NativePoint>(x, 0), offsetedPaintRect, offsetedClipRect, selection);
					::BitBlt(context.nativeObject().get(), left, y, right - left, dy, memoryDC_.get(), 0, 0, SRCCOPY);
				}
#endif
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Sets the text wrapping settings.
			 * @param newValue The new settings
			 * @param renderingContext The rendering context used to calculate the logical value of
			 *                         @a newValue.measure. This can be @c null if @a newValue.measure has absolute
			 *                         value
			 * @see #setTextWrapping, #textWrappingMeasureInPixels
			 */
			void TextRenderer::setTextWrapping(const TextWrapping<presentation::Length>& newValue, const RenderingContext2D* renderingContext) {
				const Scalar newTextWrappingMeasureInPixels = static_cast<Scalar>(newValue.measure.value(renderingContext, 0));
				const bool resetLayouts = textWrapping_.textWrap != newValue.textWrap
					|| textWrapping_.overflowWrap != newValue.overflowWrap || textWrappingMeasureInPixels_ != newTextWrappingMeasureInPixels;
				textWrapping_ = newValue;
				textWrappingMeasureInPixels_ = newTextWrappingMeasureInPixels;
				if(resetLayouts)
					layouts().invalidate();
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/// Returns the computed space widths.
			const PhysicalFourSides<Scalar>& TextRenderer::spaceWidths() const BOOST_NOEXCEPT {
				// TODO: Not implemented.
				static boost::optional<PhysicalFourSides<Scalar>> dummy;
				if(!dummy) {
					dummy = PhysicalFourSides<Scalar>();
					BOOST_FOREACH(auto& side, *dummy)
						side = static_cast<Scalar>(0);
				}
				return *dummy;
			}

			/// @internal Updates the @c defaultFont_.
			inline void TextRenderer::updateDefaultFont() {
				auto newFont(newDefaultFont());
				if(newFont.get() == nullptr) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					// TODO: Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					// TODO: Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
					// TODO: Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					LOGFONTW lf;
					win32::systemDefaultFont(lf);
					win32::Handle<HFONT> native(::CreateFontIndirectW(&lf), &::DeleteObject);
					newFont = std::make_shared<const graphics::font::Font>(native);
#endif
				}
				assert(newFont.get() != nullptr);
				std::swap(defaultFont_, newFont);
				layouts().invalidate();
#if 0
				if(/*enablesDoubleBuffering_ &&*/ memoryBitmap_.get() != nullptr) {
					BITMAP temp;
					::GetObjectW(memoryBitmap_.get(), sizeof(HBITMAP), &temp);
					if(temp.bmHeight != calculateMemoryBitmapSize(defaultFont()->metrics().linePitch()))
						memoryBitmap_.reset();
				}
#endif
				defaultFontChangedSignal_(*this);
			}

			/**
			 * @fn ascension::graphics::font::TextRenderer::computedBlockFlowDirection
			 * Returns the computed block-flow-direction.
			 */

			/// Returns the @c WritingModesChangedSignal signal connector.
			SignalConnector<TextRenderer::WritingModesChangedSignal> TextRenderer::writingModesChangedSignal() BOOST_NOEXCEPT {
				return makeSignalConnector(writingModesChangedSignal_);
			}
		}
	}
}
