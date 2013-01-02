/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2012
 */

#include <ascension/graphics/text-layout-styles.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/presentation/text-style.hpp>
#include <ascension/rules.hpp>
#ifdef ASCENSION_OS_WINDOWS
#include <shellapi.h>	// ShellExecuteW
#endif // ASCENSION_OS_WINDOWS
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::graphics;
using namespace ascension::presentation;
using namespace ascension::presentation::hyperlink;
using namespace ascension::rules;
using namespace std;
//using graphics::font::TextRenderer;


// Color //////////////////////////////////////////////////////////////////////////////////////////

/// A transparent object whose all components are zero.
const Color Color::TRANSPARENT_BLACK(0, 0, 0, 0);


// Border /////////////////////////////////////////////////////////////////////////////////////////

// TODO: these value are changed later.
const Length Border::THIN(0.05, Length::EM_HEIGHT);
const Length Border::MEDIUM(0.10, Length::EM_HEIGHT);
const Length Border::THICK(0.20, Length::EM_HEIGHT);


// TextRunStyle ///////////////////////////////////////////////////////////////////////////////////

/**
 * @param base The base style
 * @param baseIsRoot Set @c true if @a base is the root style
 * @return This object
 */
TextRunStyle& TextRunStyle::resolveInheritance(const TextRunStyle& base, bool baseIsRoot) {
	// TODO: not implemented.
	return *this;
}


// Presentation ///////////////////////////////////////////////////////////////////////////////////

shared_ptr<const TextToplevelStyle> Presentation::DEFAULT_TEXT_TOPLEVEL_STYLE;

struct Presentation::Hyperlinks {
	Index lineNumber;
	unique_ptr<Hyperlink*[]> hyperlinks;
	size_t numberOfHyperlinks;
};

/**
 * Constructor.
 * @param document The target document
 */
Presentation::Presentation(Document& document) BOOST_NOEXCEPT : document_(document) {
	if(DEFAULT_TEXT_TOPLEVEL_STYLE.get() == nullptr) {
		unique_ptr<TextLineStyle> temp1(new TextLineStyle);
		temp1->defaultRunStyle.reset(new TextRunStyle);
		unique_ptr<TextToplevelStyle> temp2(new TextToplevelStyle);
		temp2->defaultLineStyle = move(temp1);
		DEFAULT_TEXT_TOPLEVEL_STYLE = move(temp2);
	}
	setTextToplevelStyle(shared_ptr<const TextToplevelStyle>());
	document_.addListener(*this);
}

/// Destructor.
Presentation::~Presentation() BOOST_NOEXCEPT {
	document_.removeListener(*this);
	clearHyperlinksCache();
}

/**
 * Registers the global text style listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 * @see #globalTextLineStyle, #removeGlobalTextStyleListener
 */
void Presentation::addGlobalTextStyleListener(GlobalTextStyleListener& listener) {
	globalTextStyleListeners_.add(listener);
}

/**
 * Registers the text line color specifier.
 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
 * @param specifier The specifier to register
 * @throw NullPointerException @a specifier is @c null
 */
void Presentation::addTextLineColorSpecifier(shared_ptr<TextLineColorSpecifier> specifier) {
	if(specifier.get() == nullptr)
		throw NullPointerException("specifier");
	textLineColorSpecifiers_.push_back(specifier);
}

void Presentation::clearHyperlinksCache() BOOST_NOEXCEPT {
	for(list<Hyperlinks*>::iterator i(hyperlinks_.begin()), e(hyperlinks_.end()); i != e; ++i) {
		for(size_t j = 0; j < (*i)->numberOfHyperlinks; ++j)
			delete (*i)->hyperlinks[j];
		delete *i;
	}
	hyperlinks_.clear();
}

namespace {
//	template<typename PropertyType>
//	inline typename sp::IntrinsicType<PropertyType>::Type resolveProperty(PropertyType TextLineStyle::*pointerToMember, const TextLineStyle& declared, const TextLineStyle& toplevel) {
//		return !(declared.*pointerToMember).inherits() ? (declared.*pointerToMember).get() : (toplevel.*pointerToMember).getOrInitial();
//	}
	template<typename PropertyType>
	inline void resolveProperty(PropertyType TextLineStyle::*pointerToMember, const TextLineStyle& toplevel, TextLineStyle& property) {
		if((property.*pointerToMember).inherits())
			property.*pointerToMember = (toplevel.*pointerToMember).getOrInitial();
	}
}

