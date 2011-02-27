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
//#include <ascension/graphics/special-character-renderer.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace std;
namespace k = ascension::kernel;

#pragma comment(lib, "usp10.lib")

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;


// LineLayoutBuffer ///////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document The document
 * @param bufferSize The maximum number of lines cached
 * @param autoRepair Set @c true to repair disposed layout automatically if the line number of its
 *                   line was not changed
 * @throw std#invalid_argument @a bufferSize is zero
 */
LineLayoutBuffer::LineLayoutBuffer(k::Document& document, length_t bufferSize, bool autoRepair) :
		document_(document), bufferSize_(bufferSize), autoRepair_(autoRepair), documentChangePhase_(NONE),
		maximumIpd_(0), longestLine_(INVALID_INDEX), numberOfVisualLines_(document.numberOfLines()) {
	pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	if(bufferSize == 0)
		throw invalid_argument("size of the buffer can't be zero.");
	document_.addPrenotifiedListener(*this);
	document_.addPartitioningListener(*this);
}

/// Destructor.
LineLayoutBuffer::~LineLayoutBuffer() /*throw()*/ {
//	clearCaches(startLine_, startLine_ + bufferSize_, false);
	for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i)
		delete i->second;
	document_.removePrenotifiedListener(*this);
	document_.removePartitioningListener(*this);
}

/**
 * Registers the visual lines listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void LineLayoutBuffer::addVisualLinesListener(VisualLinesListener& listener) {
	listeners_.add(listener);
	const length_t lines = document_.numberOfLines();
	if(lines > 1)
		listener.visualLinesInserted(1, lines);
}

/**
 * Clears the layout caches of the specified lines. This method calls @c #layoutModified.
 * @param first The start of lines
 * @param last The end of lines (exclusive. this line will not be cleared)
 * @param repair Set @c true to recreate layouts for the lines. If @c true, this method calls
 *               @c #layoutModified. Otherwise calls @c #layoutDeleted
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
void LineLayoutBuffer::clearCaches(length_t first, length_t last, bool repair) {
	if(first > last /*|| last > viewer_.getDocument().getNumberOfLines()*/)
		throw invalid_argument("either line number is invalid.");
	if(documentChangePhase_ == ABOUT_CHANGE) {
		pendingCacheClearance_.first = (pendingCacheClearance_.first == INVALID_INDEX) ? first : min(first, pendingCacheClearance_.first);
		pendingCacheClearance_.last = (pendingCacheClearance_.last == INVALID_INDEX) ? last : max(last, pendingCacheClearance_.last);
		return;
	}
	if(first == last)
		return;

