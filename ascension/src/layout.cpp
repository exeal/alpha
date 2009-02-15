/**
 * @file layout.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2009
 */

#include <ascension/layout.hpp>
#include <ascension/viewer.hpp>
#include <limits>	// std.numeric_limits
#include <usp10.h>
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::layout;
using namespace ascension::layout::internal;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace manah::win32;
using namespace manah::win32::gdi;
using namespace std;
using manah::toBoolean;

#pragma comment(lib, "usp10.lib")

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;

namespace {

	// SystemColor caches the system colors.
	class SystemColors {
	public:
		SystemColors() /*throw()*/ {update();}
		COLORREF get(int index) const {assert(index >= 0 && index < MANAH_COUNTOF(c_)); return c_[index];}
		COLORREF get(const Color& color, int index) const {return color.isValid() ? color.asCOLORREF() : get(index);}
		void update() /*throw()*/ {for(int i = 0; i < MANAH_COUNTOF(c_); ++i) c_[i] = ::GetSysColor(i);}
	private:
		COLORREF c_[128];
	} systemColors;

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	typedef ulong OPENTYPE_TAG;
	ASCENSION_DEFINE_SHARED_LIB_ENTRIES(Uniscribe16, 1);
	ASCENSION_SHARED_LIB_ENTRY(Uniscribe16, 0, "ScriptSubstituteSingleGlyph",
		HRESULT(WINAPI *signature)(HDC, SCRIPT_CACHE*, SCRIPT_ANALYSIS*,
			OPENTYPE_TAG, OPENTYPE_TAG, OPENTYPE_TAG, LONG, WORD, WORD*));
	auto_ptr<ascension::internal::SharedLibrary<Uniscribe16> > uspLib(new ascension::internal::SharedLibrary<Uniscribe16>("usp10.dll"));

	OPENTYPE_TAG makeOpenTypeTag(const char name[]) {
		const size_t len = strlen(name);
		assert(len > 0 && len <= 4);
		OPENTYPE_TAG tag = name[0];
		if(len > 1) tag |= name[1] << 8;
		if(len > 2) tag |= name[2] << 16;
		if(len > 3) tag |= name[3] << 24;
		return tag;
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	const class ScriptProperties {
	public:
		ScriptProperties() /*throw()*/ : p_(0), c_(0) {::ScriptGetProperties(&p_, &c_);}
		const SCRIPT_PROPERTIES& get(int script) const {if(script >= c_) throw out_of_range("script"); return *p_[script];}
		int numberOfOfScripts() const /*throw()*/ {return c_;}
	private:
		const SCRIPT_PROPERTIES** p_;
		int c_;
	} scriptProperties;

	class UserSettings {
	public:
		UserSettings() /*throw()*/ {update();}
		LANGID getDefaultLanguage() const /*throw()*/ {return langID_;}
		const SCRIPT_DIGITSUBSTITUTE& getDigitSubstitution() const /*throw()*/ {return digitSubstitution_;}
		void update() /*throw()*/ {langID_ = ::GetUserDefaultLangID();
			::ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &digitSubstitution_);}
	private:
		LANGID langID_;
		SCRIPT_DIGITSUBSTITUTE digitSubstitution_;
	} userSettings;

	inline bool isC0orC1Control(CodePoint c) /*throw()*/ {return c < 0x20 || c == 0x7f || (c >= 0x80 && c < 0xa0);}
	inline Orientation getLineTerminatorOrientation(const LayoutSettings& c) /*throw()*/ {
		switch(c.alignment) {
		case ALIGN_LEFT:	return LEFT_TO_RIGHT;
		case ALIGN_RIGHT:	return RIGHT_TO_LEFT;
		case ALIGN_CENTER:
		default:			return c.orientation;
		}
	}
	LANGID getUserCJKLanguage() /*throw()*/ {
		// this code is preliminary...
		static const WORD CJK_LANGUAGES[] = {LANG_CHINESE, LANG_JAPANESE, LANG_KOREAN};	// sorted by numeric values
		LANGID result = getUserDefaultUILanguage();
		if(find(CJK_LANGUAGES, MANAH_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != MANAH_ENDOF(CJK_LANGUAGES))
			return result;
		result = ::GetUserDefaultLangID();
		if(find(CJK_LANGUAGES, MANAH_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != MANAH_ENDOF(CJK_LANGUAGES))
			return result;
		result = ::GetSystemDefaultLangID();
		if(find(CJK_LANGUAGES, MANAH_ENDOF(CJK_LANGUAGES), PRIMARYLANGID(result)) != MANAH_ENDOF(CJK_LANGUAGES))
			return result;
		switch(::GetACP()) {
		case 932:	return MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
		case 936:	return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
		case 949:	return MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
		case 950:	return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
		}
		return result;
	}
} // namespace @0

struct ascension::layout::internal::Run : public StyledText {
	SCRIPT_ANALYSIS analysis;
	HFONT font;
	length_t firstCharacter, lastCharacter;
	int firstGlyph, lastGlyph;
	ABC width;
	struct SharedData {
		size_t numberOfReferences;
		SCRIPT_CACHE cache;
		manah::AutoBuffer<WORD> glyphs, clusters;
		manah::AutoBuffer<SCRIPT_VISATTR> visualAttributes;
		manah::AutoBuffer<int> advances, justifiedAdvances;
		manah::AutoBuffer<GOFFSET> glyphOffsets;
	} * shared;
	Run(const TextStyle& textStyle) /*throw()*/;
	~Run() /*throw()*/ {dispose();}
	const int* advances() const /*throw()*/ {return shared->advances.get() + firstGlyph;}
	const WORD* clusters() const /*throw()*/ {return shared->clusters.get() + firstCharacter;}
	void dispose() /*throw()*/;
	const int* justifiedAdvances() const /*throw()*/ {return (shared->justifiedAdvances.get() != 0) ? shared->justifiedAdvances.get() + firstGlyph : 0;}
	const WORD* glyphs() const /*throw()*/ {return shared->glyphs.get() + firstGlyph;}
	const GOFFSET* glyphOffsets() const /*throw()*/ {return shared->glyphOffsets.get() + firstGlyph;}
	length_t length() const /*throw()*/ {return lastCharacter - firstCharacter;}
	HRESULT logicalWidths(int widths[]) const /*throw()*/ {
		return ::ScriptGetLogicalWidths(&analysis, static_cast<int>(length()),
			numberOfGlyphs(), advances(), clusters(), visualAttributes(), widths);
	}
	int numberOfGlyphs() const /*throw()*/ {return lastGlyph - firstGlyph;}
	Orientation orientation() const /*throw()*/ {return ((analysis.s.uBidiLevel & 0x01) == 0x00) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;}
	bool overhangs() const /*throw()*/ {return width.abcA < 0 || width.abcC < 0;}
	void setLength(length_t newLength) {assert(shared->numberOfReferences == 1 && firstCharacter == 0); lastCharacter = newLength;}
	void setNumberOfGlyphs(int newNumber) {assert(shared->numberOfReferences == 1 && firstGlyph == 0); lastGlyph = newNumber;}
	auto_ptr<Run> split(DC& dc, length_t at);	// 'at' is from the beginning of the line
	int totalWidth() const /*throw()*/ {return width.abcA + width.abcB + width.abcC;}
	const SCRIPT_VISATTR* visualAttributes() const /*throw()*/ {return shared->visualAttributes.get() + firstGlyph;}
	int x(size_t offset, bool trailing) const {
		int result;
		const HRESULT hr = ::ScriptCPtoX(static_cast<int>(offset), trailing,
			static_cast<int>(length()), numberOfGlyphs(), clusters(),
			visualAttributes(), (justifiedAdvances() == 0) ? advances() : justifiedAdvances(), &analysis, &result);
		if(FAILED(hr))
			throw hr;
		return result;
	}
};

Run::Run(const TextStyle& textStyle) /*throw()*/ : firstCharacter(0), firstGlyph(0), shared(new SharedData) {
	style = textStyle;
	shared->numberOfReferences = 1;
	shared->cache = 0;
}

void Run::dispose() /*throw()*/ {
	if(--shared->numberOfReferences == 0) {
		if(shared->cache != 0) {
			::ScriptFreeCache(&shared->cache);
			shared->cache = 0;
		}
		delete shared;
	}
	shared = 0;
	font = 0;
	firstCharacter = firstGlyph = 0;
}

auto_ptr<Run> Run::split(DC& dc, length_t at) {
	assert(at > column && at < column + length());
	assert(clusters()[at - column] != clusters()[at - column - 1]);

	const bool rtl = orientation() == RIGHT_TO_LEFT;
	const length_t newLength = at - column;
	const int newNumberOfGlyphs = clusters()[!rtl ? newLength : (length() - newLength - 1)];
	assert(rtl == (analysis.fRTL == 1));

	// create the new following run
	auto_ptr<Run> following(new Run(*this));
	++shared->numberOfReferences;
	following->column = at;

	// split width
	if(newNumberOfGlyphs <= following->numberOfGlyphs()) {
		int w = 0;
		for(int i = 0; i < newNumberOfGlyphs; ++i)
			w += advances()[i];
		following->width.abcB = width.abcB - w;
	} else {
		int w = 0;
		for(int i = newNumberOfGlyphs; i < numberOfGlyphs(); ++i)
			w += advances()[i];
		following->width.abcB = w;
	}
	width.abcB -= following->width.abcB;
	if(!rtl)
		width.abcC = following->width.abcA = 0;
	else
		following->width.abcC = width.abcA = 0;

	// split logical data
	lastCharacter = firstCharacter + newLength;
	following->firstCharacter += newLength;
	assert(lastCharacter - firstCharacter == newLength);

	// split visual data
	if(!rtl) {
		lastGlyph = firstGlyph + newNumberOfGlyphs;
		following->firstGlyph += newNumberOfGlyphs;
	} else {
		firstGlyph = lastGlyph - newNumberOfGlyphs;
		following->lastGlyph -= newNumberOfGlyphs;
	}
	assert(lastGlyph - firstGlyph == newNumberOfGlyphs);

	// update clusters array
	Run& target = !rtl ? *following : *this;
	WORD* const cl = shared->clusters.get();
	transform(cl + target.firstCharacter, cl + target.lastCharacter, cl + target.firstCharacter,
		bind2nd(minus<WORD>(), cl[!rtl ? target.firstCharacter : (target.lastCharacter - 1)]));

	// update placements
	HRESULT hr = ::ScriptPlace(dc.use(), &shared->cache, glyphs(), numberOfGlyphs(),
		visualAttributes(), &analysis, const_cast<int*>(advances()), const_cast<GOFFSET*>(glyphOffsets()), &width);
	hr = ::ScriptPlace(dc.get(), &shared->cache, following->glyphs(), following->numberOfGlyphs(),
		following->visualAttributes(), &analysis, const_cast<int*>(following->advances()),
		const_cast<GOFFSET*>(following->glyphOffsets()), &following->width);

	return following;
}

void dumpRuns(const LineLayout& layout) {
#ifdef _DEBUG
	ostringstream s;
	layout.dumpRuns(s);
	::OutputDebugStringA(s.str().c_str());
#endif // _DEBUG
}

void ascension::updateSystemSettings() /*throw()*/ {
	systemColors.update();
	userSettings.update();
}

/**
 * Returns metrics of underline and/or strikethrough for the currently selected font.
 * @param dc the device context
 * @param[out] baselineOffset the baseline position relative to the top in pixels
 * @param[out] underlineOffset the underline position relative to the baseline in pixels
 * @param[out] underlineThickness the thickness of underline in pixels
 * @param[out] strikethroughOffset the linethrough position relative to the baseline in pixels
 * @param[out] strikethroughThickness the thickness of linethrough in pixels
 * @return succeeded or not
 */
bool layout::getDecorationLineMetrics(HDC dc, int* baselineOffset,
		int* underlineOffset, int* underlineThickness, int* strikethroughOffset, int* strikethroughThickness) /*throw()*/ {
	OUTLINETEXTMETRICW* otm = 0;
	TEXTMETRICW tm;
	if(const UINT c = ::GetOutlineTextMetricsW(dc, 0, 0)) {
		otm = static_cast<OUTLINETEXTMETRICW*>(::operator new(c));
		if(!toBoolean(::GetOutlineTextMetricsW(dc, c, otm)))
			return false;
	} else if(!toBoolean(::GetTextMetricsW(dc, &tm)))
		return false;
	const int baseline = (otm != 0) ? otm->otmTextMetrics.tmAscent : tm.tmAscent;
	if(baselineOffset != 0)
		*baselineOffset = baseline;
	if(underlineOffset != 0)
		*underlineOffset = (otm != 0) ? otm->otmsUnderscorePosition : baseline;
	if(underlineThickness != 0)
		*underlineThickness = (otm != 0) ? otm->otmsUnderscoreSize : 1;
	if(strikethroughOffset != 0)
		*strikethroughOffset = (otm != 0) ? otm->otmsStrikeoutPosition : (baseline / 3);
	if(strikethroughThickness != 0)
		*strikethroughThickness = (otm != 0) ? otm->otmsStrikeoutSize : 1;
	::operator delete(otm);
	return true;
}

/// Returns true if complex scripts are supported.
bool layout::supportsComplexScripts() /*throw()*/ {
	return true;
}

/// Returns true if OpenType features are supported.
bool layout::supportsOpenTypeFeatures() /*throw()*/ {
	return uspLib->get<0>() != 0;
}


// LineLayout ///////////////////////////////////////////////////////////////

// helpers for LineLayout.draw
namespace {
	const size_t MAXIMUM_RUN_LENGTH = 1024;
	inline HPEN createPen(COLORREF color, int width, int style) {
		LOGBRUSH brush;
		brush.lbColor = color;
		brush.lbStyle = BS_SOLID;
		switch(style) {
		case 1:	// solid
			return (width == 1) ? ::CreatePen(PS_SOLID, 1, color) : ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 2:	// dashed
			return ::ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		case 3:	// dotted
			return ::ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_ENDCAP_FLAT, width, &brush, 0, 0);
		}
		throw UnknownValueException("style");
	}
	inline void drawDecorationLines(DC& dc, const TextStyle& style, COLORREF foregroundColor, int x, int y, int width, int height) {
		if(style.underlineStyle != NO_UNDERLINE || style.strikeout) {
			int baselineOffset, underlineOffset, underlineThickness, linethroughOffset, linethroughThickness;
			if(getDecorationLineMetrics(dc.get(), &baselineOffset, &underlineOffset, &underlineThickness, &linethroughOffset, &linethroughThickness)) {
				// draw underline
				if(style.underlineStyle != NO_UNDERLINE) {
					HPEN oldPen = dc.selectObject(createPen(style.underlineColor.isValid() ?
						style.underlineColor.asCOLORREF() : foregroundColor, underlineThickness, style.underlineStyle));
					const int underlineY = y + baselineOffset - underlineOffset + underlineThickness / 2;
					dc.moveTo(x, underlineY);
					dc.lineTo(x + width, underlineY);
					::DeleteObject(dc.selectObject(oldPen));
				}
				// draw strikeout line
				if(style.strikeout) {
					HPEN oldPen = dc.selectObject(createPen(foregroundColor, linethroughThickness, 1));
					const int strikeoutY = y + baselineOffset - linethroughOffset + linethroughThickness / 2;
					dc.moveTo(x, strikeoutY);
					dc.lineTo(x + width, strikeoutY);
					::DeleteObject(dc.selectObject(oldPen));
				}
			}
		}

		// draw border
		if(style.borderStyle != NO_BORDER) {
			HPEN oldPen = dc.selectObject(createPen(
				style.borderColor.isValid() ? style.borderColor.asCOLORREF() : foregroundColor, 1, style.borderStyle));
			HBRUSH oldBrush = dc.selectObject(static_cast<HBRUSH>(::GetStockObject(NULL_BRUSH)));
			dc.rectangle(x, y, x + width, y + height);
			::DeleteObject(dc.selectObject(oldPen));
			dc.selectObject(oldBrush);
		}
	}
} // namespace @0

/**
 * @class ascension::layout::LineLayout
 * @c LineLayout represents a layout of styled line text. Provides support for drawing, cursor
 * navigation, hit testing, text wrapping, etc.
 *
 * A long run will be split into smaller runs automatically because Uniscribe rejects too long text
 * (especially @c ScriptShape and @c ScriptTextOut). For this reason, a combining character will be
 * rendered incorrectly if it is presented at the boundary. The maximum length of a run is 1024.
 *
 * In present, this class supports only text layout horizontal against the output device.
 *
 * @note This class is not intended to derive.
 * @see LineLayoutBuffer#lineLayout, LineLayoutBuffer::lineLayoutIfCached
 */

/**
 * Constructor.
 * @param layoutInformation the layout information
 * @param line the line
 * @throw kernel#BadPositionException @a line is invalid
 */
LineLayout::LineLayout(const ILayoutInformationProvider& layoutInformation, length_t line) : lip_(layoutInformation), lineNumber_(line),
		runs_(0), numberOfRuns_(0), sublineOffsets_(0), sublineFirstRuns_(0), numberOfSublines_(0), longestSublineWidth_(-1), wrapWidth_(-1) {
	// calculate the wrapping width
	if(layoutInformation.getLayoutSettings().lineWrap.wraps()) {
		wrapWidth_ = layoutInformation.getWidth();
		if(ISpecialCharacterRenderer* scr = lip_.getSpecialCharacterRenderer()) {
			auto_ptr<DC> dc(lip_.getFontSelector().deviceContext());
			ISpecialCharacterRenderer::LayoutContext context(*dc);
			context.orientation = lip_.getLayoutSettings().orientation;
			wrapWidth_ -= scr->getLineWrappingMarkWidth(context);
		}
	}
	// construct the layout
	if(!text().empty()) {
		itemize(line);
		shape();
		if(numberOfRuns_ == 0 || wrapWidth_ == -1) {
			numberOfSublines_ = 1;
			sublineFirstRuns_ = new size_t[1];
			sublineFirstRuns_[0] = 0;
			reorder();
			expandTabsWithoutWrapping();
		} else {
			wrap();
			reorder();
			if(lip_.getLayoutSettings().justifiesLines)
				justify();
		}
	} else {	// an empty line
		numberOfRuns_ = 0;
		numberOfSublines_ = 1;
		longestSublineWidth_ = 0;
	}
}

/// Destructor.
LineLayout::~LineLayout() /*throw()*/ {
	dispose();
}

/**
 * Returns the bidirectional embedding level at specified position.
 * @param column the column
 * @return the embedding level
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
ascension::byte LineLayout::bidiEmbeddingLevel(length_t column) const {
	if(numberOfRuns_ == 0) {
		if(column != 0)
			throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
		// use the default level
		return (lip_.getLayoutSettings().orientation == RIGHT_TO_LEFT) ? 1 : 0;
	}
	const size_t i = findRunForPosition(column);
	if(i == numberOfRuns_)
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	return static_cast<uchar>(runs_[i]->analysis.s.uBidiLevel);
}

/**
 * Returns the black box bounds of the characters in the specified range. The black box bounds is
 * an area consisting of the union of the bounding boxes of the all of the characters in the range.
 * The result region can be disjoint.
 * @param first the start of the range
 * @param last the end of the range
 * @return the Win32 GDI region object encompasses the black box bounds. the coordinates are based
 *         on the left-top of the first visual subline in the layout
 * @throw kernel#BadPositionException @a first or @a last is greater than the length of the line
 * @throw std#invalid_argument @a first is greater than @a last
 * @see #bounds(void), #bounds(length_t, length_t), #sublineBounds, #sublineIndent
 */
Rgn LineLayout::blackBoxBounds(length_t first, length_t last) const {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	else if(last > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, last));

