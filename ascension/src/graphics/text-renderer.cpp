/**
 * @file text-renderer.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2012
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/corelib/text/break-iterator.hpp>	// text.GraphemeBreakIterator
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/text-renderer.hpp>
#include <ascension/kernel/document-character-iterator.hpp>

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
	void update(const TextRenderer& textRenderer, const NativeSize& size, const AbstractFourSides<Space>& spaces);
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

void TextRenderer::SpacePainter::update(const TextRenderer& textRenderer, const NativeSize& size, const AbstractFourSides<Space>& spaces) {
	canvasSize_ = size;
	AbstractFourSides<Scalar> spacesInPixels;
	for(size_t i = 0; i < spaces.size(); ++i)
		spacesInPixels[i] = static_cast<Scalar>(spaces[i].value(0, 0));
	mapAbstractToPhysical(textRenderer.writingMode(), spacesInPixels, computedValues_);
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
		bind1st(mem_fun(&TextRenderer::generateLineLayout), this), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
	updateDefaultFont();
	AbstractFourSides<Space> zeroSpaces;
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
		bind1st(mem_fun(&TextRenderer::generateLineLayout), this), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
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
 * 
 * @param writingMode 
 */
void TextRenderer::setDefaultUIWritingMode(const WritingMode& writingMode) {
	const WritingMode used(defaultUIWritingMode());
	defaultUIWritingMode_ = writingMode;
	fireComputedWritingModeChanged(presentation().globalTextStyle(), used);
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
	if(/*enablesDoubleBuffering_ &&*/ memoryBitmap_.get() != nullptr) {
		BITMAP temp;
		::GetObjectW(memoryBitmap_.get(), sizeof(HBITMAP), &temp);
		if(temp.bmHeight != calculateMemoryBitmapSize(defaultFont()->metrics().linePitch()))
			memoryBitmap_.reset();
	}
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


// BaselineIterator ///////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewport The text viewport
 * @param line The line number this iterator addresses
 * @param trackOutOfViewport Set @c true to 
 */
BaselineIterator::BaselineIterator(const TextViewport& viewport, Index line,
		bool trackOutOfViewport) : viewport_(&viewport), tracksOutOfViewport_(trackOutOfViewport) {
	initializeWithFirstVisibleLine();
	advance(line - this->line());
}

/// @see boost#iterator_facade#advance
void BaselineIterator::advance(BaselineIterator::difference_type n) {
	const TextRenderer& renderer = viewport().textRenderer();
	if(n == 0)
		return;
	else if(n > 0 && line() + n > renderer.presentation().document().numberOfLines())
		throw invalid_argument("n");
	else if(n < 0 && static_cast<Index>(-n) - 1 > line())
		throw invalid_argument("n");

	const Index destination = line() + n;
	if(!tracksOutOfViewport()
			&& (distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::min()
			|| distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::max())) {
		if((n > 0 && distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::max())
				|| (n < 0 && distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::min())) {
			line_ = VisualLine(destination, 0);
			return;
		}
		swap(*this, BaselineIterator(viewport(), destination, tracksOutOfViewport()));
		return;
	}

	const WritingMode writingMode(renderer.writingMode());
	Scalar viewportExtent;
	if(!tracksOutOfViewport() && n > 0)
		viewportExtent = isHorizontal(writingMode.blockFlowDirection) ?
			(geometry::dy(viewport().boundsInView())) : (geometry::dx(viewport().boundsInView()));

	VisualLine i(line_);
	Scalar newBaseline = distanceFromViewportBeforeEdge_;
	const TextLayout* layout = &renderer.layouts()[line()];
	if(n > 0) {
		newBaseline += layout->lineMetrics(line_.subline).descent();
		for(Index ln = line(), subline = line_.subline; ; ) {
			if(++subline == layout->numberOfLines()) {
				subline = 0;
				if(++ln == renderer.presentation().document().numberOfLines()) {
					newBaseline = numeric_limits<Scalar>::max();
					break;
				}
				layout = &renderer.layouts()[++ln];
			}
			newBaseline += layout->lineMetrics(subline).ascent();
			if(ln == destination && subline == 0)
				break;
			newBaseline += layout->lineMetrics(subline).descent();
			if(!tracksOutOfViewport() && newBaseline >= viewportExtent) {
				newBaseline = numeric_limits<Scalar>::max();
				break;
			}
		}
	} else {	// n < 0
		newBaseline -= layout->lineMetrics(line_.subline).ascent();
		for(Index ln = line(), subline = line_.subline; ; ) {
			if(subline == 0) {
				if(ln-- == 0) {
					subline = 0;
					newBaseline = numeric_limits<Scalar>::min();
					break;
				}
				layout = &renderer.layouts()[ln];
				subline = layout->numberOfLines() - 1;
			} else
				--subline;
			newBaseline -= layout->lineMetrics(subline).descent();
			if(ln == destination && subline == 0)
				break;
			newBaseline -= layout->lineMetrics(subline).ascent();
			if(!tracksOutOfViewport() && newBaseline < 0) {
				newBaseline = numeric_limits<Scalar>::min();
				break;
			}
		}
	}

	NativePoint newAxis(positionInViewport_);
	switch(writingMode.blockFlowDirection) {
		case HORIZONTAL_TB:
			geometry::y(newAxis) += newBaseline - distanceFromViewportBeforeEdge_;
			break;
		case VERTICAL_RL:
			geometry::x(newAxis) -= newBaseline - distanceFromViewportBeforeEdge_;
			break;
		case VERTICAL_LR:
			geometry::x(newAxis) += newBaseline - distanceFromViewportBeforeEdge_;
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// commit
	line_ = VisualLine(destination, 0);
	distanceFromViewportBeforeEdge_ = newBaseline;
	positionInViewport_ = newAxis;
}

/// @see boost#iterator_facade#decrement
void BaselineIterator::decrement() {
	return advance(-1);
}

/// @see boost#iterator_facade#dereference
const BaselineIterator::reference BaselineIterator::dereference() const {
	return distanceFromViewportBeforeEdge_;
}

/// @see boost#iterator_facade#increment
void BaselineIterator::increment() {
	return advance(+1);
}

/// @internal Moves this iterator to the first visible line in the viewport.
void BaselineIterator::initializeWithFirstVisibleLine() {
	const VisualLine firstVisibleLine(
		viewport().firstVisibleLineInLogicalNumber(), viewport().firstVisibleSublineInLogicalLine());
	const Scalar baseline = viewport().textRenderer().layouts().at(firstVisibleLine.line).lineMetrics(firstVisibleLine.subline).ascent();
	NativePoint axis;
	const NativeRectangle bounds(geometry::make<NativeRectangle>(
		geometry::make<NativePoint>(0, 0), geometry::size(viewport().boundsInView())));
	switch(viewport().textRenderer().writingMode().blockFlowDirection) {
		case HORIZONTAL_TB:
			axis = geometry::make<NativePoint>(0, geometry::top(bounds) + baseline);
			break;
		case VERTICAL_RL:
			axis = geometry::make<NativePoint>(geometry::right(bounds) - baseline, 0);
			break;
		case VERTICAL_LR:
			axis = geometry::make<NativePoint>(geometry::left(bounds) + baseline, 0);
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// commit
	line_ = firstVisibleLine;
	distanceFromViewportBeforeEdge_ = baseline;
	positionInViewport_ = axis;
}

inline void BaselineIterator::invalidate() /*throw()*/ {
	geometry::x(positionInViewport_) = geometry::y(positionInViewport_) = 1;
}

inline bool BaselineIterator::isValid() const /*throw()*/ {
	return geometry::x(positionInViewport_) != 0 && geometry::y(positionInViewport_) != 0;
}
#if 0
void TextViewer::BaselineIterator::move(Index line) {
	if(line >= viewer_.document().numberOfLines())
		throw k::BadPositionException(k::Position(line, 0));
	Scalar newBaseline;
	if(!isValid()) {
		Index firstVisibleLine, firstVisibleSubline;
		viewer_.firstVisibleLine(&firstVisibleLine, nullptr, &firstVisibleSubline);
		const PhysicalFourSides<Scalar> spaces(viewer_.spaceWidths());
		Scalar spaceBefore;
		switch(utils::writingMode(viewer_).blockFlowDirection) {
			case WritingModeBase::HORIZONTAL_TB:
				spaceBefore = spaces.top;
				break;
			case WritingModeBase::VERTICAL_RL:
				spaceBefore = spaces.right;
				break;
			case WritingModeBase::VERTICAL_LR:
				spaceBefore = spaces.left;
				break;
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		if(line == firstVisibleLine) {
			if(firstVisibleSubline == 0)
				newBaseline = textRenderer().layouts()[line].lineMetrics(0).ascent();
			else if(!tracksOutOfViewport())
				newBaseline = numeric_limits<Scalar>::min();
			else {
				const TextLayout& layout = textRenderer().layouts()[line];
				newBaseline = 0;
				for(Index subline = firstVisibleSubline - 1; ; --subline) {
					newBaseline -= layout.lineMetrics(subline).descent();
					if(subline == 0)
						break;
					newBaseline -= layout.lineMetrics(subline).ascent();
				}
			}
		} else if(line > firstVisibleLine) {
			const NativeRectangle clientBounds(viewer_.bounds(false));
			const Scalar viewportExtent = WritingModeBase::isHorizontal(utils::writingMode(viewer_).blockFlowDirection) ?
				(geometry::dy(clientBounds) - spaces.top - spaces.bottom) : (geometry::dx(clientBounds) - spaces.left - spaces.right);
			newBaseline = 0;
			const TextLayout* layout = &viewer_.textRenderer().layouts()[firstVisibleLine];
			for(Index ln = firstVisibleLine, subline = firstVisibleLine; ; ) {
				newBaseline += layout->lineMetrics(subline).ascent();
				if(ln == line && subline == 0)
					break;
				newBaseline += layout->lineMetrics(subline).descent();
				if(!tracksOutOfViewport() && newBaseline >= viewportExtent) {
					newBaseline = numeric_limits<Scalar>::max();
					break;
				}
				if(++subline == layout->numberOfLines()) {
					layout = &viewer_.textRenderer().layouts()[++ln];
					subline = 0;
				}
			}
		} else if(!tracksOutOfViewport())
			newBaseline = numeric_limits<Scalar>::min();
		else {
			const TextLayout* layout = &viewer_.textRenderer().layouts()[firstVisibleLine];
			for(Index ln = firstVisibleLine, subline = firstVisibleSubline; ; --subline) {
				newBaseline -= layout->lineMetrics(subline).descent();
				if(subline == 0 && ln == line)
					break;
				newBaseline -= layout->lineMetrics(subline).ascent();
				if(subline == 0) {
					layout = &viewer_.textRenderer().layouts()[--ln];
					subline = layout->numberOfLines();
				}
			}
		}
	}
}
#endif


// TextViewport ///////////////////////////////////////////////////////////////////////////////////

namespace {
	inline Scalar mapLineLayoutIpdToViewport(const TextViewport& viewport, Index line, Scalar ipd) {
		return ipd + viewport.inlineProgressionOffset()
			+ lineStartEdge(viewport.textRenderer().layouts().at(line), viewport.contentMeasure());
	}
	inline Scalar mapViewportIpdToLineLayout(const TextViewport& viewport, Index line, Scalar ipd) {
		return ipd - viewport.inlineProgressionOffset()
			- lineStartEdge(viewport.textRenderer().layouts().at(line), viewport.contentMeasure());
	}
}

/**
 * Returns the measure of the 'allocation-rectangle'.
 * @return The measure of the 'allocation-rectangle' in pixels
 * @see #contentMeasure
 */
Scalar TextViewport::allocationMeasure() const /*throw()*/ {
	const TextRenderer& renderer = textRenderer();
	const bool horizontal = isHorizontal(renderer.writingMode().blockFlowDirection);
	const Scalar spaces = horizontal ?
		renderer.spaceWidths().left() + renderer.spaceWidths().right()
		: renderer.spaceWidths().top() + renderer.spaceWidths().bottom();
	const Scalar borders = 0;
	const Scalar paddings = 0;
	return max(renderer.layouts().maximumMeasure() + spaces + borders + paddings,
		static_cast<Scalar>(horizontal ? geometry::dx(boundsInView()) : geometry::dy(boundsInView())));
}

/**
 * Returns the measure of the 'content-rectangle'.
 * @return The measure of the 'content-rectangle' in pixels
 * @see #allocationMeasure
 */
Scalar TextViewport::contentMeasure() const /*throw()*/ {
	return max(
		textRenderer().layouts().maximumMeasure(),
		static_cast<Scalar>(isHorizontal(textRenderer().writingMode().blockFlowDirection) ?
			geometry::dx(boundsInView()) : geometry::dy(boundsInView())));
}

/**
 * @internal Returns the document position nearest from the specified point.
 * @param pointInView The point in view-coordinates (not viewport-coordinates). This can be outside
 *                    of the view
 * @param edge If set @c TextLayout#LEADING, the result is the leading of the character at @a p.
 *             Otherwise the result is the position nearest @a p
 * @param abortNoCharacter If set to @c true, this method returns @c boost#none immediately when
 *                         @a p hovered outside of the text layout (e.g. far left or right of the
 *                         line, beyond the last line, ...)
 * @param snapPolicy Which character boundary the returned position snapped to
 * @return The document position, or @c boost#none if @a abortNoCharacter was @c true and @a p is
 *         outside of the layout
 * @throw UnknownValueException @a edge and/or snapPolicy are invalid
 * @see #location, TextLayout#offset
 */
boost::optional<k::Position> TextViewport::internalCharacterForPoint(
		const NativePoint& pointInView, TextLayout::Edge edge,
		bool abortNoCharacter, k::locations::CharacterUnit snapPolicy) const {
	NativePoint p(pointInView);
	geometry::translate(p, geometry::make<NativeSize>(geometry::left(boundsInView()), geometry::top(boundsInView())));
	k::Position result;

	// locate the logical line
	Index subline;
	bool outside;
	{
		const VisualLine temp(locateLine(p, &outside));
		result.line = temp.line;
		subline = temp.subline;
	}
	if(abortNoCharacter && outside)
		return boost::none;
	const TextLayout& layout = textRenderer().layouts()[result.line];
	const BaselineIterator baseline(*this, result.line, true);

	// locate the position in the line
	const bool horizontal = isHorizontal(layout.writingMode().blockFlowDirection);
	NativePoint lineLocalPoint(horizontal ?
		geometry::make<NativePoint>(
			mapViewportIpdToLineLayout(*this, result.line, geometry::x(p)),
			geometry::y(p) + geometry::y(baseline.positionInViewport()))
		: geometry::make<NativePoint>(
			geometry::x(p) + geometry::x(baseline.positionInViewport()),
			mapViewportIpdToLineLayout(*this, result.line, geometry::y(p))));
	if(edge == TextLayout::LEADING)
		result.offsetInLine = layout.offset(lineLocalPoint, &outside).first;
	else if(edge == TextLayout::TRAILING)
		result.offsetInLine = layout.offset(lineLocalPoint, &outside).second;
	else
		throw UnknownValueException("edge");
	if(abortNoCharacter && outside)
		return boost::none;

	// snap intervening position to the boundary
	if(result.offsetInLine != 0 && snapPolicy != k::locations::UTF16_CODE_UNIT) {
		using namespace text;
		const k::Document& document = textRenderer().presentation().document();
		const String& s = document.line(result.line);
		const bool interveningSurrogates =
			surrogates::isLowSurrogate(s[result.offsetInLine]) && surrogates::isHighSurrogate(s[result.offsetInLine - 1]);
		const Scalar ipd = horizontal ? static_cast<Scalar>(geometry::x(lineLocalPoint)) : geometry::y(lineLocalPoint);
		if(snapPolicy == k::locations::UTF32_CODE_UNIT) {
			if(interveningSurrogates) {
				if(edge == TextLayout::LEADING)
					--result.offsetInLine;
				else {
					const NativePoint leading(layout.location(result.offsetInLine - 1));
					const NativePoint trailing(layout.location(result.offsetInLine + 1));
					const Scalar leadingIpd = horizontal ? geometry::x(leading) : geometry::y(leading);
					const Scalar trailingIpd = horizontal ? geometry::x(trailing) : geometry::y(trailing);
					(detail::distance<Scalar>(ipd, leadingIpd)
						<= detail::distance<Scalar>(ipd, trailingIpd)) ? --result.offsetInLine : ++result.offsetInLine;
				}
			}
		} else if(snapPolicy == k::locations::GRAPHEME_CLUSTER) {
			text::GraphemeBreakIterator<k::DocumentCharacterIterator> i(
				k::DocumentCharacterIterator(document, k::Region(result.line, make_pair(0, s.length())), result));
			if(interveningSurrogates || !i.isBoundary(i.base())) {
				--i;
				if(edge == TextLayout::LEADING)
					result.offsetInLine = i.base().tell().offsetInLine;
				else {
					const k::Position backward(i.base().tell()), forward((++i).base().tell());
					const NativePoint leading(layout.location(backward.offsetInLine)), trailing(layout.location(forward.offsetInLine));
					const Scalar backwardIpd = horizontal ? geometry::x(leading) : geometry::y(leading);
					const Scalar forwardIpd = horizontal ? geometry::x(trailing) : geometry::y(trailing);
					result.offsetInLine = ((detail::distance<Scalar>(ipd, backwardIpd)
						<= detail::distance<Scalar>(ipd, forwardIpd)) ? backward : forward).offsetInLine;
				}
			}
		} else
			throw UnknownValueException("snapPolicy");
	}
	return result;
}

/**
 * Converts the point in the viewport into the logical line number and visual subline offset.
 * @param p The point in the viewport in pixels
 * @param[out] snapped @c true if there was not a line at @a p. Optional
 * @see #location, #mapBpdToLine, TextLayout#locateLine, TextLayout#offset
 */
VisualLine TextViewport::locateLine(const NativePoint& p, bool* snapped /* = nullptr */) const /*throw()*/ {
	const NativeRectangle bounds(geometry::make<NativeRectangle>(
		geometry::make<NativePoint>(0, 0), geometry::size(boundsInView())));
	switch(textRenderer().writingMode().blockFlowDirection) {
		case HORIZONTAL_TB:
			return mapBpdToLine(geometry::y(p) - geometry::top(bounds), snapped);
		case VERTICAL_RL:
			return mapBpdToLine(geometry::right(bounds) - geometry::x(p), snapped);
		case VERTICAL_LR:
			return mapBpdToLine(geometry::x(p) - geometry::left(bounds), snapped);
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Converts the specified position in the document to a point in the view-coordinates.
 * @param position The document position
 * @param fullSearchBpd If this is @c false, this method stops at before- or after-edge of the
 *                      viewport. If this happened, the block-progression-dimension of the returned
 *                      point is @c std#numeric_limits&lt;Scalar&gt;::max() (for the before-edge)
 *                      or @c std#numeric_limits&lt;Scalar&gt;::min() (for the after-edge). If this
 *                      is @c true, the calculation is performed completely and returns an exact
 *                      location will (may be very slow)
 * @param edge The edge of the character. If this is @c graphics#font#TextLayout#LEADING, the
 *             returned point is the leading edge if the character (left if the character is
 *             left-to-right), otherwise returned point is the trailing edge (right if the
 *             character is left-to-right)
 * @return The point in view-coordinates (not viewport-coordinates) in pixels. The
 *         block-progression-dimension addresses the baseline of the line
 * @throw BadPositionException @a position is outside of the document
 * @see #characterForLocalPoint, TextLayout#location
 */
NativePoint TextViewport::location(const k::Position& position,
		bool fullSearchBpd, TextLayout::Edge edge /* = TextLayout::LEADING */) const {
//	checkInitialization();

	// get alignment-point
	const BaselineIterator baseline(*this, position.line, fullSearchBpd);
	NativePoint p(baseline.positionInViewport());
	const bool horizontal = isHorizontal(textRenderer().writingMode().blockFlowDirection);

	// apply offset in line layout
	const NativePoint offset(textRenderer().layouts().at(position.line).location(position.offsetInLine, edge));
	if(fullSearchBpd || horizontal || (*baseline != numeric_limits<Scalar>::max() && *baseline != numeric_limits<Scalar>::min())) {
//		assert(geometry::x(p) != numeric_limits<Scalar>::max() && geometry::x(p) != numeric_limits<Scalar>::min());
		geometry::x(p) += geometry::x(offset);
	}
	if(fullSearchBpd || !horizontal || (*baseline != numeric_limits<Scalar>::max() && *baseline != numeric_limits<Scalar>::min())) {
//		assert(geometry::y(p) != numeric_limits<Scalar>::max() && geometry::y(p) != numeric_limits<Scalar>::min()));
		geometry::y(p) += geometry::y(offset);
	}

	// apply viewport offset in inline-progression-direction
	if(horizontal)
		geometry::x(p) = mapLineLayoutIpdToViewport(*this, position.line, geometry::x(p));
	else
		geometry::y(p) = mapLineLayoutIpdToViewport(*this, position.line, geometry::y(p));

	return geometry::translate(p, geometry::make<NativeSize>(geometry::left(boundsInView()), geometry::top(boundsInView())));
}

/**
 * Converts the distance from the 'before-edge' of the 'allocation-rectangle' into the logical line
 * and visual subline offset. The results are snapped to the first/last visible line in the
 * viewport (this includes partially visible line) if the given distance addresses outside of the
 * viewport.
 * @param bpd The distance from the 'before-edge' of the 'allocation-rectangle' in pixels
 * @param[out] snapped @c true if there was not a line at @a bpd. Optional
 * @return The logical and visual line numbers
 * @see #BaselineIterator, TextViewer#mapLocalBpdToLine
 */
VisualLine TextViewport::mapBpdToLine(Scalar bpd, bool* snapped /* = nullptr */) const /*throw()*/ {
	const WritingMode writingMode(textRenderer().writingMode());
	const PhysicalFourSides<Scalar>& physicalSpaces = textRenderer().spaceWidths();
	AbstractFourSides<Scalar> abstractSpaces;
	mapPhysicalToAbstract(writingMode, physicalSpaces, abstractSpaces);
	const Scalar spaceBefore = abstractSpaces.before();
	const Scalar spaceAfter = abstractSpaces.after();
	const Scalar borderBefore = 0, borderAfter = 0, paddingBefore = 0, paddingAfter = 0;
	const Scalar before = spaceBefore + borderBefore + paddingBefore;
	const Scalar after = (isHorizontal(writingMode.blockFlowDirection) ?
		geometry::dy(boundsInView()) : geometry::dx(boundsInView())) - spaceAfter - borderAfter - paddingBefore;

	VisualLine result(firstVisibleLineInLogicalNumber(), firstVisibleSublineInLogicalLine());
	bool outside;	// for 'snapped'
	if(bpd <= before)
		outside = bpd != before;
	else {
		const bool beyondAfter = bpd >= after;
		if(beyondAfter)
			bpd = after;
		Scalar lineBefore = before;
		const TextLayout* layout = &textRenderer().layouts()[result.line];
		while(result.subline > 0)	// back to the first subline
			lineBefore -= layout->lineMetrics(--result.subline).height();
		while(true) {
			assert(bpd >= lineBefore);
			Scalar lineAfter = lineBefore;
			for(Index sl = 0; sl < layout->numberOfLines(); ++sl)
				lineAfter += layout->lineMetrics(sl).height();
			if(bpd < lineAfter) {
				result.subline = layout->locateLine(bpd - lineBefore, outside);
				if(!outside)
					break;	// bpd is this line
				assert(result.subline == layout->numberOfLines() - 1);
			}
			layout = &textRenderer().layouts()[++result.line];
			lineBefore = lineAfter;
		}
		outside = beyondAfter;
	}
	if(snapped != nullptr)
		*snapped = outside;
	return result;
}

/**
 * Returns the number of the drawable columns in the window.
 * @return The number of columns
 */
float TextViewport::numberOfVisibleCharactersInLine() const /*throw()*/ {
	const bool horizontal = isHorizontal(textRenderer().writingMode().blockFlowDirection);
	Scalar ipd(horizontal ? geometry::dx(boundsInView()) : geometry::dy(boundsInView()));
	if(ipd == 0)
		return 0;
//	ipd -= horizontal ? (spaceWidths().left() + spaceWidths().right()) : (spaceWidths().top() + spaceWidths().bottom());
	return static_cast<float>(ipd) / textRenderer().defaultFont()->metrics().averageCharacterWidth();
}

/**
 * Returns the number of the drawable visual lines in the viewport.
 * @return The number of visual lines
 */
float TextViewport::numberOfVisibleLines() const /*throw()*/ {
	const bool horizontal = isHorizontal(textRenderer().writingMode().blockFlowDirection);
	Scalar bpd(horizontal ? geometry::dy(boundsInView()) : geometry::dx(boundsInView()));
	if(bpd <= 0)
		return 0;
//	bpd -= horizontal ? (spaceWidths().top() + spaceWidths().bottom()) : (spaceWidths().left() + spaceWidths().right());

	VisualLine line(firstVisibleLineInLogicalNumber(), firstVisibleSublineInLogicalLine());
	const TextLayout* layout = &textRenderer().layouts().at(line.line);
	float lines = 0;
	while(true) {
		const Scalar lineHeight = layout->lineMetrics(line.subline).height();
		if(lineHeight >= bpd)
			return lines += bpd / lineHeight;
		bpd -= lineHeight;
		++lines;
		if(line.subline == layout->numberOfLines() - 1) {
			if(line.line == textRenderer().presentation().document().numberOfLines() - 1)
				return lines;
			layout = &textRenderer().layouts()[++line.line];
			line.subline = 0;
		} else
			++line.subline;
	}
}

void TextViewport::scroll(const NativeSize& offset, viewers::base::Widget* widget) {
	switch(textRenderer().writingMode().blockFlowDirection) {
		case HORIZONTAL_TB:
			return scroll(geometry::dy(offset), geometry::dx(offset), widget);
		case VERTICAL_RL:
			return scroll(-geometry::dx(offset), geometry::dy(offset), widget);
		case VERTICAL_LR:
			return scroll(+geometry::dx(offset), geometry::dy(offset), widget);
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Scrolls the viewport to the specified position.
 * @param position
 * @param widget
 */
void TextViewport::scrollTo(const NativePoint& position, viewers::base::Widget* widget) {
	const bool horizontal = isHorizontal(textRenderer().writingMode().blockFlowDirection);
	return scrollTo(
		horizontal ? geometry::y(position) : geometry::x(position),
		horizontal ? geometry::x(position) : geometry::y(position),
		widget);
}

/**
 * Scrolls the viewport to the specified position.
 * @param bpd
 * @param ipd
 * @param widget
 */
void TextViewport::scrollTo(Index bpd, Index ipd, viewers::base::Widget* widget) {
	const Index maximumBpd =
		textRenderer().layouts().numberOfVisualLines() - static_cast<Index>(numberOfVisibleLines()) + 1;
	const Index maximumIpd =
		static_cast<Index>(contentMeasure()
			/ textRenderer().defaultFont()->metrics().averageCharacterWidth())
			- static_cast<Index>(numberOfVisibleCharactersInLine()) + 1;
	bpd = max<Index>(min(bpd, maximumBpd), 0);
	ipd = max<Index>(min(ipd, maximumIpd), 0);
	const ptrdiff_t dbpd = bpd - firstVisibleLineInVisualNumber();
	const ptrdiff_t dipd = ipd - inlineProgressionOffset();
	if(dbpd != 0 || dipd != 0)
		scroll(dbpd, dipd, widget);
}

void TextViewport::scrollTo(const VisualLine& line, Index ipd, viewers::base::Widget* widget) {
	// TODO: not implemented.
}


// free functions /////////////////////////////////////////////////////////////////////////////////

/**
 * Returns distance from the edge of content-area to the edge of the specified visual line in
 * pixels. The edges are the start side of @a layout (ex. left if left-to-right, bottom if
 * bottom-to-top).
 * @param layout The layout of the line
 * @param contentMeasure The measure of 'content-rectangle' of the content-area in pixels
 * @param subline The visual subline number
 * @return The indentation in pixels
 * @throw IndexOutOfBoundsException @a subline is invalid
 * @see font#lineStartEdge
 */
Scalar font::lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline /* = 0 */) {
	switch(layout.anchor()) {
		case TEXT_ANCHOR_START:
			return 0;
		case TEXT_ANCHOR_MIDDLE:
			return (contentMeasure - layout.measure(subline)) / 2;
		case TEXT_ANCHOR_END:
			return contentMeasure - layout.measure(subline);
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Returns distance from left/top-edge of the content-area to 'start-edge' of the specified line in
 * pixels.
 * @param layout The layout of the line
 * @param contentMeasure The measure of 'content-rectangle' of the content-area in pixels
 * @param subline The visual subline number
 * @return Distance from left/top-edge of the content-area to start-edge of @a line in pixels
 * @throw IndexOutOfBoundsException @a subline is invalid
 * @see font#lineIndent, TextLayout#lineStartEdge, TextViewer#inlineProgressionOffsetInViewport
 */
Scalar font::lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline /* = 0 */) {
	const Scalar indent = lineIndent(layout, contentMeasure, subline);
	return (layout.writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? indent : contentMeasure - indent;
}