/**
 * Returns the style of the specified text line.
 * @param line The line number
 * @param context 
 * @param contextSize 
 * @param globalSwitch 
 * @return The computed text line style
 * @throw BadPositionException @a line is outside of the document
 */
font::ComputedTextLineStyle&& Presentation::computeTextLineStyle(Index line,
		const RenderingContext2D& context, const NativeSize& contextSize, const GlobalTextStyleSwitch* globalSwitch) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));

	TextToplevelStyle toplevel(textToplevelStyle());
	if(toplevel.writingMode.inherits() && globalSwitch != nullptr)
		toplevel.writingMode = globalSwitch->writingMode();

	shared_ptr<const TextLineStyle> declared;
	if(textLineStyleDeclarator_.get() != nullptr)
		declared = textLineStyleDeclarator_->declareTextLineStyle(line);
	assert(toplevel.defaultLineStyle != nullptr);

	TextLineStyle precomputed((declared.get() != nullptr) ? *declared : TextLineStyle());
	if(globalSwitch != nullptr) {
		if(!precomputed.direction.inherits())
			precomputed.direction = globalSwitch->direction();
		if(!precomputed.textAlignment.inherits())
			precomputed.textAlignment = globalSwitch->textAlignment();
		if(!precomputed.textOrientation.inherits())
			precomputed.textOrientation = globalSwitch->textOrientation();
		if(!precomputed.whiteSpace.inherits())
			precomputed.whiteSpace = globalSwitch->whiteSpace();
	}
	resolveProperty(&TextLineStyle::direction, *toplevel.defaultLineStyle, precomputed);
//	resolveProperty(&TextLineStyle::unicodeBidi, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::textOrientation, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::lineBoxContain, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::inlineBoxAlignment, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::whiteSpace, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::tabSize, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::lineBreak, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::wordBreak, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::overflowWrap, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::textAlignment, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::textAlignmentLast, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::textJustification, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::textIndent, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::hangingPunctuation, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::dominantBaseline, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::lineHeight, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::measure, *toplevel.defaultLineStyle, precomputed);
	resolveProperty(&TextLineStyle::numberSubstitution, *toplevel.defaultLineStyle, precomputed);

	font::ComputedTextLineStyle computed;
	computed.writingMode = WritingMode(precomputed.direction.getOrInitial(), toplevel.writingMode.getOrInitial(), precomputed.textOrientation.getOrInitial());
	computed.lineBoxContain = precomputed.lineBoxContain.getOrInitial();
	computed.whiteSpace = precomputed.whiteSpace.getOrInitial();
	precomputed.tabSize.getOrInitial();
	computed.lineBreak = precomputed.lineBreak.getOrInitial();
	computed.wordBreak = precomputed.wordBreak.getOrInitial();
	computed.overflowWrap = precomputed.overflowWrap.getOrInitial();
	computed.alignment = precomputed.textAlignment.getOrInitial();
	computed.alignmentLast = precomputed.textAlignmentLast.getOrInitial();
	computed.justification = precomputed.textJustification.getOrInitial();
	computed.indent.length = static_cast<Scalar>(precomputed.textIndent.getOrInitial().length.value(&context, &contextSize));
	computed.indent.hanging = precomputed.textIndent.getOrInitial().hanging;
	computed.indent.eachLine = precomputed.textIndent.getOrInitial().eachLine;
	computed.hangingPunctuation = precomputed.hangingPunctuation.getOrInitial();
	computed.dominantBaseline = precomputed.dominantBaseline.getOrInitial();
	{
		// TODO: This code is temporary.
		auto precomputedLineHeight(precomputed.lineHeight.getOrInitial());
		if(LineHeightEnums* const keyword = boost::get<LineHeightEnums>(&precomputedLineHeight)) {
			if(*keyword == LineHeightEnums::NONE)
				*keyword = LineHeightEnums::NORMAL;
			if(*keyword == LineHeightEnums::NORMAL || true)
				precomputedLineHeight = Length(1.15, Length::EM_HEIGHT);
		} else if(const double* const number = boost::get<double>(&precomputedLineHeight))
			precomputedLineHeight = Length(*number, Length::EM_HEIGHT);
		if(const Length* const length = boost::get<Length>(&precomputedLineHeight))
			computed.lineHeight = static_cast<Scalar>(length->value(&context, &contextSize));
		else
			ASCENSION_ASSERT_NOT_REACHED();
	}
	computed.measure = static_cast<Scalar>(precomputed.measure.getOrInitial()->value(&context, &contextSize));
	computed.numberSubstitution.method = precomputed.numberSubstitution.getOrInitial().method;
	computed.numberSubstitution.localeName = precomputed.numberSubstitution.getOrInitial().localeName;
	computed.numberSubstitution.ignoreUserOverride = precomputed.numberSubstitution.getOrInitial().ignoreUserOverride;

	return move(computed);
}

