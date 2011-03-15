/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2011
 */

#include <ascension/graphics/rendering.hpp>	// TextRenderer
#include <ascension/presentation.hpp>
#include <ascension/rules.hpp>
#ifdef ASCENSION_WINDOWS
#include <shellapi.h>	// ShellExecuteW
#endif // ASCENSION_WINDOWS
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::graphics;
using namespace ascension::presentation;
using namespace ascension::presentation::hyperlink;
using namespace ascension::rules;
using namespace std;
using graphics::font::TextRenderer;


// Color //////////////////////////////////////////////////////////////////////////////////////////

/// A transparent object whose all components are zero.
const Color Color::TRANSPARENT_COLOR(0, 0, 0, 0);


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


// StyledTextRunEnumerator ////////////////////////////////////////////////////////////////////////

/**
 * Default constructor.
 */
StyledTextRunEnumerator::StyledTextRunEnumerator() : end_(0) {
	current_.first = false;
}

/**
 * Constructor.
 * @param sourceIterator The iterator to encapsulate
 * @param end The end character position
 * @throw NullPointerException @a sourceIterator is @c null
 */
StyledTextRunEnumerator::StyledTextRunEnumerator(
		auto_ptr<StyledTextRunIterator> sourceIterator, length_t end) : iterator_(sourceIterator), end_(end) {
	if(iterator_.get() == 0)
		throw NullPointerException("sourceIterator");
	if(current_.first = iterator_->hasNext()) {
		iterator_->current(current_.second);
		if(current_.second.column < end_) {
			iterator_->next();
			if(next_.first = iterator_->hasNext()) {
				iterator_->current(next_.second);
				if(next_.second.column < end_)
					iterator_->next();
				else {
					next_.first = false;
					next_.second.column = end_;
				}
			} else
				next_.second.column = end_;
		} else
			current_.first = false;
	} else
		next_.first = false;
}

/**
 * Implements @c detail#IteratorAdapter#current.
 * @throw NoSuchElementException The enumerator addresses the end
 */
const StyledTextRunEnumerator::reference StyledTextRunEnumerator::current() const {
	if(!current_.first)
		throw NoSuchElementException();
	return make_pair(makeRange(current_.second.column, next_.second.column), current_.second.style);
}

/// Implements @c detail#IteratorAdapter#equals.
bool StyledTextRunEnumerator::equals(const StyledTextRunEnumerator& other) const /*throw()*/ {
	return !current_.first && !other.current_.first;
}

/**
 * Implements @c detail#IteratorAdapter#next.
 * @throw NoSuchElementException The enumerator addresses the end
 */
void StyledTextRunEnumerator::next() {
	if(!current_.first)
		throw NoSuchElementException();
	current_ = next_;
	if(next_.first = iterator_->hasNext()) {
		iterator_->current(next_.second);
		if(next_.second.column >= end_) {
			next_.first = false;
			next_.second.column = end_;
		} else
			iterator_->next();
	} else
		next_.second.column = end_;
}


#ifdef ASCENSION_WRITING_MODES

// WritingMode ////////////////////////////////////////////////////////////////////////////////////

/**
 * @class ascension::presentation::WritingMode
 * Specifies the block-progression-direction and the inline-progression-direction of text.
 * Semantics of these values are different from the same properties in XSL 1.1, SVG 1.1 and CSS 3.
 * @see Direction
 * @see XSL 1.1, 7.29 Writing-mode-related Properties (http://www.w3.org/TR/xsl/#writing-mode-related)
 * @see SVG 1.1, 10.7.2 Setting the inline-progression-direction (http://www.w3.org/TR/SVG/text.html#SettingInlineProgressionDirection)
 * @see CSS Writing Modes Module Level 3 (http://www.w3.org/TR/2010/WD-css3-writing-modes-20101202/)
 */

const WritingMode WritingMode::LR_TB();

/// Default constructor creates a writing mode means the properties inherit from the parent.
WritingMode::Writing() /*throw()*/ : block_(42), inline_(42), inlineAlternating_(true) {
}

/**
 * Constructor.
 * @param blockProgressionDirection The block-progression-direction
 * @param inlineProgressionDirection The inline-progression-direction
 * @param inlineAlternating
 * @throw std#invalid_argument @a blockProgressionDirection and @a inlineProgressionDirection have
 *                             same dimension
 */
