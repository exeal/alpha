/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2008
 */

#include "presentation.hpp"
#include "rules.hpp"
#include "viewer.hpp"
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::presentation;
using namespace ascension::presentation::hyperlink;
using namespace ascension::rules;
using namespace ascension::viewers;
using namespace std;
using ascension::layout::Colors;


// Presentation /////////////////////////////////////////////////////////////

struct Presentation::Hyperlinks {
	length_t lineNumber;
	manah::AutoBuffer<IHyperlink*> hyperlinks;
	size_t numberOfHyperlinks;
};

/**
 * Constructor.
 * @param document the target document
 */
Presentation::Presentation(Document& document) throw() : document_(document) {
	document_.addListener(*this);
}

/// Destructor.
Presentation::~Presentation() throw() {
	document_.removeListener(*this);
	clearHyperlinksCache();
}

/// @see internal#ITextViewerCollection#addTextViewer
void Presentation::addTextViewer(TextViewer& textViewer) throw() {
	textViewers_.insert(&textViewer);
}

void Presentation::clearHyperlinksCache() throw() {
	for(list<Hyperlinks*>::iterator i(hyperlinks_.begin()), e(hyperlinks_.end()); i != e; ++i)
		delete *i;
	hyperlinks_.clear();
}

/// Returns the document to which the presentation connects.
const Document& Presentation::document() const throw() {
	return document_;
}

/// Returns the document to which the presentation connects.
Document& Presentation::document() throw() {
	return document_;
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
bool Presentation::documentAboutToBeChanged(const Document& document, const DocumentChange& change) {
	// TODO: not implemented.
	return true;
}

/// @see kernel#IDocumentListener#documentChanged
void Presentation::documentChanged(const Document&, const DocumentChange& change) {
	const Range<length_t> lines(change.region().first.line, change.region().second.line);
	for(list<Hyperlinks*>::iterator i(hyperlinks_.begin()), e(hyperlinks_.end()); i != e; ) {
		const length_t line = (*i)->lineNumber;
		if(line == lines.beginning() || (change.isDeletion() && lines.includes(line))) {
			delete *i;
			i = hyperlinks_.erase(i);
			continue;
		} else if(line >= lines.end() && !lines.isEmpty()) {
			if(change.isDeletion())
				(*i)->lineNumber -= lines.end() - lines.beginning();
			else
				(*i)->lineNumber += lines.end() - lines.beginning();
		}
		++i;
	}
}

/// Returns an iterator addresses the first text viewer.
set<TextViewer*>::iterator Presentation::firstTextViewer() throw() {
	return textViewers_.begin();
}

/// Returns an iterator addresses the first text viewer.
set<TextViewer*>::const_iterator Presentation::firstTextViewer() const throw() {
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
		throw BadPositionException();
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
		const Region r(h->region());
		if(r.first.line != line || r.second.line != line || r.beginning().column < column)
			break;
		column = r.end().column;
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
 * @return the colors of the line. unspecified if its members are @c STANDARD_COLOR
 * @throw BadPositionException @a line is outside of the document
 */
Colors Presentation::getLineColor(length_t line) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException();
	ILineColorDirector::Priority highestPriority = 0, p;
	Colors result = Colors::STANDARD, c;
	for(list<ASCENSION_SHARED_POINTER<ILineColorDirector> >::const_iterator
			i(lineColorDirectors_.begin()), e(lineColorDirectors_.end()); i != e; ++i) {
		p = (*i)->queryLineColor(line, c);
		if(p > highestPriority) {
			highestPriority = p;
			result = c;
		}
	}
	return result;
}

/**
 * Returns the style of the specified line.
 * @param line the line
 * @param delegatedOwnership true if the caller must delete the returned value
 * @return the line style or @c LineStyle#NULL_STYLE
 * @throw BadPositionException @a line is outside of the document
 */
const LineStyle& Presentation::getLineStyle(length_t line, bool& delegatedOwnership) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException();
	const LineStyle& styles = (lineStyleDirector_.get() != 0) ?
		lineStyleDirector_->queryLineStyle(line, delegatedOwnership) : LineStyle::NULL_STYLE;
	if(&styles == &LineStyle::NULL_STYLE)
		delegatedOwnership = false;
	return styles;
}

/// Returns an iterator addresses the location succeeding the last text viewer.
set<TextViewer*>::iterator Presentation::lastTextViewer() throw() {
	return textViewers_.end();
}

/// Returns an iterator addresses the location succeeding the last text viewer.
set<TextViewer*>::const_iterator Presentation::lastTextViewer() const throw() {
	return textViewers_.end();
}

/// @see internal#ITextViewerCollection#removeTextViewer
void Presentation::removeTextViewer(TextViewer& textViewer) throw() {
	textViewers_.erase(&textViewer);
}

/**
 * Sets the hyperlink detector.
 * @param newDirector the director. @c null to unregister
 * @param delegateOwnership set true to transfer the ownership of @a newDirector to the callee
 */
void Presentation::setHyperlinkDetector(IHyperlinkDetector* newDetector, bool delegateOwnership) throw() {
	hyperlinkDetector_.reset(newDetector, delegateOwnership);
	clearHyperlinksCache();
}

/**
 * Sets the line style director.
 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
 * @param newDirector the director. @c null to unregister
 * @param delegateOwnership set true to transfer the ownership of @a newDirector to the callee
 */
void Presentation::setLineStyleDirector(ASCENSION_SHARED_POINTER<ILineStyleDirector> newDirector) throw() {
	lineStyleDirector_ = newDirector;
}


