/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2006
 */

#include "stdafx.h"
#include "presentation.hpp"
#include "viewer.hpp"
using namespace ascension;
using namespace ascension::presentation;
using namespace ascension::rules;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace std;


// Presentation /////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the target document
 */
Presentation::Presentation(Document& document) throw() : document_(document) {
//	document_.addListener(*this);
}

/// @see internal#ITextViewerCollection#addTextViewer
void Presentation::addTextViewer(TextViewer& textViewer) throw() {
	textViewers_.insert(&textViewer);
}

/// @see text#IDocumentListener#documentAboutToBeChanged
void Presentation::documentAboutToBeChanged(const Document& document) {
	// TODO: not implemented.
}

/// @see text#IDocumentListener#documentChanged
void Presentation::documentChanged(const Document& document, const DocumentChange& change) {
	// TODO: not implemented.
}

/// Returns the document to which the presentation connects.
const Document& Presentation::getDocument() const throw() {
	return document_;
}

/// Returns the document to which the presentation connects.
Document& Presentation::getDocument() throw() {
	return document_;
}

/// Returns an iterator addresses the first text viewer.
set<TextViewer*>::iterator Presentation::getFirstTextViewer() throw() {
	return textViewers_.begin();
}

/// Returns an iterator addresses the first text viewer.
set<TextViewer*>::const_iterator Presentation::getFirstTextViewer() const throw() {
	return textViewers_.begin();
}

/// Returns an iterator addresses the location succeeding the last text viewer.
set<TextViewer*>::iterator Presentation::getLastTextViewer() throw() {
	return textViewers_.end();
}

/// Returns an iterator addresses the location succeeding the last text viewer.
set<TextViewer*>::const_iterator Presentation::getLastTextViewer() const throw() {
	return textViewers_.end();
}

/**
 * Returns the colors of the specified line.
 * @param line the line
 * @return the colors of the line. unspecified if its members are @c STANDARD_COLOR
 * @throw BadPositionException @a line is outside of the document
 */
Colors Presentation::getLineColor(length_t line) const {
	if(line >= document_.getNumberOfLines())
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
	if(line >= document_.getNumberOfLines())
		throw BadPositionException();
	const LineStyle& styles = (lineStyleDirector_.get() != 0) ?
		lineStyleDirector_->queryLineStyle(line, delegatedOwnership) : LineStyle::NULL_STYLE;
	if(&styles == &LineStyle::NULL_STYLE)
		delegatedOwnership = false;
	return styles;
}

/// @see internal#ITextViewerCollection#removeTextViewer
void Presentation::removeTextViewer(TextViewer& textViewer) throw() {
	textViewers_.erase(&textViewer);
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
	result->array[0].column = region.getTop().column;
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
	presentation_.getDocument().addPartitioningListener(*this);
}

/// Destructor.
PresentationReconstructor::~PresentationReconstructor() throw() {
//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<ILineStyleDirector>(0));
	presentation_.getDocument().removePartitioningListener(*this);
	for(map<ContentType, IPartitionPresentationReconstructor*>::iterator i(reconstructors_.begin()); i != reconstructors_.end(); ++i)
		delete i->second;
}

/// @see text#IDocumentPartitioningListener#documentPartitioningChanged
void PresentationReconstructor::documentPartitioningChanged(const Region& changedRegion) {
	for(Presentation::TextViewerIterator i(presentation_.getFirstTextViewer()); i != presentation_.getLastTextViewer(); ++i)
		(*i)->getTextRenderer().invalidate(changedRegion.getTop().line, changedRegion.getBottom().line + 1);
}

/// @see ILineStyleDirector#queryLineStyle
const LineStyle& PresentationReconstructor::queryLineStyle(length_t line, bool& delegates) const {
	const length_t lineLength = presentation_.getDocument().getLineLength(line);
	if(lineLength == 0) {	// empty line
		delegates = false;
		return LineStyle::NULL_STYLE;
	}

	// get partitions in this line
	vector<DocumentPartition> partitions;
	const DocumentPartitioner& partitioner = presentation_.getDocument().getPartitioner();
	delegates = true;
	for(length_t column = 0; column < lineLength; ) {
		DocumentPartition temp;
		partitioner.getPartition(Position(line, column), temp);
		if(!temp.region.isEmpty()) {	// skip an empty partition
			partitions.push_back(temp);
			if(temp.region.getBottom().line != line)
				break;
			column = temp.region.getBottom().column;
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
 * @throw std#invalid_argument @a reconstructor is @c null
 */
void PresentationReconstructor::setPartitionReconstructor(
		ContentType contentType, auto_ptr<IPartitionPresentationReconstructor> reconstructor) {
	if(reconstructor.get() == 0)
		throw invalid_argument("the specified reconstructor is null.");
	const map<ContentType, IPartitionPresentationReconstructor*>::iterator old(reconstructors_.find(contentType));
	if(old != reconstructors_.end()) {
		delete old->second;
		reconstructors_.erase(old);
	}
	reconstructors_.insert(make_pair(contentType, reconstructor.release()));
}
