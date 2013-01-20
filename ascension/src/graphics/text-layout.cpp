/**
 * @file text-layout.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2011
 * @date 2010-11-20 renamed from ascension/layout.cpp
 * @date 2012
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE, ...
#include <ascension/graphics/text-layout.hpp>
#include <ascension/graphics/text-layout-styles.hpp>
#include <ascension/graphics/text-run.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/rendering-device.hpp>
//#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/corelib/shared-library.hpp>
#include <ascension/corelib/text/character-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <limits>	// std.numeric_limits
#include <numeric>	// std.accumulate

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace std;
namespace k = ascension::kernel;

//#define TRACE_LAYOUT_CACHES
extern bool DIAGNOSE_INHERENT_DRAWING;


// file-local free functions ////////////////////////////////////////////////

namespace {
	inline bool isC0orC1Control(CodePoint c) /*throw()*/ {
		return c < 0x20 || c == 0x7f || (c >= 0x80 && c < 0xa0);
	}

	inline Scalar readingDirectionInt(ReadingDirection direction) {
		switch(direction) {
			case LEFT_TO_RIGHT:
				return +1;
			case RIGHT_TO_LEFT:
				return -1;
			default:
				throw UnknownValueException("direction");
		}
	}

	template<typename T> inline T& shrinkToFit(T& v) {
		swap(v, T(v));
		return v;
	}
} // namespace @0

void ascension::updateSystemSettings() /*throw()*/ {
	systemColors.update();
	userSettings.update();
}


// graphics.font free functions ///////////////////////////////////////////////////////////////////

void paintTextDecoration(PaintContext& context, const TextRun& run, const NativePoint& origin, const ComputedTextDecoration& style) {
	if(style.decorations.underline.style != Decorations::NONE || style.decorations.strikethrough.style != Decorations::NONE) {
		const win32::Handle<HDC>& dc = context.asNativeObject();
		int baselineOffset, underlineOffset, underlineThickness, linethroughOffset, linethroughThickness;
		if(getDecorationLineMetrics(dc, &baselineOffset, &underlineOffset, &underlineThickness, &linethroughOffset, &linethroughThickness)) {
			// draw underline
			if(style.decorations.underline.style != Decorations::NONE) {
				win32::Handle<HPEN> pen(createPen(
					style.decorations.underline.color.get_value_or(foregroundColor), underlineThickness, style.decorations.underline.style));
				HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.get()));
				const int underlineY = y + baselineOffset - underlineOffset + underlineThickness / 2;
				context.moveTo(geometry::make<NativePoint>(x, underlineY));
				context.lineTo(geometry::make<NativePoint>(x + width, underlineY));
				::SelectObject(dc.get(), oldPen);
			}
			// draw strikethrough line
			if(style.decorations.strikethrough.style != Decorations::NONE) {
				win32::Handle<HPEN> pen(createPen(
					style.decorations.strikethrough.color.get_value_or(foregroundColor), linethroughThickness, 1));
				HPEN oldPen = static_cast<HPEN>(::SelectObject(dc.get(), pen.get()));
				const int strikeoutY = y + baselineOffset - linethroughOffset + linethroughThickness / 2;
				context.moveTo(geometry::make<NativePoint>(x, strikeoutY));
				context.lineTo(geometry::make<NativePoint>(x + width, strikeoutY));
				::SelectObject(dc.get(), oldPen);
			}
		}
	}
}


// detail.* free function /////////////////////////////////////////////////////////////////////////

/**
 * @internal Paints border.
 * @param context The graphics context
 * @param rectangle The border box. This gives the edge surrounds the border
 * @param border The presentative style
 * @param writingMode The writing mode used to compute the directions and orientation of @a border
 */
