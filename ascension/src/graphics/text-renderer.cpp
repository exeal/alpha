/**
 * @file text-renderer.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2012
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/text-renderer.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace std;
namespace k = ascension::kernel;

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;


// FontSelector /////////////////////////////////////////////////////////////
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
			const size_t fileNameLength = wcslen(fileName);
			DWORD maximumValueNameLength, maximumValueBytes;
			e = ::RegQueryInfoKeyW(key, nullptr, nullptr, nullptr,
				nullptr, nullptr, nullptr, nullptr, &maximumValueNameLength, &maximumValueBytes, nullptr, nullptr);
			if(e == ERROR_SUCCESS && (maximumValueBytes / sizeof(WCHAR)) - 1 >= fileNameLength) {
				const size_t fileNameLength = wcslen(fileName);
				AutoBuffer<WCHAR> valueName(new WCHAR[maximumValueNameLength + 1]);
				AutoBuffer<BYTE> value(new BYTE[maximumValueBytes]);
				DWORD valueNameLength = maximumValueNameLength + 1, valueBytes = maximumValueBytes, type;
				for(DWORD index = 0; ; ++index, valueNameLength = maximumValueNameLength + 1, valueBytes = maximumValueBytes) {
					e = ::RegEnumValueW(key, index, valueName.get(), &valueNameLength, 0, &type, value.get(), &valueBytes);
					if(e == ERROR_SUCCESS) {
						if(type == REG_SZ && (valueBytes / sizeof(WCHAR)) - 1 == fileNameLength
								&& wmemcmp(fileName, reinterpret_cast<WCHAR*>(value.get()), fileNameLength) == 0) {
							::RegCloseKey(key);
							size_t nameLength = valueNameLength;
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

void FontSelector::linkPrimaryFont() /*throw()*/ {
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


// TextRenderer.SpacePainter ////////////////////////////////////////////////////////////////////////

class TextRenderer::SpacePainter {
public:
	SpacePainter();
	void paint(PaintContext& context);
	const PhysicalFourSides<Scalar>& spaces() const;
	void update(const TextRenderer& textRenderer, const NativeSize& size, const FlowRelativeFourSides<Space>& spaces);
private:
	NativeSize canvasSize_;
	PhysicalFourSides<Scalar> computedValues_;
};

TextRenderer::SpacePainter::SpacePainter() /*throw()*/ : canvasSize_(geometry::make<NativeSize>(0, 0)) {
	computedValues_.left() = computedValues_.top() = computedValues_.right() = computedValues_.bottom() = 0;
}

void TextRenderer::SpacePainter::paint(PaintContext& context) {
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
	const NativeRectangle boundsToPaint(context.boundsToPaint());

	// space-top
	NativeRectangle r(geometry::make<NativeRectangle>(
		geometry::topLeft(canvasBounds_), geometry::make<NativeSize>(geometry::dx(canvasBounds_), computedValues_.top)));
	r = geometry::intersected(r, boundsToPaint);
	if(!geometry::isEmpty(r))
		context.fillRectangle(r);

	// space-bottom
	r = geometry::make<NativeRectangle>(
		geometry::bottomLeft(canvasBounds_), geometry::make<NativeSize>(geometry::dx(canvasBounds_), -computedValues_.bottom));
	r = geometry::intersected(r, boundsToPaint);
	if(!geometry::isEmpty(r))
		context.fillRectangle(r);

	// space-left
	r = geometry::make<NativeRectangle>(
		makeRange(geometry::left(canvasBounds_), geometry::left(canvasBounds_) + computedValues_.left),
		makeRange(geometry::top(canvasBounds_) + computedValues_.top, geometry::bottom(canvasBounds_) - computedValues_.bottom));
	r = geometry::intersected(r, boundsToPaint);
	if(!geometry::isEmpty(r))
		context.fillRectangle(r);

	// space-right
	r = geometry::make<NativeRectangle>(
		makeRange(geometry::right(canvasBounds_), geometry::right(canvasBounds_) - computedValues_.right),
		makeRange(geometry::top(canvasBounds_) + computedValues_.top, geometry::bottom(canvasBounds_) - computedValues_.bottom));
	r = geometry::intersected(r, boundsToPaint);
	if(!geometry::isEmpty(r))
		context.fillRectangle(r);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
}

inline const PhysicalFourSides<Scalar>& TextRenderer::SpacePainter::spaces() const {
	return computedValues_;
}

void TextRenderer::SpacePainter::update(const TextRenderer& textRenderer, const NativeSize& size, const FlowRelativeFourSides<Space>& spaces) {
	canvasSize_ = size;
	FlowRelativeFourSides<Scalar> spacesInPixels;
	for(size_t i = 0; i < spaces.size(); ++i)
		spacesInPixels[i] = static_cast<Scalar>(spaces[i].value(0, 0));
	mapFlowRelativeToPhysical(textRenderer.writingMode(), spacesInPixels, computedValues_);
}


// TextRenderer ///////////////////////////////////////////////////////////////////////////////////

namespace {
	inline int calculateMemoryBitmapSize(int src) /*throw()*/ {
		const int UNIT = 32;
		return (src % UNIT != 0) ? src + UNIT - src % UNIT : src;
	}
} // namespace @0

/**
 * @class ascension::graphics::font::TextRenderer
 * @c TextRenderer renders styled text to the display or to a printer. Although this class
 * extends @c LineLayoutBuffer class and implements @c ILayoutInformationProvider interface,
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
TextRenderer::TextRenderer(Presentation& presentation,
		const FontCollection& fontCollection, const NativeSize& initialSize)
		: presentation_(presentation), fontCollection_(fontCollection), spacePainter_(new SpacePainter) {
//	textWrapping_.measure = 0;
	layouts_.reset(new LineLayoutVector(presentation.document(),
		bind(&TextRenderer::generateLineLayout, this, placeholders::_1), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
	updateDefaultFont();
	FlowRelativeFourSides<Space> zeroSpaces;
	zeroSpaces.fill(Length(0));
	spacePainter_->update(*this, initialSize, zeroSpaces);
/*	switch(PRIMARYLANGID(getUserDefaultUILanguage())) {
	case LANG_CHINESE:
	case LANG_JAPANESE:
	case LANG_KOREAN:
		enableFontLinking();
		break;
	}*/
//	updateViewerSize(); ???
	presentation_.addGlobalTextStyleListener(*this);
}