/**
 * Returns the styles of the text runs in the specified line.
 * @param line The line
 * @return An iterator enumerates the styles of the text runs in the line, or @c null if the line
 *         has no styled text runs
 * @throw BadPositionException @a line is outside of the document
 */
unique_ptr<font::ComputedStyledTextRunIterator> Presentation::computeTextRunStyles(Index line,
		const RenderingContext2D& context, const NativeSize& contextSize) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	return (textRunStyleDeclarator_.get() != nullptr) ?
		textRunStyleDeclarator_->declareTextRunStyle(line) : unique_ptr<StyledTextRunIterator>();
}

/**
 * Computes the writing mode. This method does not call @c TextLineStyleDeclarator.
 * @param globalSwitch
 * @return The computed writing mode value
 */
WritingMode&& Presentation::computeWritingMode(const GlobalTextStyleSwitch* globalSwitch) const {
	const TextToplevelStyle& toplevel = textToplevelStyle();
	boost::optional<BlockFlowDirection> writingMode(toplevel.writingMode.getOrNone());
	if(writingMode == boost::none) {
		if(globalSwitch != nullptr)
			writingMode = globalSwitch->writingMode().getOrInitial();
		else
			writingMode = toplevel.writingMode.initialValue();

	}
	assert(writingMode != boost::none);

	boost::optional<ReadingDirection> direction;
	boost::optional<TextOrientation> textOrientation;
	if(globalSwitch != nullptr) {
		if(writingMode == boost::none)
			writingMode = globalSwitch->writingMode().getOrNone();
		direction = globalSwitch->direction().getOrNone();
		textOrientation = globalSwitch->textOrientation().getOrNone();
	}
	if(direction == boost::none)
		direction = toplevel.defaultLineStyle->direction.getOrInitial();
	if(textOrientation == boost::none)
		textOrientation = toplevel.defaultLineStyle->textOrientation.getOrInitial();
	assert(direction != boost::none);
	assert(textOrientation != boost::none);

	return WritingMode(*direction, *writingMode, *textOrientation);
}

/// Returns the document to which the presentation connects.
const Document& Presentation::document() const BOOST_NOEXCEPT {
	return document_;
}

/// Returns the document to which the presentation connects.
Document& Presentation::document() BOOST_NOEXCEPT {
	return document_;
}

/// @see kernel#DocumentListener#documentAboutToBeChanged
void Presentation::documentAboutToBeChanged(const Document& document) {
	// TODO: not implemented.
}

/// @see kernel#DocumentListener#documentChanged
void Presentation::documentChanged(const Document&, const DocumentChange& change) {
	const Range<Index>
		erasedLines(change.erasedRegion().first.line, change.erasedRegion().second.line),
		insertedLines(change.insertedRegion().first.line, change.insertedRegion().second.line);
	for(list<Hyperlinks*>::iterator i(hyperlinks_.begin()), e(hyperlinks_.end()); i != e; ) {
		const Index line = (*i)->lineNumber;
		if(line == insertedLines.beginning() || includes(erasedLines, line)) {
			for(size_t j = 0; j < (*i)->numberOfHyperlinks; ++j)
				delete (*i)->hyperlinks[j];
			delete *i;
			i = hyperlinks_.erase(i);
			continue;
		} else {
			if(line >= erasedLines.end() && !isEmpty(erasedLines))
				(*i)->lineNumber -= length(erasedLines);
			if(line >= insertedLines.end() && !isEmpty(insertedLines))
				(*i)->lineNumber += length(insertedLines);
		}
		++i;
	}
}

/**
 * Returns the hyperlinks in the specified line.
 * @param line the line number
 * @param[out] numberOfHyperlinks the number of the hyperlinks
 * @return the hyperlinks or @c null
 * @throw BadPositionException @a line is outside of the document
 */