	// handle empty line
	if(numberOfRuns_ == 0)
		return Rgn::createRect(0, 0, 0, linePitch());

	const length_t firstSubline = subline(first), lastSubline = subline(last);
	vector<RECT> rectangles;
	RECT rectangle;
	rectangle.top = 0;
	rectangle.bottom = rectangle.top + linePitch();
	for(length_t subline = firstSubline; subline <= lastSubline;
			++subline, rectangle.top = rectangle.bottom, rectangle.bottom += linePitch()) {
		const size_t endOfRuns = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		int cx = sublineIndent(subline);
		if(first <= runs_[sublineFirstRuns_[subline]]->column
				&& last >= runs_[endOfRuns - 1]->column + runs_[endOfRuns - 1]->length()) {
			// whole visual subline is encompassed by the range
			rectangle.left = cx;
			rectangle.right = rectangle.left + sublineWidth(subline);
			rectangles.push_back(rectangle);
			continue;
		}
		for(size_t i = sublineFirstRuns_[subline]; i < endOfRuns; ++i) {
			const Run& run = *runs_[i];
			if(first <= run.column + run.length() && last >= run.column) {
				rectangle.left = cx + ((first > run.column) ?
					run.x(first - run.column, false) : ((run.orientation() == LEFT_TO_RIGHT) ? 0 : run.totalWidth()));
				rectangle.right = cx + ((last < run.column + run.length()) ?
					run.x(last - run.column, false) : ((run.orientation() == LEFT_TO_RIGHT) ? run.totalWidth() : 0));
				if(rectangle.left != rectangle.right) {
					if(rectangle.left > rectangle.right)
						swap(rectangle.left, rectangle.right);
					rectangles.push_back(rectangle);
				}
			}
			cx += run.totalWidth();
		}
	}

	// create the result region
	manah::AutoBuffer<POINT> vertices(new POINT[rectangles.size() * 4]);
	manah::AutoBuffer<int> numbersOfVertices(new int[rectangles.size()]);
	for(size_t i = 0, c = rectangles.size(); i < c; ++i) {
		vertices[i * 4 + 0].x = vertices[i * 4 + 3].x = rectangles[i].left;
		vertices[i * 4 + 0].y = vertices[i * 4 + 1].y = rectangles[i].top;
		vertices[i * 4 + 1].x = vertices[i * 4 + 2].x = rectangles[i].right;
		vertices[i * 4 + 2].y = vertices[i * 4 + 3].y = rectangles[i].bottom;
	}
	fill_n(numbersOfVertices.get(), rectangles.size(), 4);
	return Rgn::createPolyPolygon(vertices.get(), numbersOfVertices.get(), static_cast<int>(rectangles.size()), WINDING);
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent or overhangs of the text.
 * @return the size of the bounds
 * @see #blackBoxBounds, #bounds(length_t, length_t), #sublineBounds
 */
SIZE LineLayout::bounds() const /*throw()*/ {
	SIZE s;
	s.cx = longestSublineWidth();
	s.cy = static_cast<long>(linePitch() * numberOfSublines_);
	return s;
}

/**
 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
 * exactly the ascent, descent or overhangs of the specified region of the text.
 * @param first the start of the range
 * @param last the end of the range
 * @return the rectangle whose @c left value is the indentation of the bounds and @c top value is
 *         the distance from the top of the whole line
 * @throw kernel#BadPositionException @a first or @a last is greater than the length of the line
 * @throw std#invalid_argument @a first is greater than @a last
 * @see #blackBoxBounds, #bounds(void), #sublineBounds, #sublineIndent
 */
RECT LineLayout::bounds(length_t first, length_t last) const {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	else if(last > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, last));
	RECT bounds;	// the result
	int cx;

	// handle empty line
	if(numberOfRuns_ == 0) {
		bounds.left = bounds.top = bounds.right = 0;
		bounds.bottom = bounds.top + linePitch();
		return bounds;
	}

	// determine the top and the bottom (it's so easy)
	const length_t firstSubline = subline(first), lastSubline = subline(last);
	bounds.top = static_cast<LONG>(linePitch() * firstSubline);
	bounds.bottom = static_cast<LONG>(linePitch() * (lastSubline + 1));

	// find side bounds between 'firstSubline' and 'lastSubline'
	bounds.left = numeric_limits<LONG>::max();
	bounds.right = numeric_limits<LONG>::min();
	for(length_t subline = firstSubline + 1; subline < lastSubline; ++subline) {
		const LONG indent = sublineIndent(subline);
		bounds.left = min(indent, bounds.left);
		bounds.right = max(indent + sublineWidth(subline), bounds.right);
	}

	// find side bounds in 'firstSubline' and 'lastSubline'
	const length_t firstAndLast[2] = {firstSubline, lastSubline};
	for(size_t i = 0; i < MANAH_COUNTOF(firstAndLast); ++i) {
		const length_t subline = firstAndLast[i];
		const size_t endOfRuns = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		// find left bound
		cx = sublineIndent(subline);
		for(size_t j = sublineFirstRuns_[subline]; j < endOfRuns; ++j) {
			if(cx >= bounds.left)
				break;
			const Run& run = *runs_[j];
			if(first <= run.column + run.length() && last >= run.column) {
				const int x = run.x(((run.orientation() == LEFT_TO_RIGHT) ?
					max(first, run.column) : min(last, run.column + run.length())) - run.column, false);
				bounds.left = min<LONG>(cx + x, bounds.left);
				break;
			}
			cx += run.totalWidth();
		}
		// find right bound
		cx = sublineIndent(firstSubline) + sublineWidth(lastSubline);
		for(size_t j = endOfRuns - 1; ; --j) {
			if(cx <= bounds.right)
				break;
			const Run& run = *runs_[j];
			if(first <= run.column + run.length() && last >= run.column) {
				const int x = run.x(((run.orientation() == LEFT_TO_RIGHT) ?
					min(last, run.column + run.length()) : max(first, run.column)) - run.column, false);
				bounds.right = max<LONG>(cx - run.totalWidth() + x, bounds.right);
				break;
			}
			if(j == sublineFirstRuns_[subline])
				break;
			cx -= run.totalWidth();
		}
	}

	return bounds;
}

/// Disposes the layout.
inline void LineLayout::dispose() /*throw()*/ {
	for(size_t i = 0; i < numberOfRuns_; ++i)
		delete runs_[i];
	delete[] runs_;
	runs_ = 0;
	numberOfRuns_ = 0;
	delete[] sublineOffsets_;
	delete[] sublineFirstRuns_;
	sublineFirstRuns_ = 0;
	numberOfSublines_ = 0;
}

/**
 * Draws the layout to the output device.
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection defines the region and the color of the selection
 */
void LineLayout::draw(DC& dc, int x, int y, const RECT& paintRect, const RECT& clipRect, const Selection* selection) const /*throw()*/ {
	const int dy = linePitch();

	// empty line
	if(isDisposed()) {
		RECT r;
		r.left = max(paintRect.left, clipRect.left);
		r.top = max(clipRect.top, max<long>(paintRect.top, y));
		r.right = min(paintRect.right, clipRect.right);
		r.bottom = min(clipRect.bottom, min<long>(paintRect.bottom, y + dy));
		const Colors lineColor = lip_.getPresentation().getLineColor(lineNumber_);
		dc.fillSolidRect(r, systemColors.get(lineColor.background, COLOR_WINDOW));
		return;
	}

	// skip to the subline needs to draw
	length_t subline = (y + dy >= paintRect.top) ? 0 : (paintRect.top - (y + dy)) / dy;
	if(subline >= numberOfSublines_)
		return;	// this logical line does not need to draw
	y += static_cast<int>(dy * subline);

	for(; subline < numberOfSublines_; ++subline) {
		draw(subline, dc, x, y, paintRect, clipRect, selection);
		if((y += dy) >= paintRect.bottom)	// to next subline
			break;
	}
}

/**
 * Draws the specified subline layout to the output device.
 * @param subline the visual subline
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection defines the region and the color of the selection. can be @c null
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
void LineLayout::draw(length_t subline, DC& dc,
		int x, int y, const RECT& paintRect, const RECT& clipRect, const Selection* selection) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		manah::win32::DumpContext() << L"@LineLayout.draw draws line " << lineNumber_ << L" (" << subline << L")\n";
#endif // _DEBUG

	// the following topic describes how to draw a selected text using masking by clipping
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/editor10.asp)

	const int dy = linePitch();
	const int lineHeight = lip_.getFontSelector().lineHeight();
	const Colors lineColor = lip_.getPresentation().getLineColor(lineNumber_);
	const COLORREF marginColor = systemColors.get(lineColor.background, COLOR_WINDOW);
	ISpecialCharacterRenderer::DrawingContext context(dc);
	ISpecialCharacterRenderer* specialCharacterRenderer = lip_.getSpecialCharacterRenderer();

	if(specialCharacterRenderer != 0) {
		context.rect.top = y;
		context.rect.bottom = y + lineHeight;
	}

	const int savedCookie = dc.save();
	dc.setTextAlign(TA_BASELINE | TA_LEFT | TA_NOUPDATECP);
	if(isDisposed()) {	// empty line
		RECT r;
		r.left = max(paintRect.left, clipRect.left);
		r.top = max(clipRect.top, max<long>(paintRect.top, y));
		r.right = min(paintRect.right, clipRect.right);
		r.bottom = min(clipRect.bottom, min<long>(paintRect.bottom, y + dy));
		dc.fillSolidRect(r, marginColor);
	} else {
		const String& line = text();
		HRESULT hr;
		length_t selStart, selEnd;
		if(selection != 0) {
			if(!selection->caret().selectedRangeOnVisualLine(lineNumber_, subline, selStart, selEnd))
				selection = 0;
		}

		// paint between sublines
		Rgn clipRegion(Rgn::createRect(clipRect.left, max<long>(y, clipRect.top), clipRect.right, min<long>(y + dy, clipRect.bottom)));
//		dc.selectClipRgn(clipRegion.getHandle());
		if(dy - lineHeight > 0)
			dc.fillSolidRect(paintRect.left, y + lineHeight, paintRect.right - paintRect.left, dy - lineHeight, marginColor);

		x += sublineIndent(subline);

		// 1. paint background of the runs
		// 2. determine the first and the last runs need to draw
		// 3. mask selected region
		size_t firstRun = sublineFirstRuns_[subline];
		size_t lastRun = (subline < numberOfSublines_ - 1) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		// paint the left margin
		if(x > paintRect.left)
			dc.fillSolidRect(paintRect.left, y, x - paintRect.left, lineHeight, marginColor);
		// paint background of the runs
		int startX = x;
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			if(x + run.totalWidth() < paintRect.left) {	// this run does not need to draw
				++firstRun;
				startX = x + run.totalWidth();
			} else {
				const COLORREF bgColor = lineColor.background.isValid() ?
					marginColor : systemColors.get(run.style.color.background, COLOR_WINDOW);
				if(selection == 0 || run.column >= selEnd || run.column + run.length() <= selStart)
					// no selection in this run
					dc.fillSolidRect(x, y, run.totalWidth(), lineHeight, bgColor);
				else if(selection != 0 && run.column >= selStart && run.column + run.length() <= selEnd) {
					// this run is selected entirely
					dc.fillSolidRect(x, y, run.totalWidth(), lineHeight, selection->color().background.asCOLORREF());
					dc.excludeClipRect(x, y, x + run.totalWidth(), y + lineHeight);
				} else {
					// selected partially
					int left = run.x(max(selStart, run.column) - run.column, false);
					int right = run.x(min(selEnd, run.column + run.length()) - 1 - run.column, true);
					if(left > right)
						swap(left, right);
					left += x;
					right += x;
					if(left > x/* && left > paintRect.left*/)
						dc.fillSolidRect(x, y, left - x, lineHeight, bgColor);
					if(right > left) {
						dc.fillSolidRect(left, y, right - left, lineHeight, selection->color().background.asCOLORREF());
						dc.excludeClipRect(left, y, right, y + lineHeight);
					}
					if(right < x + run.totalWidth())
						dc.fillSolidRect(right, y, run.totalWidth() - (left - x), lineHeight, bgColor);
				}
			}
			x += run.totalWidth();
			if(x >= paintRect.right) {
				lastRun = i + 1;
				break;
			}
		}
		// paint the right margin
		if(x < paintRect.right)
			dc.fillSolidRect(x, y, paintRect.right - x, dy, marginColor);

		// draw outside of the selection
		RECT runRect;
		runRect.top = y;
		runRect.bottom = y + dy;
		runRect.left = x = startX;
		dc.setBkMode(TRANSPARENT);
		for(size_t i = firstRun; i < lastRun; ++i) {
			Run& run = *runs_[i];
			const COLORREF foregroundColor = lineColor.foreground.isValid() ?
				lineColor.foreground.asCOLORREF() : systemColors.get(run.style.color.foreground, COLOR_WINDOWTEXT);
			if(line[run.column] != L'\t') {
				if(selection == 0 || run.overhangs() || !(run.column >= selStart && run.column + run.length() <= selEnd)) {
					dc.selectObject(run.font);
					dc.setTextColor(foregroundColor);
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = ::ScriptTextOut(dc.get(), &run.shared->cache, x, y + lip_.getFontSelector().ascent(),
						0, &runRect, &run.analysis, 0, 0, run.glyphs(), run.numberOfGlyphs(),
						run.advances(), run.justifiedAdvances(), run.glyphOffsets());
				}
			}
			// decoration (underline and border)
			drawDecorationLines(dc, run.style, foregroundColor, x, y, run.totalWidth(), dy);
			x += run.totalWidth();
			runRect.left = x;
		}

		// draw selected text segment (also underline and border)
		if(selection != 0) {
			x = startX;
			clipRegion.setRect(clipRect);
			dc.selectClipRgn(clipRegion.get(), RGN_XOR);
			for(size_t i = firstRun; i < lastRun; ++i) {
				Run& run = *runs_[i];
				// text
				if(selection != 0 && line[run.column] != L'\t'
						&& (run.overhangs() || (run.column < selEnd && run.column + run.length() > selStart))) {
					dc.selectObject(run.font);
					dc.setTextColor(selection->color().foreground.asCOLORREF());
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = ::ScriptTextOut(dc.get(), &run.shared->cache, x, y + lip_.getFontSelector().ascent(),
						0, &runRect, &run.analysis, 0, 0, run.glyphs(), run.numberOfGlyphs(),
						run.advances(), run.justifiedAdvances(), run.glyphOffsets());
				}
				// decoration (underline and border)
				drawDecorationLines(dc, run.style, selection->color().foreground.asCOLORREF(), x, y, run.totalWidth(), dy);
				x += run.totalWidth();
			}
		}

		// special character substitution
		if(specialCharacterRenderer != 0) {
			// white spaces and C0/C1 control characters
			dc.selectClipRgn(clipRegion.get());
			x = startX;
			for(size_t i = firstRun; i < lastRun; ++i) {
				Run& run = *runs_[i];
				context.orientation = run.orientation();
				for(length_t j = run.column; j < run.column + run.length(); ++j) {
					if(BinaryProperty::is(line[j], BinaryProperty::WHITE_SPACE)) {	// IdentifierSyntax.isWhiteSpace() is preferred?
						context.rect.left = x + run.x(j - run.column, false);
						context.rect.right = x + run.x(j - run.column, true);
						if(context.rect.left > context.rect.right)
							swap(context.rect.left, context.rect.right);
						specialCharacterRenderer->drawWhiteSpaceCharacter(context, line[j]);
					} else if(isC0orC1Control(line[j])) {
						context.rect.left = x + run.x(j - run.column, false);
						context.rect.right = x + run.x(j - run.column, true);
						if(context.rect.left > context.rect.right)
							swap(context.rect.left, context.rect.right);
						specialCharacterRenderer->drawControlCharacter(context, line[j]);
					}
				}
				x += run.totalWidth();
			}
		}
		if(subline == numberOfSublines_ - 1 && lip_.getLayoutSettings().alignment == ALIGN_RIGHT)
			x = startX;
	} // end of nonempty line case
	
	// line terminator and line wrapping mark
	const Document& document = lip_.getPresentation().document();
	if(specialCharacterRenderer != 0) {
		context.orientation = getLineTerminatorOrientation(lip_.getLayoutSettings());
		if(subline < numberOfSublines_ - 1) {	// line wrapping mark
			const int markWidth = specialCharacterRenderer->getLineWrappingMarkWidth(context);
			if(context.orientation == LEFT_TO_RIGHT) {
				context.rect.right = lip_.getWidth();
				context.rect.left = context.rect.right - markWidth;
			} else {
				context.rect.left = 0;
				context.rect.right = markWidth;
			}
			specialCharacterRenderer->drawLineWrappingMark(context);
		} else if(lineNumber_ < document.numberOfLines() - 1) {	// line teminator
			const kernel::Newline nlf = document.getLineInformation(lineNumber_).newline();
			const int nlfWidth = specialCharacterRenderer->getLineTerminatorWidth(context, nlf);
			if(context.orientation == LEFT_TO_RIGHT) {
				context.rect.left = x;
				context.rect.right = x + nlfWidth;
			} else {
				context.rect.left = x - nlfWidth;
				context.rect.right = x;
			}
			if(selection != 0) {
				const Position eol(lineNumber_, document.lineLength(lineNumber_));
				if(!selection->caret().isSelectionRectangle()
						&& selection->caret().beginning().position() <= eol
						&& selection->caret().end().position() > eol)
					dc.fillSolidRect(x - (context.orientation == RIGHT_TO_LEFT ? nlfWidth : 0),
						y, nlfWidth, dy, selection->color().background.asCOLORREF());
			}
			dc.setBkMode(TRANSPARENT);
			specialCharacterRenderer->drawLineTerminator(context, nlf);
		}
	}

	dc.restore(savedCookie);
}

