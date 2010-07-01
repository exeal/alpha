/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2010
 */

#include <ascension/presentation.hpp>
#include <ascension/rules.hpp>
#include <ascension/viewer.hpp>
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::presentation;
using namespace ascension::presentation::hyperlink;
using namespace ascension::rules;
using namespace ascension::viewers;
using namespace std;
using ascension::presentation::Colors;


// Presentation /////////////////////////////////////////////////////////////

tr1::shared_ptr<const LineStyle> Presentation::DEFAULT_LINE_STYLE(new LineStyle());

struct Presentation::Hyperlinks {
	length_t lineNumber;
	manah::AutoBuffer<IHyperlink*> hyperlinks;
	size_t numberOfHyperlinks;
};

/**
 * Constructor.
 * @param document the target document
 */
Presentation::Presentation(Document& document) /*throw()*/ : document_(document) {
	setDefaultLineStyle(tr1::shared_ptr<const LineStyle>());
	document_.addListener(*this);
}

/// Destructor.
Presentation::~Presentation() /*throw()*/ {
	document_.removeListener(*this);
	clearHyperlinksCache();
}

/// @see internal#ITextViewerCollection#addTextViewer
void Presentation::addTextViewer(TextViewer& textViewer) /*throw()*/ {
	textViewers_.insert(&textViewer);
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

/// Returns an iterator addresses the first text viewer.
set<TextViewer*>::iterator Presentation::firstTextViewer() /*throw()*/ {
	return textViewers_.begin();
}

/// Returns an iterator addresses the first text viewer.
set<TextViewer*>::const_iterator Presentation::firstTextViewer() const /*throw()*/ {
	return textViewers_.begin();
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
 * Returns the colors of the specified line.
 * @param line the line
 * @return the colors of the line. unspecified if its members are not valid
 * @throw BadPositionException @a line is outside of the document
 * @see Color#isValid
 */
Colors Presentation::getLineColor(length_t line) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	ILineColorDirector::Priority highestPriority = 0, p;
	Colors result, c;
	for(list<tr1::shared_ptr<ILineColorDirector> >::const_iterator
			i(lineColorDirectors_.begin()), e(lineColorDirectors_.end()); i != e; ++i) {
		p = (*i)->queryLineColor(line, c);
		if(p > highestPriority) {
			highestPriority = p;
			result = c;
		}
	}
	return result;
}

/// Returns an iterator addresses the location succeeding the last text viewer.
set<TextViewer*>::iterator Presentation::lastTextViewer() /*throw()*/ {
	return textViewers_.end();
}

/// Returns an iterator addresses the location succeeding the last text viewer.
set<TextViewer*>::const_iterator Presentation::lastTextViewer() const /*throw()*/ {
	return textViewers_.end();
}

/// @see internal#ITextViewerCollection#removeTextViewer
void Presentation::removeTextViewer(TextViewer& textViewer) /*throw()*/ {
	textViewers_.erase(&textViewer);
}

/**
 * Sets the default line style.
 * @param newStyle the style to set
 * @see #defaultTextLineStyle, #setDefaultTextRunStyle
 */
void Presentation::setDefaultLineStyle(tr1::shared_ptr<const LineStyle> newStyle) {
	defaultLineStyle_ = (newStyle.get() != 0) ? newStyle : DEFAULT_LINE_STYLE;
}

/**
 * Sets the default text run style.
 * @param newStyle the style to set
 * @see #defaultTextRunStyle, #setDefaultTextLineStyle
 */
void Presentation::setDefaultTextRunStyle(tr1::shared_ptr<const RunStyle> newStyle) {
	defaultTextRunStyle_ = newStyle;
	for(set<TextViewer*>::iterator i(textViewers_.begin()), e(textViewers_.end()); i != e; ++i)
		(*i)->textRenderer().updateTextMetrics();
}

/**
 * Sets the hyperlink detector.
 * @param newDirector the director. @c null to unregister
 * @param delegateOwnership set @c true to transfer the ownership of @a newDirector to the callee
 */
void Presentation::setHyperlinkDetector(IHyperlinkDetector* newDetector, bool delegateOwnership) /*throw()*/ {
	hyperlinkDetector_.reset(newDetector, delegateOwnership);
	clearHyperlinksCache();
}

/**
 * Sets the line style director.
 * @param newDirector the director. @c null to unregister
 */
void Presentation::setLineStyleDirector(tr1::shared_ptr<ILineStyleDirector> newDirector) /*throw()*/ {
	lineStyleDirector_ = newDirector;
}

/**
 * Sets the text run style director.
 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
 * @param newDirector the director. @c null to unregister
 */
void Presentation::setTextRunStyleDirector(tr1::shared_ptr<ITextRunStyleDirector> newDirector) /*throw()*/ {
	textRunStyleDirector_ = newDirector;
}

/**
 * Returns the styles of the text runs in the specified line.
 * @param line the line
 * @return an iterator enumerates the styles of the text runs in the line, or @c null if the line
 *         has no styled text runs
 * @throw BadPositionException @a line is outside of the document
 */
auto_ptr<IStyledRunIterator> Presentation::textRunStyles(length_t line) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	return (textRunStyleDirector_.get() != 0) ? textRunStyleDirector_->queryTextRunStyle(line) : auto_ptr<IStyledRunIterator>();
}