const Hyperlink* const* Presentation::getHyperlinks(Index line, size_t& numberOfHyperlinks) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	else if(hyperlinkDetector_.get() == nullptr) {
		numberOfHyperlinks = 0;
		return nullptr;
	}

	// find in the cache
	for(list<Hyperlinks*>::iterator i(hyperlinks_.begin()), e(hyperlinks_.end()); i != e; ++i) {
		if((*i)->lineNumber == line) {
			Hyperlinks& result = **i;
			if(&result != hyperlinks_.front()) {
				// bring to the front
				hyperlinks_.erase(i);
				hyperlinks_.push_front(&result);
			}
			numberOfHyperlinks = result.numberOfHyperlinks;
			return result.hyperlinks.get();
		}
	}

	// not found in the cache
	if(hyperlinks_.size() == ASCENSION_HYPERLINKS_CACHE_SIZE) {
		delete hyperlinks_.back();
		hyperlinks_.pop_back();
	}
	vector<Hyperlink*> temp;
	for(Index offsetInLine = 0, eol = document_.lineLength(line); offsetInLine < eol; ) {
		unique_ptr<Hyperlink> h(hyperlinkDetector_->nextHyperlink(document_, line, Range<Index>(offsetInLine, eol)));
		if(h.get() == nullptr)
			break;
		// check result
		const Range<Index>& r(h->region());
		if(r.beginning() < offsetInLine)
			break;
		offsetInLine = r.end();
		temp.push_back(h.release());
	}
	unique_ptr<Hyperlinks> newItem(new Hyperlinks);
	newItem->lineNumber = line;
	newItem->hyperlinks.reset(new Hyperlink*[numberOfHyperlinks = newItem->numberOfHyperlinks = temp.size()]);
	copy(temp.begin(), temp.end(), newItem->hyperlinks.get());
	hyperlinks_.push_front(newItem.release());
	return hyperlinks_.front()->hyperlinks.get();
}

/**
 * Removes the global text style listener.
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 * @see #globalTextLineStyle, #addGlobalTextStyleListener
 */
void Presentation::removeGlobalTextStyleListener(GlobalTextStyleListener& listener) {
	globalTextStyleListeners_.remove(listener);
}

/**
 * Removes the specified text line color specifier.
 * @param specifier The director to remove
 */
void Presentation::removeTextLineColorSpecifier(TextLineColorSpecifier& specifier) BOOST_NOEXCEPT {
	for(list<shared_ptr<TextLineColorSpecifier>>::iterator
			i(textLineColorSpecifiers_.begin()), e(textLineColorSpecifiers_.end()); i != e; ++i) {
		if(i->get() == &specifier) {
			textLineColorSpecifiers_.erase(i);
			return;
		}
	}
}

/**
 * Sets the hyperlink detector.
 * @param newDirector The director. Set @c null to unregister
 */
void Presentation::setHyperlinkDetector(shared_ptr<HyperlinkDetector> newDetector) BOOST_NOEXCEPT {
	hyperlinkDetector_ = newDetector;
	clearHyperlinksCache();
}

/**
 * Sets the line style declarator.
 * @param newDeclarator The declarator. @c null to unregister
 */
void Presentation::setTextLineStyleDeclarator(shared_ptr<TextLineStyleDeclarator> newDeclarator) BOOST_NOEXCEPT {
	textLineStyleDeclarator_ = newDeclarator;
}

/**
 * Sets the text run style declarator.
 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
 * @param newDirector The declarator. @c null to unregister
 */
void Presentation::setTextRunStyleDeclarator(shared_ptr<TextRunStyleDeclarator> newDeclarator) BOOST_NOEXCEPT {
	textRunStyleDeclarator_ = newDeclarator;
}

/**
 * Sets the global text line style.
 * @param newStyle The style to set
 * @see #globalTextStyle
 */
void Presentation::setTextToplevelStyle(shared_ptr<const TextToplevelStyle> newStyle) {
	const shared_ptr<const TextToplevelStyle> used(textToplevelStyle_);
	textToplevelStyle_ = (newStyle.get() != nullptr) ? newStyle : DEFAULT_TEXT_TOPLEVEL_STYLE;
	globalTextStyleListeners_.notify<shared_ptr<const TextToplevelStyle>>(
		&GlobalTextStyleListener::globalTextStyleChanged, used);
}

/**
 * Returns the colors of the specified text line.
 * @param line The line
 * @param[out] foreground The foreground color of the line. Unspecified if an invalid value
 * @param[out] background The background color of the line. Unspecified if an invalid value
 * @throw BadPositionException @a line is outside of the document
 */