WritingMode::WritingMode(ProgressionDirection blockProgressionDirection,
		ProgressionDirection inlineProgressionDirection, bool inlineAlternating /* = false */)
		: block_(blockProgressionDirection), inline_(inlineProgressionDirection),
		inlineAlternating_(inlineAlternating) {
	if(isHorizontal(blockProgressionDirection)) {
		if(!isVerticalDirection(inlineProgressionDirection))
			throw invalid_argument("");
	} else if(isVertical(blockProgressionDirection)) {
		if(!isHorizontalDirection(inlineProgressionDirection))
			throw invalid_argument("");
	} else
		throw UnknownValueException("blockProgressionDirection");
}
#endif // ASCENSION_WRITING_MODES


// Presentation ///////////////////////////////////////////////////////////////////////////////////

tr1::shared_ptr<const TextLineStyle> Presentation::DEFAULT_TEXT_LINE_STYLE(new TextLineStyle());
tr1::shared_ptr<const TextRunStyle> Presentation::DEFAULT_TEXT_RUN_STYLE(new TextRunStyle());

struct Presentation::Hyperlinks {
	length_t lineNumber;
	AutoBuffer<Hyperlink*> hyperlinks;
	size_t numberOfHyperlinks;
};

/**
 * Constructor.
 * @param document The target document
 */
Presentation::Presentation(Document& document) /*throw()*/ : document_(document) {
	setDefaultTextLineStyle(tr1::shared_ptr<const TextLineStyle>());
	setDefaultTextRunStyle(tr1::shared_ptr<const TextRunStyle>());
	document_.addListener(*this);
}

/// Destructor.
Presentation::~Presentation() /*throw()*/ {
	document_.removeListener(*this);
	clearHyperlinksCache();
}

/**
 * Registers the default text style listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 * @see #defaultTextLineStyle, #defaultTextRunStyle
 */
void Presentation::addDefaultTextStyleListener(DefaultTextStyleListener& listener) {
	defaultTextStyleListeners_.add(listener);
}

void Presentation::clearHyperlinksCache() /*throw()*/ {
	for(list<Hyperlinks*>::iterator i(hyperlinks_.begin()), e(hyperlinks_.end()); i != e; ++i) {
		for(size_t j = 0; j < (*i)->numberOfHyperlinks; ++j)
			delete (*i)->hyperlinks[j];
		delete *i;
	}
	hyperlinks_.clear();
}

/// Returns the document to which the presentation connects.
const Document& Presentation::document() const /*throw()*/ {
	return document_;
}

/// Returns the document to which the presentation connects.
Document& Presentation::document() /*throw()*/ {
	return document_;
}

/// @see kernel#DocumentListener#documentAboutToBeChanged
void Presentation::documentAboutToBeChanged(const Document& document) {
	// TODO: not implemented.
}