// SingleStyledPartitionPresentationReconstructor.StyledRunIterator /////////

class SingleStyledPartitionPresentationReconstructor::StyledRunIterator : public IStyledRunIterator {
public:
	StyledRunIterator(length_t column, tr1::shared_ptr<const RunStyle> style) : run_(column, style), done_(false) {}
private:
	// IStyledRunIterator
	const StyledRun& current() const {if(done_) throw IllegalStateException("the iterator addresses the end."); return run_;}
	bool isDone() const {return done_;}
	void next() {if(done_) throw IllegalStateException("the iterator addresses the end."); done_ = true;}
private:
	const StyledRun run_;
	bool done_;
};


// SingleStyledPartitionPresentationReconstructor ///////////////////////////

/**
 * Constructor.
 * @param style the style
 */
SingleStyledPartitionPresentationReconstructor::SingleStyledPartitionPresentationReconstructor(tr1::shared_ptr<const RunStyle> style) /*throw()*/ : style_(style) {
}

/// @see IPartitionPresentationReconstructor#getPresentation
auto_ptr<IStyledRunIterator> SingleStyledPartitionPresentationReconstructor::getPresentation(length_t, const Range<length_t>& columnRange) const /*throw()*/ {
	return auto_ptr<IStyledRunIterator>(new StyledRunIterator(columnRange.beginning(), style_));
}


// PresentationReconstructor.StyledRunIterator //////////////////////////////

class PresentationReconstructor::StyledRunIterator : public IStyledRunIterator {
public:
	StyledRunIterator(const Presentation& presentation,
		const map<kernel::ContentType, IPartitionPresentationReconstructor*> reconstructors, length_t line);
private:
	void updateSubiterator();
	// IStyledRunIterator
	const StyledRun& current() const;
	bool isDone() const;
	void next();
private:
	const Presentation& presentation_;
	const map<kernel::ContentType, IPartitionPresentationReconstructor*> reconstructors_;
	const length_t line_;
	DocumentPartition currentPartition_;
	auto_ptr<IStyledRunIterator> subiterator_;
	StyledRun current_;
};

/**
 * Constructor.
 */
