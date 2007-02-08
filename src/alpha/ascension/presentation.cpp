/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2006
 */

#include "StdAfx.h"
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
	return (lineColorDirector_.get() != 0) ? lineColorDirector_->queryLineColor(line) : Colors::STANDARD;
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