/// Copy-constructor.
TextRenderer::TextRenderer(const TextRenderer& other) :
		presentation_(other.presentation_), layouts_(), fontCollection_(other.fontCollection_), defaultFont_() {
	layouts_.reset(new LineLayoutVector(other.presentation_.document(),
		bind(&TextRenderer::generateLineLayout, this, placeholders::_1), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
	updateDefaultFont();
//	updateViewerSize(); ???
	presentation_.addGlobalTextStyleListener(*this);
}

/// Destructor.
TextRenderer::~TextRenderer() /*throw()*/ {
	presentation_.removeGlobalTextStyleListener(*this);
//	getTextViewer().removeDisplaySizeListener(*this);
//	layouts_.removeVisualLinesListener(*this);
}

/**
 * Registers the default font selector listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void TextRenderer::addDefaultFontListener(DefaultFontListener& listener) {
	defaultFontListeners_.add(listener);
}

/**
 * Returns the distance from the baseline of the line @a from to the baseline of the line @a to in
 * block progression direction in pixels.
 * @param lines The first and second lines
 * @return The distance between the two baselines in pixels
 */
Scalar TextRenderer::baselineDistance(const Range<VisualLine>& lines) const {
	// TODO: This code does not consider 'line-stacking-strategy'.
	if(lines.beginning().line == lines.end().line) {
		if(lines.beginning().subline == lines.end().subline)
			return 0;
		const TextLayout& layout = layouts().at(lines.beginning().line);
		return layout.baseline(lines.end().subline) - layout.baseline(lines.beginning().subline);
	} else {
		const TextLayout* layout = &layouts().at(lines.beginning().line);
		Scalar bpd = layout->extent().end() - layout->baseline(lines.beginning().subline);
		for(Index line = lines.beginning().line + 1; line < lines.end().line; ++line)
			bpd += length(layouts().at(line).extent());
		layout = &layouts().at(lines.end().line);
		return bpd += layout->baseline(lines.end().subline) - layout->extent().beginning();
	}
}

/**
 * Builds construction parameters for @c TextLayout object.
 * @param[in] line The line number
 * @param[out] parameters
 * @see #createLineLayout
 */
void TextRenderer::buildLineLayoutConstructionParameters(Index line, TextLayout::ConstructionParameters& parameters) const {
	presentation_.textLineStyle(line, parameters);
	parameters.writingMode = writingMode();
}

/**
 * @fn ascension::graphics::font::TextRenderer::createLineLayout
 * Creates and returns the text layout for the specified line.
 * @param line The line number
 * @return The generated line layout
 * @see #buildLineLayoutConstructionParameters
 */

/**
 * @fn ascension::graphics::font::TextRenderer::defaultUIWritingMode
 * Returns the default writing mode of UI. The value this method returns is
 * treated as "last resort" for resolution of writing mode of text layout.
 * @return The default writing mode
 * @see presentation#TextToplevel#writingMode
 * @see presentation#Presentation#globalTextStyle
 */

namespace {
	inline void computeWritingMode(const Inheritable<WritingMode>& primary, const WritingMode& secondary, WritingMode& result) {
		result.inlineFlowDirection = resolveInheritance(primary.inlineFlowDirection, secondary.inlineFlowDirection);
		result.blockFlowDirection = resolveInheritance(primary.blockFlowDirection, secondary.blockFlowDirection);
		result.textOrientation = resolveInheritance(primary.textOrientation, secondary.textOrientation);
	}
}

inline void TextRenderer::fireComputedWritingModeChanged(const TextToplevelStyle& textStyle, const WritingMode& defaultUI) {
	WritingMode used;
	computeWritingMode(textStyle.writingMode, defaultUI, used);
	computedWritingModeListeners_.notify<const WritingMode&>(&ComputedWritingModeListener::computedWritingModeChanged, used);
}

/// @see GlobalTextStyleListener#GlobalTextStyleChanged
void TextRenderer::globalTextStyleChanged(shared_ptr<const TextToplevelStyle> used) {
	fireComputedWritingModeChanged(*used, defaultUIWritingMode());
	updateDefaultFont();
}

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
 */
void TextRenderer::paint(Index line, PaintContext& context, const NativePoint& alignmentPoint) const /*throw()*/ {
//	if(!enablesDoubleBuffering_) {
		layouts().at(line).draw(context, alignmentPoint,
			(lineRenderingOptions_.get() != nullptr) ? lineRenderingOptions_->textPaintOverride(line) : nullptr,
			(lineRenderingOptions_.get() != nullptr) ? lineRenderingOptions_->endOfLine(line) : nullptr,
			(lineRenderingOptions_.get() != nullptr) ? lineRenderingOptions_->textWrappingMark(line) : nullptr);
		return;
//	}

#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
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
 * Removes the default font selector listener.
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void TextRenderer::removeDefaultFontListener(DefaultFontListener& listener) {
	defaultFontListeners_.remove(listener);
}

/**
 * Sets the default UI writing mode. This method invalidates the all layouts and call listeners'
 * @c ComputedWritingModeListener#computedWritingModeChanged.
 * @param writingMode The new value to set
 */
void TextRenderer::setDefaultUIWritingMode(const WritingMode& writingMode) {
	if(writingMode != defaultUIWritingMode()) {
		const WritingMode used(defaultUIWritingMode());
		defaultUIWritingMode_ = writingMode;
		layouts().invalidate();
		fireComputedWritingModeChanged(presentation().globalTextStyle(), used);
	}
}

/**
 * Sets the text wrapping settings.
 * @param newValue The new settings
 * @param renderingContext The rendering context used to calculate the logical value of
 *                         @a newValue.measure. This can be @c null if @a newValue.measure has
 *                         absolute value
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

void TextRenderer::updateDefaultFont() {
	shared_ptr<const TextRunStyle> defaultStyle(presentation_.globalTextStyle().defaultLineStyle->defaultRunStyle);
	if(defaultStyle.get() != nullptr && !defaultStyle->fontFamily.empty())
		defaultFont_ = fontCollection().get(defaultStyle->fontFamily, defaultStyle->fontProperties);
	else {
		LOGFONTW lf;
		if(::GetObjectW(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)), sizeof(LOGFONTW), &lf) == 0)
			throw runtime_error("");
		FontProperties<> fps(static_cast<FontPropertiesBase::Weight>(lf.lfWeight), FontPropertiesBase::NORMAL_STRETCH,
			(lf.lfItalic != 0) ? FontPropertiesBase::ITALIC : FontPropertiesBase::NORMAL_STYLE,
			FontPropertiesBase::NORMAL_VARIANT,
			FontPropertiesBase::HORIZONTAL,	// TODO: Use lfEscapement and lfOrientation ?
			(lf.lfHeight < 0) ? -lf.lfHeight : 0);
		defaultFont_ = fontCollection().get(lf.lfFaceName, fps);
	}

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

/**
 * Returns the computed writing mode.
 */
WritingMode TextRenderer::writingMode() const /*throw()*/ {
	WritingMode computed;
	computeWritingMode(presentation().globalTextStyle().writingMode, defaultUIWritingMode(), computed);
	return computed;
}
