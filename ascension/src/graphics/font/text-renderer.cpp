/**
 * @file text-renderer.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2014
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/font/text-renderer.hpp>

namespace ascension {
//#define TRACE_LAYOUT_CACHES
	extern bool DIAGNOSE_INHERENT_DRAWING;

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
				for(vector<Fontset*>::iterator i(linkedFonts_->begin()), e(linkedFonts_->end()); i != e; ++i)
					delete *i;
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

			namespace {
				inline int calculateMemoryBitmapSize(int src) BOOST_NOEXCEPT {
					const int UNIT = 32;
					return (src % UNIT != 0) ? src + UNIT - src % UNIT : src;
				}
			} // namespace @0

			/**
			 * @class ascension::graphics::font::TextRenderer
			 * @c TextRenderer renders styled text to the display or to a printer. Although this class extends
			 * @c LineLayoutBuffer class and implements @c ILayoutInformationProvider interface,
			 * @c LineLayoutBuffer#deviceContext, @c ILayoutInformationProvider#layoutSettings, and
			 * @c ILayoutInformationProvider#width methods are not defined (An internal extension
			 * @c TextViewer#Renderer class implements these).
			 * @see TextLayout, LineLayoutBuffer, Presentation
			 */

			/**
			 * Constructor.
			 * @param presentation The presentation
			 * @param fontCollection The font collection provides fonts this renderer uses
			 * @param enableDoubleBuffering Set @c true to use double-buffering for non-flicker drawing
			 */
			TextRenderer::TextRenderer(presentation::Presentation& presentation,
					const FontCollection& fontCollection, const Dimension& initialSize)
					: presentation_(presentation), fontCollection_(fontCollection)/*, spacePainter_(new SpacePainter)*/ {
			//	textWrapping_.measure = 0;
				layouts_.reset(new LineLayoutVector(presentation.document(),
					std::bind(&TextRenderer::generateLineLayout, this, std::placeholders::_1), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
				updateComputedBlockFlowDirectionChanged();	// this initializes 'computedBlockFlowDirection_'
#if defined(BOOST_OS_WINDOWS)
				LOGFONTW lf;
				if(::GetObjectW(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)), sizeof(LOGFONTW), &lf) == 0)
					throw makePlatformError();
				defaultFont_ = std::make_shared<Font>(win32::Handle<HFONT>::Type(::CreateFontIndirectW(&lf), &::DeleteObject));
#else
				// TODO: Not implemented.
#endif
				assert(defaultFont_.get() != nullptr);
				presentation::FlowRelativeFourSides<presentation::Length> zeroSpaces;
				zeroSpaces.fill(presentation::Length(0));
//				spacePainter_->update(*this, initialSize, zeroSpaces);
/*				switch(PRIMARYLANGID(getUserDefaultUILanguage())) {
					case LANG_CHINESE:
					case LANG_JAPANESE:
					case LANG_KOREAN:
						enableFontLinking();
						break;
				}*/
//				updateViewerSize(); ???
				presentation_.addTextToplevelStyleListener(*this);
			}

			/// Copy-constructor.
			TextRenderer::TextRenderer(const TextRenderer& other) :
					presentation_(other.presentation_), layouts_(), fontCollection_(other.fontCollection_), defaultFont_(other.defaultFont_) {
				layouts_.reset(new LineLayoutVector(other.presentation_.document(),
					std::bind(&TextRenderer::generateLineLayout, this, std::placeholders::_1), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
				updateComputedBlockFlowDirectionChanged();	// this initializes 'computedBlockFlowDirection_'
			//	updateViewerSize(); ???
				presentation_.addTextToplevelStyleListener(*this);
			}

			/// Destructor.
			TextRenderer::~TextRenderer() BOOST_NOEXCEPT {
				presentation_.removeTextToplevelStyleListener(*this);
//				getTextViewer().removeDisplaySizeListener(*this);
//				layouts_.removeVisualLinesListener(*this);
			}

			/**
			 * Registers the computed block-flow-direction listener.
			 * @param listener The listener to be registered
			 * @throw std#invalid_argument @a listener is already registered
			 * @see #computedBlockFlowDirection
			 */
			void TextRenderer::addComputedBlockFlowDirectionListener(ComputedBlockFlowDirectionListener& listener) {
				computedBlockFlowDirectionListeners_.add(listener);
			}

			/**
			 * Registers the default font selector listener.
			 * @param listener The listener to be registered
			 * @throw std#invalid_argument @a listener is already registered
			 * @see #defaultFont
			 */
			void TextRenderer::addDefaultFontListener(DefaultFontListener& listener) {
				defaultFontListeners_.add(listener);
			}

			/**
			 * Returns the distance from the baseline of the line @a from to the baseline of the line @a to in
			 * block progression direction in user units.
			 * @param lines The first and second lines
			 * @return The distance between the two baselines in user units
			 * @note This method calls LineLayoutVector#at(Index, const LineLayoutVector#UseCalculatedLayoutTag&)
			 *       which may change the layout
			 */
			Scalar TextRenderer::baselineDistance(const boost::integer_range<VisualLine>& lines) const {
				// TODO: This code does not consider 'line-stacking-strategy'.
				TextRenderer& self = const_cast<TextRenderer&>(*this);
				if(lines.empty()) {
					if(lines.begin()->subline == lines.end()->subline)
						return 0;
					const TextLayout& layout = self.layouts().at(lines.begin()->line, LineLayoutVector::USE_CALCULATED_LAYOUT);
					return layout.lineMetrics(lines.end()->subline).baselineOffset() - layout.lineMetrics(lines.begin()->subline).baselineOffset();
				} else {
					const TextLayout* layout = &self.layouts().at(lines.begin()->line, LineLayoutVector::USE_CALCULATED_LAYOUT);
					Scalar bpd = *layout->extent().end() - layout->lineMetrics(lines.begin()->subline).baselineOffset();
					for(Index line = lines.begin()->line + 1; line < lines.end()->line; ++line) {
//						bpd += layouts().at(line).height();
						const boost::integer_range<Scalar> lineExtent(ordered(self.layouts().at(line, LineLayoutVector::USE_CALCULATED_LAYOUT).extent()));
						bpd += *lineExtent.end() - *lineExtent.begin();
					}
					layout = &self.layouts().at(lines.end()->line, LineLayoutVector::USE_CALCULATED_LAYOUT);
					return bpd += layout->lineMetrics(lines.end()->subline).baselineOffset() - layout->extent().front();
				}
			}

			/**
			 * Builds construction parameters for @c TextLayout object.
			 * @param[in] line The line number
			 * @param[out] lineStyle
			 * @param[out] runStyles
			 * @param[out] fontCollection
			 * @see #createLineLayout
			 */
			void TextRenderer::buildLineLayoutConstructionParameters(Index line, ComputedTextLineStyle& lineStyle,
					std::unique_ptr<ComputedStyledTextRunIterator>& runStyles, FontCollection& fontCollection) const {
//				presentation().computeTextLineStyle(line, );
//				presentation().computeTextRunStyles(line, );
			}

			/**
			 * @fn ascension::graphics::font::TextRenderer::createLineLayout
			 * Creates and returns the text layout for the specified line.
			 * @param line The line number
			 * @return The generated line layout
			 * @see #buildLineLayoutConstructionParameters
			 */

			/**
			 * Paints the specified output device with text layout. The line rendering options provided by
			 * @c #setLineRenderingOptions method is considered.
			 * @param context The graphics context
			 */
			void TextRenderer::paint(PaintContext& context) const {
			}

			/**
			 * Paints the specified output device with text layout of the specified line. The line rendering
			 * options provided by @c #setLineRenderingOptions method is considered.
			 * @param line The line number
			 * @param context The graphics context
			 * @param alignmentPoint The alignment point of the text layout of the line to draw
			 * @note This method calls LineLayoutVector#at(Index, const LineLayoutVector#UseCalculatedLayoutTag&)
			 *       which may change the layout
			 */
			void TextRenderer::paint(Index line, PaintContext& context, const Point& alignmentPoint) const BOOST_NOEXCEPT {
//				if(!enablesDoubleBuffering_) {
					const_cast<TextRenderer*>(this)->layouts().at(line, LineLayoutVector::USE_CALCULATED_LAYOUT).draw(context, alignmentPoint,
						(lineRenderingOptions_.get() != nullptr) ? lineRenderingOptions_->textPaintOverride(line) : nullptr,
						(lineRenderingOptions_.get() != nullptr) ? lineRenderingOptions_->endOfLine(line) : nullptr,
						(lineRenderingOptions_.get() != nullptr) ? lineRenderingOptions_->textWrappingMark(line) : nullptr);
					return;
//				}

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
				NativeRectangle offsetedPaintRect(paintRect), offsetedClipRect(clipRect);
				geometry::translate(offsetedPaintRect, geometry::make<NativeSize>(-left, -y));
				geometry::translate(offsetedClipRect, geometry::make<NativeSize>(-left, -y));
				for(; subline < layout.numberOfLines() && geometry::bottom(offsetedPaintRect) >= 0; ++subline, y += dy,
						geometry::translate(offsetedPaintRect, geometry::make<NativeSize>(0, -dy)),
						geometry::translate(offsetedClipRect, geometry::make<NativeSize>(0, -dy))) {
					layout.draw(subline, memoryDC_, geometry::make<NativePoint>(x, 0), offsetedPaintRect, offsetedClipRect, selection);
					::BitBlt(context.nativeObject().get(), left, y, right - left, dy, memoryDC_.get(), 0, 0, SRCCOPY);
				}
#endif
			}

			/**
			 * Removes the computed block-flow-direction listener.
			 * @param listener The listener to be removed
			 * @throw std#invalid_argument @a listener is not registered
			 * @see #computedBlockFlowDirection
			 */
			void TextRenderer::removeComputedBlockFlowDirectionListener(ComputedBlockFlowDirectionListener& listener) {
				computedBlockFlowDirectionListeners_.remove(listener);
			}

			/**
			 * Removes the default font selector listener.
			 * @param listener The listener to be removed
			 * @throw std#invalid_argument @a listener is not registered
			 * @see #defaultFont
			 */
			void TextRenderer::removeDefaultFontListener(DefaultFontListener& listener) {
				defaultFontListeners_.remove(listener);
			}

			/**
			 * Sets the default UI writing mode. This method invalidates the all layouts and call listeners'
			 * @c ComputedWritingModeListener#computedWritingModeChanged.
			 * @param writingMode The new value to set
			 */
			void TextRenderer::setWritingMode(decltype(presentation::TextToplevelStyle().writingMode) writingMode) {
				if(writingMode != this->writingMode()) {
					writingMode_ = writingMode;
					layouts().invalidate();
					updateComputedBlockFlowDirectionChanged();
				}
			}

			/**
			 * Sets the new default font.
			 * @param familyName The family name of the font
			 * @param pointSize The size of the font in points
			 */
			void TextRenderer::setDefaultFont(const String& familyName, double pointSize) {
				std::shared_ptr<const Font> newFont(fontCollection_.get(FontDescription(FontFamily(familyName), pointSize)));
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
				defaultFontListeners_.notify(&DefaultFontListener::defaultFontChanged);
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

			/// @see TextToplevelStyleListener#textToplevelStyleChanged
			void TextRenderer::textToplevelStyleChanged(std::shared_ptr<const presentation::TextToplevelStyle>) {
				updateComputedBlockFlowDirectionChanged();
			}

			inline void TextRenderer::updateComputedBlockFlowDirectionChanged() {
				const presentation::WritingMode writingMode(presentation().computeWritingMode(this));
				const presentation::BlockFlowDirection used = computedBlockFlowDirection();
				computedBlockFlowDirection_ = writingMode.blockFlowDirection;
				computedBlockFlowDirectionListeners_.notify<presentation::BlockFlowDirection>(
					&ComputedBlockFlowDirectionListener::computedBlockFlowDirectionChanged, used);
			}
		}
	}
}
