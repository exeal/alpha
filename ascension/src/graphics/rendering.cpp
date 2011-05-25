/**
 * @file rendering.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2011
 * @date 2010-11-20 separated from ascension/layout.cpp
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/rendering.hpp>
#include <ascension/graphics/graphics.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace std;
namespace k = ascension::kernel;

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;


// FontSelector /////////////////////////////////////////////////////////////

namespace {
	AutoBuffer<WCHAR> ASCENSION_FASTCALL mapFontFileNameToTypeface(const WCHAR* fileName) {
		assert(fileName != 0);
		static const WCHAR KEY_NAME[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
		HKEY key;
		long e = ::RegOpenKeyExW(HKEY_CURRENT_USER, KEY_NAME, 0, KEY_QUERY_VALUE, &key);
		if(e != ERROR_SUCCESS)
			e = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_NAME, 0, KEY_QUERY_VALUE, &key);
		if(e == ERROR_SUCCESS) {
			const size_t fileNameLength = wcslen(fileName);
			DWORD maximumValueNameLength, maximumValueBytes;
			e = ::RegQueryInfoKeyW(key, 0, 0, 0, 0, 0, 0, 0, &maximumValueNameLength, &maximumValueBytes, 0, 0);
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
								return AutoBuffer<WCHAR>(0);
						}
					} else	// ERROR_NO_MORE_ITEMS
						break;
				}
			}
			::RegCloseKey(key);
		}
		return AutoBuffer<WCHAR>(0);
	}
} // namespace @0
#if 0
void FontSelector::linkPrimaryFont() /*throw()*/ {
	// TODO: this does not support nested font linking.
	assert(linkedFonts_ != 0);
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
					if(comma != 0 && comma[1] != 0)	// "<file-name>,<typeface>"
						linkedFonts_->push_back(new Fontset(comma + 1));
					else {	// "<file-name>"
						AutoBuffer<WCHAR> typeface(mapFontFileNameToTypeface(sz));
						if(typeface.get() != 0)
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
		const FontCollection& fontCollection, bool enableDoubleBuffering) : presentation_(presentation),
		fontCollection_(fontCollection), enablesDoubleBuffering_(enableDoubleBuffering) {
	layouts_.reset(new LineLayoutVector(presentation.document(),
		bind1st(mem_fun(&TextRenderer::generateLineLayout), this), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true));
	updateDefaultFont();
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
TextRenderer::TextRenderer(const TextRenderer& other) : presentation_(other.presentation_), layouts_(),
		fontCollection_(other.fontCollection_), enablesDoubleBuffering_(other.enablesDoubleBuffering_), defaultFont_() {
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
 * @fn ascension::graphics::font::TextRenderer::createLineLayout
 * Creates and returns the text layout for the specified line.
 * @param line The line number
 * @return The generated line layout
 */

/**
 * @fn ascension::graphics::font::TextRenderer::defaultUIWritingMode
 * Returns the default writing mode of UI. The value this method returns is
 * treated as "last resort" for resolution of writing mode of text layout.
 * @return The default writing mode
 * @see presentation#TextToplevel#writingMode
 * @see presentation#Presentation#globalTextStyle
 */

/// @see GlobalTextStyleListener#GlobalTextStyleChanged
void TextRenderer::globalTextStyleChanged(tr1::shared_ptr<const TextToplevelStyle>) {
	updateDefaultFont();
}

/**
 * Returns the indentation of the specified visual line from the left most.
 * @param line The line number
 * @param subline The visual subline number
 * @return The indentation in pixel
 * @throw kernel#BadPositionException @a line is invalid
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
int TextRenderer::lineIndent(length_t line, length_t subline /* = 0 */) const {
	const TextLayout& layout = layouts().at(line);
	const TextAlignment resolvedAlignment = resolveTextAlignment(layout.anchor(), layout.readingDirection());
	if(resolvedAlignment == ALIGN_LEFT || resolvedAlignment == JUSTIFY)	// TODO: recognize the last subline of a justified line.
		return 0;
	else {
		int w = width();
		switch(resolvedAlignment) {
		case ALIGN_RIGHT:
			return w - layout.lineWidth(subline);
		case ALIGN_CENTER:
			return (w - layout.lineWidth(subline)) / 2;
		default:
			return 0;
		}
	}
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
 * Renders the specified logical line to the output device.
 * @param line The line number
 * @param context The graphics context
 * @param origin The position to draw
 * @param colorOverride 
 * @param endOfLine 
 * @param lineWrappingMark 
 */
void TextRenderer::renderLine(length_t line, PaintContext& context,
		const Point<>& origin, const TextPaintOverride* paintOverride /* = 0 */,
		const InlineObject* endOfLine /* = 0 */, const InlineObject* lineWrappingMark /* = 0 */) const /*throw()*/ {
	if(!enablesDoubleBuffering_) {
		layouts().at(line).draw(context, origin, paintOverride, endOfLine, lineWrappingMark);
		return;
	}

	// TODO: this code uses deprecated terminologies for text coordinates.

	const TextLayout& layout = layouts().at(line);
	const Scalar dy = textMetrics().linePitch();

	// skip to the subline needs to draw
	const Scalar top = max(context.boundsToPaint().top(), clipRect.top());
	Scalar y = origin.y;
	length_t subline = (y + dy >= top) ? 0 : (top - (y + dy)) / dy;
	if(subline >= layout.numberOfLines())
		return;	// this logical line does not need to draw
	y += static_cast<Scalar>(dy * subline);

	if(memoryDC_.get() == 0)		
		memoryDC_.reset(::CreateCompatibleDC(context.nativeHandle().get()), &::DeleteDC);
	const int horizontalResolution = calculateMemoryBitmapSize(context.device()->size().cx);
	if(memoryBitmap_.get() != 0) {
		BITMAP temp;
		::GetObjectW(memoryBitmap_.get(), sizeof(HBITMAP), &temp);
		if(temp.bmWidth < horizontalResolution)
			memoryBitmap_.reset();
	}
	if(memoryBitmap_.get() == 0)
		memoryBitmap_.reset(::CreateCompatibleBitmap(
			context.nativeHandle().get(),
			horizontalResolution, calculateMemoryBitmapSize(dy)), &::DeleteObject);
	::SelectObject(memoryDC_.get(), memoryBitmap_.get());

	const int left = max(context.boundsToPaint().left(), clipRect.left());
	const int right = min(context.boundsToPaint().right(), clipRect.right());
	const Scalar x = origin.x - left;
	Rect<> offsetedPaintRect(paintRect), offsetedClipRect(clipRect);
	offsetedPaintRect.translate(Dimension<>(-left, -y));
	offsetedClipRect.translate(Dimension<>(-left, -y));
	for(; subline < layout.numberOfLines() && offsetedPaintRect.bottom() >= 0;
			++subline, y += dy, offsetedPaintRect.translate(Dimension<>(0, -dy)), offsetedClipRect.translate(Dimension<>(0, -dy))) {
		layout.draw(subline, memoryDC_, Point<>(x, 0), offsetedPaintRect, offsetedClipRect, selection);
		::BitBlt(context.nativeHandle().get(), left, y, right - left, dy, memoryDC_.get(), 0, 0, SRCCOPY);
	}
}

void TextRenderer::updateDefaultFont() {
	tr1::shared_ptr<const TextRunStyle> defaultStyle(presentation_.defaultTextRunStyle());
	if(defaultStyle.get() != 0 && !defaultStyle->fontFamily.empty())
		defaultFont_ = fontCollection().get(defaultStyle->fontFamily, defaultStyle->fontProperties);
	else {
		LOGFONTW lf;
		if(::GetObjectW(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)), sizeof(LOGFONTW), &lf) == 0)
			throw runtime_error("");
		FontProperties fps(static_cast<FontProperties::Weight>(lf.lfWeight), FontProperties::INHERIT_STRETCH,
			(lf.lfItalic != 0) ? FontProperties::ITALIC : FontProperties::NORMAL_STYLE,
			FontProperties::HORIZONTAL,	// TODO: Use lfEscapement and lfOrientation ?
			(lf.lfHeight < 0) ? -lf.lfHeight : 0);
		defaultFont_ = fontCollection().get(lf.lfFaceName, fps);
	}

	layouts().invalidate();
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != 0) {
		BITMAP temp;
		::GetObjectW(memoryBitmap_.get(), sizeof(HBITMAP), &temp);
		if(temp.bmHeight != calculateMemoryBitmapSize(defaultFont()->metrics().linePitch()))
			memoryBitmap_.reset();
	}
	defaultFontListeners_.notify(&DefaultFontListener::defaultFontChanged);
}