#ifdef _DEBUG
/**
 * Dumps the all runs to the specified output stream.
 * @param out the output stream
 */
void LineLayout::dumpRuns(ostream& out) const {
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		const Run& run = *runs_[i];
		out << static_cast<uint>(i)
			<< ":column=" << static_cast<uint>(run.column)
			<< ",length=" << static_cast<uint>(run.length()) << endl;
	}
}
#endif // _DEBUG

/// Expands the all tabs and resolves each width.
inline void LineLayout::expandTabsWithoutWrapping() /*throw()*/ {
	const bool rtl = getLineTerminatorOrientation(lip_.getLayoutSettings()) == RIGHT_TO_LEFT;
	const String& line = text();
	int x = 0;
	Run* run;
	if(!rtl) {	// expand from the left most
		for(size_t i = 0; i < numberOfRuns_; ++i) {
			run = runs_[i];
			if(run->length() == 1 && line[run->column] == L'\t') {
				run->shared->advances[run->firstGlyph] = nextTabStop(x, Direction::FORWARD) - x;
				run->width.abcB = run->advances()[0];
				run->width.abcA = run->width.abcC = 0;
			}
			x += run->totalWidth();
		}
	} else {	// expand from the right most
		for(size_t i = numberOfRuns_; i > 0; --i) {
			run = runs_[i - 1];
			if(run->length() == 1 && line[run->column] == L'\t') {
				run->shared->advances[run->firstGlyph] = nextTabStop(x, Direction::FORWARD) - x;
				run->width.abcB = run->advances()[0];
				run->width.abcA = run->width.abcC = 0;
			}
			x += run->totalWidth();
		}
	}
	longestSublineWidth_ = x;
}

/**
 * Returns the space string added to the end of the specified line to reach the specified virtual
 * point. If the end of the line is over @a virtualX, the result is an empty string.
 * @param x the x-coordinate of the virtual point
 * @return the space string consists of only white spaces (U+0020) and horizontal tabs (U+0009)
 * @throw kernel#BadPositionException @a line is outside of the document
 * @deprecated 0.8
 * @note This does not support line wrapping and bidirectional context.
 */
String LineLayout::fillToX(int x) const {
	int cx = longestSublineWidth();
	if(cx >= x)
		return L"";

	size_t numberOfTabs = 0;
	while(true) {
		const int next = nextTabStopBasedLeftEdge(cx, true);
		if(next > x)
			break;
		++numberOfTabs;
		cx = next;
	}

	if(cx == x)
		return String(numberOfTabs, L'\t');

	auto_ptr<DC> dc = lip_.getFontSelector().deviceContext();
	HFONT oldFont = dc->selectObject(lip_.getFontSelector().font(Script::COMMON));
	int spaceWidth;
	dc->getCharWidth(L' ', L' ', &spaceWidth);
	dc->selectObject(oldFont);
	size_t numberOfSpaces = 0;
	while(true) {
		if(cx + spaceWidth > x)
			break;
		++numberOfSpaces;
		cx += spaceWidth;
	}

	String result(numberOfTabs + numberOfSpaces, L' ');
	result.replace(0, numberOfTabs, numberOfTabs, L'\t');
	return result;
}

/**
 * Returns the index of run containing the specified column.
 * @param column the column
 * @return the index of the run
 */
inline size_t LineLayout::findRunForPosition(length_t column) const /*throw()*/ {
	assert(numberOfRuns_ > 0);
	if(column == text().length())
		return numberOfRuns_ - 1;
	const length_t sl = subline(column);
	const size_t lastRun = (sl + 1 < numberOfSublines_) ? sublineFirstRuns_[sl + 1] : numberOfRuns_;
	for(size_t i = sublineFirstRuns_[sl]; i < lastRun; ++i) {
		if(runs_[i]->column <= column && runs_[i]->column + runs_[i]->length() > column)
			return i;
	}
	assert(false);
	return lastRun - 1;	// never reachable...
}

/// Returns an iterator addresses the first styled segment.
LineLayout::StyledSegmentIterator LineLayout::firstStyledSegment() const /*throw()*/ {
	const Run* temp = *runs_;
	return StyledSegmentIterator(temp);
}

/// Returns if the line contains right-to-left run.
bool LineLayout::isBidirectional() const /*throw()*/ {
	if(lip_.getLayoutSettings().orientation == RIGHT_TO_LEFT)
		return true;
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(runs_[i]->orientation() == RIGHT_TO_LEFT)
			return true;
	}
	return false;
}

/**
 * Itemizes the text into shapable runs.
 * @param lineNumber the line number of the line
 */