/// @see kernel#DocumentListener#documentChanged
void Presentation::documentChanged(const Document&, const DocumentChange& change) {
	const Range<length_t>
		erasedLines(change.erasedRegion().first.line, change.erasedRegion().second.line),
		insertedLines(change.insertedRegion().first.line, change.insertedRegion().second.line);
	for(list<Hyperlinks*>::iterator i(hyperlinks_.begin()), e(hyperlinks_.end()); i != e; ) {
		const length_t line = (*i)->lineNumber;
		if(line == insertedLines.beginning() || erasedLines.includes(line)) {
			for(size_t j = 0; j < (*i)->numberOfHyperlinks; ++j)
				delete (*i)->hyperlinks[j];
			delete *i;
			i = hyperlinks_.erase(i);
			continue;
		} else {
			if(line >= erasedLines.end() && !erasedLines.isEmpty())
				(*i)->lineNumber -= erasedLines.length();
			if(line >= insertedLines.end() && !insertedLines.isEmpty())
				(*i)->lineNumber += insertedLines.length();
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
const Hyperlink* const* Presentation::getHyperlinks(length_t line, size_t& numberOfHyperlinks) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	else if(hyperlinkDetector_.get() == 0) {
		numberOfHyperlinks = 0;
		return 0;
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
	for(length_t column = 0, eol = document_.lineLength(line); column < eol; ) {
		auto_ptr<Hyperlink> h(hyperlinkDetector_->nextHyperlink(document_, line, Range<length_t>(column, eol)));
		if(h.get() == 0)
			break;
		// check result
		const Range<length_t>& r(h->region());
		if(r.beginning() < column)
			break;
		column = r.end();
		temp.push_back(h.release());
	}
	auto_ptr<Hyperlinks> newItem(new Hyperlinks);
	newItem->lineNumber = line;
	newItem->hyperlinks.reset(new Hyperlink*[numberOfHyperlinks = newItem->numberOfHyperlinks = temp.size()]);
	copy(temp.begin(), temp.end(), newItem->hyperlinks.get());
	hyperlinks_.push_front(newItem.release());
	return hyperlinks_.front()->hyperlinks.get();
}

/**
 * Removes the default text style listener.
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 * @see #defaultTextLineStyle, #defaultTextRunStyle
 */
void Presentation::removeDefaultTextStyleListener(DefaultTextStyleListener& listener) {
	defaultTextStyleListeners_.remove(listener);
}

/**
 * Sets the default text line style.
 * @param newStyle The style to set
 * @see #defaultTextLineStyle, #setDefaultTextRunStyle
 */
void Presentation::setDefaultTextLineStyle(tr1::shared_ptr<const TextLineStyle> newStyle) {
	const tr1::shared_ptr<const TextLineStyle> used(defaultTextLineStyle_);
	defaultTextLineStyle_ = (newStyle.get() != 0) ? newStyle : DEFAULT_TEXT_LINE_STYLE;
	defaultTextStyleListeners_.notify<tr1::shared_ptr<const TextLineStyle> >(
		&DefaultTextStyleListener::defaultTextLineStyleChanged, used);
}

/**
 * Sets the default text run style.
 * @param newStyle The style to set
 * @see #defaultTextRunStyle, #setDefaultTextLineStyle
 */
void Presentation::setDefaultTextRunStyle(tr1::shared_ptr<const TextRunStyle> newStyle) {
	const tr1::shared_ptr<const TextRunStyle> used(defaultTextRunStyle_);
	defaultTextRunStyle_ = (newStyle.get() != 0) ? newStyle : DEFAULT_TEXT_RUN_STYLE;
	defaultTextStyleListeners_.notify<tr1::shared_ptr<const TextRunStyle> >(
		&DefaultTextStyleListener::defaultTextRunStyleChanged, used);
}

/**
 * Sets the hyperlink detector.
 * @param newDirector The director. @c null to unregister
 * @param delegateOwnership Set @c true to transfer the ownership of @a newDirector to the callee
 */
void Presentation::setHyperlinkDetector(HyperlinkDetector* newDetector, bool delegateOwnership) /*throw()*/ {
	hyperlinkDetector_.reset(newDetector, delegateOwnership);
	clearHyperlinksCache();
}

/**
 * Sets the line style director.
 * @param newDirector The director. @c null to unregister
 */
void Presentation::setTextLineStyleDirector(tr1::shared_ptr<TextLineStyleDirector> newDirector) /*throw()*/ {
	textLineStyleDirector_ = newDirector;
}

/**
 * Sets the text run style director.
 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
 * @param newDirector The director. @c null to unregister
 */
void Presentation::setTextRunStyleDirector(tr1::shared_ptr<TextRunStyleDirector> newDirector) /*throw()*/ {
	textRunStyleDirector_ = newDirector;
}

/**
 * Returns the colors of the specified text line.
 * @param line The line
 * @param[out] foreground The foreground color of the line. Unspecified if an invalid value
 * @param[out] background The background color of the line. Unspecified if an invalid value
 * @throw BadPositionException @a line is outside of the document
 */
void Presentation::textLineColors(length_t line, Color& foreground, Color& background) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	TextLineColorDirector::Priority highestPriority = 0, p;
	pair<Color, Color> temp;
	for(list<tr1::shared_ptr<TextLineColorDirector> >::const_iterator
			i(textLineColorDirectors_.begin()), e(textLineColorDirectors_.end()); i != e; ++i) {
		p = (*i)->queryTextLineColors(line, temp.first, temp.second);
		if(p > highestPriority) {
			highestPriority = p;
			foreground = temp.first;
			background = temp.second;
		}
	}
}

/**
 * Returns the styles of the text runs in the specified line.
 * @param line The line
 * @return An iterator enumerates the styles of the text runs in the line, or @c null if the line
 *         has no styled text runs
 * @throw BadPositionException @a line is outside of the document
 */
auto_ptr<StyledTextRunIterator> Presentation::textRunStyles(length_t line) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	return (textRunStyleDirector_.get() != 0) ? textRunStyleDirector_->queryTextRunStyle(line) : auto_ptr<StyledTextRunIterator>();
}


// SingleStyledPartitionPresentationReconstructor.StyledTextRunIterator ///////////////////////////

class SingleStyledPartitionPresentationReconstructor::StyledTextRunIterator : public presentation::StyledTextRunIterator {
public:
	StyledTextRunIterator(length_t column, tr1::shared_ptr<const TextRunStyle> style) : run_(column, style), done_(false) {}
private:
	// StyledTextRunIterator
	void current(StyledTextRun& run) const {
		if(done_)
			throw IllegalStateException("the iterator addresses the end.");
		run = run_;
	}
	bool hasNext() const {
		return !done_;
	}
	void next() {
		if(done_)
			throw IllegalStateException("the iterator addresses the end.");
		done_ = true;
	}
private:
	const StyledTextRun run_;
	bool done_;
};


// SingleStyledPartitionPresentationReconstructor /////////////////////////////////////////////////

/**
 * Constructor.
 * @param style The style
 */
SingleStyledPartitionPresentationReconstructor::SingleStyledPartitionPresentationReconstructor(tr1::shared_ptr<const TextRunStyle> style) /*throw()*/ : style_(style) {
}

/// @see PartitionPresentationReconstructor#getPresentation
auto_ptr<StyledTextRunIterator> SingleStyledPartitionPresentationReconstructor::getPresentation(length_t, const Range<length_t>& columnRange) const /*throw()*/ {
	return auto_ptr<presentation::StyledTextRunIterator>(new StyledTextRunIterator(columnRange.beginning(), style_));
}


// PresentationReconstructor.StyledTextRunIterator ////////////////////////////////////////////////

class PresentationReconstructor::StyledTextRunIterator : public presentation::StyledTextRunIterator {
public:
	StyledTextRunIterator(const Presentation& presentation,
		const map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors, length_t line);
private:
	void updateSubiterator();
	// StyledTextRunIterator
	void current(StyledTextRun& run) const;
	bool hasNext() const;
	void next();
private:
	const Presentation& presentation_;
	const map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors_;
	const length_t line_;
	DocumentPartition currentPartition_;
	auto_ptr<presentation::StyledTextRunIterator> subiterator_;
	StyledTextRun current_;
};

/**
 * Constructor.
 * @param presentation
 * @param reconstructors
 * @param line
 */
PresentationReconstructor::StyledTextRunIterator::StyledTextRunIterator(
		const Presentation& presentation, const map<kernel::ContentType,
		PartitionPresentationReconstructor*> reconstructors, length_t line)
		: presentation_(presentation), reconstructors_(reconstructors), line_(line) {
	const DocumentPartitioner& partitioner = presentation_.document().partitioner();
	length_t column = 0;
	for(const length_t lineLength = presentation_.document().lineLength(line);;) {
		partitioner.partition(Position(line, column), currentPartition_);	// this may throw BadPositionException
		if(!currentPartition_.region.isEmpty())
			break;
		if(++column >= lineLength) {	// rare case...
			currentPartition_.contentType = DEFAULT_CONTENT_TYPE;
			currentPartition_.region = Region(line, make_pair(0, lineLength));
			break;
		}
	}
	updateSubiterator();
}

/// @see StyledTextRunIterator#current
void PresentationReconstructor::StyledTextRunIterator::current(StyledTextRun& run) const {
	if(subiterator_.get() != 0)
		subiterator_->current(run);
	else if(hasNext())
		run = current_;
	throw IllegalStateException("the iterator addresses the end.");
}

/// @see StyledTextRunIterator#hasNext
bool PresentationReconstructor::StyledTextRunIterator::hasNext() const {
	return !currentPartition_.region.isEmpty();
}

/// @see StyledTextRunIterator#next
void PresentationReconstructor::StyledTextRunIterator::next() {
	if(subiterator_.get() != 0) {
		subiterator_->next();
		if(!subiterator_->hasNext())
			subiterator_.reset();
	}
	if(subiterator_.get() == 0) {
		const Document& document = presentation_.document();
		const length_t lineLength = document.lineLength(line_);
		if(currentPartition_.region.end() >= Position(line_, lineLength)) {
			// done
			currentPartition_.region = Region(currentPartition_.region.end());
			return;
		}
		// find the next partition
		const DocumentPartitioner& partitioner = document.partitioner();
		for(length_t column = currentPartition_.region.end().column; ; ) {
			partitioner.partition(Position(line_, column), currentPartition_);
			if(!currentPartition_.region.isEmpty())
				break;
			if(++column >= lineLength) {	// rare case...
				currentPartition_.contentType = DEFAULT_CONTENT_TYPE;
				currentPartition_.region = Region(line_, make_pair(column, lineLength));
			}
		}
		updateSubiterator();
	}
}

inline void PresentationReconstructor::StyledTextRunIterator::updateSubiterator() {
	map<ContentType, PartitionPresentationReconstructor*>::const_iterator r(reconstructors_.find(currentPartition_.contentType));
	subiterator_ = (r != reconstructors_.end()) ?
		r->second->getPresentation(currentPartition_.region) : auto_ptr<presentation::StyledTextRunIterator>();
	if(subiterator_.get() == 0)
		current_ = StyledTextRun(currentPartition_.region.beginning().column, presentation_.defaultTextRunStyle());
}


// PresentationReconstructor //////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation The presentation
 */
PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
	presentation_.setTextRunStyleDirector(tr1::shared_ptr<TextRunStyleDirector>(this));	// TODO: danger call (may delete this).
}