void detail::paintBorder(PaintContext& context, const NativeRectangle& rectangle,
		const PhysicalFourSides<ComputedBorderSide>& border, const WritingMode& writingMode) {
	// TODO: not implemented.
	for(PhysicalFourSides<ComputedBorderSide>::const_iterator side(begin(border)), e(border.cend()); side != e; ++side) {
		if(!side->hasVisibleStyle() || side->computedWidth() <= 0)
			continue;
		if(!geometry::includes(context.boundsToPaint(), rectangle))
			continue;
		const Color& color = side->color;
		if(color.isFullyTransparent())
			continue;
		context.setStrokeStyle(shared_ptr<Paint>(new SolidColor(color)));
		context.setLineWidth(side->width);
//		context.setStrokeDashArray();
//		context.setStrokeDashOffset();
		context.beginPath();
		switch(static_cast<PhysicalDirection>(side - begin(border))) {
			case TOP:
				context
					.moveTo(geometry::make<NativePoint>(geometry::left(rectangle), geometry::top(rectangle)))
					.lineTo(geometry::make<NativePoint>(geometry::right(rectangle) + 1, geometry::top(rectangle)));
				break;
			case RIGHT:
				context
					.moveTo(geometry::make<NativePoint>(geometry::right(rectangle), geometry::top(rectangle)))
					.lineTo(geometry::make<NativePoint>(geometry::right(rectangle), geometry::bottom(rectangle) + 1));
				break;
			case BOTTOM:
				context
					.moveTo(geometry::make<NativePoint>(geometry::left(rectangle), geometry::bottom(rectangle)))
					.lineTo(geometry::make<NativePoint>(geometry::right(rectangle) + 1, geometry::bottom(rectangle)));
				break;
			case LEFT:
				context
					.moveTo(geometry::make<NativePoint>(geometry::left(rectangle), geometry::top(rectangle)))
					.lineTo(geometry::make<NativePoint>(geometry::left(rectangle), geometry::bottom(rectangle) + 1));
				break;
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		context.stroke();
	}
}


// TextRun ////////////////////////////////////////////////////////////////////////////////////////

/**
 * Returns 'allocation-rectangle' of this text run.
 * @see #borderRectangle, #contentRectangle
 */
FlowRelativeFourSides<Scalar> TextRun::allocationRectangle() const BOOST_NOEXCEPT {
	FlowRelativeFourSides<Scalar> rectangle(contentRectangle());
	if(const FlowRelativeFourSides<ComputedBorderSide>* const borderSides = borders()) {
		rectangle.start() -= borderSides->start().computedWidth();
		rectangle.end() += borderSides->end().computedWidth();
	}
	return rectangle;
}

/**
 * Returns 'border-rectangle' of this text run.
 * @see #allocationRectangle, #borders, #contentRectangle
 */
FlowRelativeFourSides<Scalar> TextRun::borderRectangle() const BOOST_NOEXCEPT {
	FlowRelativeFourSides<Scalar> rectangle(contentRectangle());
	if(const FlowRelativeFourSides<ComputedBorderSide>* const borderSides = borders()) {
		rectangle.before() -= borderSides->before().computedWidth();
		rectangle.after() += borderSides->after().computedWidth();
		rectangle.start() -= borderSides->start().computedWidth();
		rectangle.end() += borderSides->end().computedWidth();
	}
	return rectangle;
}

/**
 * Returns 'content-rectangle' of this text run.
 * @see #allocationRectangle, #borderRectangle
 */
FlowRelativeFourSides<Scalar> TextRun::contentRectangle() const BOOST_NOEXCEPT {
	FlowRelativeFourSides<Scalar> rectangle;
	rectangle.start() = leadingEdge(beginning());
	rectangle.end() = rectangle.start() + measure();
	const shared_ptr<const Font::Metrics> fontMetrics(font()->metrics());
	rectangle.before() = -fontMetrics->ascent();
	rectangle.after() = +fontMetrics->descent();
	return rectangle;
}


// FixedWidthTabExpander //////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param width The fixed width in pixels
 */
FixedWidthTabExpander::FixedWidthTabExpander(Scalar width) BOOST_NOEXCEPT : width_(width) {
}

/// @see TabExpander#nextTabStop
Scalar FixedWidthTabExpander::nextTabStop(Scalar ipd, Index) const BOOST_NOEXCEPT {
	return ipd - ipd % width_ + width_;
}


// TextLayout.LineArea ////////////////////////////////////////////////////////////////////////////

class TextLayout::LineArea {
public:
	// block-area
	Scalar endIndent() const;
	Scalar lineHeight() const;
	LineStackingStrategy lineStackingStrategy() const;
	Scalar spaceAfter() const;
	Scalar spaceBefore() const;
	Scalar startIndent() const;
	// line-area
	NativeRectangle maximumLineRectangle() const;
	NativeRectangle nominalRequestedLineRectangle() const;
	NativeRectangle perInlineHeightRectangle() const;
};


// InlineProgressionDimensionRangeIterator file-local class ///////////////////////////////////////

namespace {
	class InlineProgressionDimensionRangeIterator :
		public boost::iterator_facade<InlineProgressionDimensionRangeIterator,
			Range<Scalar>, input_iterator_tag, Range<Scalar>, ptrdiff_t
		> {
	public:
		InlineProgressionDimensionRangeIterator() /*throw()*/ : currentRun_(nullptr), lastRun_(nullptr) {}
		InlineProgressionDimensionRangeIterator(
			const Range<const TextLayout::TextRun* const*>& textRuns, const Range<Index>& characterRange,
			ReadingDirection scanningDirection, Scalar initialIpd);
		const Range<Index> characterRange() const /*throw()*/ {return characterRange_;}
		Range<Scalar> dereference() const;
		bool equal(const InlineProgressionDimensionRangeIterator& other) const /*throw()*/ {
			if(currentRun_ == nullptr)
				return other.isDone();
			else if(other.currentRun_ == nullptr)
				return isDone();
			return currentRun_ == other.currentRun_;
		}
		void increment() {return next(false);}
		ReadingDirection scanningDirection() const /*throw()*/ {
			return (currentRun_ <= lastRun_) ? LEFT_TO_RIGHT : RIGHT_TO_LEFT;
		}
	private:
		void next(bool initializing);
		bool isDone() const /*throw()*/ {return currentRun_ == lastRun_;}
	private:
		friend class boost::iterator_core_access;
		/*const*/ Range<Index> characterRange_;
		const TextLayout::TextRun* const* currentRun_;
		const TextLayout::TextRun* const* /*const*/ lastRun_;
		Scalar ipd_;
	};
}

InlineProgressionDimensionRangeIterator::InlineProgressionDimensionRangeIterator(
		const Range<const TextLayout::TextRun* const*>& textRuns, const Range<Index>& characterRange,
		ReadingDirection direction, Scalar initialIpd) : characterRange_(characterRange),
		currentRun_((direction == LEFT_TO_RIGHT) ? textRuns.beginning() - 1 : textRuns.end()),
		lastRun_((direction == LEFT_TO_RIGHT) ? textRuns.end() : textRuns.beginning() - 1),
		ipd_(initialIpd) {
	next(true);
}

Range<Scalar> InlineProgressionDimensionRangeIterator::dereference() const {
	if(isDone())
		throw NoSuchElementException();
	assert(intersects(**currentRun_, characterRange()));
	Scalar start, end;
	if(characterRange().beginning() > (*currentRun_)->beginning())
		start = (*currentRun_)->x(characterRange().beginning(), false);
	else
		start = ((*currentRun_)->readingDirection() == LEFT_TO_RIGHT) ? 0 : (*currentRun_)->totalWidth();
	if(characterRange().end() < (*currentRun_)->end())
		end = (*currentRun_)->x(characterRange().end(), false);
	else
		end = ((*currentRun_)->readingDirection() == LEFT_TO_RIGHT) ? (*currentRun_)->totalWidth() : 0;

	if(scanningDirection() == RIGHT_TO_LEFT) {
		start -= (*currentRun_)->totalWidth();
		end -= (*currentRun_)->totalWidth();
	}
	return Range<Scalar>(start + ipd_, end + ipd_);
}

void InlineProgressionDimensionRangeIterator::next(bool initializing) {
	if(isDone())
		throw NoSuchElementException();
	const TextLayout::TextRun* const* nextRun = currentRun_;
	Scalar nextIpd = ipd_;
	while(true) {
		if(scanningDirection() == LEFT_TO_RIGHT) {
			if(!initializing)
				nextIpd += (*nextRun)->totalWidth();
			++nextRun;
		} else {
			if(!initializing)
				nextIpd -= (*nextRun)->totalWidth();
			--nextRun;
		}
		if(nextRun != lastRun_ || intersects(**nextRun, characterRange()))
			break;
	}
	// commit
	currentRun_ = nextRun;
	ipd_ = nextIpd;
}


// TextLayout /////////////////////////////////////////////////////////////////////////////////////

// helpers for TextLayout.draw
namespace {
	const size_t MAXIMUM_RUN_LENGTH = 1024;
	inline win32::Handle<HPEN> createPen(const Color& color, int width, int style) {
		if(color.alpha() < 0xff)
			throw invalid_argument("color");
		LOGBRUSH brush;
		brush.lbColor = color.as<COLORREF>();
		brush.lbStyle = BS_SOLID;
		HPEN pen = nullptr;
		switch(style) {
		case 1:	// solid
			pen = (width == 1) ? ::CreatePen(PS_SOLID, 1, color.as<COLORREF>())
				: ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
		case 2:	// dashed
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DASH | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
		case 3:	// dotted
			pen = ::ExtCreatePen(PS_GEOMETRIC | PS_DOT | PS_ENDCAP_FLAT, width, &brush, 0, nullptr);
		}
		if(pen == nullptr)
			throw UnknownValueException("style");
		return win32::Handle<HPEN>(pen, &::DeleteObject);
	}
} // namespace @0

/**
 * @class ascension::graphics::font::TextLayout
 * @c TextLayout is an immutable graphical representation of styled text. Provides support for
 * drawing, cursor navigation, hit testing, text wrapping, etc.
 *
 * <h3>Coordinate system</h3>
 * All graphical information returned from a @c TextLayout object' method is relative to the origin
 * of @c TextLayout, which is the intersection of the start edge with the baseline of the first
 * line of @c TextLayout. The start edge is determined by the reading direction (inline progression
 * dimension) of the line. Also, coordinates passed into a @c TextLayout object's method are
 * assumed to be relative to the @c TextLayout object's origin.
 *
 * <h3>Constraints by Win32/Uniscribe</h3>
 * <del>A long run will be split into smaller runs automatically because Uniscribe rejects too long
 * text (especially @c ScriptShape and @c ScriptTextOut). For this reason, a combining character
 * will be rendered incorrectly if it is presented at the boundary. The maximum length of a run is
 * 1024.
 *
 * In present, this class supports only text layout horizontal against the output device.
 *
 * @note This class is not intended to be derived.
 * @see TextLayoutBuffer#lineLayout, TextLayoutBuffer#lineLayoutIfCached
 */

namespace {
	template<typename T, size_t staticCapacity>
	class AutoArray {
	public:
		typedef T ElementType;
		static const size_t STATIC_CAPACITY = staticCapacity;
	public:
		AutoArray() : capacity_(STATIC_CAPACITY) {
		}
		ElementType& operator[](size_t i) {
			return p_[i];
		}
		const ElementType& operator[](size_t i) const {
			return p_[i];
		}
		ElementType& at(size_t i) {
			if(i >= capacity_)
				throw out_of_range("i");
			return operator[](i);
		}
		const ElementType& at(size_t i) const {
			if(i >= capacity_)
				throw out_of_range("i");
			return operator[](i);
		}
		ElementType* get() const {
			return p_;
		}
		void reallocate(size_t n) {
			if(n <= STATIC_CAPACITY) {
				allocated_.reset();
				p_ = auto_;
				capacity_ = STATIC_CAPACITY;
			} else {
				if(n > capacity_) {
					allocated_.reset(new ElementType[n]);
					capacity_ = n;
				}
				p_ = allocated_.get();
			}
		}
	private:
		ElementType auto_[STATIC_CAPACITY];
		unique_ptr<ElementType[]> allocated_;
		size_t capacity_;
		ElementType* p_;
	};

	// TODO: this implementation is temporary, and should rewrite later
	class SillyLineMetrics : public LineMetrics {
	public:
		SillyLineMetrics(Scalar ascent, Scalar descent) /*throw()*/ : ascent_(ascent), descent_(descent) {}
	private:
		Scalar ascent() const /*throw()*/ {return ascent_;}
		DominantBaseline baseline() const /*throw()*/ {return DOMINANT_BASELINE_ALPHABETIC;}
		Scalar baselineOffset(AlignmentBaseline baseline) const /*throw()*/ {return 0;}
		Scalar descent() const /*throw()*/ {return descent_;}
//		Scalar leading() const /*throw()*/ {return 0;}
	private:
		Scalar ascent_, descent_;
	};
}

/**
 * Constructor.
 * @param text The text string to display
 * @param otherParameters The other parameters
 * @throw UnknownValueException @a writingMode or @a otherParameters.anchor is invalid
 */
TextLayout::TextLayout(const String& text,
		const TextLayout::ConstructionParameters& otherParameters /* = TextLayout::ConstructionParameters() */)
		: text_(text), writingMode_(otherParameters.writingMode), anchor_(otherParameters.anchor),
		dominantBaseline_(otherParameters.dominantBaseline), numberOfRuns_(0), numberOfLines_(0),
		maximumMeasure_(-1), wrappingMeasure_(otherParameters.textWrapping.measure) {

	// sanity checks...
	if(writingMode_.inlineFlowDirection != LEFT_TO_RIGHT && writingMode_.inlineFlowDirection != RIGHT_TO_LEFT)
		throw UnknownValueException("writingMode.inlineFlowDirection");
	if(anchor_ != TEXT_ANCHOR_START && anchor_ != TEXT_ANCHOR_MIDDLE && anchor_ != TEXT_ANCHOR_END)
		throw UnknownValueException("otherParameters.anchor");

	// handle logically empty line
	if(text_.empty()) {
		numberOfRuns_ = 0;
		numberOfLines_ = 1;
		maximumMeasure_ = 0;
		assert(isEmpty());
		return;
	}
#if 0
	// calculate the wrapping width
	if(layoutInformation.layoutSettings().lineWrap.wraps()) {
		wrapWidth_ = layoutInformation.width();
		if(ISpecialCharacterRenderer* scr = layoutInformation.specialCharacterRenderer()) {
			ISpecialCharacterRenderer::LayoutContext lc(context);
			lc.readingDirection = readingDirection();
			wrapWidth_ -= scr->getLineWrappingMarkWidth(lc);
		}
	}
#endif
	// split the text line into text runs as following steps:
	// 1. split the text into script runs (SCRIPT_ITEMs) by Uniscribe
	// 2. split each script runs into atomically-shapable runs (TextRuns) with StyledRunIterator
	// 3. generate glyphs for each text runs
	// 4. position glyphs for each text runs
	// 5. position each text runs
	// 6. justify each text runs if specified
	// 7. stack the lines

	// 1. split the text into script runs by Uniscribe
	HRESULT hr;

	// 1-1. configure Uniscribe's itemize
	win32::AutoZero<SCRIPT_CONTROL> control;
	win32::AutoZero<SCRIPT_STATE> initialState;
	initialState.uBidiLevel = (writingMode_.inlineFlowDirection == RIGHT_TO_LEFT) ? 1 : 0;
//	initialState.fOverrideDirection = 1;
	initialState.fInhibitSymSwap = otherParameters.inhibitSymmetricSwapping;
	initialState.fDisplayZWG = otherParameters.displayShapingControls;
	resolveNumberSubstitution(&otherParameters.numberSubstitution, control, initialState);	// ignore result...

	// 1-2. itemize
	// note that ScriptItemize can cause a buffer overflow (see Mozilla bug 366643)
	AutoArray<SCRIPT_ITEM, 128> scriptRuns;
	AutoArray<OPENTYPE_TAG, scriptRuns.STATIC_CAPACITY> scriptTags;
	int estimatedNumberOfScriptRuns = max(static_cast<int>(text_.length()) / 4, 2), numberOfScriptRuns;
	HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
		const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
	while(true) {
		scriptRuns.reallocate(estimatedNumberOfScriptRuns);
		scriptTags.reallocate(estimatedNumberOfScriptRuns);
		if(scriptItemizeOpenType != nullptr)
			hr = (*scriptItemizeOpenType)(text_.data(), static_cast<int>(text_.length()),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), scriptTags.get(), &numberOfScriptRuns);
		else
			hr = ::ScriptItemize(text_.data(), static_cast<int>(text_.length()),
				estimatedNumberOfScriptRuns, &control, &initialState, scriptRuns.get(), &numberOfScriptRuns);
		if(hr != E_OUTOFMEMORY)	// estimatedNumberOfRuns was enough...
			break;
		estimatedNumberOfScriptRuns *= 2;
	}
	if(otherParameters.disableDeprecatedFormatCharacters) {
		for(int i = 0; i < numberOfScriptRuns; ++i) {
			scriptRuns[i].a.s.fInhibitSymSwap = initialState.fInhibitSymSwap;
			scriptRuns[i].a.s.fDigitSubstitute = initialState.fDigitSubstitute;
		}
	}
	if(scriptItemizeOpenType == nullptr)
		fill_n(scriptTags.get(), numberOfScriptRuns, SCRIPT_TAG_UNKNOWN);

	// 2. split each script runs into text runs with StyledRunIterator
	vector<TextRun*> textRuns;
	vector<const StyledTextRun> styledRanges;
	const FontCollection& fontCollection = (otherParameters.fontCollection != nullptr) ? *otherParameters.fontCollection : systemFonts();
	TextRun::mergeScriptsAndStyles(text_.data(),
		scriptRuns.get(), scriptTags.get(), numberOfScriptRuns,
		fontCollection, otherParameters.defaultTextRunStyle, move(otherParameters.textRunStyles), textRuns, styledRanges);
	runs_.reset(new TextRun*[numberOfRuns_ = textRuns.size()]);
	copy(textRuns.begin(), textRuns.end(), runs_.get());
//	shrinkToFit(styledRanges_);

	// 3. generate glyphs for each text runs
	const win32::Handle<HDC> dc(detail::screenDC());
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->shape(dc, text_);
	TextRun::substituteGlyphs(Range<TextRun**>(runs_.get(), runs_.get() + numberOfRuns_), text_);

	// 4. position glyphs for each text runs
	for(size_t i = 0; i < numberOfRuns_; ++i)
		runs_[i]->positionGlyphs(dc, text_, SimpleStyledTextRunIterator(styledRanges, runs_[i]->beginning()));

	// 5. position each text runs
	String nominalFontFamilyName;
	FontProperties<> nominalFontProperties;
	resolveFontSpecifications(fontCollection,
		shared_ptr<const TextRunStyle>(), otherParameters.defaultTextRunStyle, &nominalFontFamilyName, &nominalFontProperties, nullptr);
	const shared_ptr<const Font> nominalFont(fontCollection.get(nominalFontFamilyName, nominalFontProperties));
	// wrap into visual lines and reorder runs in each lines
	if(numberOfRuns_ == 0 || wrappingMeasure_ == numeric_limits<Scalar>::max()) {
		numberOfLines_ = 1;
		lineOffsets_.reset(&SINGLE_LINE_OFFSETS);
		lineFirstRuns_.reset(&SINGLE_LINE_OFFSETS);
		// 5-2. reorder each text runs
		reorder();
		// 5-3. reexpand horizontal tabs
		expandTabsWithoutWrapping();
	} else {
		// 5-1. expand horizontal tabs and wrap into lines
		const TabExpander* tabExpander = otherParameters.tabExpander;
		unique_ptr<TabExpander> temp;
		if(tabExpander == nullptr) {
			// create default tab expander
			temp.reset(new FixedWidthTabExpander(nominalFont->metrics().averageCharacterWidth() * 8));
			tabExpander = temp.get();
		}
		wrap(*tabExpander);
		// 5-2. reorder each text runs
		reorder();
		// 5-3. reexpand horizontal tabs
		// TODO: not implemented.
		// 6. justify each text runs if specified
		if(otherParameters.textJustification != NONE_JUSTIFICATION)
			justify(otherParameters.textJustification);
	}

	// 7. stack the lines
	stackLines(otherParameters.lineStackingStrategy, *nominalFont, otherParameters.lineHeight);
}