inline void LineLayout::itemize(length_t lineNumber) /*throw()*/ {
	const String& line = text();
	assert(!line.empty());

	HRESULT hr;
	const LayoutSettings& c = lip_.getLayoutSettings();
	const Presentation& presentation = lip_.getPresentation();

	// configure
	AutoZero<SCRIPT_CONTROL> control;
	AutoZero<SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (c.orientation == RIGHT_TO_LEFT) ? 1 : 0;
//	initialState.fOverrideDirection = 1;
	initialState.fInhibitSymSwap = c.inhibitsSymmetricSwapping;
	initialState.fDisplayZWG = c.displaysShapingControls;
	switch(c.digitSubstitutionType) {
	case DST_NOMINAL:
		initialState.fDigitSubstitute = 0;
		break;
	case DST_NATIONAL:
		control.uDefaultLanguage = userSettings.getDefaultLanguage();
		initialState.fDigitSubstitute = 1;
		break;
	case DST_CONTEXTUAL:
		control.uDefaultLanguage = userSettings.getDefaultLanguage();
		control.fContextDigits = 1;
		initialState.fDigitSubstitute = 1;
		break;
	case DST_USER_DEFAULT:
		hr = ::ScriptApplyDigitSubstitution(&userSettings.getDigitSubstitution(), &control, &initialState);
		break;
	}

	// itemize
	static SCRIPT_ITEM fastItems[128];
	int expectedNumberOfRuns = max(static_cast<int>(line.length()) / 8, 2);
	SCRIPT_ITEM* items;
	int numberOfItems;
	if(expectedNumberOfRuns <= MANAH_COUNTOF(fastItems)) {
		hr = ::ScriptItemize(line.data(), static_cast<int>(line.length()),
			expectedNumberOfRuns = MANAH_COUNTOF(fastItems), &control, &initialState, items = fastItems, &numberOfItems);
		if(hr == E_OUTOFMEMORY)
			expectedNumberOfRuns *= 2;
	} else
		hr = E_OUTOFMEMORY;
	if(hr == E_OUTOFMEMORY) {
		while(true) {
			items = new SCRIPT_ITEM[expectedNumberOfRuns];
			hr = ::ScriptItemize(line.data(), static_cast<int>(line.length()),
				expectedNumberOfRuns, &control, &initialState, items, &numberOfItems);
			if(hr != E_OUTOFMEMORY)	// expectedNumberOfRuns was enough...
				break;
			delete[] items;
			expectedNumberOfRuns *= 2;
		}
	}
	if(c.disablesDeprecatedFormatCharacters) {
		for(int i = 0; i < numberOfItems; ++i) {
			items[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
			items[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
		}
	}

	// style
	bool mustDelete;
	const LineStyle& styles = presentation.getLineStyle(lineNumber, mustDelete);
	if(&styles != &LineStyle::NULL_STYLE) {
#ifdef _DEBUG
		// verify the given styles
		for(size_t i = 0; i < styles.count - 1; ++i)
			assert(styles.array[i].column < styles.array[i + 1].column);
#endif // _DEBUG
		merge(items, numberOfItems, styles);
		if(mustDelete) {
			delete[] styles.array;
			delete &styles;
		}
	} else {
		StyledText segment;
		segment.column = 0;
		LineStyle simpleStyle;
		simpleStyle.array = &segment;
		simpleStyle.count = 1;
		merge(items, numberOfItems, simpleStyle);
	}

	if(items != fastItems)
		delete[] items;
}

/// Justifies the wrapped visual lines.
inline void LineLayout::justify() /*throw()*/ {
	assert(wrapWidth_ != -1);
	for(length_t subline = 0; subline < numberOfSublines_; ++subline) {
		const int lineWidth = sublineWidth(subline);
		const size_t last = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		for(size_t i = sublineFirstRuns_[subline]; i < last; ++i) {
			Run& run = *runs_[i];
			const int newRunWidth = ::MulDiv(run.totalWidth(), wrapWidth_, lineWidth);	// TODO: there is more precise way.
			if(newRunWidth != run.totalWidth()) {
				run.shared->justifiedAdvances.reset(new int[run.numberOfGlyphs()]);
				::ScriptJustify(run.visualAttributes(), run.advances(),
					run.numberOfGlyphs(), newRunWidth - run.totalWidth(), 2, run.shared->justifiedAdvances.get());
				run.width.abcB += newRunWidth - run.totalWidth();
			}
		}
	}
}

/// Returns an iterator addresses the last styled segment.
LineLayout::StyledSegmentIterator LineLayout::lastStyledSegment() const /*throw()*/ {
	const Run* temp = runs_[numberOfRuns_];
	return StyledSegmentIterator(temp);
}

/// Returns the line pitch in pixels.
inline int LineLayout::linePitch() const /*throw()*/ {
	return lip_.getFontSelector().lineHeight() + max(lip_.getLayoutSettings().lineSpacing, lip_.getFontSelector().lineGap());
}

/**
 * Return the location for the specified character offset.
 * @param column the character offset from the line start
 * @param edge the edge of the character to locate
 * @return the location. x-coordinate is distance from the left edge of the renderer, y-coordinate is relative in the visual lines
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
POINT LineLayout::location(length_t column, Edge edge /* = LEADING */) const {
	POINT location;
	if(column > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	else if(isDisposed())
		location.x = location.y = 0;
	else {
		const length_t sl = subline(column);
		const length_t firstRun = sublineFirstRuns_[sl];
		const length_t lastRun = (sl + 1 < numberOfSublines_) ? sublineFirstRuns_[sl + 1] : numberOfRuns_;
		// about x
		if(lip_.getLayoutSettings().orientation == LEFT_TO_RIGHT) {	// LTR
			location.x = sublineIndent(sl);
			for(size_t i = firstRun; i < lastRun; ++i) {
				const Run& run = *runs_[i];
				if(column >= run.column && column <= run.column + run.length()) {
					location.x += run.x(column - run.column, edge == TRAILING);
					break;
				}
				location.x += run.totalWidth();
			}
		} else {	// RTL
			location.x = sublineIndent(sl) + sublineWidth(sl);
			for(size_t i = lastRun - 1; ; --i) {
				const Run& run = *runs_[i];
				location.x -= run.totalWidth();
				if(column >= run.column && column <= run.column + run.length()) {					
					location.x += run.x(column - run.column, edge == TRAILING);
					break;
				}
				if(i == firstRun)
					break;
			}
		}
		// about y
		location.y = static_cast<long>(sl * linePitch());
	}
	return location;
}

/// Returns the width of the longest subline.
int LineLayout::longestSublineWidth() const /*throw()*/ {
	if(longestSublineWidth_ == -1) {
		int width = 0;
		for(length_t subline = 0; subline < numberOfSublines_; ++subline)
			width = max<long>(sublineWidth(subline), width);
		const_cast<LineLayout*>(this)->longestSublineWidth_ = width;
	}
	return longestSublineWidth_;
}

/**
 * Merges the given item runs and the given style runs.
 * @param items the items itemized by @c #itemize()
 * @param numberOfItems the length of the array @a items
 * @param styles the attributed text segments in the line (style runs)
 */
inline void LineLayout::merge(const SCRIPT_ITEM items[], size_t numberOfItems, const LineStyle& styles) /*throw()*/ {
	assert(runs_ == 0 && items != 0 && numberOfItems > 0 && styles.count > 0);
	const String& line = text();
	vector<Run*> runs;
	Run* run = new Run(styles.array[0].style);
	run->column = 0;
	run->analysis = items[0].a;
	runs.reserve(numberOfItems + styles.count);
	runs.push_back(run);

#define SPLIT_LAST_RUN()												\
	while(runs.back()->length() > MAXIMUM_RUN_LENGTH) {					\
		Run& back = *runs.back();										\
		Run* piece = new Run(back.style);								\
		length_t pieceLength = MAXIMUM_RUN_LENGTH;						\
		if(surrogates::isLowSurrogate(line[back.column + pieceLength]))	\
			--pieceLength;												\
		piece->analysis = back.analysis;								\
		piece->column = back.column + pieceLength;						\
		piece->setLength(back.length() - pieceLength);					\
		back.setLength(pieceLength);									\
		runs.push_back(piece);											\
	}

	for(size_t itemIndex = 1, styleIndex = 1; itemIndex < numberOfItems || styleIndex < styles.count; ) {
		bool brokeItem = false;
		const length_t nextItem = (itemIndex < numberOfItems) ? items[itemIndex].iCharPos : line.length();
		const length_t nextStyle = (styleIndex < styles.count) ? styles.array[styleIndex].column : line.length();
		length_t column;
		if(nextItem < nextStyle)
			column = items[itemIndex++].iCharPos;
		else if(nextStyle < nextItem) {
			column = styles.array[styleIndex++].column;
			brokeItem = true;
		} else {
			++itemIndex;
			column = styles.array[styleIndex++].column;
		}
		run->setLength(column - run->column);
		run = new Run(styles.array[styleIndex - 1].style);
		run->column = column;
		run->analysis = items[itemIndex - 1].a;
		if(brokeItem
				&& !legacyctype::isspace(line[styles.array[styleIndex - 1].column])
				&& !legacyctype::isspace(line[styles.array[styleIndex - 1].column - 1]))
			runs[runs.size() - 1]->analysis.fLinkAfter = run->analysis.fLinkBefore = 1;
		runs.back()->setLength(run->column - runs.back()->column);
		SPLIT_LAST_RUN();
		runs.push_back(run);
	}
	run->setLength(line.length() - run->column);
	SPLIT_LAST_RUN();
	runs_ = new Run*[numberOfRuns_ = runs.size()];
	copy(runs.begin(), runs.end(), runs_);

#undef SPLIT_LAST_RUN
}

/// Reorders the runs in visual order.
inline void LineLayout::reorder() /*throw()*/ {
	if(numberOfRuns_ == 0)
		return;
	Run** temp = new Run*[numberOfRuns_];
	memcpy(temp, runs_, sizeof(Run*) * numberOfRuns_);
	for(length_t subline = 0; subline < numberOfSublines_; ++subline) {
		const size_t numberOfRunsInSubline = ((subline < numberOfSublines_ - 1) ?
			sublineFirstRuns_[subline + 1] : numberOfRuns_) - sublineFirstRuns_[subline];
		BYTE* const levels = new BYTE[numberOfRunsInSubline];
		for(size_t i = 0; i < numberOfRunsInSubline; ++i)
			levels[i] = static_cast<BYTE>(runs_[i + sublineFirstRuns_[subline]]->analysis.s.uBidiLevel & 0x1f);
		int* const log2vis = new int[numberOfRunsInSubline];
		const HRESULT hr = ::ScriptLayout(static_cast<int>(numberOfRunsInSubline), levels, 0, log2vis);
		assert(SUCCEEDED(hr));
		delete[] levels;
		for(size_t i = sublineFirstRuns_[subline]; i < sublineFirstRuns_[subline] + numberOfRunsInSubline; ++i)
			runs_[sublineFirstRuns_[subline] + log2vis[i - sublineFirstRuns_[subline]]] = temp[i];
		delete[] log2vis;
	}
	delete[] temp;
}

/**
 * Returns the next tab stop position.
 * @param x the distance from leading edge of the line (can not be negative)
 * @param direction the direction
 * @return the distance from leading edge of the line to the next tab position
 */
inline int LineLayout::nextTabStop(int x, Direction direction) const /*throw()*/ {
	assert(x >= 0);
	const int tabWidth = lip_.getFontSelector().averageCharacterWidth() * lip_.getLayoutSettings().tabWidth;
	return (direction == Direction::FORWARD) ? x + tabWidth - x % tabWidth : x - x % tabWidth;
}

/**
 * Returns the next tab stop.
 * @param x the distance from the left edge of the line to base position (can not be negative)
 * @param right true to find the next right position
 * @return the tab stop position in pixel
 */
int LineLayout::nextTabStopBasedLeftEdge(int x, bool right) const /*throw()*/ {
	assert(x >= 0);
	const LayoutSettings& c = lip_.getLayoutSettings();
	const int tabWidth = lip_.getFontSelector().averageCharacterWidth() * c.tabWidth;
	if(getLineTerminatorOrientation(c) == LEFT_TO_RIGHT)
		return nextTabStop(x, right ? Direction::FORWARD : Direction::BACKWARD);
	else
		return right ? x + (x - longestSublineWidth()) % tabWidth : x - (tabWidth - (x - longestSublineWidth()) % tabWidth);
}

/**
 * Returns the character column (offset) for the specified point.
 * @param x the x coordinate of the point. distance from the left edge of the first subline
 * @param y the y coordinate of the point. distance from the top edge of the first subline
 * @param[out] trailing the trailing buffer
 * @param[out] outside true if the specified point is outside of the layout. optional
 * @return the character offset
 * @see #location
 */
length_t LineLayout::offset(int x, int y, length_t& trailing, bool* outside /* = 0 */) const /*throw()*/ {
	if(text().empty())
		return trailing = 0;

	// determine the subline
	length_t subline = 0;
	for(; subline < numberOfSublines_ - 1; ++subline) {
		if(static_cast<int>(linePitch() * subline) >= y)
			break;
	}

	// determine the column
	assert(numberOfRuns_ > 0);
	const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
	int cx = sublineIndent(subline);
	if(x <= cx) {	// on the left margin
		trailing = 0;
		if(outside != 0)
			*outside = true;
		const Run& firstRun = *runs_[sublineFirstRuns_[subline]];
		return firstRun.column + ((firstRun.analysis.fRTL == 0) ? 0 : firstRun.length());
	}
	for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i) {
		const Run& run = *runs_[i];
		if(x >= cx && x <= cx + run.totalWidth()) {
			int cp, t;
			::ScriptXtoCP(x - cx, static_cast<int>(run.length()), run.numberOfGlyphs(), run.clusters(),
				run.visualAttributes(), (run.justifiedAdvances() == 0) ? run.advances() : run.justifiedAdvances(), &run.analysis, &cp, &t);
			trailing = static_cast<length_t>(t);
			if(outside != 0)
				*outside = false;
			return run.column + static_cast<length_t>(cp);
		}
		cx += run.totalWidth();
	}
	trailing = 0;	// on the right margin
	if(outside != 0)
		*outside = true;
	return runs_[lastRun - 1]->column + ((runs_[lastRun - 1]->analysis.fRTL == 0) ? runs_[lastRun - 1]->length() : 0);
}

/**
 * Returns the styled segment containing the specified column.
 * @param column the column
 * @return the styled segment
 * @throw kernel#BadPositionException @a column is greater than the length of the line
 */
const StyledText& LineLayout::styledSegment(length_t column) const {
	if(column > text().length())
		throw kernel::BadPositionException(kernel::Position(lineNumber_, column));
	return *runs_[findRunForPosition(column)];
}

/**
 * Returns the smallest rectangle emcompasses the specified visual line. It might not coincide
 * exactly the ascent, descent or overhangs of the specified subline.
 * @param subline the wrapped line
 * @return the rectangle whose @c left value is the indentation of the subline and @c top value is
 *         the distance from the top of the whole line
 * @throw IndexOutOfBoundsException @a subline is greater than the number of the wrapped lines
 * @see #sublineIndent
 */
RECT LineLayout::sublineBounds(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");
	RECT rc;
	rc.left = sublineIndent(subline);
	rc.top = linePitch() * static_cast<long>(subline);
	rc.right = rc.left + sublineWidth(subline);
	rc.bottom = rc.top + linePitch();
	return rc;
}

/**
 * Returns the indentation of the specified subline. An indent is a horizontal distance from the
 * leftmost of the first subline to the leftmost of the given subline. If the subline is longer
 * than the first subline, the result is negative. The first subline's indent is always zero.
 * @param subline the visual line
 * @return the indentation in pixel
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
int LineLayout::sublineIndent(length_t subline) const {
	if(subline == 0)
		return 0;
	const LayoutSettings& c = lip_.getLayoutSettings();
	if(c.alignment == ALIGN_LEFT || c.justifiesLines)
		return 0;
	switch(c.alignment) {
	case ALIGN_LEFT:
	default:
		return 0;
	case ALIGN_RIGHT:
		return sublineWidth(0) - sublineWidth(subline);
	case ALIGN_CENTER:
		return (sublineWidth(0) - sublineWidth(subline)) / 2;
	}
}

/**
 * Returns the width of the specified wrapped line.
 * @param subline the visual line
 * @return the width
 * @throw IndexOutOfBoundsException @a subline is greater than the number of visual lines
 */
int LineLayout::sublineWidth(length_t subline) const {
	if(subline >= numberOfSublines_)
		throw IndexOutOfBoundsException("subline");
	else if(isDisposed())
		return 0;
	else if(numberOfSublines_ == 1 && longestSublineWidth_ != -1)
		return longestSublineWidth_;
	else {
		const size_t lastRun = (subline + 1 < numberOfSublines_) ? sublineFirstRuns_[subline + 1] : numberOfRuns_;
		int cx = 0;
		for(size_t i = sublineFirstRuns_[subline]; i < lastRun; ++i)
			cx += runs_[i]->totalWidth();
		return cx;
	}
}

/// Returns the text of the line.
inline const String& LineLayout::text() const /*throw()*/ {
	return lip_.getPresentation().document().line(lineNumber_);
}

// shaping stuffs
namespace {
	/**
	 * Builds glyphs into the run structure.
	 * @param dc the device context
	 * @param text the text to generate glyphs
	 * @param run the run to shape. if this function failed (except @c USP_E_SCRIPT_NOT_IN_FONT),
	 * both @c glyphs and @c visualAttributes members will be null
	 * @param[in,out] expectedNumberOfGlyphs the length of @a run.shared-&gt;glyphs
	 * @retval S_OK succeeded
	 * @retval USP_E_SCRIPT_NOT_IN_FONT the font does not support the required script
	 * @retval E_OUTOFMEMORY failed to allocate buffer for glyph indices or visual attributes array
	 * @retval E_INVALIDARG other Uniscribe error. usually, too long run was specified
	 */
	HRESULT buildGlyphs(const DC& dc, const Char* text, Run& run, size_t& expectedNumberOfGlyphs) /*throw()*/ {
		while(true) {
			int numberOfGlyphs;
			HRESULT hr = ::ScriptShape(dc.get(), &run.shared->cache, text,
				static_cast<int>(run.length()), static_cast<int>(expectedNumberOfGlyphs), &run.analysis,
				run.shared->glyphs.get(), run.shared->clusters.get(), run.shared->visualAttributes.get(), &numberOfGlyphs);
			if(SUCCEEDED(hr))
				run.setNumberOfGlyphs(numberOfGlyphs);
			if(hr == S_OK || hr == USP_E_SCRIPT_NOT_IN_FONT)
				return hr;
			run.shared->glyphs.reset();
			run.shared->visualAttributes.reset();
			if(hr != E_OUTOFMEMORY)
				return hr;
			// repeat until a large enough buffer is provided
			run.shared->glyphs.reset(new(nothrow) WORD[expectedNumberOfGlyphs *= 2]);
			if(run.glyphs() == 0)
				return E_OUTOFMEMORY;
			run.shared->visualAttributes.reset(new(nothrow) SCRIPT_VISATTR[expectedNumberOfGlyphs]);
			if(run.visualAttributes() == 0) {
				run.shared->glyphs.reset();
				return E_OUTOFMEMORY;
			}
		}
	}
	/**
	 * Returns the number of missing glyphs in the given run.
	 * @param dc the device context
	 * @param run the run
	 * @return the number of missing glyphs
	 */
	inline int countMissingGlyphs(const DC& dc, Run& run) /*throw()*/ {
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
		if(FAILED(ScriptGetFontProperties(dc.get(), &run.shared->cache, &fp)))
			return 0;	// can't handle
		// following is not offical way, but from Mozilla (gfxWindowsFonts.cpp)
		int result = 0;
		for(int i = 0; i < run.numberOfGlyphs(); ++i) {
			const WORD glyph = run.glyphs()[i];
			if(glyph == fp.wgDefault || (glyph == fp.wgInvalid && glyph != fp.wgBlank))
				++result;
			else if(run.visualAttributes()[i].fZeroWidth == 1 && scriptProperties.get(run.analysis.eScript).fComplex == 0)
				++result;
		}
		return result;
	}
	/**
	 * @param dc the device context
	 * @param text the text to generate glyphs
	 * @param run the run
	 * @param[in,out] expectedNumberOfGlyphs the length of @a run.shared-&gt;glyphs
	 * @param[out] numberOfMissingGlyphs the number of the missing glyphs
	 * @retval S_FALSE there are missing glyphs
	 * @retval otherwise see @c buildGlyphs function
	 */
	inline HRESULT generateGlyphs(const DC& dc, const Char* text,
			Run& run, size_t& expectedNumberOfGlyphs, int* numberOfMissingGlyphs) {
		HRESULT hr = buildGlyphs(dc, text, run, expectedNumberOfGlyphs);
		if(SUCCEEDED(hr) && numberOfMissingGlyphs != 0 && 0 != (*numberOfMissingGlyphs = countMissingGlyphs(dc, run)))
			hr = S_FALSE;
		else if(hr == USP_E_SCRIPT_NOT_IN_FONT && numberOfMissingGlyphs != 0)
			*numberOfMissingGlyphs = static_cast<int>(expectedNumberOfGlyphs);
		return hr;
	}
	/// Fills default glyphs into @a run instead of using @c ScriptShape.
	inline void generateDefaultGlyphs(const DC& dc, Run& run) {
		assert(run.shared->numberOfReferences == 1);
		SCRIPT_FONTPROPERTIES fp;
		fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
		if(FAILED(::ScriptGetFontProperties(dc.get(), &run.shared->cache, &fp)))
			fp.wgDefault = 0;	// hmm...
		run.setNumberOfGlyphs(static_cast<int>(run.length()));
		fill_n(run.shared->glyphs.get(), run.numberOfGlyphs(), fp.wgDefault);
		const bool ltr = run.analysis.fRTL == 0 || run.analysis.fLogicalOrder == 1;
		for(int i = 0; i < run.numberOfGlyphs(); ++i)
			run.shared->clusters[i] = static_cast<WORD>(ltr ? i : (run.numberOfGlyphs() - i));
		const SCRIPT_VISATTR va = {SCRIPT_JUSTIFY_NONE, 1, 0, 0, 0, 0};
		fill_n(run.shared->visualAttributes.get(), run.numberOfGlyphs(), va);
	}
	/**
	 * Returns a Unicode script corresponds to Win32 language identifier for digit substitution.
	 * @param id the language identifier
	 * @return the script or @c NOT_PROPERTY
	 */
	inline int convertWin32LangIDtoUnicodeScript(LANGID id) /*throw()*/ {
		switch(id) {
		case LANG_ARABIC:		return Script::ARABIC;
		case LANG_ASSAMESE:		return Script::BENGALI;
		case LANG_BENGALI:		return Script::BENGALI;
		case 0x5c:				return Script::CHEROKEE;
		case LANG_DIVEHI:		return Script::THAANA;
		case 0x5e:				return Script::ETHIOPIC;
		case LANG_FARSI:		return Script::ARABIC;	// Persian
		case LANG_GUJARATI:		return Script::GUJARATI;
		case LANG_HINDI:		return Script::DEVANAGARI;
		case LANG_KANNADA:		return Script::KANNADA;
		case 0x53:				return Script::KHMER;
		case 0x54:				return Script::LAO;
		case LANG_MALAYALAM:	return Script::MALAYALAM;
		case 0x55:				return Script::MYANMAR;
		case LANG_ORIYA:		return Script::ORIYA;
		case LANG_PUNJABI:		return Script::GURMUKHI;
		case 0x5b:				return Script::SINHALA;
		case LANG_SYRIAC:		return Script::SYRIAC;
		case LANG_TAMIL:		return Script::TAMIL;
		case 0x51:				return Script::TIBETAN;
		case LANG_TELUGU:		return Script::TELUGU;
		case LANG_THAI:			return Script::THAI;
		case LANG_URDU:			return Script::ARABIC;
		}
		return NOT_PROPERTY;
	}

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	const OPENTYPE_TAG HANI_TAG = makeOpenTypeTag("hani");
	const OPENTYPE_TAG JP78_TAG = makeOpenTypeTag("jp78");
	const OPENTYPE_TAG JP90_TAG = makeOpenTypeTag("jp90");
	const OPENTYPE_TAG JP04_TAG = makeOpenTypeTag("jp04");
	struct IVStoOTFT {
		ulong ivs;	// (base character) << 8 | (variation selector number)
		OPENTYPE_TAG featureTag;
	};
	const IVStoOTFT IVS_TO_OTFT[] = {
#include "generated/ivs-otft.ipp"
	};
	struct GetIVS {
		ulong operator()(size_t index) /*throw()*/ {return IVS_TO_OTFT[index].ivs;}
	};
	inline bool uniscribeSupportsVSS() /*throw()*/ {
		static bool checked = false, supports = false;
		if(!checked) {
			static const WCHAR text[] = L"\x82a6\xdb40\xdd00";	// <芦, U+E0100>
			SCRIPT_ITEM items[4];
			int numberOfItems;
			if(SUCCEEDED(::ScriptItemize(text, MANAH_COUNTOF(text) - 1,
					MANAH_COUNTOF(items), 0, 0, items, &numberOfItems)) && numberOfItems == 1)
				supports = true;
			checked = true;
		}
		return supports;
	}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
} // namespace @0

/// Generates the glyphs for the text.
void LineLayout::shape() /*throw()*/ {
	HRESULT hr;
	auto_ptr<DC> dc = lip_.getFontSelector().deviceContext();
#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
	const Run* lastIVSProcessedRun = 0;
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

	// for each runs...
	for(size_t runIndex = 0; runIndex < numberOfRuns_; ++runIndex) {
		Run* const run = runs_[runIndex];
		assert(run->shared->numberOfReferences == 1 && run->glyphs() == 0);
		const Char* textString = text().data() + run->column;
		run->shared->clusters.reset(new WORD[run->length()]);
		if(lip_.getLayoutSettings().inhibitsShaping)
			run->analysis.eScript = SCRIPT_UNDEFINED;

		HFONT oldFont;
		size_t expectedNumberOfGlyphs;
		if(run->analysis.s.fDisplayZWG != 0 && scriptProperties.get(run->analysis.eScript).fControl != 0) {
			// bidirectional format controls
			expectedNumberOfGlyphs = run->length();
			run->shared->glyphs.reset(new WORD[expectedNumberOfGlyphs]);
			run->shared->visualAttributes.reset(new SCRIPT_VISATTR[expectedNumberOfGlyphs]);
			oldFont = dc->selectObject(run->font = lip_.getFontSelector().fontForShapingControls());
			if(USP_E_SCRIPT_NOT_IN_FONT == (hr = buildGlyphs(*dc, textString, *run, expectedNumberOfGlyphs))) {
				assert(run->analysis.eScript != SCRIPT_UNDEFINED);
				run->analysis.eScript = SCRIPT_UNDEFINED;	// hmm...
				hr = buildGlyphs(*dc, textString, *run, expectedNumberOfGlyphs);
				assert(SUCCEEDED(hr));
			}
			dc->selectObject(oldFont);
		} else {
			// generateGlyphs() returns glyphs for the run. however it can contain missing glyphs.
			// we try candidate fonts in following order:
			//
			// 1. the primary font
			// 2. the national font for digit substitution
			// 3. the linked fonts
			// 4. the fallback font
			//
			// with storing the fonts failed to shape ("failedFonts"). and then retry the failed
			// fonts returned USP_E_SCRIPT_NOT_IN_FONT with shaping)

			expectedNumberOfGlyphs = run->length() * 3 / 2 + 16;
			run->shared->glyphs.reset(new WORD[expectedNumberOfGlyphs]);
			run->shared->visualAttributes.reset(new SCRIPT_VISATTR[expectedNumberOfGlyphs]);

			int script = NOT_PROPERTY;	// script of the run for fallback
			vector<pair<HFONT, int> > failedFonts;	// failed fonts (font handle vs. # of missings)
			int numberOfMissingGlyphs;

#define ASCENSION_MAKE_TEXT_STRING_SAFE()													\
	assert(safeText.get() == 0);															\
	safeText.reset(new Char[run->length()]);												\
	wmemcpy(safeText.get(), textString, run->length());										\
	replace_if(safeText.get(),																\
		safeText.get() + run->length(), surrogates::isSurrogate, REPLACEMENT_CHARACTER);	\
	textString = safeText.get();

#define ASCENSION_CHECK_FAILED_FONTS()										\
	bool skip = false;														\
	for(vector<pair<HFONT, int> >::const_iterator							\
			i(failedFonts.begin()), e(failedFonts.end()); i != e; ++i) {	\
		if(i->first == run->font) {											\
			skip = true;													\
			break;															\
		}																	\
	}

			// ScriptShape may crash if the shaping is disabled (see Mozilla bug 341500).
			// Following technique is also from Mozilla (gfxWindowsFonts.cpp).
			manah::AutoBuffer<Char> safeText;
			if(run->analysis.eScript == SCRIPT_UNDEFINED
					&& find_if(textString, textString + run->length(), surrogates::isSurrogate) != textString + run->length()) {
				ASCENSION_MAKE_TEXT_STRING_SAFE();
			}

			// 1. the primary font
			oldFont = dc->selectObject(run->font = lip_.getFontSelector().font(Script::COMMON, run->style.bold, run->style.italic));
			hr = generateGlyphs(*dc, textString, *run, expectedNumberOfGlyphs, &numberOfMissingGlyphs);
			if(hr != S_OK) {
				::ScriptFreeCache(&run->shared->cache);
				failedFonts.push_back(make_pair(run->font, (hr == S_FALSE) ? numberOfMissingGlyphs : numeric_limits<int>::max()));
			}

			// 2. the national font for digit substitution
			if(hr == USP_E_SCRIPT_NOT_IN_FONT && run->analysis.eScript != SCRIPT_UNDEFINED && run->analysis.s.fDigitSubstitute != 0) {
				script = convertWin32LangIDtoUnicodeScript(scriptProperties.get(run->analysis.eScript).langid);
				if(script != NOT_PROPERTY && 0 != (run->font = lip_.getFontSelector().font(script, run->style.bold, run->style.italic))) {
					ASCENSION_CHECK_FAILED_FONTS()
					if(!skip) {
						dc->selectObject(run->font);
						hr = generateGlyphs(*dc, textString, *run, expectedNumberOfGlyphs, &numberOfMissingGlyphs);
						if(hr != S_OK) {
							::ScriptFreeCache(&run->shared->cache);
							failedFonts.push_back(make_pair(run->font, numberOfMissingGlyphs));	// not material what hr is...
						}
					}
				}
			}

			// 3. the linked fonts
			if(hr != S_OK) {
				for(size_t i = 0; i < lip_.getFontSelector().numberOfLinkedFonts(); ++i) {
					run->font = lip_.getFontSelector().linkedFont(i, run->style.bold, run->style.italic);
					ASCENSION_CHECK_FAILED_FONTS()
					if(!skip) {
						dc->selectObject(run->font);
						if(S_OK == (hr = generateGlyphs(*dc, textString, *run, expectedNumberOfGlyphs, &numberOfMissingGlyphs)))
							break;
						::ScriptFreeCache(&run->shared->cache);
						failedFonts.push_back(make_pair(run->font,
							(hr == S_FALSE) ? numberOfMissingGlyphs : numeric_limits<int>::max()));
					}
				}
			}

			// 4. the fallback font
			if(hr != S_OK) {
				for(StringCharacterIterator i(textString, textString + run->length()); i.hasNext(); i.next()) {
					script = Script::of(i.current());
					if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
						break;
				}
				if(script != Script::UNKNOWN && script != Script::COMMON && script != Script::INHERITED)
					run->font = lip_.getFontSelector().font(script, run->style.bold, run->style.italic);
				else {
					run->font = 0;
					// ambiguous CJK?
					if(script == Script::COMMON && scriptProperties.get(run->analysis.eScript).fAmbiguousCharSet != 0) {
						switch(Block::of(surrogates::decodeFirst(textString, textString + run->length()))) {
						case Block::CJK_SYMBOLS_AND_PUNCTUATION:
						case Block::ENCLOSED_CJK_LETTERS_AND_MONTHS:
						case Block::CJK_COMPATIBILITY:
						case Block::VERTICAL_FORMS:	// as of GB 18030
						case Block::CJK_COMPATIBILITY_FORMS:
						case Block::SMALL_FORM_VARIANTS:	// as of CNS-11643
						case Block::HALFWIDTH_AND_FULLWIDTH_FORMS:
							run->font = lip_.getFontSelector().font(Script::HAN, run->style.bold, run->style.italic);
							break;
						}
					}
					if(run->font == 0 && runIndex > 0) {
						// use the previous run setting (but this will copy the style of the font...)
						run->analysis.eScript = runs_[runIndex - 1]->analysis.eScript;
						run->font = runs_[runIndex - 1]->font;
					}
				}
				if(run->font != 0) {
					ASCENSION_CHECK_FAILED_FONTS()
					if(!skip) {
						dc->selectObject(run->font);
						hr = generateGlyphs(*dc, textString, *run, expectedNumberOfGlyphs, &numberOfMissingGlyphs);
						if(hr != S_OK) {
							::ScriptFreeCache(&run->shared->cache);
							failedFonts.push_back(make_pair(run->font,
								(hr == S_FALSE) ? numberOfMissingGlyphs : numeric_limits<int>::max()));
						}
					}
				}
			}

			// retry without shaping
			if(hr != S_OK) {
				if(run->analysis.eScript != SCRIPT_UNDEFINED) {
					const WORD oldScript = run->analysis.eScript;
					run->analysis.eScript = SCRIPT_UNDEFINED;	// disable shaping
					if(find_if(textString, textString + run->length(), surrogates::isSurrogate) != textString + run->length()) {
						ASCENSION_MAKE_TEXT_STRING_SAFE();
					}
					for(vector<pair<HFONT, int> >::iterator i(failedFonts.begin()), e(failedFonts.end()); i != e; ++i) {
						if(i->second == numeric_limits<int>::max()) {
							dc->selectObject(run->font = i->first);
							hr = generateGlyphs(*dc, textString, *run, expectedNumberOfGlyphs, &numberOfMissingGlyphs);
							if(hr == S_OK)
								break;	// found the best
							::ScriptFreeCache(&run->shared->cache);
							if(hr == S_FALSE)
								i->second = -numberOfMissingGlyphs;
						}
					}
					run->analysis.eScript = oldScript;
				}
				if(hr != S_OK) {
					// select the font which generated the least missing glyphs
					assert(!failedFonts.empty());
					vector<pair<HFONT, int> >::const_iterator bestFont = failedFonts.begin();
					for(vector<pair<HFONT, int> >::const_iterator i(bestFont + 1), e(failedFonts.end()); i != e; ++i) {
						if(i->second < bestFont->second)
							bestFont = i;
					}
					dc->selectObject(run->font = bestFont->first);
					if(bestFont->second < 0)
						run->analysis.eScript = SCRIPT_UNDEFINED;
					hr = generateGlyphs(*dc, textString, *run, expectedNumberOfGlyphs, 0);
					if(FAILED(hr)) {
						::ScriptFreeCache(&run->shared->cache);
						generateDefaultGlyphs(*dc, *run);	// worst case...
					}
				}
			}

#undef ASCENSION_MAKE_TEXT_STRING_SAFE
#undef ASCENSION_CHECK_FAILED_FONTS
		}

#ifdef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
		if(!uniscribeSupportsVSS() && supportsOpenTypeFeatures()) {
			if(runIndex + 1 < numberOfRuns_ && runs_[runIndex + 1]->length() > 1) {
				const CodePoint vs = surrogates::decodeFirst(
					textString + run->length(), textString + run->length() + 2);
				if(vs >= 0xe0100 && vs <= 0xe01ef) {
					const CodePoint baseCharacter = surrogates::decodeLast(textString, textString + run->length());
					const size_t i = ascension::internal::searchBound(
						0U, MANAH_COUNTOF(IVS_TO_OTFT), (baseCharacter << 8) | (vs - 0xe0100), GetIVS());
					if(i != MANAH_COUNTOF(IVS_TO_OTFT) && IVS_TO_OTFT[i].ivs == ((baseCharacter << 8) | (vs - 0xe0100))) {
						// found valid IVS -> apply OpenType feature tag to obtain the variant
						// note that this glyph alternation is not effective unless the script in run->analysis is 'hani'
						hr = uspLib->get<0>()(dc->get(), &run->shared->cache, &run->analysis,
							HANI_TAG, 0, IVS_TO_OTFT[i].featureTag, 1, run->glyphs()[0], run->shared->glyphs.get());
						lastIVSProcessedRun = run;
					}
				}
			}

			if(lastIVSProcessedRun == runs_[runIndex - 1]) {
				// last character in the previous run and first character in this run are an IVS
				// => so, remove glyphs correspond to the first character
				WORD blankGlyph;
				if(S_OK != (hr = ::ScriptGetCMap(dc->get(), &run->shared->cache, L"\x0020", 1, 0, &blankGlyph))) {
					SCRIPT_FONTPROPERTIES fp;
					fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
					if(FAILED(::ScriptGetFontProperties(dc->get(), &run->shared->cache, &fp)))
						fp.wgBlank = 0;	// hmm...
					blankGlyph = fp.wgBlank;
				}
				run->shared->glyphs[run->clusters()[0]] = run->shared->glyphs[run->clusters()[1]] = blankGlyph;
				SCRIPT_VISATTR* va = run->shared->visualAttributes.get();
				va[run->clusters()[0]].uJustification = va[run->clusters()[1]].uJustification = SCRIPT_JUSTIFY_BLANK;
				va[run->clusters()[0]].fZeroWidth = va[run->clusters()[1]].fZeroWidth = 1;
			}
		}
#endif // ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND

		run->shared->advances.reset(new int[run->numberOfGlyphs()]);
		run->shared->glyphOffsets.reset(new GOFFSET[run->numberOfGlyphs()]);
		hr = ::ScriptPlace(dc->get(), &run->shared->cache, run->glyphs(), run->numberOfGlyphs(),
			run->visualAttributes(), &run->analysis, run->shared->advances.get(), run->shared->glyphOffsets.get(), &run->width);

		// query widths of C0 and C1 controls in this logical line
		if(ISpecialCharacterRenderer* scr = lip_.getSpecialCharacterRenderer()) {
			ISpecialCharacterRenderer::LayoutContext context(*dc);
			context.orientation = run->orientation();
			SCRIPT_FONTPROPERTIES fp;
			fp.cBytes = 0;
			for(length_t i = 0; i < run->length(); ++i) {
				if(isC0orC1Control(textString[i])) {
					if(const int width = scr->getControlCharacterWidth(context, textString[i])) {
						// substitute the glyph
						run->width.abcB += width - run->advances()[i];
						run->shared->advances[i] = width;
						if(fp.cBytes == 0) {
							fp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
							::ScriptGetFontProperties(dc->get(), &run->shared->cache, &fp);
						}
						run->shared->glyphs[i] = fp.wgBlank;
					}
				}
			}
		}
		dc->selectObject(oldFont);
	}
}

/// Locates the wrap points and resolves tab expansions.
void LineLayout::wrap() /*throw()*/ {
	assert(numberOfRuns_ != 0 && lip_.getLayoutSettings().lineWrap.wraps());
	assert(numberOfSublines_ == 0 && sublineOffsets_ == 0 && sublineFirstRuns_ == 0);

	const String& line = text();
	vector<length_t> sublineFirstRuns;
	sublineFirstRuns.push_back(0);
	auto_ptr<DC> dc = lip_.getFontSelector().deviceContext();
	const int cookie = dc->save();
	int cx = 0;
	manah::AutoBuffer<int> logicalWidths;
	manah::AutoBuffer<SCRIPT_LOGATTR> logicalAttributes;
	length_t longestRunLength = 0;	// for efficient allocation
	vector<Run*> newRuns;
	newRuns.reserve(numberOfRuns_ * 3 / 2);
	// for each runs... (at this time, runs_ is in logical order)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		Run* run = runs_[i];

		// if the run is a tab, expand and calculate actual width
		if(line[run->column] == L'\t') {
			assert(run->length() == 1);
			if(cx == wrapWidth_) {
				cx = run->width.abcB = nextTabStop(0, Direction::FORWARD);
				run->width.abcA = run->width.abcC = 0;
				newRuns.push_back(run);
				sublineFirstRuns.push_back(newRuns.size());
			} else {
				run->width.abcB = min(nextTabStop(cx, Direction::FORWARD), wrapWidth_) - cx;
				run->width.abcA = run->width.abcC = 0;
				cx += run->totalWidth();
				newRuns.push_back(run);
			}
			run->shared->advances[0] = run->totalWidth();
			continue;
		}

		// obtain logical widths and attributes for all characters in this run to determine line break positions
		if(run->length() > longestRunLength) {
			longestRunLength = run->length();
			longestRunLength += 16 - longestRunLength % 16;
			logicalWidths.reset(new int[longestRunLength]);
			logicalAttributes.reset(new SCRIPT_LOGATTR[longestRunLength]);
		}
		dc->selectObject(run->font);
		HRESULT hr = run->logicalWidths(logicalWidths.get());
		hr = ::ScriptBreak(line.data() + run->column, static_cast<int>(run->length()), &run->analysis, logicalAttributes.get());
		const length_t originalRunPosition = run->column;
		int widthInThisRun = 0;
		length_t lastBreakable = run->column, lastGlyphEnd = run->column;
		int lastBreakableCx = cx, lastGlyphEndCx = cx;
		// for each characters in the run...
		for(length_t j = run->column; j < run->column + run->length(); ) {	// j is position in the LOGICAL line
			const int x = cx + widthInThisRun;
			// remember this opportunity
			if(logicalAttributes[j - originalRunPosition].fCharStop != 0) {
				lastGlyphEnd = j;
				lastGlyphEndCx = x;
				if(logicalAttributes[j - originalRunPosition].fSoftBreak != 0
						|| logicalAttributes[j - originalRunPosition].fWhiteSpace != 0) {
					lastBreakable = j;
					lastBreakableCx = x;
				}
			}
			// break if the width of the visual line overs the wrap width
			if(x + logicalWidths[j - originalRunPosition] > wrapWidth_) {
				// the opportunity is the start of this run
				if(lastBreakable == run->column) {
					// break at the last glyph boundary if no opportunities
					if(sublineFirstRuns.empty() || sublineFirstRuns.back() == newRuns.size()) {
						if(lastGlyphEnd == run->column) {	// break here if no glyph boundaries
							lastBreakable = j;
							lastBreakableCx = x;
						} else {
							lastBreakable = lastGlyphEnd;
							lastBreakableCx = lastGlyphEndCx;
						}
					}
				}

				// case 1: break at the start of the run
				if(lastBreakable == run->column) {
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run start.\n";
				}
				// case 2: break at the end of the run
				else if(lastBreakable == run->column + run->length()) {
					if(lastBreakable < line.length()) {
						assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
						sublineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					auto_ptr<Run> followingRun(run->split(*dc, lastBreakable));
					newRuns.push_back(run);
					assert(sublineFirstRuns.empty() || newRuns.size() != sublineFirstRuns.back());
					sublineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run meddle.\n";
					run = followingRun.release();	// continue the process about this run
				}
				widthInThisRun = cx + widthInThisRun - lastBreakableCx;
				lastBreakableCx -= cx;
				lastGlyphEndCx -= cx;
				cx = 0;
				j = max(lastBreakable, j);
			} else
				widthInThisRun += logicalWidths[j++ - originalRunPosition];
		}
		newRuns.push_back(run);
		cx += widthInThisRun;
	}
//dout << L"...broke the all lines.\n";
	dc->restore(cookie);
	if(newRuns.empty())
		newRuns.push_back(0);
	delete[] runs_;
	runs_ = new Run*[numberOfRuns_ = newRuns.size()];
	copy(newRuns.begin(), newRuns.end(), runs_);
	sublineFirstRuns_ = new length_t[numberOfSublines_ = sublineFirstRuns.size()];
	copy(sublineFirstRuns.begin(), sublineFirstRuns.end(), sublineFirstRuns_);
	sublineOffsets_ = new length_t[numberOfSublines_];
	for(size_t i = 0; i < numberOfSublines_; ++i)
		sublineOffsets_[i] = runs_[sublineFirstRuns_[i]]->column;
}


// LineLayout.Selection /////////////////////////////////////////////////////

namespace {
	inline Colors fallbackSelectionColors(const Colors& source, bool focused) {
		return Colors(
			source.foreground.isValid() ? source.foreground :
				Color::fromCOLORREF(::GetSysColor(focused ? COLOR_HIGHLIGHTTEXT : COLOR_INACTIVECAPTIONTEXT)),
			source.background.isValid() ? source.background :
				Color::fromCOLORREF(::GetSysColor(focused ? COLOR_HIGHLIGHT : COLOR_INACTIVECAPTION)));
	}
}

/**
 * Constructor.
 */
LineLayout::Selection::Selection(const viewers::Caret& caret) /*throw()*/ :
		caret_(caret), color_(fallbackSelectionColors(Colors(), caret.textViewer().hasFocus())) {
}

/// Constructor.
LineLayout::Selection::Selection(const viewers::Caret& caret, const Colors& color) /*throw()*/ :
		caret_(caret), color_(fallbackSelectionColors(color, caret.textViewer().hasFocus())) {
}


// LineLayout.StyledSegmentIterator /////////////////////////////////////////

/**
 * Private constructor.
 * @param start
 */
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const Run*& start) /*throw()*/ : p_(&start) {
}