/// Destructor.
PresentationReconstructor::~PresentationReconstructor() /*throw()*/ {
//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<LineStyleDirector>(0));
	for(map<ContentType, PartitionPresentationReconstructor*>::iterator i(reconstructors_.begin()); i != reconstructors_.end(); ++i)
		delete i->second;
}

/// @see LineStyleDirector#queryTextRunStyle
auto_ptr<StyledTextRunIterator> PresentationReconstructor::queryTextRunStyle(length_t line) const {
	return auto_ptr<presentation::StyledTextRunIterator>(
		new StyledTextRunIterator(presentation_, reconstructors_, line));
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
		ContentType contentType, auto_ptr<PartitionPresentationReconstructor> reconstructor) {
	if(reconstructor.get() == 0)
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
		explicit URIHyperlink(const Range<length_t>& region, const String& uri) /*throw()*/ : Hyperlink(region), uri_(uri) {}
		String description() const /*throw()*/ {return L"\x202a" + uri_ + L"\x202c\nCTRL + click to follow the link.";}
		void invoke() const /*throw()*/ {
#ifdef ASCENSION_WINDOWS
			::ShellExecuteW(0, 0, uri_.c_str(), 0, 0, SW_SHOWNORMAL);
#else
#endif
		}
	private:
		const String uri_;
	};
} // namespace @0