/// Destructor.
TextLayout::~TextLayout() /*throw()*/ {
	for(size_t i = 0; i < numberOfRuns_; ++i)
		delete runs_[i];
	for(vector<const InlineArea*>::const_iterator i(inlineAreas_.begin()), e(inlineAreas_.end()); i != e; ++i)
		delete *i;
	if(numberOfLines() == 1) {
		assert(lineOffsets_.get() == &SINGLE_LINE_OFFSETS);
		lineOffsets_.release();
		assert(lineFirstRuns_.get() == &SINGLE_LINE_OFFSETS);
		lineFirstRuns_.release();
	}
	for(size_t i = 0; i < numberOfLines(); ++i)
		delete lineMetrics_[i];
}
#if 0
/**
 * Returns the computed text alignment of the line. The returned value may be
 * @c presentation#ALIGN_START or @c presentation#ALIGN_END.
 * @see #readingDirection, presentation#resolveTextAlignment
 */
TextAlignment TextLayout::alignment() const /*throw()*/ {
	if(style_.get() != nullptr && style_->readingDirection != INHERIT_TEXT_ALIGNMENT)
		style_->readingDirection;
	shared_ptr<const TextLineStyle> defaultStyle(lip_.presentation().defaultTextLineStyle());
	return (defaultStyle.get() != nullptr
		&& defaultStyle->alignment != INHERIT_TEXT_ALIGNMENT) ? defaultStyle->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT;
}
#endif

/**
 * Returns distance from the baseline of the first line to the baseline of the
 * specified line in pixels.
 * @param line The line number
 * @return The baseline position 
 * @throw kernel#BadPositionException @a line is greater than the number of lines
 */
double TextLayout::baseline(Index line) const {
	if(line >= numberOfLines())
		throw kernel::BadPositionException(kernel::Position(line, 0));
	else if(line == 0)
		return 0;
	double result = 0;
	for(Index i = 1; i <= line; ++i) {
		const LineMetrics& preceding = lineMetrics(i - 1);
		result += preceding.descent + preceding.leading;
		result += lineMetrics(i).ascent;
	}
	return result;
}

/**
 * Returns the smallest rectangle emcompasses the whole text of the line. It might not coincide
 * exactly the ascent, descent or overhangs of the text.
 * @return The size of the bounds
 * @see #blackBoxBounds, #bounds(const Range&lt;Index&gt;&amp;), #lineBounds
 */
FlowRelativeFourSides<Scalar> TextLayout::bounds() const BOOST_NOEXCEPT {
	// TODO: this implementation can't handle vertical text.
	FlowRelativeFourSides<Scalar> result;
	result.before() = /*-lineMetrics(0).leading()*/ - lineMetrics(0).ascent();
	result.after() = result.before();
	result.start() = numeric_limits<Scalar>::max();
	result.end() = numeric_limits<Scalar>::min();
	for(Index line = 0; line < numberOfLines(); ++line) {
		result.after() += lineMetrics(line).height();
		const Scalar lineStart = lineStartEdge(line);
		result.start() = min(lineStart, result.start());
		result.end() = max(lineStart + measure(line), result.end());
	}
	return result;
}

/**
 * Returns the smallest rectangle emcompasses all characters in the range. It might not coincide
 * exactly the ascent, descent or overhangs of the specified region of the text.
 * @param characterRange The character range
 * @return The bounds
 * @throw kernel#BadPositionException @a characterRange intersects with the outside of the line
 * @see #blackBoxBounds, #bounds(void), #lineBounds
 */