/// Copy-constructor.
LineLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/ : p_(rhs.p_) {
}

/// Returns the current segment.
const StyledText& LineLayout::StyledSegmentIterator::current() const /*throw()*/ {
	return **p_;
}


// LineLayoutBuffer /////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document
 * @param bufferSize the maximum number of lines cached
 * @param autoRepair true to repair disposed layout automatically if the line number of its line was not changed
 * @throw std#invalid_argument @a bufferSize is zero
 */
LineLayoutBuffer::LineLayoutBuffer(Document& document, length_t bufferSize, bool autoRepair) :
		document_(document), bufferSize_(bufferSize), autoRepair_(autoRepair), documentChangePhase_(NONE),
		longestLineWidth_(0), longestLine_(INVALID_INDEX), numberOfVisualLines_(document.numberOfLines()) {
	pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	if(bufferSize == 0)
		throw invalid_argument("size of the buffer can't be zero.");
	document_.addPrenotifiedListener(*this);
}

/// Destructor.
LineLayoutBuffer::~LineLayoutBuffer() /*throw()*/ {
//	clearCaches(startLine_, startLine_ + bufferSize_, false);
	for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i)
		delete *i;
	document_.removePrenotifiedListener(*this);
}

/**
 * Registers the visual lines listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void LineLayoutBuffer::addVisualLinesListener(IVisualLinesListener& listener) {
	listeners_.add(listener);
	const length_t lines = document_.numberOfLines();
	if(lines > 1)
		listener.visualLinesInserted(1, lines);
}

/**
 * Clears the layout caches of the specified lines. This method calls @c #layoutModified.
 * @param first the start of lines
 * @param last the end of lines (exclusive. this line will not be cleared)
 * @param repair set true to recreate layouts for the lines. if true, this method calls
 * @c #layoutModified. otherwise calls @c #layoutDeleted
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
		length_t newSublines = 0, actualFirst = last, actualLast = first;
		for(list<LineLayout*>::iterator i(layouts_.begin()); i != layouts_.end(); ++i) {
			LineLayout*& layout = *i;
			const length_t lineNumber = layout->lineNumber();
			if(lineNumber >= first && lineNumber < last) {
				oldSublines += layout->numberOfSublines();
				delete layout;
				layout = new LineLayout(*lip_, lineNumber);
				newSublines += layout->numberOfSublines();
				++cachedLines;
				actualFirst = min(actualFirst, lineNumber);
				actualLast = max(actualLast, lineNumber);
			}
		}
		if(actualFirst == last)	// no lines cleared
			return;
		++actualLast;
		fireVisualLinesModified(actualFirst, actualLast, newSublines += actualLast - actualFirst - cachedLines,
			oldSublines += actualLast - actualFirst - cachedLines, documentChangePhase_ == CHANGING);
	} else {
		for(list<LineLayout*>::iterator i(layouts_.begin()); i != layouts_.end(); ) {
			if((*i)->lineNumber() >= first && (*i)->lineNumber() < last) {
				oldSublines += (*i)->numberOfSublines();
				delete *i;
				i = layouts_.erase(i);
				++cachedLines;
			} else
				++i;
		}
		fireVisualLinesDeleted(first, last, oldSublines += last - first - cachedLines);
	}
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void LineLayoutBuffer::documentAboutToBeChanged(const kernel::Document&, const kernel::DocumentChange&) {
	documentChangePhase_ = ABOUT_CHANGE;
}

/// @see kernel#IDocumentListener#documentChanged
void LineLayoutBuffer::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
	const length_t top = change.region().beginning().line, bottom = change.region().end().line;
	documentChangePhase_ = CHANGING;
	if(top != bottom) {
		if(change.isDeletion()) {	// deleted region includes newline(s)
			clearCaches(top + 1, bottom + 1, false);
			for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
				if((*i)->lineNumber() > top)
					(*i)->lineNumber_ -= bottom - top;	// $friendly-access
			}
		} else {	// inserted text is multiline
			for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
				if((*i)->lineNumber() > top)
					(*i)->lineNumber_ += bottom - top;	// $friendly-access
			}
			fireVisualLinesInserted(top + 1, bottom + 1);
		}
	}
	if(pendingCacheClearance_.first == INVALID_INDEX
			|| top < pendingCacheClearance_.first || top >= pendingCacheClearance_.last)
		invalidate(top);
	documentChangePhase_ = NONE;
	if(pendingCacheClearance_.first != INVALID_INDEX) {
		clearCaches(pendingCacheClearance_.first, pendingCacheClearance_.last, autoRepair_);
		pendingCacheClearance_.first = pendingCacheClearance_.last = INVALID_INDEX;
	}
}

void LineLayoutBuffer::fireVisualLinesDeleted(length_t first, length_t last, length_t sublines) {
	numberOfVisualLines_ -= sublines;
	const bool widthChanged = longestLine_ >= first && longestLine_ < last;
	if(widthChanged)
		updateLongestLine(static_cast<length_t>(-1), 0);
	listeners_.notify<length_t, length_t, length_t>(&IVisualLinesListener::visualLinesDeleted, first, last, sublines, widthChanged);
}

void LineLayoutBuffer::fireVisualLinesInserted(length_t first, length_t last) /*throw()*/ {
	numberOfVisualLines_ += last - first;
	listeners_.notify<length_t, length_t>(&IVisualLinesListener::visualLinesInserted, first, last);
}