//	const size_t originalSize = layouts_.size();
	length_t oldSublines = 0, cachedLines = 0;
	if(repair) {
		auto_ptr<Context> context;
		length_t newSublines = 0, actualFirst = last, actualLast = first;
		for(Iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			if(i->first >= first && i->first < last) {
				oldSublines += i->second->numberOfLines();
				delete i->second;
				if(context.get() == 0)
					context = renderingContext();
				auto_ptr<TextLayout> newLayout(createLineLayout(i->first));
				assert(newLayout.get() != 0);	// TODO:
				i->second = newLayout.release();
				newSublines += i->second->numberOfLines();
				++cachedLines;
				actualFirst = min(actualFirst, i->first);
				actualLast = max(actualLast, i->first);
			}
		}
		if(actualFirst == last)	// no lines cleared
			return;
		++actualLast;
		fireVisualLinesModified(actualFirst, actualLast, newSublines += actualLast - actualFirst - cachedLines,
			oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
	} else {
		for(Iterator i(layouts_.begin()); i != layouts_.end(); ) {
			if(i->first >= first && i->first < last) {
				oldSublines += i->second->numberOfLines();
				delete i->second;
				i = layouts_.erase(i);
				++cachedLines;
			} else
				++i;
		}
		fireVisualLinesDeleted(first, last, oldSublines += last - first - cachedLines);
	}
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void LineLayoutBuffer::documentAboutToBeChanged(const kernel::Document&) {
	documentChangePhase_ = ABOUT_CHANGE;
}

/// @see kernel#IDocumentListener#documentChanged
void LineLayoutBuffer::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
	documentChangePhase_ = CHANGING;
	assert(change.erasedRegion().isNormalized() && change.insertedRegion().isNormalized());
	if(change.erasedRegion().first.line != change.erasedRegion().second.line) {	// erased region includes newline(s)
		const k::Region& region = change.erasedRegion();
		clearCaches(region.first.line + 1, region.second.line + 1, false);
		for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->first > region.first.line)
				i->first -= region.second.line - region.first.line;	// $friendly-access
		}
	}
	if(change.insertedRegion().first.line != change.insertedRegion().second.line) {	// inserted text is multiline
		const k::Region& region = change.insertedRegion();
		for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
			if(i->first > region.first.line)
				i->first += region.second.line - region.first.line;	// $friendly-access
		}
		fireVisualLinesInserted(region.first.line + 1, region.second.line + 1);
	}
	const length_t firstLine = min(change.erasedRegion().first.line, change.insertedRegion().first.line);
	if(pendingCacheClearance_.first == INVALID_INDEX
			|| firstLine < pendingCacheClearance_.first || firstLine >= pendingCacheClearance_.last)
		invalidate(firstLine);
	documentChangePhase_ = NONE;
	if(pendingCacheClearance_.first != INVALID_INDEX) {
		clearCaches(pendingCacheClearance_.first, pendingCacheClearance_.last, autoRepair_);
		pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	}
}

/// @see kernel#IDocumentPartitioningListener#documentPartitioningChanged
void LineLayoutBuffer::documentPartitioningChanged(const k::Region& changedRegion) {
	invalidate(changedRegion.beginning().line, changedRegion.end().line + 1);
}

void LineLayoutBuffer::fireVisualLinesDeleted(length_t first, length_t last, length_t sublines) {
	numberOfVisualLines_ -= sublines;
	const bool widthChanged = longestLine_ >= first && longestLine_ < last;
	if(widthChanged)
		updateLongestLine(static_cast<length_t>(-1), 0);
	listeners_.notify<length_t, length_t, length_t>(
		&VisualLinesListener::visualLinesDeleted, first, last, sublines, widthChanged);
}

void LineLayoutBuffer::fireVisualLinesInserted(length_t first, length_t last) /*throw()*/ {
	numberOfVisualLines_ += last - first;
	listeners_.notify<length_t, length_t>(&VisualLinesListener::visualLinesInserted, first, last);
}

void LineLayoutBuffer::fireVisualLinesModified(length_t first, length_t last,
		length_t newSublines, length_t oldSublines, bool documentChanged) /*throw()*/ {
	numberOfVisualLines_ += newSublines;
	numberOfVisualLines_ -= oldSublines;

	// update the longest line
	bool longestLineChanged = false;
	if(longestLine_ >= first && longestLine_ < last) {
		updateLongestLine(static_cast<length_t>(-1), 0);
		longestLineChanged = true;
	} else {
		length_t newLongestLine = longestLine_;
		Scalar newMaximumIpd = maximumInlineProgressionDimension();
		for(ConstIterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
			if(i->second->maximumInlineProgressionDimension() > newMaximumIpd) {
				newLongestLine = i->first;
				newMaximumIpd = i->second->maximumInlineProgressionDimension();
			}
		}
		if(longestLineChanged = (newLongestLine != longestLine_))
			updateLongestLine(newLongestLine, newMaximumIpd);
	}

	listeners_.notify<length_t, length_t, signed_length_t>(
		&VisualLinesListener::visualLinesModified, first, last,
		static_cast<signed_length_t>(newSublines) - static_cast<signed_length_t>(oldSublines), documentChanged, longestLineChanged);
}

/// Invalidates all layouts.
void LineLayoutBuffer::invalidate() /*throw()*/ {
	clearCaches(0, document().numberOfLines(), autoRepair_);
}