void Presentation::textLineColors(Index line,
		boost::optional<Color>& foreground, boost::optional<Color>& background) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	TextLineColorSpecifier::Priority highestPriority = 0, p;
	boost::optional<Color> f, g;
	for(list<shared_ptr<TextLineColorSpecifier>>::const_iterator
			i(textLineColorSpecifiers_.begin()), e(textLineColorSpecifiers_.end()); i != e; ++i) {
		p = (*i)->specifyTextLineColors(line, f, g);
		if(p > highestPriority) {
			highestPriority = p;
			foreground = f;
			background = g;
		}
	}
}


// SingleStyledPartitionPresentationReconstructor.Iterator ////////////////////////////////////////

class SingleStyledPartitionPresentationReconstructor::Iterator : public presentation::StyledTextRunIterator {
public:
	Iterator(const Range<Index>& range, shared_ptr<const TextRunStyle> style) BOOST_NOEXCEPT : range_(range), style_(style), done_(false) {
	}
private:
	// StyledTextRunIterator
	Range<Index> currentRange() const {
		if(done_)
			throw NoSuchElementException();
		return range_;
	}
	shared_ptr<const TextRunStyle> currentStyle() const {
		if(done_)
			throw NoSuchElementException();
		return style_;
	}
	bool isDone() const BOOST_NOEXCEPT {
		return done_;
	}
	void next() {
		if(done_)
			throw NoSuchElementException();
		done_ = true;
	}
private:
	const Range<Index> range_;
	const shared_ptr<const TextRunStyle> style_;
	bool done_;
};


// SingleStyledPartitionPresentationReconstructor /////////////////////////////////////////////////

/**
 * Constructor.
 * @param style The style
 */
SingleStyledPartitionPresentationReconstructor::SingleStyledPartitionPresentationReconstructor(shared_ptr<const TextRunStyle> style) BOOST_NOEXCEPT : style_(style) {
}

/// @see PartitionPresentationReconstructor#getPresentation
unique_ptr<StyledTextRunIterator> SingleStyledPartitionPresentationReconstructor::getPresentation(Index, const Range<Index>& rangeInLine) const {
	return unique_ptr<presentation::StyledTextRunIterator>(new Iterator(rangeInLine, style_));
}


// PresentationReconstructor.Iterator /////////////////////////////////////////////////////////////

class PresentationReconstructor::Iterator : public presentation::StyledTextRunIterator {
public:
	Iterator(const Presentation& presentation,
		const map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors, Index line);
private:
	void updateSubiterator();
	// StyledTextRunIterator
	Range<Index> currentRange() const;
	shared_ptr<const TextRunStyle> currentStyle() const;
	bool isDone() const BOOST_NOEXCEPT;
	void next();
private:
	static const shared_ptr<const TextRunStyle> DEFAULT_TEXT_RUN_STYLE;
	const Presentation& presentation_;
	const map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors_;
	const Index line_;
	DocumentPartition currentPartition_;
	unique_ptr<presentation::StyledTextRunIterator> subiterator_;
	Range<Index> currentRange_;
	shared_ptr<const TextRunStyle> currentStyle_;
};

const shared_ptr<const TextRunStyle> PresentationReconstructor::Iterator::DEFAULT_TEXT_RUN_STYLE(new TextRunStyle);

/**
 * Constructor.
 * @param presentation
 * @param reconstructors
 * @param line
 */
PresentationReconstructor::Iterator::Iterator(
		const Presentation& presentation, const map<kernel::ContentType,
		PartitionPresentationReconstructor*> reconstructors, Index line)
		: presentation_(presentation), reconstructors_(reconstructors), line_(line) {
	const DocumentPartitioner& partitioner = presentation_.document().partitioner();
	Index offsetInLine = 0;
	for(const Index lineLength = presentation_.document().lineLength(line);;) {
		partitioner.partition(Position(line, offsetInLine), currentPartition_);	// this may throw BadPositionException
		if(!currentPartition_.region.isEmpty())
			break;
		if(++offsetInLine >= lineLength) {	// rare case...
			currentPartition_.contentType = DEFAULT_CONTENT_TYPE;
			currentPartition_.region = Region(line, make_pair(0, lineLength));
			break;
		}
	}
	updateSubiterator();
}