PresentationReconstructor::StyledRunIterator::StyledRunIterator(
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

/// @see IStyledRunIterator#current
const StyledRun& PresentationReconstructor::StyledRunIterator::current() const {
	if(subiterator_.get() != 0)
		return subiterator_->current();
	else if(!isDone())
		return current_;
	throw IllegalStateException("the iterator addresses the end.");
}

/// @see IStyledRunIterator#isDone
bool PresentationReconstructor::StyledRunIterator::isDone() const {
	return currentPartition_.region.isEmpty();
}

/// @see IStyledRunIterator#next
void PresentationReconstructor::StyledRunIterator::next() {
	if(subiterator_.get() != 0) {
		subiterator_->next();
		if(subiterator_->isDone())
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

inline void PresentationReconstructor::StyledRunIterator::updateSubiterator() {
	map<ContentType, IPartitionPresentationReconstructor*>::const_iterator r(reconstructors_.find(currentPartition_.contentType));
	subiterator_ = (r != reconstructors_.end()) ? r->second->getPresentation(currentPartition_.region) : auto_ptr<IStyledRunIterator>();
	if(subiterator_.get() == 0)
		current_ = StyledRun(currentPartition_.region.beginning().column, presentation_.defaultTextRunStyle());
}


// PresentationReconstructor ////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation the presentation
 */
PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
	presentation_.setTextRunStyleDirector(tr1::shared_ptr<ITextRunStyleDirector>(this));	// TODO: danger call (may delete this).
	presentation_.document().addPartitioningListener(*this);
}

/// Destructor.
PresentationReconstructor::~PresentationReconstructor() /*throw()*/ {
//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<ILineStyleDirector>(0));
	presentation_.document().removePartitioningListener(*this);
	for(map<ContentType, IPartitionPresentationReconstructor*>::iterator i(reconstructors_.begin()); i != reconstructors_.end(); ++i)
		delete i->second;
}

/// @see kernel#IDocumentPartitioningListener#documentPartitioningChanged
void PresentationReconstructor::documentPartitioningChanged(const Region& changedRegion) {
	for(Presentation::TextViewerIterator i(presentation_.firstTextViewer()); i != presentation_.lastTextViewer(); ++i)
		(*i)->textRenderer().invalidate(changedRegion.beginning().line, changedRegion.end().line + 1);
}

/// @see ILineStyleDirector#queryTextRunStyle
auto_ptr<IStyledRunIterator> PresentationReconstructor::queryTextRunStyle(length_t line) const {
	return auto_ptr<IStyledRunIterator>(new StyledRunIterator(presentation_, reconstructors_, line));
}

/**
 * Sets the partition presentation reconstructor for the specified content type.
 * @param contentType the content type. if a reconstructor for this content type was already be
 *                    set, the old will be deleted
 * @param reconstructor the partition presentation reconstructor to set. can't be @c null. the
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


// hyperlink.URIHyperlinkDetector ///////////////////////////////////////////

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
	const String& s = document.line(line);
	const Char* p = s.data();
	pair<const Char*, const Char*> result;
	if(uriDetector_->search(p, p + s.length(), result))
		return auto_ptr<IHyperlink>(new URIHyperlink(
			Range<length_t>(result.first - p, result.second - p), String(result.first, result.second)));
	else
		return auto_ptr<IHyperlink>(0);
}


// hyperlink.CompositeHyperlinkDetector /////////////////////////////////////

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
 * @param contentType the content type. if a detector for this content type was already be set, the
 *                    old will be deleted
 * @param detector the hyperlink detector to set. can't be @c null. the ownership will be
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


// LexicalPartitionPresentationReconstructor.StyledRunIterator //////////////

/// @internal
class LexicalPartitionPresentationReconstructor::StyledRunIterator : public IStyledRunIterator {
public:
	StyledRunIterator(const Document& document, ITokenScanner& tokenScanner,
		const map<Token::Identifier, tr1::shared_ptr<const RunStyle> >& styles,
		tr1::shared_ptr<const RunStyle> defaultStyle, const Region& region);
private:
	void nextRun();
	// IStyledRunIterator
	const StyledRun& current() const;
	bool isDone() const;
	void next();
private:
//	const LexicalPartitionPresentationReconstructor& reconstructor_;
	ITokenScanner& tokenScanner_;
	const map<Token::Identifier, tr1::shared_ptr<const RunStyle> >& styles_;
	tr1::shared_ptr<const RunStyle> defaultStyle_;
	Region region_;
	StyledRun current_;
	auto_ptr<Token> next_;
	Position lastTokenEnd_;
};

LexicalPartitionPresentationReconstructor::StyledRunIterator::StyledRunIterator(
		const Document& document, ITokenScanner& tokenScanner,
		const map<Token::Identifier, tr1::shared_ptr<const RunStyle> >& styles,
		tr1::shared_ptr<const RunStyle> defaultStyle, const Region& region) :
		tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), lastTokenEnd_(region.beginning()) {
	tokenScanner_.parse(document, region);
	nextRun();
}

/// @see IStyledRunIterator#current
const StyledRun& LexicalPartitionPresentationReconstructor::StyledRunIterator::current() const {
	if(isDone())
		throw IllegalStateException("the iterator addresses the end.");
	return current_;
}

/// @see IStyledRunIterator#isDone
bool LexicalPartitionPresentationReconstructor::StyledRunIterator::isDone() const {
	return lastTokenEnd_.column == region_.end().column;
}

/// @see IStyledRunIterator#next
void LexicalPartitionPresentationReconstructor::StyledRunIterator::next() {
	if(isDone())
		throw IllegalStateException("the iterator addresses the end.");
	nextRun();
}

inline void LexicalPartitionPresentationReconstructor::StyledRunIterator::nextRun() {
	if(next_.get() != 0) {
		map<Token::Identifier, tr1::shared_ptr<const RunStyle> >::const_iterator style(styles_.find(next_->id));
		current_.column = next_->region.beginning().column;
		current_.style = (style != styles_.end()) ? style->second : defaultStyle_;
		lastTokenEnd_ = next_->region.end();
		next_.reset();
	} else if(!tokenScanner_.isDone()) {
		next_ = tokenScanner_.nextToken();
		assert(next_.get() != 0);
		if(next_->region.beginning() != lastTokenEnd_) {
			// 
			current_.column = lastTokenEnd_.column;
			current_.style = defaultStyle_;
			lastTokenEnd_ = next_->region.beginning();
		} else {
			map<Token::Identifier, tr1::shared_ptr<const RunStyle> >::const_iterator style(styles_.find(next_->id));
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


// LexicalPartitionPresentationReconstructor ////////////////////////////////

/// @see presentation#IPartitionPresentationReconstructor#getPresentation
auto_ptr<IStyledRunIterator> LexicalPartitionPresentationReconstructor::getPresentation(const Region& region) const /*throw()*/ {
	return auto_ptr<IStyledRunIterator>(new StyledRunIterator(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
}