/**
 * Invalidates the layouts of the specified lines.
 * @param first The start of the lines
 * @param last The end of the lines (exclusive. this line will not be cleared)
 * @throw std#invalid_argument @a first &gt;= @a last
 */
void LineLayoutBuffer::invalidate(length_t first, length_t last) {
	if(first >= last)
		throw invalid_argument("Any line number is invalid.");
	clearCaches(first, last, autoRepair_);
}

/**
 * Resets the cached layout of the specified line and repairs if necessary.
 * @param line The line to invalidate layout
 */
inline void LineLayoutBuffer::invalidate(length_t line) {
	for(Iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		if(i->first == line) {
			const length_t oldSublines = i->second->numberOfLines();
			delete i->second;
			if(autoRepair_) {
				i->second = createLineLayout(line).release();
				fireVisualLinesModified(line, line + 1,
					i->second->numberOfLines(), oldSublines, documentChangePhase_ == CHANGING);
			} else {
				layouts_.erase(i);
				fireVisualLinesModified(line, line + 1,
					1, oldSublines, documentChangePhase_ == CHANGING);
			}
			break;
		}
	}
}

/**
 * Returns the layout of the specified line.
 * @param line The line
 * @return The layout
 * @throw kernel#BadPositionException @a line is greater than the number of the lines
 */
const TextLayout& LineLayoutBuffer::lineLayout(length_t line) const {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
	manah::win32::DumpContext dout;
	dout << "finding layout for line " << line;
#endif
	if(line > document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	LineLayoutBuffer& self = *const_cast<LineLayoutBuffer*>(this);
	Iterator i(self.layouts_.begin());
	for(const Iterator e(self.layouts_.end()); i != e; ++i) {
		if(i->first == line)
			break;
	}

	if(i != layouts_.end()) {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache found\n";
#endif
		if(i->second != layouts_.front().second) {
			// bring to the top
			const LineLayout temp(*i);
			self.layouts_.erase(i);
			self.layouts_.push_front(temp);
			i = self.layouts_.begin();
		}
		return *i->second;
	} else {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache not found\n";
#endif
		if(layouts_.size() == bufferSize_) {
			// delete the last
			self.layouts_.pop_back();
			self.fireVisualLinesModified(i->first, i->first + 1,
				1, i->second->numberOfLines(), documentChangePhase_ == CHANGING);
			delete i->second;
		}
		const TextLayout* const layout = createLineLayout(line).release();
		self.layouts_.push_front(make_pair(line, layout));
		self.fireVisualLinesModified(line, line + 1, layout->numberOfLines(), 1, documentChangePhase_ == CHANGING);
		return *layout;
	}
}

/**
 * Returns the first visual line number of the specified logical line.
 * @param line The logical line
 * @return The first visual line of @a line
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #mapLogicalPositionToVisualPosition
 */
length_t LineLayoutBuffer::mapLogicalLineToVisualLine(length_t line) const {
	if(line >= document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
//	else if(!wrapLongLines())
//		return line;
	length_t result = 0, cachedLines = 0;
	for(ConstIterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
		if(i->first < line) {
			result += i->second->numberOfLines();
			++cachedLines;
		}
	}
	return result + line - cachedLines;
}

/**
 * Returns the visual line number and the visual column number of the specified logical position.
 * @param position The logical coordinates of the position to be mapped
 * @param[out] column The visual column of @a position. Can be @c null if not needed
 * @return The visual line of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalLineToVisualLine
 */
length_t LineLayoutBuffer::mapLogicalPositionToVisualPosition(const k::Position& position, length_t* column) const {
//	if(!wrapsLongLines()) {
//		if(column != 0)
//			*column = position.column;
//		return position.line;
//	}
	const TextLayout& layout = lineLayout(position.line);
	const length_t line = layout.lineAt(position.column);
	if(column != 0)
		*column = position.column - layout.lineOffset(line);
	return mapLogicalLineToVisualLine(position.line) + line;
}

#if 0
/**
 * Returns the logical line number and the visual subline number of the specified visual line.
 * @param line The visual line
 * @param[out] subline The visual subline of @a line. can be @c null if not needed
 * @return The logical line
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #mapLogicalLineToVisualLine, #mapVisualPositionToLogicalPosition
 */
length_t LineLayoutBuffer::mapVisualLineToLogicalLine(length_t line, length_t* subline) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps()) {
		if(subline != 0)
			*subline = 0;
		return line;
	}
	length_t c = getCacheFirstLine();
	for(length_t i = getCacheFirstLine(); ; ++i) {
		if(c + getNumberOfSublinesOfLine(i) > line) {
			if(subline != 0)
				*subline = line - c;
			return i;
		}
		c += getNumberOfSublinesOfLine(i);
	}
	assert(false);
	return getCacheLastLine();	// ここには来ない
}

