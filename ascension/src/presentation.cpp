/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2012
 */

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


// StyledTextRunEnumerator ////////////////////////////////////////////////////////////////////////

/**
 * Default constructor.
 */
StyledTextRunEnumerator::StyledTextRunEnumerator() : end_(0) {
}

/**
 * Constructor.
 * @param sourceIterator The iterator to encapsulate
 * @param end The end character position
 * @throw NullPointerException @a sourceIterator is @c null
 */
StyledTextRunEnumerator::StyledTextRunEnumerator(
		unique_ptr<StyledTextRunIterator> sourceIterator, Index end) : iterator_(move(sourceIterator)), end_(end) {
	if(iterator_.get() == nullptr)
		throw NullPointerException("sourceIterator");
	if(iterator_->hasNext()) {
		current_ = iterator_->current();
		if(current_->position() < end_) {
			iterator_->next();
			if(iterator_->hasNext()) {
				next_ = iterator_->current();
				if(next_->position() < end_)
					iterator_->next();
				else
					next_ = boost::none;
			}
		} else
			current_ = boost::none;
	}
}

/**
 * Implements @c boost#iterator_facade#dereference.
 * @throw NoSuchElementException The enumerator addresses the end
 */
const StyledTextRunEnumerator::reference StyledTextRunEnumerator::dereference() const {
	if(!current_)
		throw NoSuchElementException();
	return make_pair(makeRange(current_->position(), next_ ? next_->position() : end_), current_->style());
}

/// Implements @c boost#iterator_facade#equal.
bool StyledTextRunEnumerator::equal(const StyledTextRunEnumerator& other) const /*throw()*/ {
	return !current_ && !other.current_;
}

/**
 * Implements @c boost#iterator_facade#increment.
 * @throw NoSuchElementException The enumerator addresses the end
 */
void StyledTextRunEnumerator::increment() {
	if(!current_)
		throw NoSuchElementException();
	current_ = next_;
	if(iterator_->hasNext()) {
		next_ = iterator_->current();
		if(next_->position() >= end_)
			next_ = boost::none;
		else
			iterator_->next();
	} else
		next_ = boost::none;
}


// Presentation ///////////////////////////////////////////////////////////////////////////////////

/// 
TextAnchor presentation::defaultTextAnchor(const Presentation& presentation) {
	const shared_ptr<const TextLineStyle> lineStyle(presentation.globalTextStyle().defaultLineStyle);
	return (lineStyle.get() != nullptr) ? lineStyle->anchor : ASCENSION_DEFAULT_TEXT_ANCHOR;
}

shared_ptr<const TextToplevelStyle> Presentation::DEFAULT_GLOBAL_TEXT_STYLE;

struct Presentation::Hyperlinks {
	Index lineNumber;
	unique_ptr<Hyperlink*[]> hyperlinks;
	size_t numberOfHyperlinks;
};

/**
 * Constructor.
 * @param document The target document
 */
Presentation::Presentation(Document& document) /*throw()*/ : document_(document) {
	if(DEFAULT_GLOBAL_TEXT_STYLE.get() == nullptr) {
		unique_ptr<TextLineStyle> temp1(new TextLineStyle);
		temp1->defaultRunStyle.reset(new TextRunStyle);
		unique_ptr<TextToplevelStyle> temp2(new TextToplevelStyle);
		temp2->defaultLineStyle = move(temp1);
		DEFAULT_GLOBAL_TEXT_STYLE = move(temp2);
	}
	setGlobalTextStyle(shared_ptr<const TextToplevelStyle>());
	document_.addListener(*this);
}