// SingleStyledPartitionPresentationReconstructor ///////////////////////////

/**
 * Constructor.
 * @param style the style
 */
SingleStyledPartitionPresentationReconstructor::SingleStyledPartitionPresentationReconstructor(const TextStyle& style) throw() : style_(style) {
}

/// @see IPartitionPresentationReconstructor#getPresentation
auto_ptr<LineStyle> SingleStyledPartitionPresentationReconstructor::getPresentation(const Region& region) const throw() {
	auto_ptr<LineStyle> result(new LineStyle);
	result->array = new StyledText[result->count = 1];
	result->array[0].column = region.beginning().column;
	result->array[0].style = style_;
	return result;
}


// PresentationReconstructor ////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation the presentation
 */
PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<ILineStyleDirector>(this));	// TODO: danger call (may delete this).
	presentation_.document().addPartitioningListener(*this);
}

/// Destructor.
PresentationReconstructor::~PresentationReconstructor() throw() {
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

/// @see ILineStyleDirector#queryLineStyle
const LineStyle& PresentationReconstructor::queryLineStyle(length_t line, bool& delegates) const {
	const length_t lineLength = presentation_.document().lineLength(line);
	if(lineLength == 0) {	// empty line
		delegates = false;
		return LineStyle::NULL_STYLE;
	}

	// get partitions in this line
	vector<DocumentPartition> partitions;
	const DocumentPartitioner& partitioner = presentation_.document().partitioner();
	delegates = true;
	for(length_t column = 0; column < lineLength; ) {
		DocumentPartition temp;
		partitioner.partition(Position(line, column), temp);
		if(!temp.region.isEmpty()) {	// skip an empty partition
			partitions.push_back(temp);
			if(temp.region.end().line != line)
				break;
			column = temp.region.end().column;
		} else
			++column;
	}
	partitions.front().region.first = max(Position(line, 0), partitions.front().region.first);
	partitions.back().region.second = min(Position(line, lineLength), partitions.back().region.second);

	// get styles of each partitions
	manah::AutoBuffer<auto_ptr<LineStyle> > partitionStyles(new auto_ptr<LineStyle>[partitions.size()]);
	for(size_t i = 0; i < partitions.size(); ++i) {
		map<ContentType, IPartitionPresentationReconstructor*>::const_iterator r(reconstructors_.find(partitions[i].contentType));
		if(r != reconstructors_.end())
			partitionStyles[i] = r->second->getPresentation(partitions[i].region);
	}

	// build the result
	auto_ptr<LineStyle> result(new LineStyle);
	result->count = 0;
	for(size_t i = 0; i < partitions.size(); ++i)
		result->count += (partitionStyles[i].get() != 0) ? partitionStyles[i]->count : 0;
	if(result->count == 0) {
		delegates = false;
		return LineStyle::NULL_STYLE;
	}
	result->array = new StyledText[result->count];
	for(size_t i = 0, c = 0; i < partitions.size(); ++i) {
		if(partitionStyles[i].get() != 0) {
			copy(partitionStyles[i]->array, partitionStyles[i]->array + partitionStyles[i]->count, result->array + c);
			c += partitionStyles[i]->count;
			delete[] partitionStyles[i]->array;
		}
	}
	return *result.release();
}

/**
 * Sets the partition presentation reconstructor for the specified content type.
 * @param contentType the content type. if a reconstructor for this content type was already be
 * set, the old will be deleted
 * @param reconstructor the partition presentation reconstructor to set. can't be @c null. the
 * ownership will be transferred to the callee
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


// hyperlink.URLHyperlinkDetector ///////////////////////////////////////////

namespace {
	class URLHyperlink : virtual public IHyperlink {
	public:
		explicit URLHyperlink(const Region& region, const String& uri) throw() : IHyperlink(region), uri_(uri) {}
		String description() const throw() {return uri_ + L"\nCTRL + click to follow the link.";}
		void invoke() throw() {
#ifdef ASCENSION_WINDOWS
			::ShellExecuteW(0, 0, uri_.c_str(), 0, 0, SW_SHOWNORMAL);
#endif
		}
	private:
		const String uri_;
	};
} // namespace @0

/// @see IHyperlinkDetector#nextHyperlink
auto_ptr<IHyperlink> URLHyperlinkDetector::nextHyperlink(
		const Document& document, length_t line, const Range<length_t>& range) const throw() {
	const Char* const s = document.line(line).data();
	for(text::StringCharacterIterator i(s + range.beginning(), s + range.end()); i.hasNext(); i.next()) {
		const Char* const e = URIDetector::eatURL(i.tell(), i.end(), true);
		if(e > i.tell()) {	// found
			const Region r(line, make_pair(i.tell() - i.beginning(), e - i.beginning()));
			return auto_ptr<IHyperlink>(new URLHyperlink(r, String(i.tell(), e)));
		}
	}
	return auto_ptr<IHyperlink>(0);
}


// hyperlink.CompositeHyperlinkDetector /////////////////////////////////////

/// Destructor.
CompositeHyperlinkDetector::~CompositeHyperlinkDetector() throw() {
	for(map<ContentType, IHyperlinkDetector*>::iterator i(composites_.begin()), e(composites_.end()); i != e; ++i)
		delete i->second;
}

/// @see IHyperlinkDetector#nextHyperlink
auto_ptr<IHyperlink> CompositeHyperlinkDetector::nextHyperlink(
		const Document& document, length_t line, const Range<length_t>& range) const throw() {
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
 * old will be deleted
 * @param detector the hyperlink detector to set. can't be @c null. the ownership will be
 * transferred to the callee
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