/// @see StyledTextRunIterator#currentRange
Range<Index> PresentationReconstructor::Iterator::currentRange() const {
	if(subiterator_.get() != nullptr)
		return subiterator_->currentRange();
	else if(!isDone())
		return currentRange_;
	throw NoSuchElementException();
}

/// @see StyledTextRunIterator#currentStyle
shared_ptr<const TextRunStyle> PresentationReconstructor::Iterator::currentStyle() const {
	if(subiterator_.get() != nullptr)
		return subiterator_->currentStyle();
	else if(!isDone())
		return currentStyle_;
	throw NoSuchElementException();
}

/// @see StyledTextRunIterator#isDone
bool PresentationReconstructor::Iterator::isDone() const BOOST_NOEXCEPT {
	return currentPartition_.region.isEmpty();
}

/// @see StyledTextRunIterator#next
void PresentationReconstructor::Iterator::next() {
	if(subiterator_.get() != nullptr) {
		subiterator_->next();
		if(subiterator_->isDone())
			subiterator_.reset();
	}
	if(subiterator_.get() == nullptr) {
		const Document& document = presentation_.document();
		const Index lineLength = document.lineLength(line_);
		if(currentPartition_.region.end() >= Position(line_, lineLength)) {
			// done
			currentPartition_.region = Region(currentPartition_.region.end());
			return;
		}
		// find the next partition
		const DocumentPartitioner& partitioner = document.partitioner();
		for(Index offsetInLine = currentPartition_.region.end().offsetInLine; ; ) {
			partitioner.partition(Position(line_, offsetInLine), currentPartition_);
			if(!currentPartition_.region.isEmpty())
				break;
			if(++offsetInLine >= lineLength) {	// rare case...
				currentPartition_.contentType = DEFAULT_CONTENT_TYPE;
				currentPartition_.region = Region(line_, make_pair(offsetInLine, lineLength));
			}
		}
		updateSubiterator();
	}
}

inline void PresentationReconstructor::Iterator::updateSubiterator() {
	map<ContentType, PartitionPresentationReconstructor*>::const_iterator r(reconstructors_.find(currentPartition_.contentType));
	if(r != reconstructors_.end())
		subiterator_ = r->second->getPresentation(currentPartition_.region);
	else
		subiterator_.reset();
	if(subiterator_.get() == nullptr) {
		const shared_ptr<const TextLineStyle> lineStyle(presentation_.textToplevelStyle().defaultLineStyle);
		assert(lineStyle.get() != nullptr);
		shared_ptr<const TextRunStyle> runStyle(lineStyle->defaultRunStyle);
		if(runStyle.get() == nullptr)
			runStyle = DEFAULT_TEXT_RUN_STYLE;
		currentRange_ = makeRange(currentPartition_.region.beginning().offsetInLine, currentPartition_.region.end().offsetInLine);
		currentStyle_ = runStyle;
	}
}


// PresentationReconstructor //////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation The presentation
 */
PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
	presentation_.setTextRunStyleDeclarator(shared_ptr<TextRunStyleDeclarator>(this));	// TODO: danger call (may delete this).
}

/// Destructor.
PresentationReconstructor::~PresentationReconstructor() /*throw()*/ {
//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<LineStyleDirector>());
	for(map<ContentType, PartitionPresentationReconstructor*>::iterator i(reconstructors_.begin()); i != reconstructors_.end(); ++i)
		delete i->second;
}

/// @see LineStyleDeclarator#declareTextRunStyle
unique_ptr<StyledTextRunIterator> PresentationReconstructor::declareTextRunStyle(Index line) const {
	return unique_ptr<presentation::StyledTextRunIterator>(new Iterator(presentation_, reconstructors_, line));
}

/**
 * Sets the partition presentation reconstructor for the specified content type.
 * @param contentType The content type. If a reconstructor for this content type was already be
 *                    set, the old will be deleted
 * @param reconstructor The partition presentation reconstructor to set. Can't be @c null. The
 *                      ownership will be transferred to the callee
 * @throw NullPointerException @a reconstructor is @c null
 */
void PresentationReconstructor::setPartitionReconstructor(
		ContentType contentType, unique_ptr<PartitionPresentationReconstructor> reconstructor) {
	if(reconstructor.get() == nullptr)
		throw NullPointerException("reconstructor");
	const map<ContentType, PartitionPresentationReconstructor*>::iterator old(reconstructors_.find(contentType));
	if(old != reconstructors_.end()) {
		delete old->second;
		reconstructors_.erase(old);
	}
	reconstructors_.insert(make_pair(contentType, reconstructor.release()));
}