FlowRelativeFourSides<Scalar> TextLayout::bounds(const Range<Index>& characterRange) const {
	if(characterRange.end() > text_.length())
		throw kernel::BadPositionException(kernel::Position(0, characterRange.end()));

	FlowRelativeFourSides<Scalar> result;

	// TODO: this implementation can't handle vertical text.

	if(isEmpty()) {	// empty line
		result.start() = result.end() = 0;
		result.before() = -lineMetrics(0).ascent()/* - lineMetrics(0).leading()*/;
		result.after() = lineMetrics(0).descent();
	} else if(ascension::isEmpty(characterRange)) {	// an empty rectangle for an empty range
		const LineMetrics& line = lineMetrics(lineAt(characterRange.beginning()));
		return geometry::make<NativeRectangle>(
			geometry::subtract(location(range.beginning()), geometry::make<NativePoint>(0, line.ascent()/* + line.leading()*/)),
			geometry::make<NativeSize>(0, line.height()));
	} else {
		const Index firstLine = lineAt(characterRange.beginning()), lastLine = lineAt(characterRange.end());

		// calculate the block-progression-edges ('before' and 'after'; it's so easy)
		result.before() = baseline(firstLine) - lineMetrics(firstLine).ascent()/* - lineMetrics(firstLine).leading()*/;
		result.after() = baseline(lastLine) + lineMetrics(lastLine).descent();

		// calculate start-edge and end-edge of fully covered lines
		const bool firstLineIsFullyCovered = includes(characterRange,
			makeRange(lineOffset(firstLine), lineOffset(firstLine) + lineLength(firstLine)));
		const bool lastLineIsFullyCovered = includes(characterRange,
			makeRange(lineOffset(lastLine), lineOffset(lastLine) + lineLength(lastLine)));
		result.start() = numeric_limits<Scalar>::max();
		result.end() = numeric_limits<Scalar>::min();
		for(Index line = firstLine + firstLineIsFullyCovered ? 0 : 1;
				line < lastLine + lastLineIsFullyCovered ? 1 : 0; ++line) {
			const Scalar lineStart = lineStartEdge(line);
			result.start() = min(lineStart, result.start());
			result.end() = max(lineStart + measure(line), result.end());
		}

		// calculate start and end-edge of partially covered lines
		vector<Index> partiallyCoveredLines;
		if(!firstLineIsFullyCovered)
			partiallyCoveredLines.push_back(firstLine);
		if(!lastLineIsFullyCovered && (partiallyCoveredLines.empty() || partiallyCoveredLines[0] != lastLine))
			partiallyCoveredLines.push_back(lastLine);
		if(!partiallyCoveredLines.empty()) {
			Scalar left = (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? result.start() : -result.end();
			Scalar right = (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? result.end() : -result.start();
			for(vector<Index>::const_iterator
					line(partiallyCoveredLines.begin()), e(partiallyCoveredLines.end()); line != e; ++line) {
				const Index lastRun = (*line + 1 < numberOfLines()) ? lineFirstRuns_[*line + 1] : numberOfRuns_;

				// find left-edge
				InlineProgressionDimensionRangeIterator i(
					Range<const TextRun* const*>(runs_.get() + lineFirstRuns_[*line], runs_.get() + lastRun),
					range, LEFT_TO_RIGHT, (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ?
						lineStartEdge(*line) : -lineStartEdge(*line) - measure(*line));
				assert(i != InlineProgressionDimensionRangeIterator());
				left = min(i->beginning(), left);

				Scalar x = (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ?
					lineStartEdge(*line) : -lineStartEdge(*line) - measure(*line);
				for(Index i = lineFirstRuns_[*line];
						i < lastRun && x < left; x += runs_[i++]->totalWidth()) {
					const TextRun& run = *runs_[i];
					if(intersects(range, run)) {
						const Index leftEdge = (run.readingDirection() == LEFT_TO_RIGHT) ?
							max(range.beginning(), run.beginning()) : min(range.end(), run.end());
						left = min(x + run.x(leftEdge, false), left);
						break;
					}
				}

				// find right-edge
				i = InlineProgressionDimensionRangeIterator(
					Range<const TextRun* const*>(runs_.get() + lineFirstRuns_[*line], runs_.get() + lastRun),
					range, RIGHT_TO_LEFT, (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ?
						lineStartEdge(*line) + measure(*line) : -lineStartEdge(*line));
				assert(i != InlineProgressionDimensionRangeIterator());
				right = max(i->end(), right);

				x = (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ?
					lineStartEdge(*line) + measure(*line) : -lineStartEdge(*line);
				for(Index i = lastRun - 1; x > right; x -= runs_[i--]->totalWidth()) {
					const TextRun& run = *runs_[i];
					if(intersects(range, run)) {
						const Index rightEdge = (run.readingDirection() == LEFT_TO_RIGHT) ?
							min(range.end(), run.end()) : max(range.beginning(), run.beginning());
						right = max(x - run.totalWidth() + run.x(rightEdge, false), right);
						break;
					}
					if(i == lineFirstRuns_[*line])
						break;
				}
			}

			result.start() = (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? left : -right;
			result.end() = (writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? right : -left;
		}
	}

	return geometry::make<NativeRectangle>(
		geometry::make<NativePoint>(result.start(), result.before()),
		geometry::make<NativePoint>(result.end(), result.after()));
}

namespace {
	inline HRESULT callScriptItemize(const WCHAR* text, int length, int estimatedNumberOfItems,
			const SCRIPT_CONTROL& control, const SCRIPT_STATE& initialState, SCRIPT_ITEM items[], OPENTYPE_TAG scriptTags[], int& numberOfItems) {
		static HRESULT(WINAPI* scriptItemizeOpenType)(const WCHAR*, int, int,
			const SCRIPT_CONTROL*, const SCRIPT_STATE*, SCRIPT_ITEM*, OPENTYPE_TAG*, int*) = uspLib->get<0>();
		if(scriptItemizeOpenType != nullptr && scriptTags != nullptr)
			return (*scriptItemizeOpenType)(text, length, estimatedNumberOfItems, &control, &initialState, items, scriptTags, &numberOfItems);
		else
			return ::ScriptItemize(text, length, estimatedNumberOfItems, &control, &initialState, items, &numberOfItems);
	}
}

/**
 * Draws the specified line layout to the output device.
 * @param context The rendering context
 * @param origin The alignment point of the text layout
 * @param paintOverride Can be @c null
 * @param endOfLine The inline object which paints an end-of-line. Can be @c null
 * @param lineWrappingMark The inline object which paints line-wrapping-mark. Can be @c null
 */
void TextLayout::draw(PaintContext& context,
		const NativePoint& origin, const TextPaintOverride* paintOverride /* = nullptr */,
		const InlineObject* endOfLine/* = nullptr */, const InlineObject* lineWrappingMark /* = nullptr */) const {

#if /*defined(_DEBUG)*/ 0
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@TextLayout.draw draws line " << lineNumber_ << L" (" << line << L")\n";
#endif // defined(_DEBUG)

	if(isEmpty() || geometry::dy(context.boundsToPaint()) == 0)
		return;

	// TODO: this code can't handle vertical text.

	// calculate line range to draw
	Range<Index> linesToDraw(0, numberOfLines());
	NativePoint p(origin);
	for(Index line = linesToDraw.beginning(); line < linesToDraw.end(); ++line) {
		geometry::y(p) = baseline(line);
		const Scalar lineBeforeEdge = geometry::y(p) - lineMetrics_[line]->ascent();
		const Scalar lineAfterEdge = geometry::y(p) + lineMetrics_[line]->descent();
		if(geometry::top(context.boundsToPaint()) >= lineBeforeEdge && geometry::top(context.boundsToPaint()) < lineAfterEdge)
			linesToDraw = makeRange(line, linesToDraw.end());
		if(geometry::bottom(context.boundsToPaint()) >= lineBeforeEdge && geometry::bottom(context.boundsToPaint()) < lineAfterEdge) {
			linesToDraw = makeRange(linesToDraw.beginning(), line + 1);
			break;
		}
	}

	// calculate inline area range to draw
	Range<vector<const InlineArea*>::const_iterator> inlineAreasToDraw(inlineAreas_.begin(), inlineAreas_.end());
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()); i != inlineAreasToDraw.end(); ++i) {
		const Index endOfInlineArea = (i != inlineAreasToDraw.end()) ? i[1]->position() : text_.length();
		if(endOfInlineArea > lineOffset(linesToDraw.beginning())) {
			inlineAreasToDraw = makeRange(i, inlineAreasToDraw.end());
			break;
		}
	}
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()); i != inlineAreasToDraw.end(); ++i) {
		const Index endOfInlineArea = (i != inlineAreasToDraw.end()) ? i[1]->position() : text_.length();
		if(endOfInlineArea >= lineOffset(linesToDraw.beginning())) {
			inlineAreasToDraw = makeRange(inlineAreasToDraw.beginning(), i + 1);
			break;
		}
	}

	// this code paints the line in the following steps:
	// 1. calculate range of runs to paint
	// 2. paint backgrounds and borders:
	//   2-1. paint background if the property is specified
	//   2-2. paint border if the property is specified
	// 3. for each text runs:
	//   3-1. calculate range of runs to paint
	//   3-2. paint the glyphs of the text run
	//   3-3. paint the overhanging glyphs of the around text runs
	//   3-4. paint the text decoration
	// 4. paint the end of line mark
	//
	// the following topics describe how to draw a styled and selected text using masking by clipping
	// Catch 22 : Design and Implementation of a Win32 Text Editor
	// Part 10 - Transparent Text and Selection Highlighting (http://www.catch22.net/tuts/neatpad/10)
	// Part 14 - Drawing styled text with Uniscribe (http://www.catch22.net/tuts/neatpad/14)

	context.save();
//	context.setTextAlign();
//	context.setTextBaseline();
//	::SetTextAlign(context.nativeObject().get(), TA_TOP | TA_LEFT | TA_NOUPDATECP);

	// 2. paint backgrounds and borders
	for(vector<const InlineArea*>::const_iterator i(inlineAreasToDraw.beginning()), e; i != inlineAreasToDraw.end(); ++i) {
		// TODO: recognize the override.
		// TODO: this code can't handle sparse inline areas (with bidirectionality).
		boost::optional<NativeRectangle> borderRectangle;

		// 2-1. paint background if the property is specified (= if not transparent)
		if((*i)->style()->background) {
			borderRectangle = (*i)->borderRectangle();
			if(geometry::includes(context.boundsToPaint(), *borderRectangle)) {
				context.setFillStyle((*i)->style()->background);
				context.fillRectangle(*borderRectangle);
			}
		}

		// 2-2. paint border if the property is specified
		assert((*i)->style()->color);
		detail::paintBorder(context, (*i)->borderRectangle(), (*i)->style()->border, *(*i)->style()->color, writingMode());

		::ExcludeClipRect(context.asNativeObject().get(),
			geometry::left(*borderRectangle), geometry::top(*borderRectangle),
			geometry::right(*borderRectangle), geometry::bottom(*borderRectangle));
	}

	// 3. for each text runs
	for(Index line = linesToDraw.beginning(); line < linesToDraw.end(); ++line) {
		if(!isEmpty()) {
			// 3-1. calculate range of runs to paint
			Range<const TextRun* const*> runs(runs_.get() + lineFirstRuns_[line],
				runs_.get() + ((line < numberOfLines() - 1) ? lineFirstRuns_[line + 1] : numberOfRuns_));
			p = origin;
			geometry::x(p) += readingDirectionInt(writingMode().inlineFlowDirection);
			if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
				geometry::x(p) -= measure(line);
			Scalar leftEdgeOfFirstRun = geometry::x(p), rightEdgeOfLastRun = geometry::x(p) + measure(line);
			for(const TextRun* const* run = runs.beginning(); run < runs.end(); ++run) {
				if(geometry::x(p) + (*run)->totalWidth() < geometry::left(context.boundsToPaint())) {
					runs = makeRange(run + 1, runs.end());
					leftEdgeOfFirstRun = geometry::x(p) + (*run)->totalWidth();
				} else if(p.x > geometry::right(context.boundsToPaint())) {
					runs = makeRange(runs.beginning(), run);
					rightEdgeOfLastRun = geometry::x(p);
				}
			}
			if(!ascension::isEmpty(runs)) {
				const Range<Index> characterRange(runs.beginning()[0]->beginning(), runs.end()[-1]->end());
				unique_ptr<TextPaintOverride::Iterator> paintOverrideIterator;
				if(paintOverride != nullptr)
					paintOverrideIterator = paintOverride->queryTextPaintOverride(characterRange);
			}

			// 3-2. paint the glyphs of the text run

	}

#if 0
		// draw outside of the selection
		Rect<> runRect;
		runRect.top = y;
		runRect.bottom = y + dy;
		runRect.left = x = startX;
		dc.setBkMode(TRANSPARENT);
		for(size_t i = firstRun; i < lastRun; ++i) {
			TextRun& run = *runs_[i];
			COLORREF foreground;
			if(lineForeground != Color())
				foreground = lineForeground.asCOLORREF();
			else if(run.requestedStyle().get() != nullptr && run.requestedStyle()->foreground != Color())
				foreground = run.requestedStyle()->foreground.asCOLORREF();
			else
				foreground = defaultForeground;
			if(line[run.beginning()] != L'\t') {
				if(selection == nullptr /*|| run.overhangs()*/
						|| !(run.beginning() >= selectedRange.beginning() && run.end() <= selectedRange.end())) {
					dc.setTextColor(foreground);
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = run.draw(dc, x, y + lip_.textMetrics().ascent(), false, &runRect);
				}
			}
			// decoration (underline and border)
			if(run.requestedStyle().get() != nullptr)
				drawDecorationLines(dc, *run.requestedStyle(), foreground, x, y, run.totalWidth(), dy);
			x += run.totalWidth();
			runRect.left = x;
		}

		// draw selected text segment (also underline and border)
		if(selection != nullptr) {
			x = startX;
			clipRegion.setRect(clipRect);
			dc.selectClipRgn(clipRegion.get(), RGN_XOR);
			for(size_t i = firstRun; i < lastRun; ++i) {
				TextRun& run = *runs_[i];
				// text
				if(selection != nullptr && line[run.beginning()] != L'\t'
						&& (/*run.overhangs() ||*/ (run.beginning() < selectedRange.end() && run.end() > selectedRange.beginning()))) {
					dc.setTextColor(selection->color().foreground.asCOLORREF());
					runRect.left = x;
					runRect.right = runRect.left + run.totalWidth();
					hr = run.draw(dc, x, y + lip_.textMetrics().ascent(), false, &runRect);
				}
				// decoration (underline and border)
				if(run.requestedStyle().get() != nullptr)
					drawDecorationLines(dc, *run.requestedStyle(), selection->color().foreground.asCOLORREF(), x, y, run.totalWidth(), dy);
				x += run.totalWidth();
			}
		}

		// special character substitution
		if(specialCharacterRenderer != nullptr) {
			// white spaces and C0/C1 control characters
			dc.selectClipRgn(clipRegion.get());
			x = startX;
			for(size_t i = firstRun; i < lastRun; ++i) {
				TextRun& run = *runs_[i];
				context.readingDirection = run.writingMode().inlineFlowDirection;
				for(Index j = run.beginning(); j < run.end(); ++j) {
					if(BinaryProperty::is(line[j], BinaryProperty::WHITE_SPACE)) {	// IdentifierSyntax.isWhiteSpace() is preferred?
						context.rect.setX(makeRange(x + run.x(j, false), x + run.x(j, true)));
						specialCharacterRenderer->drawWhiteSpaceCharacter(context, line[j]);
					} else if(isC0orC1Control(line[j])) {
						context.rect.setX(makeRange(x + run.x(j, false), x + run.x(j, true)));
						specialCharacterRenderer->drawControlCharacter(context, line[j]);
					}
				}
				x += run.totalWidth();
			}
		}
		if(line == numberOfLines_ - 1
				&& resolveTextAlignment(alignment(), writingMode().inlineFlowDirection) == ALIGN_RIGHT)
			x = startX;
	} // end of nonempty line case
	
	// line terminator and line wrapping mark
	const Document& document = lip_.presentation().document();
	if(specialCharacterRenderer != nullptr) {
		context.readingDirection = lineTerminatorOrientation(style(), lip_.presentation().defaultLineStyle());
		if(line < numberOfLines() - 1) {	// line wrapping mark
			const int markWidth = specialCharacterRenderer->getLineWrappingMarkWidth(context);
			if(context.readingDirection == LEFT_TO_RIGHT)
				context.rect.setX(makeRange(lip_.width() - markWidth, lip_.width());
			else
				context.rect.setX(makeRange(0, markWidth));
			specialCharacterRenderer->drawLineWrappingMark(context);
		} else if(lineNumber_ < document.numberOfLines() - 1) {	// line teminator
			const kernel::Newline nlf = document.getLineInformation(lineNumber_).newline();
			const int nlfWidth = specialCharacterRenderer->getLineTerminatorWidth(context, nlf);
			if(context.readingDirection == LEFT_TO_RIGHT)
				context.rect.setX(makeRange(x, x + nlfWidth));
			else
				context.rect.setX(makeRange(x - nlfWidth, x));
			if(selection != nullptr) {
				const Position eol(lineNumber_, document.lineLength(lineNumber_));
				if(!selection->caret().isSelectionRectangle()
						&& selection->caret().beginning().position() <= eol
						&& selection->caret().end().position() > eol)
					dc.fillSolidRect(x - (context.readingDirection == RIGHT_TO_LEFT ? nlfWidth : 0),
						y, nlfWidth, dy, selection->color().background.asCOLORREF());
			}
			dc.setBkMode(TRANSPARENT);
			specialCharacterRenderer->drawLineTerminator(context, nlf);
		}
#endif
	}
	context.restore();
}

#ifdef _DEBUG
/**
 * Dumps the all runs to the specified output stream.
 * @param out the output stream
 */
void TextLayout::dumpRuns(ostream& out) const {
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		const TextRun& run = *runs_[i];
		out << static_cast<unsigned int>(i)
			<< ":beginning=" << static_cast<unsigned int>(run.beginning())
			<< ",length=" << static_cast<unsigned int>(length(run)) << endl;
	}
}
#endif // _DEBUG
#if 0
/// Expands the all tabs and resolves each width.
inline void TextLayout::expandTabsWithoutWrapping() /*throw()*/ {
	const String& s = text();
	const int fullTabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	int x = 0;

	if(lineTerminatorOrientation(style(), lip_.presentation().defaultTextLineStyle()) == LEFT_TO_RIGHT) {	// expand from the left most
		for(size_t i = 0; i < numberOfRuns_; ++i) {
			TextRun& run = *runs_[i];
			run.expandTabCharacters(s, x, fullTabWidth, numeric_limits<int>::max());
			x += run.totalWidth();
		}
	} else {	// expand from the right most
		for(size_t i = numberOfRuns_; i > 0; --i) {
			TextRun& run = *runs_[i - 1];
			run.expandTabCharacters(s, x, fullTabWidth, numeric_limits<int>::max());
			x += run.totalWidth();
		}
	}
	longestLineWidth_ = x;
}
#endif

/**
 * Returns the space string added to the end of the specified line to reach the specified virtual
 * point. If the end of the line is over @a virtualX, the result is an empty string.
 * @param x the x-coordinate of the virtual point
 * @return the space string consists of only white spaces (U+0020) and horizontal tabs (U+0009)
 * @throw kernel#BadPositionException @a line is outside of the document
 * @deprecated 0.8
 * @note This does not support line wrapping and bidirectional context.
 */
String TextLayout::fillToX(int x) const {
#if 0
	int cx = longestLineWidth();
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

	unique_ptr<DC> dc(lip_.getFontSelector().deviceContext());
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
#else
	return String();
#endif
}

shared_ptr<const Font> TextLayout::findMatchingFont(const StringPiece& textRun,
		const FontCollection& collection, const ComputedFontSpecification& specification) {
#if 0
	void resolveFontSpecifications(const FontCollection& fontCollection,
			shared_ptr<const TextRunStyle> requestedStyle, shared_ptr<const TextRunStyle> defaultStyle,
			FontDescription* computedDescription, double* computedSizeAdjust) {
		// family name
		if(computedDescription != nullptr) {
			String familyName = (requestedStyle.get() != nullptr) ? requestedStyle->fontFamily.getOrInitial() : String();
			if(familyName.empty()) {
				if(defaultStyle.get() != nullptr)
					familyName = defaultStyle->fontFamily.getOrInitial();
				if(computedFamilyName->empty())
					*computedFamilyName = fontCollection.lastResortFallback(FontDescription())->familyName();
			}
			computedDescription->setFamilyName();
		}
		// size
		if(computedPixelSize != nullptr) {
			requestedStyle->fontProperties
		}
		// properties
		if(computedProperties != 0) {
			FontProperties<Inheritable> result;
			if(requestedStyle.get() != nullptr)
				result = requestedStyle->fontProperties;
			Inheritable<double> computedSize(computedProperties->pixelSize());
			if(computedSize.inherits()) {
				if(defaultStyle.get() != nullptr)
					computedSize = defaultStyle->fontProperties.pixelSize();
				if(computedSize.inherits())
					computedSize = fontCollection.lastResortFallback(FontProperties<>())->metrics().emHeight();
			}
			*computedProperties = FontProperties<>(
				!result.weight().inherits() ? result.weight()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.weight() : FontPropertiesBase::NORMAL_WEIGHT),
				!result.stretch().inherits() ? result.stretch()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.stretch() : FontPropertiesBase::NORMAL_STRETCH),
				!result.style().inherits() ? result.style()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.style() : FontPropertiesBase::NORMAL_STYLE),
				!result.variant().inherits() ? result.variant()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.variant() : FontPropertiesBase::NORMAL_VARIANT),
				!result.orientation().inherits() ? result.orientation()
					: ((defaultStyle.get() != nullptr) ? defaultStyle->fontProperties.orientation() : FontPropertiesBase::HORIZONTAL),
				computedSize);
		}
		// size-adjust
		if(computedSizeAdjust != nullptr) {
			*computedSizeAdjust = (requestedStyle.get() != nullptr) ? requestedStyle->fontSizeAdjust : -1.0;
			if(*computedSizeAdjust < 0.0)
				*computedSizeAdjust = (defaultStyle.get() != nullptr) ? defaultStyle->fontSizeAdjust : 0.0;
		}
	}
#endif
}