void LineLayoutBuffer::fireVisualLinesModified(length_t first, length_t last,
		length_t newSublines, length_t oldSublines, bool documentChanged) /*throw()*/ {
	numberOfVisualLines_ += newSublines;
	numberOfVisualLines_ -= oldSublines;

	// update the longest line
	bool longestLineChanged = false;;
	if(longestLine_ >= first && longestLine_ < last) {
		updateLongestLine(static_cast<length_t>(-1), 0);
		longestLineChanged = true;
	} else {
		length_t newLongestLine = longestLine_;
		int newLongestLineWidth = longestLineWidth_;
		for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
			const LineLayout& layout = **i;
			if(layout.longestSublineWidth() > newLongestLineWidth) {
				newLongestLine = (*i)->lineNumber();
				newLongestLineWidth = layout.longestSublineWidth();
			}
		}
		if(longestLineChanged = (newLongestLine != longestLine_))
			updateLongestLine(newLongestLine, newLongestLineWidth);
	}

	listeners_.notify<length_t, length_t, signed_length_t>(
		&IVisualLinesListener::visualLinesModified, first, last,
		static_cast<signed_length_t>(newSublines) - static_cast<signed_length_t>(oldSublines), documentChanged, longestLineChanged);
}

/// Invalidates all layouts.
void LineLayoutBuffer::invalidate() /*throw()*/ {
	clearCaches(0, lip_->getPresentation().document().numberOfLines(), autoRepair_);
}

/**
 * Invalidates the layouts of the specified lines.
 * @param first the start of the lines
 * @param last the end of the lines (exclusive. this line will not be cleared)
 * @throw std#invalid_argument @a first &gt;= @a last
 */
void LineLayoutBuffer::invalidate(length_t first, length_t last) {
	if(first >= last)
		throw invalid_argument("Any line number is invalid.");
	clearCaches(first, last, autoRepair_);
}

/**
 * Resets the cached layout of the specified line and repairs if necessary.
 * @param line the line to invalidate layout
 */
inline void LineLayoutBuffer::invalidate(length_t line) {
	for(list<LineLayout*>::iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		LineLayout*& p = *i;
		if(p->lineNumber() == line) {
			const length_t oldSublines = p->numberOfSublines();
			delete p;
			if(autoRepair_) {
				p = new LineLayout(*lip_, line);
				fireVisualLinesModified(line, line + 1, p->numberOfSublines(), oldSublines, documentChangePhase_ == CHANGING);
			} else {
				layouts_.erase(i);
				fireVisualLinesModified(line, line + 1, 1, oldSublines, documentChangePhase_ == CHANGING);
			}
			break;
		}
	}
}

/**
 * Returns the layout of the specified line.
 * @param line the line
 * @return the layout
 * @throw kernel#BadPositionException @a line is greater than the number of the lines
 */
const LineLayout& LineLayoutBuffer::lineLayout(length_t line) const {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
	manah::win32::DumpContext dout;
	dout << "finding layout for line " << line;
#endif
	if(line > lip_->getPresentation().document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	LineLayoutBuffer& self = *const_cast<LineLayoutBuffer*>(this);
	list<LineLayout*>::iterator i(self.layouts_.begin());
	for(const list<LineLayout*>::iterator e(self.layouts_.end()); i != e; ++i) {
		if((*i)->lineNumber_ == line)
			break;
	}

	if(i != layouts_.end()) {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache found\n";
#endif
		LineLayout* layout = *i;
		if(layout != layouts_.front()) {
			// bring to the top
			self.layouts_.erase(i);
			self.layouts_.push_front(layout);
		}
		return *layout;
	} else {
#ifdef ASCENSION_TRACE_LAYOUT_CACHES
		dout << "... cache not found\n";
#endif
		if(layouts_.size() == bufferSize_) {
			// delete the last
			LineLayout* p = layouts_.back();
			self.layouts_.pop_back();
			self.fireVisualLinesModified(p->lineNumber(), p->lineNumber() + 1,
				1, p->numberOfSublines(), documentChangePhase_ == CHANGING);
			delete p;
		}
		LineLayout* const layout = new LineLayout(*lip_, line);
		self.layouts_.push_front(layout);
		self.fireVisualLinesModified(line, line + 1, layout->numberOfSublines(), 1, documentChangePhase_ == CHANGING);
		return *layout;
	}
}

/**
 * Returns the first visual line number of the specified logical line.
 * @param line the logical line
 * @return the first visual line of @a line
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #mapLogicalPositionToVisualPosition
 */
length_t LineLayoutBuffer::mapLogicalLineToVisualLine(length_t line) const {
	if(line >= lip_->getPresentation().document().numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	else if(!lip_->getLayoutSettings().lineWrap.wraps())
		return line;
	length_t result = 0, cachedLines = 0;
	for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
		if((*i)->lineNumber() < line) {
			result += (*i)->numberOfSublines();
			++cachedLines;
		}
	}
	return result + line - cachedLines;
}

/**
 * Returns the visual line number and the visual column number of the specified logical position.
 * @param position the logical coordinates of the position to be mapped
 * @param[out] column the visual column of @a position. can be @c null if not needed
 * @return the visual line of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalLineToVisualLine
 */
length_t LineLayoutBuffer::mapLogicalPositionToVisualPosition(const Position& position, length_t* column) const {
	if(!lip_->getLayoutSettings().lineWrap.wraps()) {
		if(column != 0)
			*column = position.column;
		return position.line;
	}
	const LineLayout& layout = lineLayout(position.line);
	const length_t subline = layout.subline(position.column);
	if(column != 0)
		*column = position.column - layout.sublineOffset(subline);
	return mapLogicalLineToVisualLine(position.line) + subline;
}

#if 0
/**
 * Returns the logical line number and the visual subline number of the specified visual line.
 * @param line the visual line
 * @param[out] subline the visual subline of @a line. can be @c null if not needed
 * @return the logical line
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
 * @param position the visual coordinates of the position to be mapped
 * @return the logical coordinates of @a position
 * @throw kernel#BadPositionException @a position is outside of the document
 * @see #mapLogicalPositionToVisualPosition, #mapVisualLineToLogicalLine
 */
Position LineLayoutBuffer::mapVisualPositionToLogicalPosition(const Position& position) const {
	if(!getTextViewer().getConfiguration().lineWrap.wraps())
		return position;
	Position result;
	length_t subline;
	result.line = mapVisualLineToLogicalLine(position.line, &subline);
	result.column = getLineLayout(result.line).getSublineOffset(subline) + position.column;
	return result;
}
#endif // 0

/**
 * Offsets visual line.
 * @param[in,out] line the logical line
 * @param[in,out] subline the visual subline
 * @param[in] offset the offset
 * @param[out] overflowedOrUnderflowed true if absolute value of @a offset is too large so that
 * the results were snapped to the beginning or the end of the document. optional
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
 * Sets the new layout information provider.
 * @param newProvider the layout information provider
 * @param delegateOwnership set true to transfer the ownership of @a newProvider into the callee
 */
void LineLayoutBuffer::setLayoutInformation(const ILayoutInformationProvider* newProvider, bool delegateOwnership) {
	lip_.reset(newProvider, delegateOwnership);
	invalidate();
}

/**
 * Updates the longest line and invokes @c ILongestLineListener#longestLineChanged.
 * @param line the new longest line. set -1 to recalculate
 * @param width the width of the longest line. if @a line is -1, this value is ignored
 */
void LineLayoutBuffer::updateLongestLine(length_t line, int width) /*throw()*/ {
	if(line != -1) {
		longestLine_ = line;
		longestLineWidth_ = width;
	} else {
		longestLine_ = static_cast<length_t>(-1);
		longestLineWidth_ = 0;
		for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ++i) {
			if((*i)->longestSublineWidth() > longestLineWidth_) {
				longestLine_ = (*i)->lineNumber();
				longestLineWidth_ = (*i)->longestSublineWidth();
			}
		}
	}
}


// ISpecialCharacterRenderer ////////////////////////////////////////////////

/**
 * @class ascension::layout::ISpecialCharacterRenderer
 * Interface for objects which draw special characters.
 *
 * @c ISpecialCharacterRenderer hooks shaping and drawing processes of @c LineLayout about some
 * special characters. These include:
 * - C0 controls
 * - C1 controls
 * - End of line (Line terminators)
 * - White space characters
 * - Line wrapping marks
 *
 * <h2>Characters @c ISpecialCharacterRenderer can render</h2>
 *
 * <em>C0 controls</em> include characters whose code point is U+0000..001F or U+007F. But U+0009,
 * U+000A, and U+000D are excluded. These characters can be found in "White space characters" and
 * "End of line".
 *
 * <em>C1 controls</em> include characters whose code point is U+0080..009F. But only U+0085 is
 * excluded. This is one of "End of line" character.
 *
 * <em>End of line</em> includes any NLFs in Unicode. Identified by @c kernel#Newline enumeration.
 *
 * <em>White space characters</em> include all Unicode white spaces and horizontal tab (U+0009). An
 * instance of @c ISpecialCharacterRenderer can't set the width of these glyphs.
 *
 * <em>Line wrapping marks</em> indicate a logical is wrapped visually. Note that this is not an
 * actual character.
 *
 * <h2>Process</h2>
 *
 * @c ISpecialCharacterRenderer will be invoked at the following two stages.
 * -# To layout a special character.
 * -# To draw a special character.
 *
 * (1) When layout of a line is needed, @c TextRenderer creates and initializes a @c LineLayout.
 * In this process, the widths of the all characters in the line are calculated by Unicode script
 * processor (Uniscribe). For the above special characters, @c LineLayout queries the widths to
 * @c ISpecialCharacterRenderer (However, for white spaces, this query is not performed).
 *
 * (2) When a line is drawn, @c LineLayout#draw calls @c ISpecialCharacterRenderer::drawXxxx
 * methods to draw special characters with the device context, the orientation, and the rectangle
 * to paint.
 *
 * @see TextRenderer, TextRenderer#setSpecialCharacterRenderer
 */


// DefaultSpecialCharacterRenderer //////////////////////////////////////////

namespace {
	inline void getControlPresentationString(CodePoint c, Char* buffer) {
		buffer[0] = L'^';
		buffer[1] = (c != 0x7f) ? static_cast<Char>(c) + 0x40 : L'?';
	}
}

/**
 * @class ascension::layout::DefaultSpecialCharacterRenderer
 *
 * Default implementation of @c ISpecialCharacterRenderer interface. This renders special
 * characters with the glyphs provided by the standard international font "Lucida Sans Unicode".
 * The mapping special characters to characters provide glyphs are as follows:
 * - Horizontal tab (LTR) : U+2192 Rightwards Arrow (&#x2192;)
 * - Horizontal tab (RTL) : U+2190 Leftwards Arrow (&#x2190;)
 * - Line terminator : U+2193 Downwards Arrow (&#x2193;)
 * - Line wrapping mark (LTR) : U+21A9 Leftwards Arrow With Hook (&#x21A9;)
 * - Line wrapping mark (RTL) : U+21AA Rightwards Arrow With Hook (&#x21AA;)
 * - White space : U+00B7 Middle Dot (&#x00B7;)
 *
 * Default foreground colors of glyphs are as follows:
 * - Control characters : RGB(0x80, 0x80, 0x00)
 * - Line terminators : RGB(0x00, 0x80, 0x80)
 * - Line wrapping markers: RGB(0x00, 0x80, 0x80)
 * - White space characters : RGB(0x00, 0x80, 0x80)
 */

/// Default constructor.
DefaultSpecialCharacterRenderer::DefaultSpecialCharacterRenderer() /*throw()*/ : renderer_(0),
		controlColor_(RGB(0x80, 0x80, 0x00)), eolColor_(RGB(0x00, 0x80, 0x80)), wrapMarkColor_(RGB(0x00, 0x80, 0x80)),
		whiteSpaceColor_(RGB(0x00, 0x80, 0x80)), showsEOLs_(true), showsWhiteSpaces_(true), font_(0) {
}

/// Destructor.
DefaultSpecialCharacterRenderer::~DefaultSpecialCharacterRenderer() /*throw()*/ {
	::DeleteObject(font_);
	font_ = 0;
}

/// @see ISpecialCharacterRenderer#drawControlCharacter
void DefaultSpecialCharacterRenderer::drawControlCharacter(const DrawingContext& context, CodePoint c) const {
	HFONT oldFont = context.dc.selectObject(renderer_->font());
	context.dc.setTextColor(controlColor_);
	Char buffer[2];
	getControlPresentationString(c, buffer);
	context.dc.extTextOut(context.rect.left, context.rect.top + renderer_->ascent(), 0, 0, buffer, 2, 0);
	context.dc.selectObject(oldFont);
}

/// @see ISpecialCharacterRenderer#drawLineTerminator
void DefaultSpecialCharacterRenderer::drawLineTerminator(const DrawingContext& context, kernel::Newline) const {
	if(showsEOLs_ && glyphs_[LINE_TERMINATOR] != 0xffff) {
		HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[LINE_TERMINATOR] & 0x80000000) ? font_ : renderer_->font());
		context.dc.setTextColor(eolColor_);
		context.dc.extTextOut(context.rect.left,
			context.rect.top + renderer_->ascent(), ETO_GLYPH_INDEX, 0, reinterpret_cast<const WCHAR*>(&glyphs_[LINE_TERMINATOR]), 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawLineWrappingMark
void DefaultSpecialCharacterRenderer::drawLineWrappingMark(const DrawingContext& context) const {
	const int id = (context.orientation == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK;
	const WCHAR glyph = glyphs_[id];
	if(glyph != 0xffff) {
		HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[id] & 0x80000000) ? font_ : renderer_->font());
		context.dc.setTextColor(wrapMarkColor_);
		context.dc.extTextOut(context.rect.left, context.rect.top + renderer_->ascent(), ETO_GLYPH_INDEX, 0, &glyph, 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawWhiteSpaceCharacter
void DefaultSpecialCharacterRenderer::drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const {
	if(!showsWhiteSpaces_)
		return;
	else if(c == 0x0009) {
		const int id = (context.orientation == LEFT_TO_RIGHT) ? LTR_HORIZONTAL_TAB : RTL_HORIZONTAL_TAB;
		const WCHAR glyph = glyphs_[id];
		if(glyph != 0xffff) {
			HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[id] & 0x80000000) ? font_ : renderer_->font());
			const int glyphWidth = glyphWidths_[id] & 0x7fffffff;
			const int x =
				((context.orientation == LEFT_TO_RIGHT && glyphWidth < context.rect.right - context.rect.left)
					|| (context.orientation == RIGHT_TO_LEFT && glyphWidth > context.rect.right - context.rect.left)) ?
				context.rect.left : context.rect.right - glyphWidth;
			context.dc.setTextColor(whiteSpaceColor_);
			context.dc.extTextOut(x, context.rect.top + renderer_->ascent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &context.rect, &glyph, 1, 0);
			context.dc.selectObject(oldFont);
		}
	} else if(glyphs_[WHITE_SPACE] != 0xffff) {
		HFONT oldFont = context.dc.selectObject(toBoolean(glyphWidths_[WHITE_SPACE] & 0x80000000) ? font_ : renderer_->font());
		context.dc.setTextColor(whiteSpaceColor_);
		context.dc.extTextOut((context.rect.left + context.rect.right - (glyphWidths_[WHITE_SPACE] & 0x7fffffff)) / 2,
			context.rect.top + renderer_->ascent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &context.rect,
			reinterpret_cast<const WCHAR*>(&glyphs_[WHITE_SPACE]), 1, 0);
		context.dc.selectObject(oldFont);
	}
}

/// @see IFontSelectorListener#fontChanged
void DefaultSpecialCharacterRenderer::fontChanged() {
	static const Char codes[] = {0x2192, 0x2190, 0x2193, 0x21a9, 0x21aa, 0x00b7};

	// using the primary font
	ScreenDC dc;
	HFONT oldFont = dc.selectObject(renderer_->font());
	dc.getGlyphIndices(codes, MANAH_COUNTOF(codes), glyphs_, GGI_MARK_NONEXISTING_GLYPHS);
	dc.getCharWidthI(glyphs_, MANAH_COUNTOF(codes), glyphWidths_);

	// using the fallback font
	::DeleteObject(font_);
	font_ = 0;
	if(find(glyphs_, MANAH_ENDOF(glyphs_), 0xffff) != MANAH_ENDOF(glyphs_)) {
		LOGFONTW lf;
		::GetObjectW(renderer_->font(), sizeof(LOGFONTW), &lf);
		lf.lfWeight = FW_REGULAR;
		lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = false;
		wcscpy(lf.lfFaceName, L"Lucida Sans Unicode");
		dc.selectObject(font_ = ::CreateFontIndirectW(&lf));
		WORD g[MANAH_COUNTOF(glyphs_)];
		int w[MANAH_COUNTOF(glyphWidths_)];
		dc.getGlyphIndices(codes, MANAH_COUNTOF(codes), g, GGI_MARK_NONEXISTING_GLYPHS);
		dc.getCharWidthI(g, MANAH_COUNTOF(codes), w);
		for(int i = 0; i < MANAH_COUNTOF(glyphs_); ++i) {
			if(glyphs_[i] == 0xffff) {
				if(g[i] != 0xffff) {
					glyphs_[i] = g[i];
					glyphWidths_[i] = w[i] | 0x80000000;
				} else
					glyphWidths_[i] = 0;	// missing
			}
		}
	}

	dc.selectObject(oldFont);
}

/// @see ISpecialCharacterRenderer#getControlCharacterWidth
int DefaultSpecialCharacterRenderer::getControlCharacterWidth(const LayoutContext& context, CodePoint c) const {
	Char buffer[2];
	getControlPresentationString(c, buffer);
	HFONT oldFont = context.dc.selectObject(renderer_->font());
	const int result = context.dc.getTextExtent(buffer, 2).cx;
	context.dc.selectObject(oldFont);
	return result;
}

/// @see ISpecialCharacterRenderer#getLineTerminatorWidth
int DefaultSpecialCharacterRenderer::getLineTerminatorWidth(const LayoutContext&, kernel::Newline) const {
	return showsEOLs_ ? (glyphWidths_[LINE_TERMINATOR] & 0x7fffffff) : 0;
}

/// @see ISpecialCharacterRenderer#getLineWrappingMarkWidth
int DefaultSpecialCharacterRenderer::getLineWrappingMarkWidth(const LayoutContext& context) const {
	return glyphWidths_[(context.orientation == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK] & 0x7fffffff;
}

/// @see ISpecialCharacterRenderer#install
void DefaultSpecialCharacterRenderer::install(TextRenderer& renderer) {
	(renderer_ = &renderer)->addFontListener(*this);
	fontChanged();
}

/// @see ISpecialCharacterRenderer#uninstall
void DefaultSpecialCharacterRenderer::uninstall() {
	renderer_->removeFontListener(*this);
	renderer_ = 0;
}


// FontSelector /////////////////////////////////////////////////////////////

namespace {
	inline HFONT copyFont(HFONT src) /*throw()*/ {
		LOGFONTW lf;
		::GetObjectW(src, sizeof(LOGFONTW), &lf);
		return ::CreateFontIndirectW(&lf);
	}
}

struct FontSelector::Fontset {
	MANAH_UNASSIGNABLE_TAG(Fontset);
public:
	WCHAR faceName[LF_FACESIZE];
	HFONT regular, bold, italic, boldItalic;
	explicit Fontset(const WCHAR* name) /*throw()*/ : regular(0), bold(0), italic(0), boldItalic(0) {
		if(wcslen(name) >= LF_FACESIZE)
			throw length_error("name");
		wcscpy(faceName, name);
	}
	Fontset(const Fontset& rhs) : regular(0), bold(0), italic(0), boldItalic(0) {
		wcscpy(faceName, rhs.faceName);
	}
	~Fontset() /*throw()*/ {
		clear(L"");
	}
	void clear(const WCHAR* newName = 0) {
		if(newName != 0 && wcslen(newName) >= LF_FACESIZE)
			throw length_error("newName");
		::DeleteObject(regular);
		::DeleteObject(bold);
		::DeleteObject(italic);
		::DeleteObject(boldItalic);
		regular = bold = italic = boldItalic = 0;
		if(newName != 0)
			wcscpy(faceName, newName);
	}
};

FontSelector::FontAssociations FontSelector::defaultAssociations_;

/// Constructor.
FontSelector::FontSelector() : primaryFont_(new Fontset(L"")), shapingControlsFont_(0), linkedFonts_(0) {
	resetPrimaryFont(ScreenDC(), copyFont(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT))));
}