// hyperlink.URIHyperlinkDetector /////////////////////////////////////////////////////////////////

namespace {
	class URIHyperlink : public Hyperlink {
	public:
		explicit URIHyperlink(const Range<Index>& region, const String& uri) /*throw()*/ : Hyperlink(region), uri_(uri) {}
		String description() const /*throw()*/ {
			static const Char PRECEDING[] = {0x202au, 0};
			static const Char FOLLOWING[] = {0x202cu, 0x0a,
				0x43, 0x54, 0x52, 0x4c, 0x20, 0x2b, 0x20, 0x63, 0x6c, 0x69, 0x63, 0x6b, 0x20,
				0x74, 0x6f, 0x20, 0x66, 0x6f, 0x6c, 0x6c, 0x6f, 0x77, 0x20, 0x74, 0x68, 0x65,
				0x20, 0x6c, 0x69, 0x6e, 0x6b, 0x2e, 0
			};	// "\x202c\nCTRL + click to follow the link."
			return PRECEDING + uri_ + FOLLOWING;
		}
		void invoke() const /*throw()*/ {
#ifdef ASCENSION_OS_WINDOWS
			::ShellExecuteW(nullptr, nullptr, uri_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
#endif
		}
	private:
		const String uri_;
	};
} // namespace @0

/**
 * Constructor.
 * @param uriDetector Can't be @c null
 * @throw NullPointerException @a uriDetector is @c null
 */
URIHyperlinkDetector::URIHyperlinkDetector(shared_ptr<const URIDetector> uriDetector) : uriDetector_(uriDetector) {
	if(uriDetector.get() == nullptr)
		throw NullPointerException("uriDetector");
}

/// Destructor.
URIHyperlinkDetector::~URIHyperlinkDetector() /*throw()*/ {
}

/// @see HyperlinkDetector#nextHyperlink
unique_ptr<Hyperlink> URIHyperlinkDetector::nextHyperlink(
		const Document& document, Index line, const Range<Index>& range) const /*throw()*/ {
	// TODO: ??? range is not used???
	const String& s = document.line(line);
	const Char* p = s.data();
	Range<const Char*> result;
	if(uriDetector_->search(p, p + s.length(), result))
		return unique_ptr<Hyperlink>(new URIHyperlink(
			Range<Index>(result.beginning() - p, result.end() - p), String(result.beginning(), result.end())));
	else
		return unique_ptr<Hyperlink>();
}


// hyperlink.CompositeHyperlinkDetector ///////////////////////////////////////////////////////////

/// Destructor.
CompositeHyperlinkDetector::~CompositeHyperlinkDetector() /*throw()*/ {
	for(map<ContentType, HyperlinkDetector*>::iterator i(composites_.begin()), e(composites_.end()); i != e; ++i)
		delete i->second;
}

/// @see HyperlinkDetector#nextHyperlink
unique_ptr<Hyperlink> CompositeHyperlinkDetector::nextHyperlink(
		const Document& document, Index line, const Range<Index>& range) const /*throw()*/ {
	const DocumentPartitioner& partitioner = document.partitioner();
	DocumentPartition partition;
	for(Position p(line, range.beginning()), e(line, range.end()); p < e;) {
		partitioner.partition(p, partition);
		assert(partition.region.includes(p));
		map<ContentType, HyperlinkDetector*>::const_iterator detector(composites_.find(partition.contentType));
		if(detector != composites_.end()) {
			unique_ptr<Hyperlink> found = detector->second->nextHyperlink(
				document, line, Range<Index>(p.offsetInLine, min(partition.region.end(), e).offsetInLine));
			if(found.get() != nullptr)
				return found;
		}
		p = partition.region.end();
	}
	return unique_ptr<Hyperlink>();
}

/**
 * Sets the hyperlink detector for the specified content type.
 * @param contentType The content type. if a detector for this content type was already be set, the
 *                    old will be deleted
 * @param detector The hyperlink detector to set. Can't be @c null. The ownership will be
 *                 transferred to the callee
 * @throw NullPointerException @a detector is @c null
 */
void CompositeHyperlinkDetector::setDetector(ContentType contentType, unique_ptr<HyperlinkDetector> detector) {
	if(detector.get() == nullptr)
		throw NullPointerException("detector");
	map<ContentType, HyperlinkDetector*>::iterator old(composites_.find(contentType));
	if(old != composites_.end()) {
		composites_.erase(old);
		delete old->second;
	}
	composites_.insert(make_pair(contentType, detector.release()));
}


// LexicalPartitionPresentationReconstructor.StyledTextRunIterator ////////////////////////////////

/// @internal
class LexicalPartitionPresentationReconstructor::StyledTextRunIterator : public presentation::StyledTextRunIterator {
public:
	StyledTextRunIterator(const Document& document, TokenScanner& tokenScanner,
		const map<Token::Identifier, shared_ptr<const TextRunStyle>>& styles,
		shared_ptr<const TextRunStyle> defaultStyle, const Region& region);
private:
	void nextRun();
	// StyledTextRunIterator
	Range<Index> currentRange() const;
	shared_ptr<const TextRunStyle> currentStyle() const;
	bool isDone() const BOOST_NOEXCEPT;
	void next();
private:
//	const LexicalPartitionPresentationReconstructor& reconstructor_;
	TokenScanner& tokenScanner_;
	const map<Token::Identifier, shared_ptr<const TextRunStyle>>& styles_;
	shared_ptr<const TextRunStyle> defaultStyle_;
	const Region region_;
	Region currentRegion_;
	shared_ptr<const TextRunStyle> currentStyle_;
	unique_ptr<Token> next_;
};

LexicalPartitionPresentationReconstructor::StyledTextRunIterator::StyledTextRunIterator(
		const Document& document, TokenScanner& tokenScanner,
		const map<Token::Identifier, shared_ptr<const TextRunStyle>>& styles,
		shared_ptr<const TextRunStyle> defaultStyle, const Region& region) :
		tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), currentRegion_(region.beginning()) {
	tokenScanner_.parse(document, region);
	nextRun();
}