/// Destructor.
Presentation::~Presentation() /*throw()*/ {
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
 * Returns the style of the specified text line.
 * @param[in] line The line number
 * @param[out] style The text line style
 * @return @a style
 */
TextLineStyle& Presentation::textLineStyle(Index line, TextLineStyle& style) const /*throw()*/ {
	shared_ptr<const Inheritable<TextLineStyle>> p;
	if(textLineStyleDirector_.get() != nullptr)
		p = textLineStyleDirector_->queryTextLineStyle(line);
	if(p.get() == nullptr)
		style = *DEFAULT_GLOBAL_TEXT_STYLE->defaultLineStyle;
	else {
		style.anchor = resolveInheritance(p->anchor, DEFAULT_GLOBAL_TEXT_STYLE->defaultLineStyle->anchor);
		style.dominantBaseline = resolveInheritance(
			p->dominantBaseline, DEFAULT_GLOBAL_TEXT_STYLE->defaultLineStyle->dominantBaseline);
		style.lineHeightShiftAdjustment = resolveInheritance(
			p->lineHeightShiftAdjustment, DEFAULT_GLOBAL_TEXT_STYLE->defaultLineStyle->lineHeightShiftAdjustment);
		style.lineStackingStrategy = resolveInheritance(
			p->lineStackingStrategy, DEFAULT_GLOBAL_TEXT_STYLE->defaultLineStyle->lineStackingStrategy);
	}
	return style;
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
 * Sets the global text line style.
 * @param newStyle The style to set
 * @see #globalTextStyle
 */
void Presentation::setGlobalTextStyle(shared_ptr<const TextToplevelStyle> newStyle) {
	const shared_ptr<const TextToplevelStyle> used(globalTextStyle_);
	globalTextStyle_ = (newStyle.get() != nullptr) ? newStyle : DEFAULT_GLOBAL_TEXT_STYLE;
	globalTextStyleListeners_.notify<shared_ptr<const TextToplevelStyle>>(
		&GlobalTextStyleListener::globalTextStyleChanged, used);
}

/**
 * Sets the hyperlink detector.
 * @param newDirector The director. Set @c null to unregister
 */
void Presentation::setHyperlinkDetector(shared_ptr<HyperlinkDetector> newDetector) /*throw()*/ {
	hyperlinkDetector_ = newDetector;
	clearHyperlinksCache();
}

/**
 * Sets the line style director.
 * @param newDirector The director. @c null to unregister
 */
void Presentation::setTextLineStyleDirector(shared_ptr<TextLineStyleDirector> newDirector) /*throw()*/ {
	textLineStyleDirector_ = newDirector;
}

/**
 * Sets the text run style director.
 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
 * @param newDirector The director. @c null to unregister
 */
void Presentation::setTextRunStyleDirector(shared_ptr<TextRunStyleDirector> newDirector) /*throw()*/ {
	textRunStyleDirector_ = newDirector;
}

/**
 * Returns the colors of the specified text line.
 * @param line The line
 * @param[out] foreground The foreground color of the line. Unspecified if an invalid value
 * @param[out] background The background color of the line. Unspecified if an invalid value
 * @throw BadPositionException @a line is outside of the document
 */
void Presentation::textLineColors(Index line, Color& foreground, Color& background) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	TextLineColorDirector::Priority highestPriority = 0, p;
	pair<Color, Color> temp;
	for(list<shared_ptr<TextLineColorDirector>>::const_iterator
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
unique_ptr<StyledTextRunIterator> Presentation::textRunStyles(Index line) const {
	if(line >= document_.numberOfLines())
		throw BadPositionException(Position(line, 0));
	return (textRunStyleDirector_.get() != nullptr) ?
		textRunStyleDirector_->queryTextRunStyle(line) : unique_ptr<StyledTextRunIterator>();
}


// SingleStyledPartitionPresentationReconstructor.Iterator ////////////////////////////////////////

class SingleStyledPartitionPresentationReconstructor::Iterator : public presentation::StyledTextRunIterator {
public:
	Iterator(Index offsetInLine, shared_ptr<const TextRunStyle> style) : run_(offsetInLine, style), done_(false) {}
private:
	// StyledTextRunIterator
	StyledTextRun current() const {
		if(done_)
			throw NoSuchElementException("the iterator addresses the end.");
		return run_;
	}
	bool hasNext() const {
		return !done_;
	}
	void next() {
		if(done_)
			throw NoSuchElementException("the iterator addresses the end.");
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
SingleStyledPartitionPresentationReconstructor::SingleStyledPartitionPresentationReconstructor(shared_ptr<const TextRunStyle> style) /*throw()*/ : style_(style) {
}

/// @see PartitionPresentationReconstructor#getPresentation
unique_ptr<StyledTextRunIterator> SingleStyledPartitionPresentationReconstructor::getPresentation(Index, const Range<Index>& columnRange) const /*throw()*/ {
	return unique_ptr<presentation::StyledTextRunIterator>(new Iterator(columnRange.beginning(), style_));
}


// PresentationReconstructor.Iterator /////////////////////////////////////////////////////////////

class PresentationReconstructor::Iterator : public presentation::StyledTextRunIterator {
public:
	Iterator(const Presentation& presentation,
		const map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors, Index line);
private:
	void updateSubiterator();
	// StyledTextRunIterator
	StyledTextRun current() const;
	bool hasNext() const;
	void next();
private:
	static const shared_ptr<const TextRunStyle> DEFAULT_TEXT_RUN_STYLE;
	const Presentation& presentation_;
	const map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors_;
	const Index line_;
	DocumentPartition currentPartition_;
	unique_ptr<presentation::StyledTextRunIterator> subiterator_;
	pair<Index, shared_ptr<const TextRunStyle>> current_;
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

/// @see StyledTextRunIterator#current
StyledTextRun PresentationReconstructor::Iterator::current() const {
	if(subiterator_.get() != nullptr)
		return subiterator_->current();
	else if(hasNext())
		return StyledTextRun(current_.first, current_.second);
	throw NoSuchElementException("the iterator addresses the end.");
}

/// @see StyledTextRunIterator#hasNext
bool PresentationReconstructor::Iterator::hasNext() const {
	return !currentPartition_.region.isEmpty();
}

/// @see StyledTextRunIterator#next
void PresentationReconstructor::Iterator::next() {
	if(subiterator_.get() != nullptr) {
		subiterator_->next();
		if(!subiterator_->hasNext())
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
		const shared_ptr<const TextLineStyle> lineStyle(presentation_.globalTextStyle().defaultLineStyle);
		assert(lineStyle.get() != nullptr);
		shared_ptr<const TextRunStyle> runStyle(lineStyle->defaultRunStyle);
		if(runStyle.get() == nullptr)
			runStyle = DEFAULT_TEXT_RUN_STYLE;
		current_ = make_pair(currentPartition_.region.beginning().offsetInLine, runStyle);
	}
}


// PresentationReconstructor //////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation The presentation
 */
PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
	presentation_.setTextRunStyleDirector(shared_ptr<TextRunStyleDirector>(this));	// TODO: danger call (may delete this).
}

/// Destructor.
PresentationReconstructor::~PresentationReconstructor() /*throw()*/ {
//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<LineStyleDirector>());
	for(map<ContentType, PartitionPresentationReconstructor*>::iterator i(reconstructors_.begin()); i != reconstructors_.end(); ++i)
		delete i->second;
}

/// @see LineStyleDirector#queryTextRunStyle
unique_ptr<StyledTextRunIterator> PresentationReconstructor::queryTextRunStyle(Index line) const {
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
	StyledTextRun current() const;
	bool hasNext() const;
	void next();
private:
//	const LexicalPartitionPresentationReconstructor& reconstructor_;
	TokenScanner& tokenScanner_;
	const map<Token::Identifier, shared_ptr<const TextRunStyle>>& styles_;
	shared_ptr<const TextRunStyle> defaultStyle_;
	Region region_;
	pair<Index, shared_ptr<const TextRunStyle>> current_;
	unique_ptr<Token> next_;
	Position lastTokenEnd_;
};

LexicalPartitionPresentationReconstructor::StyledTextRunIterator::StyledTextRunIterator(
		const Document& document, TokenScanner& tokenScanner,
		const map<Token::Identifier, shared_ptr<const TextRunStyle>>& styles,
		shared_ptr<const TextRunStyle> defaultStyle, const Region& region) :
		tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), lastTokenEnd_(region.beginning()) {
	tokenScanner_.parse(document, region);
	nextRun();
}

/// @see StyledTextRunIterator#current
StyledTextRun LexicalPartitionPresentationReconstructor::StyledTextRunIterator::current() const {
	if(!hasNext())
		throw NoSuchElementException("the iterator addresses the end.");
	return StyledTextRun(current_.first, current_.second);
}

/// @see StyledTextRunIterator#hasNext
bool LexicalPartitionPresentationReconstructor::StyledTextRunIterator::hasNext() const {
	return lastTokenEnd_.offsetInLine != region_.end().offsetInLine;
}

/// @see StyledTextRunIterator#next
void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::next() {
	if(!hasNext())
		throw NoSuchElementException("the iterator addresses the end.");
	nextRun();
}

inline void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::nextRun() {
	if(next_.get() != nullptr) {
		map<Token::Identifier, shared_ptr<const TextRunStyle>>::const_iterator style(styles_.find(next_->id));
		current_.first = next_->region.beginning().offsetInLine;
		current_.second = (style != styles_.end()) ? style->second : defaultStyle_;
		lastTokenEnd_ = next_->region.end();
		next_.reset();
	} else if(tokenScanner_.hasNext()) {
		next_ = move(tokenScanner_.nextToken());
		assert(next_.get() != nullptr);
		if(next_->region.beginning() != lastTokenEnd_) {
			// 
			current_.first = lastTokenEnd_.offsetInLine;
			current_.second = defaultStyle_;
			lastTokenEnd_ = next_->region.beginning();
		} else {
			map<Token::Identifier, shared_ptr<const TextRunStyle>>::const_iterator style(styles_.find(next_->id));
			current_.first = next_->region.beginning().offsetInLine;
			current_.second = (style != styles_.end()) ? style->second : defaultStyle_;
			lastTokenEnd_ = next_->region.beginning();
			next_.reset();
		}
	} else if(lastTokenEnd_ != region_.end()) {
		//
		current_.first = lastTokenEnd_.offsetInLine;
		current_.second = defaultStyle_;
		lastTokenEnd_ = region_.end();
	}
}


// LexicalPartitionPresentationReconstructor //////////////////////////////////////////////////////

/// @see presentation#PartitionPresentationReconstructor#getPresentation
unique_ptr<StyledTextRunIterator> LexicalPartitionPresentationReconstructor::getPresentation(const Region& region) const /*throw()*/ {
	return unique_ptr<presentation::StyledTextRunIterator>(
		new StyledTextRunIterator(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
}