/// Copy-constructor.
FontSelector::FontSelector(const FontSelector& rhs) : primaryFont_(new Fontset(L"")), shapingControlsFont_(0), linkedFonts_(0) {
	resetPrimaryFont(ScreenDC(), copyFont(rhs.primaryFont_->regular));
	for(map<int, Fontset*>::const_iterator i(rhs.associations_.begin()), e(rhs.associations_.end()); i != e; ++i)
		associations_.insert(make_pair(i->first, new Fontset(i->second->faceName)));
	if(rhs.linkedFonts_ != 0) {
		linkedFonts_ = new vector<Fontset*>;
		for(vector<Fontset*>::const_iterator i(rhs.linkedFonts_->begin()), e(rhs.linkedFonts_->end()); i != e; ++i)
			linkedFonts_->push_back(new Fontset(**i));
	}
}

/// Destructor.
FontSelector::~FontSelector() /*throw()*/ {
	::DeleteObject(shapingControlsFont_);
	delete primaryFont_;
	for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
		delete i->second;
	if(linkedFonts_ != 0) {
		for(vector<Fontset*>::iterator i(linkedFonts_->begin()); i != linkedFonts_->end(); ++i)
			delete *i;
		delete linkedFonts_;
	}
}

/**
 * Enables or disables the font linking feature for CJK. When this method is called,
 * @c #fontChanged method of the derived class will be called.
 * @param enable set true to enable
 */
void FontSelector::enableFontLinking(bool enable /* = true */) /*throw()*/ {
	if(enable) {
		if(linkedFonts_ == 0)
			linkedFonts_ = new std::vector<Fontset*>;
		linkPrimaryFont();
	} else if(linkedFonts_ != 0) {
		delete linkedFonts_;
		linkedFonts_ = 0;
	}
}

inline void FontSelector::fireFontChanged() {
	fontChanged();
	listeners_.notify(&IFontSelectorListener::fontChanged);
}

namespace {
	int CALLBACK checkFontInstalled(ENUMLOGFONTEXW*, NEWTEXTMETRICEXW*, DWORD, LPARAM param) {
		*reinterpret_cast<bool*>(param) = true;
		return 0;
	}
}