/// @see StyledTextRunIterator#currentRange
Range<Index> LexicalPartitionPresentationReconstructor::StyledTextRunIterator::currentRange() const {
	if(isDone())
		throw NoSuchElementException();
	if(length(currentRegion_.lines()) == 1)
		return makeRange(currentRegion_.beginning().offsetInLine, currentRegion_.end().offsetInLine);
	return makeRange(currentRegion_.beginning().offsetInLine, currentRegion_.end().offsetInLine);
}

/// @see StyledTextRunIterator#current
shared_ptr<const TextRunStyle> LexicalPartitionPresentationReconstructor::StyledTextRunIterator::currentStyle() const {
	if(isDone())
		throw NoSuchElementException();
	return currentStyle_;
}

/// @see StyledTextRunIterator#isDone
bool LexicalPartitionPresentationReconstructor::StyledTextRunIterator::isDone() const {
	return currentRegion_.end() == region_.end();
}

/// @see StyledTextRunIterator#next
void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::next() {
	if(isDone())
		throw NoSuchElementException();
	nextRun();
}

inline void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::nextRun() {
	if(next_.get() != nullptr) {
		map<Token::Identifier, shared_ptr<const TextRunStyle>>::const_iterator style(styles_.find(next_->id));
		currentRegion_ = next_->region;
		currentStyle_ = (style != styles_.end()) ? style->second : defaultStyle_;
		next_.reset();
	} else if(tokenScanner_.hasNext()) {
		next_ = move(tokenScanner_.nextToken());
		assert(next_.get() != nullptr);
		assert(next_->region.beginning() >= currentRegion_.end());
		if(next_->region.beginning() != currentRegion_.end()) {
			// 
			currentRegion_ = Region(currentRegion_.end(), next_->region.beginning());
			currentStyle_ = defaultStyle_;
		} else {
			map<Token::Identifier, shared_ptr<const TextRunStyle>>::const_iterator style(styles_.find(next_->id));
			currentRegion_ = next_->region;
			currentStyle_ = (style != styles_.end()) ? style->second : defaultStyle_;
			next_.reset();
		}
	} else if(currentRegion_.end() != region_.end()) {
		//
		currentRegion_ = Region(currentRegion_.end(), region_.end());
		currentStyle_ = defaultStyle_;
	}
}


// LexicalPartitionPresentationReconstructor //////////////////////////////////////////////////////

/// @see presentation#PartitionPresentationReconstructor#getPresentation
unique_ptr<StyledTextRunIterator> LexicalPartitionPresentationReconstructor::getPresentation(const Region& region) const /*throw()*/ {
	return unique_ptr<presentation::StyledTextRunIterator>(
		new StyledTextRunIterator(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
}