/**
 * Constructor.
 * @param uriDetector
 * @param delegateOwnership
 */
URIHyperlinkDetector::URIHyperlinkDetector(const URIDetector& uriDetector,
		bool delegateOwnership) : uriDetector_(&uriDetector, delegateOwnership) {
}

/// Destructor.
URIHyperlinkDetector::~URIHyperlinkDetector() /*throw()*/ {
}

/// @see HyperlinkDetector#nextHyperlink
auto_ptr<Hyperlink> URIHyperlinkDetector::nextHyperlink(
		const Document& document, length_t line, const Range<length_t>& range) const /*throw()*/ {
	// TODO: ??? range is not used???
	const String& s = document.line(line);
	const Char* p = s.data();
	Range<const Char*> result;
	if(uriDetector_->search(p, p + s.length(), result))
		return auto_ptr<Hyperlink>(new URIHyperlink(
			Range<length_t>(result.beginning() - p, result.end() - p), String(result.beginning(), result.end())));
	else
		return auto_ptr<Hyperlink>(0);
}


// hyperlink.CompositeHyperlinkDetector ///////////////////////////////////////////////////////////

/// Destructor.
CompositeHyperlinkDetector::~CompositeHyperlinkDetector() /*throw()*/ {
	for(map<ContentType, HyperlinkDetector*>::iterator i(composites_.begin()), e(composites_.end()); i != e; ++i)
		delete i->second;
}