/// Returns the default font association (fallback) map.
const FontSelector::FontAssociations& FontSelector::getDefaultFontAssociations() /*throw()*/ {
	if(defaultAssociations_.empty()) {
		defaultAssociations_[Script::ARABIC] = L"Microsoft Sans Serif";
		defaultAssociations_[Script::CYRILLIC] = L"Microsoft Sans Serif";
		defaultAssociations_[Script::GREEK] = L"Microsoft Sans Serif";
		defaultAssociations_[Script::HANGUL] = L"Gulim";
		defaultAssociations_[Script::HEBREW] = L"Microsoft Sans Serif";
//		defaultAssociations_[Script::HIRAGANA] = L"MS P Gothic";
//		defaultAssociations_[Script::KATAKANA] = L"MS P Gothic";
		defaultAssociations_[Script::LATIN] = L"Tahoma";
		defaultAssociations_[Script::THAI] = L"Tahoma";
		// Windows 2000
		defaultAssociations_[Script::ARMENIAN] = L"Sylfaen";
		defaultAssociations_[Script::DEVANAGARI] = L"Mangal";
		defaultAssociations_[Script::GEORGIAN] = L"Sylfaen";	// partial support?
		defaultAssociations_[Script::TAMIL] = L"Latha";
		// Windows XP
		defaultAssociations_[Script::GUJARATI] = L"Shruti";
		defaultAssociations_[Script::GURMUKHI] = L"Raavi";
		defaultAssociations_[Script::KANNADA] = L"Tunga";
		defaultAssociations_[Script::SYRIAC] = L"Estrangelo Edessa";
		defaultAssociations_[Script::TELUGU] = L"Gautami";
		defaultAssociations_[Script::THAANA] = L"MV Boli";
		// Windows XP SP2
		defaultAssociations_[Script::BENGALI] = L"Vrinda";
		defaultAssociations_[Script::MALAYALAM] = L"Kartika";
		// Windows Vista
		defaultAssociations_[Script::CANADIAN_ABORIGINAL] = L"Euphemia";
		defaultAssociations_[Script::CHEROKEE] = L"Plantagenet";
		defaultAssociations_[Script::ETHIOPIC] = L"Nyala";
		defaultAssociations_[Script::KHMER] = L"DaunPenh";	// or "MoolBoran"
		defaultAssociations_[Script::LAO] = L"DokChampa";
		defaultAssociations_[Script::MONGOLIAN] = L"Mongolian Baiti";
		defaultAssociations_[Script::ORIYA] = L"Kalinga";
		defaultAssociations_[Script::SINHALA] = L"Iskoola Pota";
		defaultAssociations_[Script::TIBETAN] = L"Microsoft Himalaya";
		defaultAssociations_[Script::YI] = L"Yi Baiti";
		// CJK
		const LANGID uiLang = getUserCJKLanguage();
		switch(PRIMARYLANGID(uiLang)) {	// yes, this is not enough...
		case LANG_CHINESE:
			defaultAssociations_[Script::HAN] = (SUBLANGID(uiLang) == SUBLANG_CHINESE_TRADITIONAL
				&& SUBLANGID(uiLang) == SUBLANG_CHINESE_HONGKONG) ? L"PMingLiu" : L"SimSun"; break;
		case LANG_JAPANESE:
			defaultAssociations_[Script::HAN] = L"MS P Gothic"; break;
		case LANG_KOREAN:
			defaultAssociations_[Script::HAN] = L"Gulim"; break;
		default:
			{
				ScreenDC dc;
				bool installed = false;
				LOGFONTW lf;
				memset(&lf, 0, sizeof(LOGFONTW));
#define ASCENSION_SELECT_INSTALLED_FONT(charset, fontname)		\
	lf.lfCharSet = charset;										\
	wcscpy(lf.lfFaceName, fontname);							\
	::EnumFontFamiliesExW(dc.get(), &lf,						\
		reinterpret_cast<FONTENUMPROCW>(checkFontInstalled),	\
		reinterpret_cast<LPARAM>(&installed), 0);				\
	if(installed) {												\
		defaultAssociations_[Script::HAN] = lf.lfFaceName;		\
		break;													\
	}
				ASCENSION_SELECT_INSTALLED_FONT(GB2312_CHARSET, L"SimSun")
				ASCENSION_SELECT_INSTALLED_FONT(SHIFTJIS_CHARSET, L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af")	// "ＭＳ Ｐゴシック"
				ASCENSION_SELECT_INSTALLED_FONT(HANGUL_CHARSET, L"Gulim")
				ASCENSION_SELECT_INSTALLED_FONT(CHINESEBIG5_CHARSET, L"PMingLiu")
#undef ASCENSION_SELECT_INSTALLED_FONT
			}
			break;
		}
		if(defaultAssociations_.find(Script::HAN) != defaultAssociations_.end())
			defaultAssociations_[Script::HIRAGANA] = defaultAssociations_[Script::KATAKANA] = defaultAssociations_[Script::HAN];
	}
	return defaultAssociations_;
}

/**
 * Returns the font associated to the specified script.
 * @param script the language. set to @c Script#COMMON to get the primary font. other special
 * script values @c Script#UNKNOWN, @c Script#INHERITED and @c Script#KATAKANA_OR_HIRAGANA can't set
 * @param bold true to get the bold variant
 * @param italic true to get the italic variant
 * @return the primary font if @a script is @c Script#COMMON. otherwise, a fallbacked font or @c null
 * @throw UnknownValueException<int> @a script is invalid
 * @see #linkedFont, #setFont
 */
HFONT FontSelector::font(int script /* = Script::COMMON */, bool bold /* = false */, bool italic /* = false */) const {
	if(script <= Script::FIRST_VALUE || script == Script::INHERITED
			|| script == Script::KATAKANA_OR_HIRAGANA || script >= Script::LAST_VALUE)
		throw UnknownValueException("script");
	if(script == Script::COMMON)
		return fontInFontset(*const_cast<FontSelector*>(this)->primaryFont_, bold, italic);
	else {
		map<int, Fontset*>::iterator i = const_cast<FontSelector*>(this)->associations_.find(script);
		return (i != associations_.end()) ? fontInFontset(*i->second, bold, italic) : 0;
	}
}

/// Returns the font to render shaping control characters.
HFONT FontSelector::fontForShapingControls() const /*throw()*/ {
	if(shapingControlsFont_ == 0)
		const_cast<FontSelector*>(this)->shapingControlsFont_ = ::CreateFontW(
			-(ascent_ + descent_ - internalLeading_), 0, 0, 0, FW_REGULAR, false, false, false,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
	return shapingControlsFont_;
}

/**
 * Returns the font in the given font set.
 * @param fontset the font set
 * @param bold set true to get a bold font
 * @param italic set true to get an italic font
 * @return the font
 */
HFONT FontSelector::fontInFontset(const Fontset& fontset, bool bold, bool italic) const /*throw()*/ {
	Fontset& fs = const_cast<Fontset&>(fontset);
	HFONT& font = bold ? (italic ? fs.boldItalic : fs.bold) : (italic ? fs.italic : fs.regular);
	if(font == 0) {
		font = ::CreateFontW(-(ascent_ + descent_ - internalLeading_),
			0, 0, 0, bold ? FW_BOLD : FW_REGULAR, italic, 0, 0, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fs.faceName);
		auto_ptr<DC> dc = deviceContext();
		HFONT oldFont = dc->selectObject(font);
		TEXTMETRICW tm;
		dc->getTextMetrics(tm);
		dc->selectObject(oldFont);
		// adjust to the primary ascent and descent
		if(tm.tmAscent > ascent_ && tm.tmAscent > 0) {	// we don't consider the descents...
			::DeleteObject(font);
			font = ::CreateFontW(-::MulDiv(ascent_ + descent_ - internalLeading_, ascent_, tm.tmAscent),
				0, 0, 0, bold ? FW_BOLD : FW_REGULAR, italic, 0, 0, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fs.faceName);
		}
	}
	return font;
}

/**
 * Returns the font linking to the primary font.
 * @param index the index in the link fonts chain
 * @param bold true to get the bold variant
 * @param italic true to get the italic variant
 * @return the font
 * @throw IndexOutOfBoundsException @a index is invalid
 * @see #font, #numberOfLinkedFonts
 */
HFONT FontSelector::linkedFont(size_t index, bool bold /* = false */, bool italic /* = false */) const {
	if(linkedFonts_ == 0)
		throw IndexOutOfBoundsException();
	return fontInFontset(*linkedFonts_->at(index), bold, italic);
}

namespace {
	manah::AutoBuffer<WCHAR> ASCENSION_FASTCALL mapFontFileNameToTypeface(const WCHAR* fileName) {
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
				manah::AutoBuffer<WCHAR> valueName(new WCHAR[maximumValueNameLength + 1]);
				manah::AutoBuffer<BYTE> value(new BYTE[maximumValueBytes]);
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
								manah::AutoBuffer<WCHAR> temp(new WCHAR[nameLength + 1]);
								wmemcpy(temp.get(), valueName.get(), nameLength);
								temp[nameLength] = 0;
								return temp;
							} else
								return manah::AutoBuffer<WCHAR>(0);
						}
					} else	// ERROR_NO_MORE_ITEMS
						break;
				}
			}
			::RegCloseKey(key);
		}
		return manah::AutoBuffer<WCHAR>(0);
	}
} // namespace @0

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
			manah::AutoBuffer<BYTE> data(new BYTE[bytes]);
			if(ERROR_SUCCESS == ::RegQueryValueExW(key, primaryFont_->faceName, 0, &type, data.get(), &bytes)) {
				const WCHAR* sz = reinterpret_cast<WCHAR*>(data.get());
				const WCHAR* const e = sz + bytes / sizeof(WCHAR);
				for(; sz < e; sz += wcslen(sz) + 1) {
					const WCHAR* comma = wcschr(sz, L',');
					if(comma != 0 && comma[1] != 0)	// "<file-name>,<typeface>"
						linkedFonts_->push_back(new Fontset(comma + 1));
					else {	// "<file-name>"
						manah::AutoBuffer<WCHAR> typeface(mapFontFileNameToTypeface(sz));
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

void FontSelector::resetPrimaryFont(DC& dc, HFONT font) {
	assert(font != 0);
	// update the font metrics
	TEXTMETRICW tm;
	HFONT oldFont = dc.selectObject(primaryFont_->regular = font);
	dc.getTextMetrics(tm);
	ascent_ = tm.tmAscent;
	descent_ = tm.tmDescent;
	internalLeading_ = tm.tmInternalLeading;
	externalLeading_ = tm.tmExternalLeading;
	averageCharacterWidth_ = (tm.tmAveCharWidth > 0) ? tm.tmAveCharWidth : ::MulDiv(tm.tmHeight, 56, 100);
	dc.selectObject(oldFont);
	// real name is needed for font linking
	LOGFONTW lf;
	::GetObjectW(primaryFont_->regular, sizeof(LOGFONTW), &lf);
	wcscpy(primaryFont_->faceName, lf.lfFaceName);
}

/**
 * Sets the primary font and the association table.
 * @param faceName the typeface name of the primary font. if this is @c null, the primary font
 * will not change
 * @param height the height of the font in logical units. if @a faceName is @c null, this value
 * will be ignored
 * @param associations the association table. script values @c Script#COMMON, @c Script#UNKNOWN,
 * @c Script#INHERITED and @c Script#KATAKANA_OR_HIRAGANA can't set. if this value is @c null,
 * the current associations will not be changed
 * @throw UnknownValueException<int> any script of @a associations is invalid
 * @throw std#length_error the length of @a faceName or any typeface name of @a associations
 * exceeds @c LF_FACESIZE
 */
void FontSelector::setFont(const WCHAR* faceName, int height, const FontAssociations* associations) {
	// check the arguments
	if(faceName != 0 && wcslen(faceName) >= LF_FACESIZE)
		throw length_error("the primary typeface name is too long.");
	else if(associations != 0) {
		for(FontAssociations::const_iterator i = associations->begin(); i != associations->end(); ++i) {
			if(i->first == Script::COMMON || i->first == Script::UNKNOWN
					|| i->first == Script::INHERITED || i->first == Script::KATAKANA_OR_HIRAGANA)
				throw UnknownValueException("the association script is invalid.");
			else if(i->second.length() >= LF_FACESIZE)
				throw length_error("the association font name is too long.");
		}
	}
	// reset the association table
	if(associations != 0) {
		for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
			delete i->second;
		associations_.clear();
		for(FontAssociations::const_iterator i = associations->begin(); i != associations->end(); ++i)
			associations_.insert(make_pair(i->first, new Fontset(i->second.c_str())));
	}
	// reset the primary font
	bool notified = false;
	if(faceName != 0) {
		primaryFont_->clear(faceName);
		for(map<int, Fontset*>::iterator i = associations_.begin(); i != associations_.end(); ++i)
			i->second->clear();
		// create the primary font and reset the text metrics
		resetPrimaryFont(*getDeviceContext(), ::CreateFontW(height, 0, 0, 0, FW_REGULAR, false, false, false,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, faceName));
		// reset the fontset for rendering shaping control characters
		if(shapingControlsFont_ != 0) {
			::DeleteObject(shapingControlsFont_);
			shapingControlsFont_ = 0;
		}
		if(linkedFonts_ != 0) {
			linkPrimaryFont();	// this calls fireFontChanged()
			notified = true;
		}
	}
	if(!notified)
		fireFontChanged();
}


// TextRenderer /////////////////////////////////////////////////////////////

namespace {
	inline int calculateMemoryBitmapSize(int src) /*throw()*/ {
		const int UNIT = 32;
		return (src % UNIT != 0) ? src + UNIT - src % UNIT : src;
	}
} // namespace @0

/**
 * Constructor.
 * @param presentation the presentation
 * @param enableDoubleBuffering set true to use double-buffering for non-flicker drawing
 */
TextRenderer::TextRenderer(Presentation& presentation, bool enableDoubleBuffering) :
		LineLayoutBuffer(presentation.document(), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		presentation_(presentation), enablesDoubleBuffering_(enableDoubleBuffering) {
	setLayoutInformation(this, false);
	setFont(0, 0, &getDefaultFontAssociations());
	switch(PRIMARYLANGID(getUserDefaultUILanguage())) {
	case LANG_CHINESE:
	case LANG_JAPANESE:
	case LANG_KOREAN:
		enableFontLinking();
		break;
	}
//	updateViewerSize(); ???
}

/// Copy-constructor.
TextRenderer::TextRenderer(const TextRenderer& rhs) : FontSelector(rhs),
		LineLayoutBuffer(rhs.presentation_.document(), ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, true),
		presentation_(rhs.presentation_), enablesDoubleBuffering_(rhs.enablesDoubleBuffering_) {
	setLayoutInformation(this, false);
//	updateViewerSize(); ???
}

/// Destructor.
TextRenderer::~TextRenderer() /*throw()*/ {
//	getTextViewer().removeDisplaySizeListener(*this);
//	layouts_.removeVisualLinesListener(*this);
}

/// @see FontSelector#fontChanged
void TextRenderer::fontChanged() {
	invalidate();
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != 0) {
		BITMAP b;
		memoryBitmap_.getBitmap(b);
		if(b.bmHeight != calculateMemoryBitmapSize(linePitch()))
			memoryBitmap_.reset();
	}
}

/// @see ILayoutInformationProvider#getFontSelector
const FontSelector& TextRenderer::getFontSelector() const /*throw()*/ {
	return *this;
}

/// @see ILayoutInformationProvider#getPresentation
const Presentation& TextRenderer::getPresentation() const /*throw()*/ {
	return presentation_;
}

/// @see ILayoutInformationProvider#getSpecialCharacterRenderer
ISpecialCharacterRenderer* TextRenderer::getSpecialCharacterRenderer() const /*throw()*/ {
	return specialCharacterRenderer_.get();
}

/**
 * Returns the indentation of the specified visual line from the left most.
 * @param line the line number
 * @param subline the visual subline number
 * @return the indentation in pixel
 * @throw kernel#BadPositionException @a line is invalid
 * @throw IndexOutOfBoundsException @a subline is invalid
 */
int TextRenderer::lineIndent(length_t line, length_t subline) const {
	const LayoutSettings& c = getLayoutSettings();
	if(c.alignment == ALIGN_LEFT || c.justifiesLines)
		return 0;
	else {
		int width = getWidth();
		switch(c.alignment) {
		case ALIGN_RIGHT:
			return width - lineLayout(line).sublineWidth(subline);
		case ALIGN_CENTER:
			return (width - lineLayout(line).sublineWidth(subline)) / 2;
		default:
			return 0;
		}
	}
}

/**
 * Returns the pitch of each lines (height + space).
 * @see FontSelector#lineHeight
 */
int TextRenderer::linePitch() const /*throw()*/ {
	return lineHeight() + max(getLayoutSettings().lineSpacing, lineGap());
}

/**
 * Renders the specified logical line to the output device.
 * @param line the line number
 * @param dc the device context
 * @param x the x-coordinate of the position to draw
 * @param y the y-coordinate of the position to draw
 * @param paintRect the region to draw
 * @param clipRect the clipping region
 * @param selection the selection
 */
void TextRenderer::renderLine(length_t line, DC& dc, int x, int y,
		const RECT& paintRect, const RECT& clipRect, const LineLayout::Selection* selection) const /*throw()*/ {
	if(!enablesDoubleBuffering_) {
		lineLayout(line).draw(dc, x, y, paintRect, clipRect, selection);
		return;
	}

	const LineLayout& layout = lineLayout(line);
	const int dy = linePitch();

	// skip to the subline needs to draw
	const int top = max(paintRect.top, clipRect.top);
	length_t subline = (y + dy >= top) ? 0 : (top - (y + dy)) / dy;
	if(subline >= layout.numberOfSublines())
		return;	// this logical line does not need to draw
	y += static_cast<int>(dy * subline);

	TextRenderer& self = *const_cast<TextRenderer*>(this);
	if(memoryDC_.get() == 0)		
		self.memoryDC_ = deviceContext()->createCompatibleDC();
	const int horizontalResolution = calculateMemoryBitmapSize(dc.getDeviceCaps(HORZRES));
	if(memoryBitmap_.get() != 0) {
		BITMAP b;
		memoryBitmap_.getBitmap(b);
		if(b.bmWidth < horizontalResolution)
			self.memoryBitmap_.reset();
	}
	if(memoryBitmap_.get() == 0)
		self.memoryBitmap_ = Bitmap::createCompatibleBitmap(*getDeviceContext(), horizontalResolution, calculateMemoryBitmapSize(dy));
	memoryDC_->selectObject(memoryBitmap_.use());

	const long left = max(paintRect.left, clipRect.left), right = min(paintRect.right, clipRect.right);
	x -= left;
	manah::win32::Rect offsetedPaintRect(paintRect), offsetedClipRect(clipRect);
	offsetedPaintRect.offset(-left, -y);
	offsetedClipRect.offset(-left, -y);
	for(; subline < layout.numberOfSublines() && offsetedPaintRect.bottom >= 0;
			++subline, y += dy, offsetedPaintRect.offset(0, -dy), offsetedClipRect.offset(0, -dy)) {
		layout.draw(subline, *memoryDC_, x, 0, offsetedPaintRect, offsetedClipRect, selection);
		dc.bitBlt(left, y, right - left, dy, memoryDC_->get(), 0, 0, SRCCOPY);
	}
}

/**
 * Sets the special character renderer.
 * @param newRenderer the new renderer or @c null
 * @param delegateOwnership set true to transfer the ownership into the callee
 * @throw std#invalid_argument @a newRenderer is already registered
 */
void TextRenderer::setSpecialCharacterRenderer(ISpecialCharacterRenderer* newRenderer, bool delegateOwnership) {
	if(newRenderer != 0 && newRenderer == specialCharacterRenderer_.get())
		throw invalid_argument("the specified renderer is already registered.");
	if(specialCharacterRenderer_.get() != 0)
		specialCharacterRenderer_->uninstall();
	specialCharacterRenderer_.reset(newRenderer, delegateOwnership);
	newRenderer->install(*this);
	invalidate();
}


// TextViewer.VerticalRulerDrawer ///////////////////////////////////////////

using viewers::TextViewer;

/**
 * Draws the vertical ruler.
 * @param dc the device context
 */
void TextViewer::VerticalRulerDrawer::draw(PaintDC& dc) {
	if(width() == 0)
		return;

	const RECT& paintRect = dc.paintStruct().rcPaint;
	const TextRenderer& renderer = viewer_.textRenderer();
	RECT clientRect;
	viewer_.getClientRect(clientRect);
	if((configuration_.alignment == ALIGN_LEFT && paintRect.left >= clientRect.left + width())
			|| (configuration_.alignment == ALIGN_RIGHT && paintRect.right < clientRect.right - width()))
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		manah::win32::DumpContext() << L"@VerticalRulerDrawer.draw draws y = " << paintRect.top << L" ~ " << paintRect.bottom << L"\n";
#endif // _DEBUG

	const int savedCookie = dc.save();
	const bool alignLeft = configuration_.alignment == ALIGN_LEFT;
	const int imWidth = configuration_.indicatorMargin.visible ? configuration_.indicatorMargin.width : 0;

	int left;
	DC* dcex;
	if(enablesDoubleBuffering_) {
		if(memoryDC_.get() == 0)
			memoryDC_ = viewer_.getDC().createCompatibleDC();
		if(memoryBitmap_.get() == 0)
			memoryBitmap_ = Bitmap::createCompatibleBitmap(dc,
				width(), clientRect.bottom - clientRect.top + ::GetSystemMetrics(SM_CYHSCROLL));
		memoryDC_->selectObject(memoryBitmap_.get());
		dcex = memoryDC_.get();
		left = 0;
	} else {
		dcex = &dc;
		left = alignLeft ? clientRect.left : clientRect.right - width();
	}
	const int right = left + width();

	// まず、描画領域全体を描いておく
	if(configuration_.indicatorMargin.visible) {
		// インジケータマージンの境界線と内側
		const int borderX = alignLeft ? left + imWidth - 1 : right - imWidth;
		HPEN oldPen = dcex->selectObject(indicatorMarginPen_.use());
		HBRUSH oldBrush = dcex->selectObject(indicatorMarginBrush_.use());
		dcex->patBlt(alignLeft ? left : borderX + 1, paintRect.top, imWidth, paintRect.bottom - paintRect.top, PATCOPY);
		dcex->moveTo(borderX, paintRect.top);
		dcex->lineTo(borderX, paintRect.bottom);
		dcex->selectObject(oldPen);
		dcex->selectObject(oldBrush);
	}
	if(configuration_.lineNumbers.visible) {
		// background of the line numbers
		HBRUSH oldBrush = dcex->selectObject(lineNumbersBrush_.use());
		dcex->patBlt(alignLeft ? left + imWidth : left, paintRect.top, right - imWidth, paintRect.bottom, PATCOPY);
		// border of the line numbers
		if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			HPEN oldPen = dcex->selectObject(lineNumbersPen_.use());
			const int x = (alignLeft ? right : left + 1) - configuration_.lineNumbers.borderWidth;
			dcex->moveTo(x, 0/*paintRect.top*/);
			dcex->lineTo(x, paintRect.bottom);
			dcex->selectObject(oldPen);
		}
		dcex->selectObject(oldBrush);

		// 次の準備...
		dcex->setBkMode(TRANSPARENT);
		dcex->setTextColor(systemColors.get(configuration_.lineNumbers.textColor.foreground, COLOR_WINDOWTEXT));
		dcex->setTextCharacterExtra(0);	// 行番号表示は文字間隔の設定を無視
		dcex->selectObject(viewer_.textRenderer().font());
	}

	// 行番号描画の準備
	Alignment lineNumbersAlignment;
	int lineNumbersX;
	if(configuration_.lineNumbers.visible) {
		lineNumbersAlignment = configuration_.lineNumbers.alignment;
		if(lineNumbersAlignment == ALIGN_AUTO)
			lineNumbersAlignment = alignLeft ? ALIGN_RIGHT : ALIGN_LEFT;
		switch(lineNumbersAlignment) {
		case ALIGN_LEFT:
			lineNumbersX = alignLeft ?
				left + imWidth + configuration_.lineNumbers.leadingMargin : left + configuration_.lineNumbers.trailingMargin + 1;
			dcex->setTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_RIGHT:
			lineNumbersX = alignLeft ?
				right - configuration_.lineNumbers.trailingMargin - 1 : right - imWidth - configuration_.lineNumbers.leadingMargin;
			dcex->setTextAlign(TA_RIGHT | TA_TOP | TA_NOUPDATECP);
			break;
		case ALIGN_CENTER:	// 中央揃えなんて誰も使わんと思うけど...
			lineNumbersX = alignLeft ?
				left + (imWidth + configuration_.lineNumbers.leadingMargin + width() - configuration_.lineNumbers.trailingMargin) / 2
				: right - (width() - configuration_.lineNumbers.trailingMargin + imWidth + configuration_.lineNumbers.leadingMargin) / 2;
			dcex->setTextAlign(TA_CENTER | TA_TOP | TA_NOUPDATECP);
			break;
		}
	}

	// 1 行ずつ細かい描画
	length_t line, visualSublineOffset;
	const length_t lines = viewer_.document().numberOfLines();
	viewer_.mapClientYToLine(paintRect.top, &line, &visualSublineOffset);	// $friendly-access
	if(visualSublineOffset > 0)	// 描画開始は次の論理行から...
		++line;
	int y = viewer_.mapLineToClientY(line, false);
	if(y != 32767 && y != -32768) {
		while(y < paintRect.bottom && line < lines) {
			const LineLayout& layout = renderer.lineLayout(line);
			const int nextY = y + static_cast<int>(layout.numberOfSublines() * renderer.linePitch());
			if(nextY >= paintRect.top) {
				// 派生クラスにインジケータマージンの描画機会を与える
				if(configuration_.indicatorMargin.visible) {
					RECT rect = {
						alignLeft ? left : right - configuration_.indicatorMargin.width,
						y, alignLeft ? left + configuration_.indicatorMargin.width : right,
						y + renderer.linePitch()};
					viewer_.drawIndicatorMargin(line, *dcex, rect);
				}

				// draw line number digits
				if(configuration_.lineNumbers.visible) {
					wchar_t buffer[32];
#if(_MSC_VER < 1400)
					swprintf(buffer, L"%lu", line + configuration_.lineNumbers.startValue);
#else
					swprintf(buffer, MANAH_COUNTOF(buffer), L"%lu", line + configuration_.lineNumbers.startValue);
#endif // _MSC_VER < 1400
					UINT option;
					switch(configuration_.lineNumbers.digitSubstitution) {
					case DST_CONTEXTUAL:
					case DST_NOMINAL:		option = ETO_NUMERICSLATIN; break;
					case DST_NATIONAL:		option = ETO_NUMERICSLOCAL; break;
					case DST_USER_DEFAULT:	option = 0; break;
					}
					dcex->extTextOut(lineNumbersX, y, option, 0, buffer, static_cast<int>(wcslen(buffer)), 0);
				}
			}
			++line;
			y = nextY;
		}
	}

	if(enablesDoubleBuffering_)
		dc.bitBlt(alignLeft ? clientRect.left : clientRect.right - width(), paintRect.top,
			right - left, paintRect.bottom - paintRect.top, memoryDC_->get(), 0, paintRect.top, SRCCOPY);
	dc.restore(savedCookie);
}

/// Recalculates the width of the vertical ruler.
void TextViewer::VerticalRulerDrawer::recalculateWidth() /*throw()*/ {
	int newWidth = 0;
	if(configuration_.lineNumbers.visible) {
		const uchar newLineNumberDigits = getLineNumberMaxDigits();
		if(newLineNumberDigits != lineNumberDigitsCache_) {
			// the width of the line numbers area is determined by the maximum width of glyphs of 0..9
			ClientDC dc = viewer_.getDC();
			HFONT oldFont = dc.selectObject(viewer_.textRenderer().font());
			SCRIPT_STRING_ANALYSIS ssa;
			AutoZero<SCRIPT_CONTROL> sc;
			AutoZero<SCRIPT_STATE> ss;
			HRESULT hr;
			switch(configuration_.lineNumbers.digitSubstitution) {
			case DST_CONTEXTUAL:
			case DST_NOMINAL:
				break;
			case DST_NATIONAL:
				ss.fDigitSubstitute = 1;
				break;
			case DST_USER_DEFAULT:
				hr = ::ScriptApplyDigitSubstitution(&userSettings.getDigitSubstitution(), &sc, &ss);
				break;
			}
			dc.setTextCharacterExtra(0);
			hr = ::ScriptStringAnalyse(dc.use(), L"0123456789", 10,
				10 * 3 / 2 + 16, -1, SSA_FALLBACK | SSA_GLYPHS | SSA_LINK, 0, &sc, &ss, 0, 0, 0, &ssa);
			dc.selectObject(oldFont);
			int glyphWidths[10];
			hr = ::ScriptStringGetLogicalWidths(ssa, glyphWidths);
			int maxGlyphWidth = *max_element(glyphWidths, MANAH_ENDOF(glyphWidths));
			lineNumberDigitsCache_ = newLineNumberDigits;
			if(maxGlyphWidth != 0) {
				newWidth += max<uchar>(newLineNumberDigits, configuration_.lineNumbers.minimumDigits) * maxGlyphWidth;
				newWidth += configuration_.lineNumbers.leadingMargin + configuration_.lineNumbers.trailingMargin;
				if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE)
					newWidth += configuration_.lineNumbers.borderWidth;
			}
		}
	}
	if(configuration_.indicatorMargin.visible)
		newWidth += configuration_.indicatorMargin.width;
	if(newWidth != width_) {
		width_ = newWidth;
		viewer_.invalidateRect(0, false);
		viewer_.updateCaretPosition();
	}
}

///
void TextViewer::VerticalRulerDrawer::updateGDIObjects() /*throw()*/ {
	indicatorMarginPen_.reset();
	indicatorMarginBrush_.reset();
	if(configuration_.indicatorMargin.visible) {
		indicatorMarginPen_ = Pen::create(PS_SOLID, 1,
			systemColors.get(configuration_.indicatorMargin.borderColor, COLOR_3DSHADOW));
		indicatorMarginBrush_ = Brush::create(systemColors.get(configuration_.indicatorMargin.color, COLOR_3DFACE));
	}

	lineNumbersPen_.reset();
	lineNumbersBrush_.reset();
	if(configuration_.lineNumbers.visible) {
		if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::SOLID)	// 実線
			lineNumbersPen_ = Pen::create(PS_SOLID, configuration_.lineNumbers.borderWidth,
				systemColors.get(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT));
		else if(configuration_.lineNumbers.borderStyle != VerticalRulerConfiguration::LineNumbers::NONE) {
			LOGBRUSH brush;
			brush.lbColor = systemColors.get(configuration_.lineNumbers.borderColor, COLOR_WINDOWTEXT);
			brush.lbStyle = BS_SOLID;
			if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DASHED)	// 破線
				lineNumbersPen_ = Pen::create(
					PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, configuration_.lineNumbers.borderWidth, brush, 0, 0);
			else if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DASHED_ROUNDED)	// 丸破線
				lineNumbersPen_ = Pen::create(
					PS_GEOMETRIC | PS_DASH | PS_ENDCAP_ROUND, configuration_.lineNumbers.borderWidth, brush, 0, 0);
			else if(configuration_.lineNumbers.borderStyle == VerticalRulerConfiguration::LineNumbers::DOTTED)	// 点線
				lineNumbersPen_ = Pen::create(PS_GEOMETRIC | PS_DOT, configuration_.lineNumbers.borderWidth, brush, 0, 0);
		}
		lineNumbersBrush_ = Brush::create(systemColors.get(configuration_.lineNumbers.textColor.background, COLOR_WINDOW));
	}
}