/**
 * Returns the logical line number and the logical column number of the specified visual position.
 * @param position The visual coordinates of the position to be mapped
 * @return The logical coordinates of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
k::Position LineLayoutBuffer::mapVisualPositionToLogicalPosition(const k::Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	k::Position result;
	length_t subline;
	result.line = mapVisualLineToLogicalLine(position.line, &subline);
	result.column = getLineLayout(result.line).getSublineOffset(subline) + position.column;
	return result;
}
#endif // 0

/**
 * Offsets visual line.
 * @param[in,out] line The logical line
 * @param[in,out] subline The visual subline
 * @param[in] offset The offset
 * @param[out] overflowedOrUnderflowed @c true if absolute value of @a offset is too large so that
 *                                     the results were snapped to the beginning or the end of the
 *                                     document. Optional
 */
void LineLayoutBuffer::offsetVisualLine(length_t& line, length_t& subline,
		signed_length_t offset, bool* overflowedOrUnderflowed) const /*throw()*/ {
	if(offset > 0) {
		if(subline + offset < numberOfSublinesOfLine(line))
			subline += offset;
		else {
			const length_t lines = document().numberOfLines();
			offset -= static_cast<signed_length_t>(numberOfSublinesOfLine(line) - subline) - 1;
			while(offset > 0 && line < lines - 1)
				offset -= static_cast<signed_length_t>(numberOfSublinesOfLine(++line));
			subline = numberOfSublinesOfLine(line) - 1;
			if(offset < 0)
				subline += offset;
			if(overflowedOrUnderflowed != 0)
				*overflowedOrUnderflowed = offset > 0;
		}
	} else if(offset < 0) {
		if(static_cast<length_t>(-offset) <= subline)
			subline += offset;
		else {
			offset += static_cast<signed_length_t>(subline);
			while(offset < 0 && line > 0)
				offset += static_cast<signed_length_t>(numberOfSublinesOfLine(--line));
			subline = (offset > 0) ? offset : 0;
			if(overflowedOrUnderflowed != 0)
				*overflowedOrUnderflowed = offset > 0;
		}
	}
}

/// @see presentation#IPresentationStylistListener
void LineLayoutBuffer::presentationStylistChanged() {
	invalidate();
}

/**
 * Updates the longest line.
 * @param line The new longest line. set -1 to recalculate
 * @param ipd The inline progression dimension of the longest line. If @a line is -1, this value is
 *            ignored
 */
void LineLayoutBuffer::updateLongestLine(length_t line, Scalar ipd) /*throw()*/ {
	if(line != -1) {
		longestLine_ = line;
		maximumIpd_ = ipd;
	} else {
		longestLine_ = static_cast<length_t>(-1);
		maximumIpd_ = 0;
		for(ConstIterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
			if(i->second->maximumInlineProgressionDimension() > maximumIpd_) {
				longestLine_ = i->first;
				maximumIpd_ = i->second->maximumInlineProgressionDimension();
			}
		}
	}
}


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