/**
 * Returns the index of run containing the specified offset in the line.
 * @param offsetInLine The offset in the line
 * @return The index of the run
 */
inline size_t TextLayout::findRunForPosition(Index offsetInLine) const /*throw()*/ {
	assert(numberOfRuns_ > 0);
	if(offsetInLine == text_.length())
		return numberOfRuns_ - 1;
	const Index sl = lineAt(offsetInLine);
	const size_t lastRun = (sl + 1 < numberOfLines()) ? lineFirstRuns_[sl + 1] : numberOfRuns_;
	for(size_t i = lineFirstRuns_[sl]; i < lastRun; ++i) {
		if(runs_[i]->beginning() <= offsetInLine && runs_[i]->end() > offsetInLine)	// TODO: replace with includes().
			return i;
	}
	ASCENSION_ASSERT_NOT_REACHED();
}
#if 0
/// Returns an iterator addresses the first styled segment.
TextLayout::StyledSegmentIterator TextLayout::firstStyledSegment() const /*throw()*/ {
	const TextRun* temp = *runs_;
	return StyledSegmentIterator(temp);
}
#endif
/// Returns if the line contains right-to-left run.
bool TextLayout::isBidirectional() const /*throw()*/ {
	if(writingMode().inlineFlowDirection == RIGHT_TO_LEFT)
		return true;
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		if(runs_[i]->readingDirection() == RIGHT_TO_LEFT)
			return true;
	}
	return false;
}

