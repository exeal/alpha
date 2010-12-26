/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2010
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
 * Constructor.
 * @param sourceIterator The iterator to encapsulate
 * @param end The end character position
 * @throw NullPointerException @a sourceIterator is @c null
 */
StyledTextRunEnumerator::StyledTextRunEnumerator(
		auto_ptr<IStyledTextRunIterator> sourceIterator, length_t end) : iterator_(sourceIterator), end_(end) {
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
 * Returns the character range of the current styled run.
 * @throw NoSuchElementException The enumerator addresses the end
 */
Range<length_t> StyledTextRunEnumerator::currentRange() const {
	if(!current_.first)
		throw NoSuchElementException();
	return Range<length_t>(current_.second.column, next_.second.column);
}

/**
 * Returns the style of the current styled run.
 * @throw NoSuchElementException The enumerator addresses the end
 */
tr1::shared_ptr<const TextRunStyle> StyledTextRunEnumerator::currentStyle() const {
	if(!current_.first)
		throw NoSuchElementException();
	return current_.second.style;
}

/// Returns @c false if the enumerator addresses the end.
bool StyledTextRunEnumerator::hasNext() const /*throw()*/ {
	return current_.first;
}

/**
 * Moves to the next styled run.
 * @throw NoSuchElementException The enumerator addresses the end
 */
void StyledTextRunEnumerator::next() {
	if(!hasNext())
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


// TextLineStyle //////////////////////////////////////////////////////////////////////////////////

/// Default constructor.
TextLineStyle::TextLineStyle() /*throw()*/ :
		readingDirection(INHERIT_READING_DIRECTION), alignment(ALIGN_START) {
}


// Presentation ///////////////////////////////////////////////////////////////////////////////////

tr1::shared_ptr<const TextLineStyle> Presentation::DEFAULT_TEXT_LINE_STYLE(new TextLineStyle());
tr1::shared_ptr<const TextRunStyle> Presentation::DEFAULT_TEXT_RUN_STYLE(new TextRunStyle());

struct Presentation::Hyperlinks {
	length_t lineNumber;
	AutoBuffer<IHyperlink*> hyperlinks;
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

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void Presentation::documentAboutToBeChanged(const Document& document) {
	// TODO: not implemented.
}

/// @see kernel#IDocumentListener#documentChanged
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
const IHyperlink* const* Presentation::getHyperlinks(length_t line, size_t& numberOfHyperlinks) const {
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
	vector<IHyperlink*> temp;
	for(length_t column = 0, eol = document_.lineLength(line); column < eol; ) {
		auto_ptr<IHyperlink> h(hyperlinkDetector_->nextHyperlink(document_, line, Range<length_t>(column, eol)));
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
	newItem->hyperlinks.reset(new IHyperlink*[numberOfHyperlinks = newItem->numberOfHyperlinks = temp.size()]);
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
void Presentation::setHyperlinkDetector(IHyperlinkDetector* newDetector, bool delegateOwnership) /*throw()*/ {
	hyperlinkDetector_.reset(newDetector, delegateOwnership);
	clearHyperlinksCache();
}

/**
 * Sets the line style director.
 * @param newDirector The director. @c null to unregister
 */
void Presentation::setTextLineStyleDirector(tr1::shared_ptr<ITextLineStyleDirector> newDirector) /*throw()*/ {
	textLineStyleDirector_ = newDirector;
}

/**
 * Sets the text run style director.
 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
 * @param newDirector The director. @c null to unregister
 */
void Presentation::setTextRunStyleDirector(tr1::shared_ptr<ITextRunStyleDirector> newDirector) /*throw()*/ {
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
	ITextLineColorDirector::Priority highestPriority = 0, p;
	pair<Color, Color> temp;
	for(list<tr1::shared_ptr<ITextLineColorDirector> >::const_iterator
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
auto_ptr<IStyledTextRunIterator> Presentation::textRunStyles(length_t line) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	return (textRunStyleDirector_.get() != 0) ? textRunStyleDirector_->queryTextRunStyle(line) : auto_ptr<IStyledTextRunIterator>();
}


// SingleStyledPartitionPresentationReconstructor.StyledTextRunIterator ///////////////////////////

class SingleStyledPartitionPresentationReconstructor::StyledTextRunIterator : public IStyledTextRunIterator {
public:
	StyledTextRunIterator(length_t column, tr1::shared_ptr<const TextRunStyle> style) : run_(column, style), done_(false) {}
private:
	// IStyledTextRunIterator
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

/// @see IPartitionPresentationReconstructor#getPresentation
auto_ptr<IStyledTextRunIterator> SingleStyledPartitionPresentationReconstructor::getPresentation(length_t, const Range<length_t>& columnRange) const /*throw()*/ {
	return auto_ptr<IStyledTextRunIterator>(new StyledTextRunIterator(columnRange.beginning(), style_));
}


// PresentationReconstructor.StyledTextRunIterator ////////////////////////////////////////////////

class PresentationReconstructor::StyledTextRunIterator : public IStyledTextRunIterator {
public:
	StyledTextRunIterator(const Presentation& presentation,
		const map<kernel::ContentType, IPartitionPresentationReconstructor*> reconstructors, length_t line);
private:
	void updateSubiterator();
	// IStyledTextRunIterator
	void current(StyledTextRun& run) const;
	bool hasNext() const;
	void next();
private:
	const Presentation& presentation_;
	const map<kernel::ContentType, IPartitionPresentationReconstructor*> reconstructors_;
	const length_t line_;
	DocumentPartition currentPartition_;
	auto_ptr<IStyledTextRunIterator> subiterator_;
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
		IPartitionPresentationReconstructor*> reconstructors, length_t line)
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

/// @see IStyledTextRunIterator#current
void PresentationReconstructor::StyledTextRunIterator::current(StyledTextRun& run) const {
	if(subiterator_.get() != 0)
		subiterator_->current(run);
	else if(hasNext())
		run = current_;
	throw IllegalStateException("the iterator addresses the end.");
}

/// @see IStyledTextRunIterator#hasNext
bool PresentationReconstructor::StyledTextRunIterator::hasNext() const {
	return !currentPartition_.region.isEmpty();
}

/// @see IStyledTextRunIterator#next
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
	map<ContentType, IPartitionPresentationReconstructor*>::const_iterator r(reconstructors_.find(currentPartition_.contentType));
	subiterator_ = (r != reconstructors_.end()) ? r->second->getPresentation(currentPartition_.region) : auto_ptr<IStyledTextRunIterator>();
	if(subiterator_.get() == 0)
		current_ = StyledTextRun(currentPartition_.region.beginning().column, presentation_.defaultTextRunStyle());
}


// PresentationReconstructor //////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation The presentation
 */
PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
	presentation_.setTextRunStyleDirector(tr1::shared_ptr<ITextRunStyleDirector>(this));	// TODO: danger call (may delete this).
}

/// Destructor.
PresentationReconstructor::~PresentationReconstructor() /*throw()*/ {
//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<ILineStyleDirector>(0));
	for(map<ContentType, IPartitionPresentationReconstructor*>::iterator i(reconstructors_.begin()); i != reconstructors_.end(); ++i)
		delete i->second;
}

/// @see ILineStyleDirector#queryTextRunStyle
auto_ptr<IStyledTextRunIterator> PresentationReconstructor::queryTextRunStyle(length_t line) const {
	return auto_ptr<IStyledTextRunIterator>(new StyledTextRunIterator(presentation_, reconstructors_, line));
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
		ContentType contentType, auto_ptr<IPartitionPresentationReconstructor> reconstructor) {
	if(reconstructor.get() == 0)
		throw NullPointerException("reconstructor");
	const map<ContentType, IPartitionPresentationReconstructor*>::iterator old(reconstructors_.find(contentType));
	if(old != reconstructors_.end()) {
		delete old->second;
		reconstructors_.erase(old);
	}
	reconstructors_.insert(make_pair(contentType, reconstructor.release()));
}


// hyperlink.URIHyperlinkDetector /////////////////////////////////////////////////////////////////

namespace {
	class URIHyperlink : public IHyperlink {
	public:
		explicit URIHyperlink(const Range<length_t>& region, const String& uri) /*throw()*/ : IHyperlink(region), uri_(uri) {}
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

/// @see IHyperlinkDetector#nextHyperlink
auto_ptr<IHyperlink> URIHyperlinkDetector::nextHyperlink(
		const Document& document, length_t line, const Range<length_t>& range) const /*throw()*/ {
	// TODO: ??? range is not used???
	const String& s = document.line(line);
	const Char* p = s.data();
	Range<const Char*> result;
	if(uriDetector_->search(p, p + s.length(), result))
		return auto_ptr<IHyperlink>(new URIHyperlink(
			Range<length_t>(result.beginning() - p, result.end() - p), String(result.beginning(), result.end())));
	else
		return auto_ptr<IHyperlink>(0);
}


// hyperlink.CompositeHyperlinkDetector ///////////////////////////////////////////////////////////

/// Destructor.
CompositeHyperlinkDetector::~CompositeHyperlinkDetector() /*throw()*/ {
	for(map<ContentType, IHyperlinkDetector*>::iterator i(composites_.begin()), e(composites_.end()); i != e; ++i)
		delete i->second;
}

/// @see IHyperlinkDetector#nextHyperlink
auto_ptr<IHyperlink> CompositeHyperlinkDetector::nextHyperlink(
		const Document& document, length_t line, const Range<length_t>& range) const /*throw()*/ {
	const DocumentPartitioner& partitioner = document.partitioner();
	DocumentPartition partition;
	for(Position p(line, range.beginning()), e(line, range.end()); p < e;) {
		partitioner.partition(p, partition);
		assert(partition.region.includes(p));
		map<ContentType, IHyperlinkDetector*>::const_iterator detector(composites_.find(partition.contentType));
		if(detector != composites_.end()) {
			auto_ptr<IHyperlink> found = detector->second->nextHyperlink(
				document, line, Range<length_t>(p.column, min(partition.region.end(), e).column));
			if(found.get() != 0)
				return found;
		}
		p = partition.region.end();
	}
	return auto_ptr<IHyperlink>(0);
}

/**
 * Sets the hyperlink detector for the specified content type.
 * @param contentType The content type. if a detector for this content type was already be set, the
 *                    old will be deleted
 * @param detector The hyperlink detector to set. Can't be @c null. The ownership will be
 *                 transferred to the callee
 * @throw NullPointerException @a detector is @c null
 */
void CompositeHyperlinkDetector::setDetector(ContentType contentType, auto_ptr<IHyperlinkDetector> detector) {
	if(detector.get() == 0)
		throw NullPointerException("detector");
	map<ContentType, IHyperlinkDetector*>::iterator old(composites_.find(contentType));
	if(old != composites_.end()) {
		composites_.erase(old);
		delete old->second;
	}
	composites_.insert(make_pair(contentType, detector.release()));
}


// LexicalPartitionPresentationReconstructor.StyledTextRunIterator ////////////////////////////////

/// @internal
class LexicalPartitionPresentationReconstructor::StyledTextRunIterator : public IStyledTextRunIterator {
public:
	StyledTextRunIterator(const Document& document, ITokenScanner& tokenScanner,
		const map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >& styles,
		tr1::shared_ptr<const TextRunStyle> defaultStyle, const Region& region);
private:
	void nextRun();
	// IStyledTextRunIterator
	void current(StyledTextRun& run) const;
	bool hasNext() const;
	void next();
private:
//	const LexicalPartitionPresentationReconstructor& reconstructor_;
	ITokenScanner& tokenScanner_;
	const map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >& styles_;
	tr1::shared_ptr<const TextRunStyle> defaultStyle_;
	Region region_;
	StyledTextRun current_;
	auto_ptr<Token> next_;
	Position lastTokenEnd_;
};

LexicalPartitionPresentationReconstructor::StyledTextRunIterator::StyledTextRunIterator(
		const Document& document, ITokenScanner& tokenScanner,
		const map<Token::Identifier, tr1::shared_ptr<const TextRunStyle> >& styles,
		tr1::shared_ptr<const TextRunStyle> defaultStyle, const Region& region) :
		tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), lastTokenEnd_(region.beginning()) {
	tokenScanner_.parse(document, region);
	nextRun();
}

/// @see IStyledTextRunIterator#current
void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::current(StyledTextRun& run) const {
	if(!hasNext())
		throw IllegalStateException("the iterator addresses the end.");
	run = current_;
}

/// @see IStyledTextRunIterator#hasNext
bool LexicalPartitionPresentationReconstructor::StyledTextRunIterator::hasNext() const {
	return lastTokenEnd_.column != region_.end().column;
}

/// @see IStyledTextRunIterator#next
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

/// @see presentation#IPartitionPresentationReconstructor#getPresentation
auto_ptr<IStyledTextRunIterator> LexicalPartitionPresentationReconstructor::getPresentation(const Region& region) const /*throw()*/ {
	return auto_ptr<IStyledTextRunIterator>(new StyledTextRunIterator(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
}