// TextRenderer /////////////////////////////////////////////////////////////

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
TextRenderer::TextRenderer(Presentation& presentation, const FontCollection& fontCollection, bool enableDoubleBuffering) :
		LineLayoutBuffer(presentation.document(), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		presentation_(presentation), fontCollection_(fontCollection), enablesDoubleBuffering_(enableDoubleBuffering) {
	updateDefaultFont();
/*	switch(PRIMARYLANGID(getUserDefaultUILanguage())) {
	case LANG_CHINESE:
	case LANG_JAPANESE:
	case LANG_KOREAN:
		enableFontLinking();
		break;
	}*/
//	updateViewerSize(); ???
	presentation_.addDefaultTextStyleListener(*this);
}

/// Copy-constructor.
TextRenderer::TextRenderer(const TextRenderer& other) :
		LineLayoutBuffer(other.presentation_.document(), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		presentation_(other.presentation_), fontCollection_(other.fontCollection_),
		enablesDoubleBuffering_(other.enablesDoubleBuffering_), defaultFont_() {
	updateDefaultFont();
//	updateViewerSize(); ???
	presentation_.addDefaultTextStyleListener(*this);
}

/// Destructor.
TextRenderer::~TextRenderer() /*throw()*/ {
	presentation_.removeDefaultTextStyleListener(*this);
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

/// @see DefaultTextStyleListener#defaultTextLineStyleChanged
void TextRenderer::defaultTextLineStyleChanged(tr1::shared_ptr<const TextLineStyle>) {
}

/// @see DefaultTextStyleListener#defaultTextRunStyleChanged
void TextRenderer::defaultTextRunStyleChanged(tr1::shared_ptr<const TextRunStyle>) {
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
int TextRenderer::lineIndent(length_t line, length_t subline) const {
	const TextLayout& layout = lineLayout(line);
	const TextAlignment resolvedAlignment = resolveTextAlignment(layout.alignment(), layout.readingDirection());
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
 * @param clipRect The clipping region
 * @param colorOverride 
 * @param endOfLine 
 * @param lineWrappingMark 
 */
void TextRenderer::renderLine(length_t line, PaintContext& context,
		const Point<>& origin, const Rect<>& clipRect, ColorOverrideIterator* colorOverride /* = 0 */,
		const InlineObject* endOfLine /* = 0 */, const InlineObject* lineWrappingMark /* = 0 */) const /*throw()*/ {
	if(!enablesDoubleBuffering_) {
		lineLayout(line).draw(context, origin, clipRect, selection);
		return;
	}

	const TextLayout& layout = lineLayout(line);
	const Scalar dy = textMetrics().linePitch();

	// skip to the subline needs to draw
	const Scalar top = max(context.boundsToPaint().top(), clipRect.top());
	Scalar y = origin.y;
	length_t subline = (y + dy >= top) ? 0 : (top - (y + dy)) / dy;
	if(subline >= layout.numberOfLines())
		return;	// this logical line does not need to draw
	y += static_cast<Scalar>(dy * subline);

	if(memoryDC_.get() == 0)		
		memoryDC_.reset(::CreateCompatibleDC(renderingContext()->nativeHandle().get()), &::DeleteDC);
	const int horizontalResolution = calculateMemoryBitmapSize(context.device()->size().cx);
	if(memoryBitmap_.get() != 0) {
		BITMAP temp;
		::GetObjectW(memoryBitmap_.get(), sizeof(HBITMAP), &temp);
		if(temp.bmWidth < horizontalResolution)
			memoryBitmap_.reset();
	}
	if(memoryBitmap_.get() == 0)
		memoryBitmap_.reset(::CreateCompatibleBitmap(
			renderingContext()->nativeHandle().get(),
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

	invalidate();
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != 0) {
		BITMAP temp;
		::GetObjectW(memoryBitmap_.get(), sizeof(HBITMAP), &temp);
		if(temp.bmHeight != calculateMemoryBitmapSize(defaultFont()->metrics().linePitch()))
			memoryBitmap_.reset();
	}
	defaultFontListeners_.notify(&DefaultFontListener::defaultFontChanged);
}