/// Justifies the wrapped visual lines.
inline void TextLayout::justify(TextJustification) /*throw()*/ {
	assert(wrappingMeasure_ != -1);
	for(Index line = 0; line < numberOfLines(); ++line) {
		const int ipd = measure(line);
		const size_t last = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		for(size_t i = lineFirstRuns_[line]; i < last; ++i) {
			TextRun& run = *runs_[i];
			const int newRunMeasure = ::MulDiv(run.totalWidth(), wrappingMeasure_, ipd);	// TODO: there is more precise way.
			run.justify(newRunMeasure);
		}
	}
}
#if 0
/// Returns an iterator addresses the last styled segment.
TextLayout::StyledSegmentIterator TextLayout::lastStyledSegment() const /*throw()*/ {
	const TextRun* temp = runs_[numberOfRuns_];
	return StyledSegmentIterator(temp);
}
#endif
/**
 * Returns the smallest rectangle emcompasses the specified line. It might not coincide exactly the
 * ascent, descent or overhangs of the specified line.
 * @param line The line number
 * @return The line bounds in pixels
 * @throw IndexOutOfBoundsException @a line is greater than the number of the lines
 * @see #lineStartEdge
 */
NativeRectangle TextLayout::lineBounds(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");

	const Scalar start = lineStartEdge(line);
	const Scalar end = start + measure(line);
	const Scalar before = baseline(line) - lineMetrics_[line]->ascent()/* - lineMetrics_[line]->leading()*/;
	const Scalar after = before + lineMetrics_[line]->height();

	// TODO: this implementation can't handle vertical text.
	const NativeSize size(geometry::make<NativeSize>(end - start, after - before));
	const NativePoint origin(geometry::make<NativePoint>(
		(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? start : start - geometry::dx(size), before));
	return geometry::make<NativeRectangle>(origin, size);
}

/**
 * Returns the start-edge of the specified line without the start-indent in pixels.
 * @par This is distance from the start-edge of the first line to the one of @a line in
 * inline-progression-dimension. Therefore, returns always zero when @a line is zero or the anchor
 * is @c TEXT_ANCHOR_START.
 * @par A positive value means positive indentation. For example, if the start-edge of a RTL line
 * is x = -10, this method returns +10.
 * @param line The line number
 * @return The start-indentation in pixels
 * @throw IndexOutOfBoundsException @a line is invalid
 * @see TextRenderer#lineStartEdge
 */
Scalar TextLayout::lineStartEdge(Index line) const {
	if(line == 0)
		return 0;
	switch(anchor()) {
	case TEXT_ANCHOR_START:
		return 0;
	case TEXT_ANCHOR_MIDDLE:
		return (measure(0) - measure(line)) / 2;
	case TEXT_ANCHOR_END:
		return measure(0) - measure(line);
	default:
		ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Converts a position in the block-progression-direction into the corresponding line.
 * @param bpd The position in block-progression-dimension in pixels
 * @param[out] outside @c true if @a bpd is outside of the line content
 * @return The line number
 * @see #basline, #lineAt, #offset
 */
Index TextLayout::locateLine(Scalar bpd, bool& outside) const /*throw()*/ {
	// TODO: This implementation can't handle tricky 'text-orientation'.

	// beyond the before-edge ?
	if(bpd < -lineMetrics_[0]->ascent()/* - lineMetrics_[0]->leading()*/)
		return (outside = true), 0;

	Index line = 0;
	for(Scalar lineAfter = 0; line < numberOfLines() - 1; ++line) {
		if(bpd < (lineAfter += lineMetrics_[line]->height()))
			return (outside = false), line;
	}

	// beyond the after-edge
	return (outside = true), numberOfLines() - 1;
}

/**
 * @internal Converts an inline-progression-dimension into character offset(s) in the line.
 * @param line The line number
 * @param ipd The inline-progression-dimension
 * @param[out] outside @c true if @a ipd is outside of the line content
 * @return See the documentation of @c #offset method
 * @throw IndexOutOfBoundsException @a line is invalid
 */
pair<Index, Index> TextLayout::locateOffsets(Index line, Scalar ipd, bool& outside) const {
	if(isEmpty())
		return (outside = true), make_pair(static_cast<Index>(0), static_cast<Index>(0));
	const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;

	if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) {
		Scalar x = lineStartEdge(line);
		if(ipd < x) {	// beyond the left-edge => the start of the first run
			const Index offsetInLine = runs_[lineFirstRuns_[line]]->beginning();
			return (outside = true), make_pair(offsetInLine, offsetInLine);
		}
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i) {	// scan left to right
			const TextRun& run = *runs_[i];
			if(ipd >= x && ipd <= x + run.totalWidth()) {
				int cp, trailing;
				run.hitTest(ipd - x, cp, trailing);	// TODO: check the returned value.
				const Index temp = run.beginning() + static_cast<Index>(cp);
				return (outside = false), make_pair(temp, temp + static_cast<Index>(trailing));
			}
			x += run.totalWidth();
		}
		// beyond the right-edge => the end of last run
		const Index offsetInLine = runs_[lastRun - 1]->end();
		return (outside = true), make_pair(offsetInLine, offsetInLine);
	} else {
		Scalar x = -lineStartEdge(line);
		if(ipd > x) {	// beyond the right-edge => the start of the last run
			const Index offsetInLine = runs_[lastRun - 1]->beginning();
			return (outside = true), make_pair(offsetInLine, offsetInLine);
		}
		// beyond the left-edge => the end of the first run
		const Index offsetInLine = runs_[lineFirstRuns_[line]]->end();
		return (outside = true), make_pair(offsetInLine, offsetInLine);
	}
}

// implements public location methods
void TextLayout::locations(Index offsetInLine, NativePoint* leading, NativePoint* trailing) const {
	assert(leading != nullptr || trailing != nullptr);
	if(offsetInLine > text_.length())
		throw kernel::BadPositionException(kernel::Position(0, offsetInLine));

	Scalar leadingIpd, trailingIpd, bpd = lineMetrics_[0]->ascent()/* + lineMetrics_[0]->leading()*/;
	if(isEmpty())
		leadingIpd = trailingIpd = 0;
	else {
		// inline-progression-dimension
		const Index line = lineAt(offsetInLine);
		const Index firstRun = lineFirstRuns_[line];
		const Index lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		if(writingMode().inlineFlowDirection == LEFT_TO_RIGHT) {	// LTR
			Scalar x = lineStartEdge(line);
			for(size_t i = firstRun; i < lastRun; ++i) {
				const TextRun& run = *runs_[i];
				if(offsetInLine >= run.beginning() && offsetInLine <= run.end()) {
					if(leading != nullptr)
						leadingIpd = x + run.x(offsetInLine, false);
					if(trailing != nullptr)
						trailingIpd = x + run.x(offsetInLine, true);
					break;
				}
				x += run.totalWidth();
			}
		} else {	// RTL
			Scalar x = -lineStartEdge(line);
			for(size_t i = lastRun - 1; ; --i) {
				const TextRun& run = *runs_[i];
				x -= run.totalWidth();
				if(offsetInLine >= run.beginning() && offsetInLine <= run.end()) {
					if(leading != nullptr)
						leadingIpd = -(x + run.x(offsetInLine, false));
					if(trailing != nullptr)
						trailingIpd = -(x + run.x(offsetInLine, true));
					break;
				}
				if(i == firstRun) {
					ASCENSION_ASSERT_NOT_REACHED();
					break;
				}
			}
		}

		// block-progression-dimension
		bpd += baseline(line);
	}
		
	// TODO: this implementation can't handle vertical text.
	if(leading != nullptr) {
		leading->x = leadingIpd;
		leading->y = bpd;
	}
	if(trailing != nullptr) {
		trailing->x = trailingIpd;
		trailing->y = bpd;
	}
}

/**
 * Returns the inline-progression-dimension of the longest line.
 * @see #measure(Index)
 */
Scalar TextLayout::measure() const /*throw()*/ {
	if(maximumMeasure_ < 0) {
		Scalar ipd = 0;
		for(Index line = 0; line < numberOfLines(); ++line)
			ipd = max(measure(line), ipd);
		const_cast<TextLayout*>(this)->maximumMeasure_ = ipd;
	}
	return maximumMeasure_;
}

/**
 * Returns the length in inline-progression-dimension without the indentations (the distance from
 * the start-edge to the end-edge) of the specified line in pixels.
 * @param line The line number
 * @return The width. Must be equal to or greater than zero
 * @throw IndexOutOfBoundsException @a line is greater than the number of lines
 * @see #measure(void)
 */
Scalar TextLayout::measure(Index line) const {
	if(line >= numberOfLines())
		throw IndexOutOfBoundsException("line");
	else if(isEmpty())
		return const_cast<TextLayout*>(this)->maximumMeasure_ = 0;
	else {
		TextLayout& self = const_cast<TextLayout&>(*this);
		if(numberOfLines() == 1) {
			if(maximumMeasure_ >= 0)
				return maximumMeasure_;
		} else {
			if(measures_.get() == nullptr) {
				self.measures_.reset(new Scalar[numberOfLines()]);
				fill_n(self.measures_.get(), numberOfLines(), -1);
			}
			if(measures_[line] >= 0)
				return measures_[line];
		}
		const size_t lastRun = (line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_;
		Scalar ipd = 0;
		for(size_t i = lineFirstRuns_[line]; i < lastRun; ++i)
			ipd += runs_[i]->totalWidth();
		assert(ipd >= 0);
		if(numberOfLines() == 1)
			self.maximumMeasure_ = ipd;
		else
			self.measures_[line] = ipd;
		return ipd;
	}
}

/**
 * Returns the hit test information corresponding to the specified point.
 * @param p The point
 * @param[out] outside @c true if the specified point is outside of the layout
 * @return A pair of the character offsets. The first element addresses the character whose black
 *         box (bounding box) encompasses the specified point. The second element addresses the
 *         character whose leading point is the closest to the specified point in the line
 * @see #locateLine, #location
 */
pair<Index, Index> TextLayout::offset(const NativePoint& p, bool* outside /* = nullptr */) const /*throw()*/ {
	const bool vertical = isVertical(writingMode().blockFlowDirection);
	bool outsides[2];
	const std::pair<Index, Index> result(locateOffsets(locateLine(
		vertical ? geometry::x(p) : geometry::y(p), outsides[0]), vertical ? geometry::y(p) : geometry::x(p), outsides[1]));
	if(outside != nullptr)
		*outside = outsides[0] | outsides[1];
	return result;
}

/// Reorders the runs in visual order.
inline void TextLayout::reorder() /*throw()*/ {
	if(numberOfRuns_ == 0)
		return;
	unique_ptr<TextRun*[]> temp(new TextRun*[numberOfRuns_]);
	copy(runs_.get(), runs_.get() + numberOfRuns_, temp.get());
	for(Index line = 0; line < numberOfLines(); ++line) {
		const size_t numberOfRunsInLine = ((line < numberOfLines() - 1) ?
			lineFirstRuns_[line + 1] : numberOfRuns_) - lineFirstRuns_[line];
		const unique_ptr<BYTE[]> levels(new BYTE[numberOfRunsInLine]);
		for(size_t i = 0; i < numberOfRunsInLine; ++i)
			levels[i] = static_cast<BYTE>(runs_[i + lineFirstRuns_[line]]->bidiEmbeddingLevel() & 0x1f);
		const unique_ptr<int[]> log2vis(new int[numberOfRunsInLine]);
		const HRESULT hr = ::ScriptLayout(static_cast<int>(numberOfRunsInLine), levels.get(), nullptr, log2vis.get());
		assert(SUCCEEDED(hr));
		for(size_t i = lineFirstRuns_[line]; i < lineFirstRuns_[line] + numberOfRunsInLine; ++i)
			runs_[lineFirstRuns_[line] + log2vis[i - lineFirstRuns_[line]]] = temp[i];
	}
}
#if 0
/**
 * Returns the next tab stop position.
 * @param x the distance from leading edge of the line (can not be negative)
 * @param direction the direction
 * @return the distance from leading edge of the line to the next tab position
 */
inline int TextLayout::nextTabStop(int x, Direction direction) const /*throw()*/ {
	assert(x >= 0);
	const int tabWidth = lip_.textMetrics().averageCharacterWidth() * lip_.layoutSettings().tabWidth;
	return (direction == Direction::FORWARD) ? x + tabWidth - x % tabWidth : x - x % tabWidth;
}

/**
 * Returns the next tab stop.
 * @param x the distance from the left edge of the line to base position (can not be negative)
 * @param right @c true to find the next right position
 * @return the tab stop position in pixel
 */
int TextLayout::nextTabStopBasedLeftEdge(int x, bool right) const /*throw()*/ {
	assert(x >= 0);
	const LayoutSettings& c = lip_.layoutSettings();
	const int tabWidth = lip_.textMetrics().averageCharacterWidth() * c.tabWidth;
	if(lineTerminatorOrientation(style(), lip_.presentation().defaultTextLineStyle()) == LEFT_TO_RIGHT)
		return nextTabStop(x, right ? Direction::FORWARD : Direction::BACKWARD);
	else
		return right ? x + (x - longestLineWidth()) % tabWidth : x - (tabWidth - (x - longestLineWidth()) % tabWidth);
}
#endif

#if 0
/**
 * Returns the computed reading direction of the line.
 * @see #alignment
 */
ReadingDirection TextLayout::readingDirection() const /*throw()*/ {
	ReadingDirection result = INHERIT_READING_DIRECTION;
	// try the requested line style
	if(style_.get() != nullptr)
		result = style_->readingDirection;
	// try the default line style
	if(result == INHERIT_READING_DIRECTION) {
		shared_ptr<const TextLineStyle> defaultLineStyle(lip_.presentation().defaultTextLineStyle());
		if(defaultLineStyle.get() != nullptr)
			result = defaultLineStyle->readingDirection;
	}
	// try the default UI style
	if(result == INHERIT_READING_DIRECTION)
		result = lip_.defaultUIReadingDirection();
	// use user default
	if(result == INHERIT_READING_DIRECTION)
		result = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
	assert(result == LEFT_TO_RIGHT || result == RIGHT_TO_LEFT);
	return result;
}
#endif

/**
 * Stacks the line boxes and compute the line metrics.
 * @param lineStackingStrategy
 * @param nominalFont
 * @param lineHeight
 */
void TextLayout::stackLines(LineStackingStrategy lineStackingStrategy, const Font& nominalFont, Scalar lineHeight) {
	// TODO: this code is temporary. should rewrite later.
	// calculate allocation-rectangle of the lines according to line-stacking-strategy
	const Scalar textAltitude = nominalFont.metrics().ascent();
	const Scalar textDepth = nominalFont.metrics().descent();
	vector<pair<Scalar, Scalar>> v;
	for(Index line = 0; line < numberOfLines(); ++line) {
		// calculate extent of the line in block-progression-direction
		Scalar ascent, descent;
		switch(lineStackingStrategy) {
			case LINE_HEIGHT: {
				// allocation-rectangle of line is per-inline-height-rectangle
				Scalar leading = lineHeight - (textAltitude + textDepth);
				ascent = textAltitude + (leading - leading / 2);
				descent = textDepth + leading / 2;
				const TextRun* const lastRun = runs_[(line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_];
				for(const TextRun* run = runs_[lineFirstRuns_[line]]; run != lastRun; ++run) {
					leading = lineHeight - nominalFont.metrics().cellHeight();
					ascent = max(run->font()->metrics().ascent() - (leading - leading / 2), ascent);
					descent = max(run->font()->metrics().descent() - leading / 2, descent);
				}
				break;
			}
			case FONT_HEIGHT:
				// allocation-rectangle of line is nominal-requested-line-rectangle
				ascent = textAltitude;
				descent = textDepth;
				break;
			case MAX_HEIGHT: {
				// allocation-rectangle of line is maximum-line-rectangle
				ascent = textAltitude;
				descent = textDepth;
				const TextRun* const lastRun = runs_[(line + 1 < numberOfLines()) ? lineFirstRuns_[line + 1] : numberOfRuns_];
				for(const TextRun* run = runs_[lineFirstRuns_[line]]; run != lastRun; ++run) {
					ascent = max(run->font()->metrics().ascent(), ascent);
					descent = max(run->font()->metrics().descent(), descent);
				}
				break;
			}
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		v.push_back(make_pair(ascent, descent));
	}

	lineMetrics_.reset(new LineMetrics*[numberOfLines()]);
	for(size_t line = 0; line != v.size(); ++line) {
		try {
			unique_ptr<SillyLineMetrics> lineMetrics(new SillyLineMetrics(v[line].first, v[line].second));
			lineMetrics_[line] = lineMetrics.release();
		} catch(...) {
			while(line > 0)
				delete lineMetrics_[--line];
			throw;
		}
	}
}

#if 0
/**
 * Returns the styled text run containing the specified offset in the line.
 * @param offsetInLine The offset in the line
 * @return the styled segment
 * @throw kernel#BadPositionException @a offsetInLine is greater than the length of the line
 */
StyledRun TextLayout::styledTextRun(Index offsetInLine) const {
	if(offsetInLine > text().length())
		throw kernel::BadPositionException(kernel::Position(INVALID_INDEX, offsetInLine));
	const TextRun& run = *runs_[findRunForPosition(offsetInLine)];
	return StyledRun(run.offsetInLine(), run.requestedStyle());
}
#endif

/// Locates the wrap points and resolves tab expansions.
void TextLayout::wrap(const TabExpander& tabExpander) /*throw()*/ {
	assert(numberOfRuns_ != 0 && wrappingMeasure_ != numeric_limits<Scalar>::max());
	assert(numberOfLines_ == 0 && lineOffsets_.get() == nullptr && lineFirstRuns_.get() == nullptr);

	vector<Index> lineFirstRuns;
	lineFirstRuns.push_back(0);
	int x1 = 0;	// addresses the beginning of the run. see x2
	unique_ptr<int[]> logicalWidths;
	unique_ptr<SCRIPT_LOGATTR[]> logicalAttributes;
	Index longestRunLength = 0;	// for efficient allocation
	vector<TextRun*> newRuns;
	newRuns.reserve(numberOfRuns_ * 3 / 2);
	// for each runs... (at this time, runs_ is in logical order)
	for(size_t i = 0; i < numberOfRuns_; ++i) {
		TextRun* run = runs_[i];

		// if the run is a tab, expand and calculate actual width
		if(run->expandTabCharacters(tabExpander, text_,
				(x1 < wrappingMeasure_) ? x1 : 0, wrappingMeasure_ - (x1 < wrappingMeasure_) ? x1 : 0)) {
			if(x1 < wrappingMeasure_) {
				x1 += run->totalWidth();
				newRuns.push_back(run);
			} else {
				x1 = run->totalWidth();
				newRuns.push_back(run);
				lineFirstRuns.push_back(newRuns.size());
			}
			continue;
		}

		// obtain logical widths and attributes for all characters in this run to determine line break positions
		if(length(*run) > longestRunLength) {
			longestRunLength = length(*run);
			longestRunLength += 16 - longestRunLength % 16;
			logicalWidths.reset(new int[longestRunLength]);
			logicalAttributes.reset(new SCRIPT_LOGATTR[longestRunLength]);
		}
		HRESULT hr = run->logicalWidths(logicalWidths.get());
		hr = run->logicalAttributes(text_, logicalAttributes.get());
		const Index originalRunPosition = run->beginning();
		int widthInThisRun = 0;
		Index lastBreakable = run->beginning(), lastGlyphEnd = run->beginning();
		int lastBreakableX = x1, lastGlyphEndX = x1;
		// for each characters in the run...
		for(Index j = run->beginning(); j < run->end(); ) {	// j is position in the LOGICAL line
			const int x2 = x1 + widthInThisRun;
			// remember this opportunity
			if(logicalAttributes[j - originalRunPosition].fCharStop != 0) {
				lastGlyphEnd = j;
				lastGlyphEndX = x2;
				if(logicalAttributes[j - originalRunPosition].fSoftBreak != 0
						|| logicalAttributes[j - originalRunPosition].fWhiteSpace != 0) {
					lastBreakable = j;
					lastBreakableX = x2;
				}
			}
			// break if the width of the visual line overs the wrap width
			if(x2 + logicalWidths[j - originalRunPosition] > wrappingMeasure_) {
				// the opportunity is the start of this run
				if(lastBreakable == run->beginning()) {
					// break at the last glyph boundary if no opportunities
					if(lineFirstRuns.empty() || lineFirstRuns.back() == newRuns.size()) {
						if(lastGlyphEnd == run->beginning()) {	// break here if no glyph boundaries
							lastBreakable = j;
							lastBreakableX = x2;
						} else {
							lastBreakable = lastGlyphEnd;
							lastBreakableX = lastGlyphEndX;
						}
					}
				}

				// case 1: break at the start of the run
				if(lastBreakable == run->beginning()) {
					assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
					lineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run start.\n";
				}
				// case 2: break at the end of the run
				else if(lastBreakable == run->end()) {
					if(lastBreakable < text_.length()) {
						assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
						lineFirstRuns.push_back(newRuns.size() + 1);
//dout << L"broke the line at " << lastBreakable << L" where the run end.\n";
					}
					break;
				}
				// case 3: break at the middle of the run -> split the run (run -> newRun + run)
				else {
					unique_ptr<TextRun> followingRun(run->breakAt(lastBreakable, text_));
					newRuns.push_back(run);
					assert(lineFirstRuns.empty() || newRuns.size() != lineFirstRuns.back());
					lineFirstRuns.push_back(newRuns.size());
//dout << L"broke the line at " << lastBreakable << L" where the run meddle.\n";
					run = followingRun.release();	// continue the process about this run
				}
				widthInThisRun = x1 + widthInThisRun - lastBreakableX;
				lastBreakableX -= x1;
				lastGlyphEndX -= x1;
				x1 = 0;
				j = max(lastBreakable, j);
			} else
				widthInThisRun += logicalWidths[j++ - originalRunPosition];
		}
		newRuns.push_back(run);
		x1 += widthInThisRun;
	}
//dout << L"...broke the all lines.\n";
	if(newRuns.empty())
		newRuns.push_back(nullptr);
	runs_.reset(new TextRun*[numberOfRuns_ = newRuns.size()]);
	copy(newRuns.begin(), newRuns.end(), runs_.get());

	{
		assert(numberOfLines() > 1);
		unique_ptr<Index[]> temp(new Index[numberOfLines_ = lineFirstRuns.size()]);
		copy(lineFirstRuns.begin(), lineFirstRuns.end(), temp.get());
		lineFirstRuns_ = move(temp);
	}

	lineOffsets_ = move(unique_ptr<Index[]>(new Index[numberOfLines()]));
	for(size_t i = 0; i < numberOfLines(); ++i)
		const_cast<Index&>(lineOffsets_[i]) = runs_[lineFirstRuns_[i]]->beginning();
}

#if 0
// TextLayout.StyledSegmentIterator ///////////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param start
 */
TextLayout::StyledSegmentIterator::StyledSegmentIterator(const TextRun*& start) /*throw()*/ : p_(&start) {
}

/// Copy-constructor.
TextLayout::StyledSegmentIterator::StyledSegmentIterator(const StyledSegmentIterator& rhs) /*throw()*/ : p_(rhs.p_) {
}

/// Returns the current segment.
StyledRun TextLayout::StyledSegmentIterator::current() const /*throw()*/ {
	const TextRun& run = **p_;
	return StyledRun(run.offsetInLine, run.style);
}
#endif