/// @see HyperlinkDetector#nextHyperlink
auto_ptr<Hyperlink> CompositeHyperlinkDetector::nextHyperlink(
		const Document& document, length_t line, const Range<length_t>& range) const /*throw()*/ {
	const DocumentPartitioner& partitioner = document.partitioner();
	DocumentPartition partition;
	for(Position p(line, range.beginning()), e(line, range.end()); p < e;) {
		partitioner.partition(p, partition);
		assert(partition.region.includes(p));
		map<ContentType, HyperlinkDetector*>::const_iterator detector(composites_.find(partition.contentType));
		if(detector != composites_.end()) {
			auto_ptr<Hyperlink> found = detector->second->nextHyperlink(
				document, line, Range<length_t>(p.column, min(partition.region.end(), e).column));
			if(found.get() != 0)
				return found;
		}
		p = partition.region.end();
	}
	return auto_ptr<Hyperlink>(0);
}

/**
 * Sets the hyperlink detector for the specified content type.
 * @param contentType The content type. if a detector for this content type was already be set, the
 *                    old will be deleted
 * @param detector The hyperlink detector to set. Can't be @c null. The ownership will be
 *                 transferred to the callee
 * @throw NullPointerException @a detector is @c null
 */
void CompositeHyperlinkDetector::setDetector(ContentType contentType, auto_ptr<HyperlinkDetector> detector) {
	if(detector.get() == 0)
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
		const map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >& styles,
		tr1::shared_ptr<const TextRunStyle> defaultStyle, const Region& region);
private:
	void nextRun();
	// StyledTextRunIterator
	void current(StyledTextRun& run) const;
	bool hasNext() const;
	void next();
private:
//	const LexicalPartitionPresentationReconstructor& reconstructor_;
	TokenScanner& tokenScanner_;
	const map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >& styles_;
	tr1::shared_ptr<const TextRunStyle> defaultStyle_;
	Region region_;
	StyledTextRun current_;
	auto_ptr<Token> next_;
	Position lastTokenEnd_;
};

LexicalPartitionPresentationReconstructor::StyledTextRunIterator::StyledTextRunIterator(
		const Document& document, TokenScanner& tokenScanner,
		const map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >& styles,
		tr1::shared_ptr<const TextRunStyle> defaultStyle, const Region& region) :
		tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), lastTokenEnd_(region.beginning()) {
	tokenScanner_.parse(document, region);
	nextRun();
}

/// @see StyledTextRunIterator#current
void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::current(StyledTextRun& run) const {
	if(!hasNext())
		throw IllegalStateException("the iterator addresses the end.");
	run = current_;
}

/// @see StyledTextRunIterator#hasNext
bool LexicalPartitionPresentationReconstructor::StyledTextRunIterator::hasNext() const {
	return lastTokenEnd_.column != region_.end().column;
}

/// @see StyledTextRunIterator#next
void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::next() {
	if(!hasNext())
		throw IllegalStateException("the iterator addresses the end.");
	nextRun();
}

inline void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::nextRun() {
	if(next_.get() != 0) {
		map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >::const_iterator style(styles_.find(next_->id));
		current_.column = next_->region.beginning().column;
		current_.style = (style != styles_.end()) ? style->second : defaultStyle_;
		lastTokenEnd_ = next_->region.end();
		next_.reset();
	} else if(tokenScanner_.hasNext()) {
		next_ = tokenScanner_.nextToken();
		assert(next_.get() != 0);
		if(next_->region.beginning() != lastTokenEnd_) {
			// 
			current_.column = lastTokenEnd_.column;
			current_.style = defaultStyle_;
			lastTokenEnd_ = next_->region.beginning();
		} else {
			map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >::const_iterator style(styles_.find(next_->id));
			current_.column = next_->region.beginning().column;
			current_.style = (style != styles_.end()) ? style->second : defaultStyle_;
			lastTokenEnd_ = next_->region.beginning();
			next_.reset();
		}
	} else if(lastTokenEnd_ != region_.end()) {
		//
		current_.column = lastTokenEnd_.column;
		current_.style = defaultStyle_;
		lastTokenEnd_ = region_.end();
	}
}


// LexicalPartitionPresentationReconstructor //////////////////////////////////////////////////////

/// @see presentation#PartitionPresentationReconstructor#getPresentation
auto_ptr<StyledTextRunIterator> LexicalPartitionPresentationReconstructor::getPresentation(const Region& region) const /*throw()*/ {
	return auto_ptr<presentation::StyledTextRunIterator>(
		new StyledTextRunIterator(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
}
