/**
 * @file undo.cpp
 * @author exeal
 * @date 2009 separated from document.cpp
 * @date 2010-2016
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/utility.hpp>	// detail.ValueSaver
#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/kernel/point.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <stack>
#include <tuple>
#include <vector>


namespace ascension {
	namespace kernel {
		namespace {
			// MEMO: merge table
			//             post-change
			// pre-change  insert  delete  replace
			// ----------  ------------------------
			// insert      yes     no      no
			// delete      no      yes     no
			// replace     yes     no      no

			class AtomicChange;

			/// @internal An abstract edit operation.
			class UndoableChange {
			public:
				/// @internal Result of @c UndoableChange#perform method.
				struct Result {
					bool completed;				// true if the change was *completely* performed
					std::size_t numberOfRevisions;	// the number of the performed changes
					Position endOfChange;		// the end position of the change
					void reset() BOOST_NOEXCEPT {
						completed = false;
						numberOfRevisions = 0;
//						endOfChange = Position();
					}
				};
			public:
				/// Destructor.
				virtual ~UndoableChange() BOOST_NOEXCEPT {}
				/// Returns @c true if the change can perform.
				virtual bool canPerform(const Document& document) const = 0;
				/// Tries to append @a postChange to this change and returns @c true, or returns @c false.
				virtual bool merge(std::unique_ptr<AtomicChange>& postChange, const Document& document) = 0;
				/// Performs the change. This method may fail.
				virtual void perform(Document& document, Result& result) = 0;
			};

			/// @internal Base interface of @c InsertionChange and @c DeletionChange.
			class AtomicChange : public UndoableChange {
			public:
				virtual ~AtomicChange() BOOST_NOEXCEPT {}
				bool merge(std::unique_ptr<AtomicChange>& postChange, const Document& document) override {
					if(doMerge(postChange, document))
						return ++revisions_, true;
					return false;
				}
				void perform(Document& document, Result& result) override {
					std::tie(result.completed, result.endOfChange) = doPerform(document);
					result.numberOfRevisions = revisions_;
				}
				virtual Position begin() const BOOST_NOEXCEPT = 0;
				virtual boost::optional<Position> end() const BOOST_NOEXCEPT = 0;
				virtual const String* text() const BOOST_NOEXCEPT = 0;
			protected:
				AtomicChange() : revisions_(1) {}
				virtual bool doMerge(std::unique_ptr<AtomicChange>& postChange, const Document& document) = 0;
				virtual std::pair<bool, Position> doPerform(Document& document) = 0;
			private:
				std::size_t revisions_;
			};

			/// @internal An atomic insertion change
			class InsertionChange : public AtomicChange, public FastArenaObject<InsertionChange> {
			public:
				InsertionChange(const Position& position, const String& text) : position_(position), text_(text) {}
				bool canPerform(const Document& document) const BOOST_NOEXCEPT override {
					return !document.isNarrowed() || encompasses(document.region(), position_);
				}
			private:
				Position begin() const BOOST_NOEXCEPT override {return position_;}
				bool doMerge(std::unique_ptr<AtomicChange>& postChange, const Document& document) BOOST_NOEXCEPT override;
				std::pair<bool, Position> doPerform(Document& document) override;
				boost::optional<Position> end() const BOOST_NOEXCEPT override {return boost::none;}
				const String* text() const BOOST_NOEXCEPT override {return &text_;}
			private:
				Position position_;
				String text_;
			};

			/// @internal An atomic deletion change.
			class DeletionChange : public AtomicChange, public FastArenaObject<DeletionChange> {
			public:
				explicit DeletionChange(const Region& region) BOOST_NOEXCEPT : region_(region) {}
				bool canPerform(const Document& document) const BOOST_NOEXCEPT override {
					return !document.isNarrowed() || encompasses(document.region(), region_);
				}
			private:
				Position begin() const BOOST_NOEXCEPT override {return *boost::const_begin(region_);}
				bool doMerge(std::unique_ptr<AtomicChange>& postChange, const Document& document) BOOST_NOEXCEPT override;
				std::pair<bool, Position> doPerform(Document& document) override;
				boost::optional<Position> end() const BOOST_NOEXCEPT override {return *boost::const_end(region_);}
				const String* text() const BOOST_NOEXCEPT override {return nullptr;}
			private:
				Region region_;
			};

			/// @internal An atomic replacement change.
			class ReplacementChange : public AtomicChange, public FastArenaObject<ReplacementChange> {
			public:
				explicit ReplacementChange(const Region& region, const String& text) : region_(region), text_(text) {}
				bool canPerform(const Document& document) const BOOST_NOEXCEPT override {
					return !document.isNarrowed() || encompasses(document.region(), region_);
				}
			private:
				Position begin() const BOOST_NOEXCEPT override {return *boost::const_begin(region_);}
				bool doMerge(std::unique_ptr<AtomicChange>& postChange, const Document& document) BOOST_NOEXCEPT override;
				std::pair<bool, Position> doPerform(Document& document) override;
				boost::optional<Position> end() const BOOST_NOEXCEPT override {return *boost::const_end(region_);}
				const String* text() const BOOST_NOEXCEPT override {return &text_;}
			private:
				Region region_;
				const String text_;
			};

			/// @internal A compound change
			class CompoundChange : public UndoableChange {
			public:
				~CompoundChange() BOOST_NOEXCEPT;
				bool canPerform(const Document& document) const BOOST_NOEXCEPT override {
					return !changes_.empty() && changes_.back()->canPerform(document);
				}
				bool merge(std::unique_ptr<AtomicChange>& postChange, const Document& document) override;
				void perform(Document& document, Result& result) override;
			private:
				std::vector<std::unique_ptr<AtomicChange>> changes_;
			};

			/// @internal Implements @c AtomicChange#doMerge.
			inline bool InsertionChange::doMerge(std::unique_ptr<AtomicChange>& postChange, const Document&) BOOST_NOEXCEPT {
				if(postChange->end() != boost::none)
					return false;
				const auto postPosition(postChange->begin());
				if(line(postPosition) != line(position_) || offsetInLine(postPosition) > offsetInLine(position_))
					return false;
				else if(offsetInLine(postPosition) == offsetInLine(position_))
					text_ += *postChange->text();
				else {
					const String* const postString = postChange->text();
					if((boost::find_first_of(*postString, text::NEWLINE_CHARACTERS) != boost::const_end(*postString))
							|| (offsetInLine(postPosition) + postString->length() != offsetInLine(position_)))
						return false;
					position_ = postPosition;
					text_ = *postString + text_;
				}
				postChange.reset();
				return true;
			}

			/// @internal Implements @c AtomicChange#doPerform.
			inline std::pair<bool, Position> InsertionChange::doPerform(Document& document) {
				try {
					return std::make_pair(true, insert(document, position_, text_));
				} catch(DocumentAccessViolationException&) {
					return std::make_pair(false, position_);	// the position was inaccessible
				}	// std.bad_alloc is ignored...
			}

			/// @internal Implements @c AtomicChange#doMerge.
			inline bool DeletionChange::doMerge(std::unique_ptr<AtomicChange>& postChange, const Document&) BOOST_NOEXCEPT {
				if(postChange->text() != nullptr)
					return false;
				const Region postChangeRegion(postChange->begin(), boost::get(postChange->end()));
				if(boost::size(postChangeRegion.lines()) > 1
//						|| offsetInLine(*boost::const_end(region_)) == 0
						|| *boost::const_begin(postChangeRegion) != *boost::const_end(region_))
					return false;
				else {
					region_ = Region(*boost::const_begin(region_), *boost::const_end(postChangeRegion));
					postChange.reset();
					return true;
				}
			}

			/// @internal Implements @c AtomicChange#doPerform.
			inline std::pair<bool, Position> DeletionChange::doPerform(Document& document) {
				try {
					erase(document, region_);
				} catch(DocumentAccessViolationException&) {
					return std::make_pair(false, *boost::const_end(region_));	// the region was inaccessible
				}	// std.bad_alloc is ignored...
				return std::make_pair(true, *boost::const_begin(region_));
			}

			/// @internal Implements @c AtomicChange#doMerge.
			inline bool ReplacementChange::doMerge(std::unique_ptr<AtomicChange>& postChange, const Document&) BOOST_NOEXCEPT {
				if(postChange->text() != nullptr)
					return false;
				const Region postChangeRegion(postChange->begin(), boost::get(postChange->end()));
				if(boost::size(postChangeRegion.lines()) > 1
//						|| offsetInLine(*boost::const_end(region_)) == 0
						|| *boost::const_begin(postChangeRegion) != *boost::const_end(region_))
					return false;
				region_ = Region(*boost::const_begin(region_), *boost::const_end(postChangeRegion));
				postChange.reset();
				return true;
			}

			/// @internal Implements @c AtomicChange#doPerform.
			inline std::pair<bool, Position> ReplacementChange::doPerform(Document& document) {
				try {
					return std::make_pair(true, document.replace(region_, text_));
				} catch(DocumentAccessViolationException&) {
					return std::make_pair(false, *boost::const_end(region_));	// the region was inaccessible
				}	// std.bad_alloc is ignored...
			}

			CompoundChange::~CompoundChange() BOOST_NOEXCEPT {
//				BOOST_FOREACH(AtomicChange* change, changes_)
//					delete change;
			}

			/// @internal Implements @c UndoableChange#merge.
			inline bool CompoundChange::merge(std::unique_ptr<AtomicChange>& postChange, const Document& document) {
				if(changes_.empty() || !changes_.back()->merge(postChange, document))
					changes_.push_back(std::move(postChange));
				return true;
			}

			/// @internal Implements @c UndoableChange#perform.
			void CompoundChange::perform(Document& document, Result& result) {
				assert(!changes_.empty());
				result.reset();
				Result delta;
				const auto e(std::begin(changes_));
				for(auto i(std::prev(std::end(changes_))); ; --i) {
					(*i)->perform(document, delta);
					result.numberOfRevisions += delta.numberOfRevisions;
					if(!delta.completed) {
						if(i != --std::end(changes_))
							// partially completed
							changes_.erase(++i, std::end(changes_));
						result.endOfChange = delta.endOfChange;
						break;
					} else if(i == e) {	// completed
						result.completed = true;
						result.endOfChange = delta.endOfChange;
						break;
					}
				}
			}
		} // namespace @0


		// Document.UndoManager ///////////////////////////////////////////////////////////////////////////////////////

		/// @internal Manages undo/redo of the document.
		class Document::UndoManager : private boost::noncopyable {
		public:
			// constructors
			explicit UndoManager(Document& document) BOOST_NOEXCEPT;
			virtual ~UndoManager() BOOST_NOEXCEPT {clear();}
			// attributes
			std::size_t numberOfRedoableChanges() const BOOST_NOEXCEPT {
				return redoableChanges_.size();
			}
			std::size_t numberOfUndoableChanges() const BOOST_NOEXCEPT {
				return undoableChanges_.size() + ((pendingAtomicChange_.get() != nullptr) ? 1 : 0);
			}
			bool isStackingCompoundOperation() const BOOST_NOEXCEPT {return compoundChangeDepth_ > 0;}
			// rollbacks
			void redo(UndoableChange::Result& result);
			void undo(UndoableChange::Result& result);
			// recordings
			void addUndoableChange(std::unique_ptr<AtomicChange> c);
			void beginCompoundChange() BOOST_NOEXCEPT;
			void clear() BOOST_NOEXCEPT;
			void endCompoundChange() BOOST_NOEXCEPT;
			void insertBoundary() BOOST_NOEXCEPT;
		private:
			void commitPendingChange();
			Document& document_;
			std::stack<std::unique_ptr<UndoableChange>> undoableChanges_, redoableChanges_;
			std::unique_ptr<AtomicChange> pendingAtomicChange_;	// automatic composition is applied to only this
			std::size_t compoundChangeDepth_;
			bool rollbacking_;
			std::unique_ptr<CompoundChange> rollbackingChange_;
			CompoundChange* currentCompoundChange_;
		};

		// constructor takes the target document
		Document::UndoManager::UndoManager(Document& document) BOOST_NOEXCEPT : document_(document),
				compoundChangeDepth_(0), rollbacking_(false), rollbackingChange_(nullptr), currentCompoundChange_(nullptr) {
		}

		// pushes the operation into the undo stack
		void Document::UndoManager::addUndoableChange(std::unique_ptr<AtomicChange> c) {
			if(!rollbacking_) {
				if(isStackingCompoundOperation()) {
					if(currentCompoundChange_ == nullptr) {
						commitPendingChange();
						std::unique_ptr<CompoundChange> newCompound(new CompoundChange());
						currentCompoundChange_ = newCompound.get();
						undoableChanges_.push(std::move(newCompound));
					}
					currentCompoundChange_->merge(c, document_);	// CompoundChange.merge always returns true
				} else {
					bool merged = false;
					if(pendingAtomicChange_.get() != nullptr) {
						merged = pendingAtomicChange_->merge(c, document_);
						if(!merged)
							commitPendingChange();
					}
					if(!merged) {
						assert(pendingAtomicChange_.get() == nullptr);
						pendingAtomicChange_ = std::move(c);
					}
				}

				// make the redo stack empty
				while(!redoableChanges_.empty())
					redoableChanges_.pop();
			} else {
				// delay pushing to the stack when rollbacking
				if(rollbackingChange_.get() == nullptr)
					rollbackingChange_.reset(new CompoundChange());
				rollbackingChange_->merge(c, document_);	// CompoundChange.merge always returns true
			}
		}

		inline void Document::UndoManager::beginCompoundChange() BOOST_NOEXCEPT {
			if(compoundChangeDepth_ == 0)
				insertBoundary();
			++compoundChangeDepth_;
		}

		// clears the stacks
		inline void Document::UndoManager::clear() BOOST_NOEXCEPT {
			while(!undoableChanges_.empty())
				undoableChanges_.pop();
			while(!redoableChanges_.empty())
				redoableChanges_.pop();
			pendingAtomicChange_.reset();
			compoundChangeDepth_ = 0;
			currentCompoundChange_ = nullptr;
		}

		// commits the pending undoable change
		inline void Document::UndoManager::commitPendingChange() {
			if(pendingAtomicChange_.get() != nullptr) {
				if(currentCompoundChange_ == nullptr || !currentCompoundChange_->merge(pendingAtomicChange_, document_)) {
					undoableChanges_.push(std::move(pendingAtomicChange_));
					currentCompoundChange_ = nullptr;
				}
				assert(pendingAtomicChange_.get() == nullptr);
			}
		}

		// ends the compound change
		inline void Document::UndoManager::endCompoundChange() BOOST_NOEXCEPT {
			if(compoundChangeDepth_ == 0)
				// this does not throw IllegalStateException even if the internal counter is zero, because undo()
				// and redo() reset the counter to zero.
				//		throw IllegalStateException("there is no compound change in this document.");
				return;
			if(--compoundChangeDepth_ == 0) {
				assert(pendingAtomicChange_.get() == nullptr);
				currentCompoundChange_ = nullptr;
			}
		}

		// stops the current compound chaining
		inline void Document::UndoManager::insertBoundary() BOOST_NOEXCEPT {
			if(!isStackingCompoundOperation())
				commitPendingChange();
		}

		// redoes one change
		void Document::UndoManager::redo(UndoableChange::Result& result) {
			commitPendingChange();
			if(redoableChanges_.empty()) {
				result.reset();
				return;
			}
			rollbacking_ = true;
			redoableChanges_.top()->perform(document_, result);
			if(result.completed)
				redoableChanges_.pop();
			if(rollbackingChange_ != nullptr)
				undoableChanges_.push(std::move(rollbackingChange_));	// move the rollbcked change(s) into the undo stack
			rollbackingChange_.reset();
			currentCompoundChange_ = nullptr;
			rollbacking_ = false;
			compoundChangeDepth_ = 0;
		}

		// undoes one change
		void Document::UndoManager::undo(UndoableChange::Result& result) {
			commitPendingChange();
			if(undoableChanges_.empty()) {
				result.reset();
				return;
			}
			rollbacking_ = true;
			undoableChanges_.top()->perform(document_, result);
			if(result.completed)
				undoableChanges_.pop();
			if(rollbackingChange_ != nullptr)
				redoableChanges_.push(std::move(rollbackingChange_));	// move the rollbacked change(s) into the redo stack
			rollbackingChange_.reset();
			currentCompoundChange_ = nullptr;
			rollbacking_ = false;
			compoundChangeDepth_ = 0;
		}


		// Document ///////////////////////////////////////////////////////////////////////////////////////////////////

		/// Constructor.
		Document::Document() : session_(nullptr), partitioner_(),
				contentTypeInformationProvider_(new DefaultContentTypeInformationProvider),
				readOnly_(false), length_(0), revisionNumber_(0), lastUnmodifiedRevisionNumber_(0),
				onceUndoBufferCleared_(false), recordingChanges_(true), changing_(false), rollbacking_(false)/*, locker_(nullptr)*/ {
			bookmarker_.reset(new Bookmarker(*this));
			undoManager_.reset(new UndoManager(*this));
			resetContent();
		}

		/// Destructor.
		Document::~Document() {
			BOOST_FOREACH(AbstractPoint* p, points_)
				p->documentDisposed();
			accessibleRegion_.reset();
			bookmarker_.reset();	// Bookmarker.~Bookmarker() calls Document...
			for(std::size_t i = 0, c = lines_.size(); i < c; ++i)
				delete lines_[i];
		}

		/**
		 * Starts the compound change.
		 * @throw ReadOnlyDocumentException The document is read only
		 * @see #endCompoundChange, #isCompoundChanging
		 */
		void Document::beginCompoundChange() {
			if(isReadOnly())
				throw ReadOnlyDocumentException();
//			const bool init = !undoManager_->isStackingCompoundOperation();
			undoManager_->beginCompoundChange();
//			if(init)
//				compoundChangeListeners_.notify<const Document&>(&ICompoundChangeListener::documentCompoundChangeStarted, *this);
		}

		/// Clears the undo/redo stacks and deletes the history.
		void Document::clearUndoBuffer() BOOST_NOEXCEPT {
			undoManager_->clear();
			onceUndoBufferCleared_ = true;
		}

		/**
		 * Ends the active compound change.
		 * @throw IllegalStateException There is no compound change in this document
		 * @see #beginCompoundChange, #isCompoundChanging
		 */
		void Document::endCompoundChange() {
			undoManager_->endCompoundChange();	// this may throw IllegalStateException (this is doubt now. see UndoManager)
//			if(!undoManager_->isStackingCompoundOperation())
//				compoundChangeListeners_.notify<const Document&>(&ICompoundChangeListener::documentCompoundChangeStopped, *this);
		}

		/**
		 * Marks a boundary between units of undo.
		 * An undo call will stop at this point. However, see the documentation of @c Document. This method does not
		 * throw @c DocumentCantChangeException.
		 * @throw ReadOnlyDocumentException The document is read only
		 * @throw IllegalStateException The method was called in @c{IDocumentListener}s' notification
		 */
		void Document::insertUndoBoundary() {
			if(changing_)
				throw IllegalStateException("called in IDocumentListeners' notification.");
			else if(isReadOnly())
				throw ReadOnlyDocumentException();
			undoManager_->insertBoundary();
		}

		/**
		 * Returns true if the document is compound changing.
		 * @see #beginCompoundChange, #endCompoundChange
		 */
		bool Document::isCompoundChanging() const BOOST_NOEXCEPT {
			return undoManager_->isStackingCompoundOperation();
		}

		/// Returns the number of undoable changes.
		std::size_t Document::numberOfUndoableChanges() const BOOST_NOEXCEPT {
			return undoManager_->numberOfUndoableChanges();
		}

		/// Returns the number of redoable changes.
		std::size_t Document::numberOfRedoableChanges() const BOOST_NOEXCEPT {
			return undoManager_->numberOfRedoableChanges();
		}

		/**
		 * Sets whether the document records or not the changes for undo/redo. Recording in a newly created document is
		 * enabled to start with.
		 * @param record If set to @c true, this method enables the recording and subsequent changes can be undone. If
		 *               set to @c false, discards the undo/redo information and disables the recording
		 * @see #isRecordingChanges, #undo, #redo
		 */
		void Document::recordChanges(bool record) BOOST_NOEXCEPT {
			if(!(recordingChanges_ = record))
				clearUndoBuffer();
		}

		namespace {
			class FirstChangeHolder {
			public:
				FirstChangeHolder(const Document& document, std::weak_ptr<DocumentInput> input,
						void(DocumentInput::*post)(const Document&)) : document_(document), input_(input), post_(post) {
					assert(post_ != nullptr);
				}
				~FirstChangeHolder() {
					if(std::shared_ptr<DocumentInput> input = input_.lock())
						(input.get()->*post_)(document_);
				}
			private:
				const Document& document_;
				std::weak_ptr<DocumentInput> input_;
				void(DocumentInput::*post_)(const Document&);
			};
		}

#define ASCENSION_PREPARE_FIRST_CHANGE(skip)								\
	const bool firstChange = !(skip) && !isModified() && !input_.expired();	\
	if(firstChange) {														\
		if(const std::shared_ptr<DocumentInput> p = input_.lock()) {		\
			if(!p->isChangeable(*this))										\
				throw DocumentInput::ChangeRejectedException();				\
		}																	\
	}																		\
	FirstChangeHolder fch(*this, firstChange ? input_ : std::weak_ptr<DocumentInput>(), &DocumentInput::postFirstDocumentChange)

		/**
		 * Performs the redo. Does nothing if the target region is inaccessible.
		 * @param n The repeat count
		 * @return @c false if the redo was not completely performed
		 * @throw ReadOnlyDocumentException The document is read only
		 * @throw IDocumentInput#ChangeRejectedException The change was rejected
		 * @throw std#invalid_argument @a n &gt; #numberOfRedoableChanges()
		 * @see undo
		 */
		bool Document::redo(std::size_t n /* = 1 */) {
			if(n == 0)
				return true;
			else if(isReadOnly())
				throw ReadOnlyDocumentException();
			else if(n > numberOfRedoableChanges())
				throw std::invalid_argument("n");
			ASCENSION_PREPARE_FIRST_CHANGE(false);

			const bool modified = isModified();
			UndoableChange::Result result;
			result.completed = true;
			rollbackListeners_.notify<const Document&>(&DocumentRollbackListener::documentUndoSequenceStarted, *this);

			for(; n > 0 && result.completed; --n) {
				beginCompoundChange();
				rollbacking_ = true;
				undoManager_->redo(result);	// this shouldn't throw
				rollbacking_ = false;
				endCompoundChange();
			}
			assert(n == 0 || !result.completed);

			rollbackListeners_.notify<const Document&, const Position&>(
				&DocumentRollbackListener::documentUndoSequenceStopped, *this, result.endOfChange);
			if(isModified() != modified)
				modificationSignChangedSignal_(*this);
			return result.completed;
		}

#if 0
		/**
		 * Removes the compound change listener.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		void Document::removeCompoundChangeListener(ICompoundChangeListener& listener) {
			compoundChangeListeners_.remove(listener);
		}
#endif

		/**
		 * Substitutes the given text for the specified region in the document.
		 * This method sets the modification flag and calls the listeners'
		 * @c DocumentListener#documentAboutToBeChanged and @c DocumentListener#documentChanged.
		 * @param region The region to erase. If this is empty, no text is erased
		 * @param text The text string for replacement. If this is @c null or empty, no text is inserted
		 * @return The position of the end of the inserted text
		 * @throw ReadOnlyDocumentException The document is read only
		 * @throw BadRegionException @a region intersects with outside of the document
		 * @throw DocumentAccessViolationException @a region intersects the inaccesible region
		 * @throw IllegalStateException The method was called in @c{IDocumentListener}s' notification
		 * @throw IDocumentInput#ChangeRejectedException The input of the document rejected this change
		 * @throw std#bad_alloc The internal memory allocation failed
		 */
		Position Document::replace(const Region& region, const StringPiece& text) {
			if(changing_)
				throw IllegalStateException("called in DocumentListeners' notification.");
			else if(isReadOnly())
				throw ReadOnlyDocumentException();
			else if(kernel::line(*boost::const_end(region)) >= numberOfLines()
					|| kernel::offsetInLine(*boost::const_begin(region)) > lineLength(kernel::line(*boost::const_begin(region)))
					|| kernel::offsetInLine(*boost::const_end(region)) > lineLength(kernel::line(*boost::const_end(region))))
				throw BadRegionException(region);
			else if(isNarrowed() && !encompasses(accessibleRegion(), region))
				throw DocumentAccessViolationException();
			else if(boost::empty(region) && (text.cbegin() == nullptr || text.empty()))
				return *boost::const_begin(region);	// nothing to do
			ASCENSION_PREPARE_FIRST_CHANGE(rollbacking_);

			// preprocess. these can't throw
			ascension::detail::ValueSaver<bool> writeLock(changing_);
			changing_ = true;

			// change the content
			const Position& beginning = *boost::const_begin(region);
			const Position& end = *boost::const_end(region);
			StringPiece::const_iterator nextNewline((text.cbegin() != nullptr) ? boost::find_first_of(text, text::NEWLINE_CHARACTERS) : text.cend());
			std::basic_stringbuf<Char> erasedString;
			Index erasedStringLength = 0, insertedStringLength = 0;
			Region insertedRegion;
			try {
				// simple cases: both erased region and inserted string are single line
				if(kernel::line(beginning) == kernel::line(end) && (text.cbegin() == nullptr || text.empty())) {	// erase in single line
					insertedRegion = Region::makeEmpty(beginning);
					fireDocumentAboutToBeChanged(DocumentChange(region, insertedRegion));
					Line& line = *lines_[beginning.line];
					erasedString.sputn(line.text().data() + offsetInLine(beginning), static_cast<std::streamsize>(offsetInLine(end) - offsetInLine(beginning)));
					line.text_.erase(offsetInLine(beginning), offsetInLine(end) - offsetInLine(beginning));
					erasedStringLength += offsetInLine(end) - offsetInLine(beginning);
				} else if(boost::empty(region) && nextNewline == text.cend()) {	// insert single line
					insertedRegion = Region::makeSingleLine(kernel::line(beginning), boost::irange(offsetInLine(beginning), offsetInLine(beginning) + text.length()));
					fireDocumentAboutToBeChanged(DocumentChange(region, insertedRegion));
					lines_[kernel::line(beginning)]->text_.insert(offsetInLine(beginning), text.cbegin(), text.length());
					insertedStringLength += text.length();
				} else if(kernel::line(beginning) == kernel::line(end) && nextNewline == text.cend()) {	// replace in single line
					insertedRegion = Region::makeSingleLine(kernel::line(beginning), boost::irange(offsetInLine(beginning), offsetInLine(beginning) + text.length()));
					fireDocumentAboutToBeChanged(DocumentChange(region, insertedRegion));
					Line& line = *lines_[beginning.line];
					erasedString.sputn(line.text().data() + offsetInLine(beginning), static_cast<std::streamsize>(offsetInLine(end) - offsetInLine(beginning)));
					line.text_.replace(offsetInLine(beginning), offsetInLine(end) - offsetInLine(beginning), text.cbegin(), text.length());
					erasedStringLength += offsetInLine(end) - offsetInLine(beginning);
					insertedStringLength += text.length();
				}
				// complex case: erased region and/or inserted string are/is multi-line
				else {
					// 1. save undo information
					if(!boost::empty(region)) {
						for(Position p(beginning); ; ++p.line, p.offsetInLine = 0) {
							const Line& line = *lines_[p.line];
							const bool last = p.line == end.line;
							const Index e = !last ? line.text().length() : offsetInLine(end);
							if(isRecordingChanges()) {
								erasedString.sputn(line.text().data() + offsetInLine(p), static_cast<std::streamsize>(e - offsetInLine(p)));
								if(!last) {
									const String eol(line.newline().asString());
									erasedString.sputn(eol.data(), static_cast<std::streamsize>(eol.length()));
								}
							}
//							erasedStringLength += e - offsetInLine(p);
							if(last)
								break;
						}
					}

					// 2. allocate strings (lines except first) to insert newly. only when inserted string was multiline
					std::vector<Line*> allocatedLines;
					insertedRegion = Region::makeEmpty(beginning);
					if(text.cbegin() != nullptr && nextNewline != text.cend()) {
						try {
							StringPiece::const_iterator p(std::next(nextNewline, text::eatNewline(nextNewline, text.cend())->asString().length()));
							while(true) {
								nextNewline = boost::find_first_of(boost::make_iterator_range(p, text.cend()), text::NEWLINE_CHARACTERS);
								std::unique_ptr<Line> temp(
									(nextNewline != text.cend()) ?
										new Line(revisionNumber_ + 1, String(p, nextNewline), *text::eatNewline(nextNewline, text.cend()))
										: new Line(revisionNumber_ + 1, String(p, nextNewline)));
								allocatedLines.push_back(temp.get());
								temp.release();
								insertedStringLength += allocatedLines.back()->text().length();
								if(nextNewline == text.cend())
									break;
								p = std::next(nextNewline, allocatedLines.back()->newline().asString().length());
							}
							// merge last line
							Line& lastAllocatedLine = *allocatedLines.back();
							insertedRegion = Region(*boost::const_begin(insertedRegion), Position(kernel::line(beginning) + allocatedLines.size(), lastAllocatedLine.text().length()));
							const Line& lastLine = *lines_[kernel::line(end)];
							const auto n = lastLine.text().length() - offsetInLine(end);
							lastAllocatedLine.text_.append(lastLine.text(), offsetInLine(end), n);
							insertedStringLength += n;
							lastAllocatedLine.newline_ = lastLine.newline();
						} catch(...) {
							BOOST_FOREACH(Line* line, allocatedLines)
								delete line;
							throw;
						}
					}
					const StringPiece::const_iterator firstNewline(nextNewline);
					const Index insertedLength = firstNewline - text.cbegin();
					if(allocatedLines.empty()) {
						auto temp(*boost::end(insertedRegion));
						temp.offsetInLine += insertedLength;
						insertedRegion = Region(*boost::const_begin(insertedRegion), temp);
					}
					fireDocumentAboutToBeChanged(DocumentChange(region, insertedRegion));

					try {
						// 3. insert allocated strings
						if(!allocatedLines.empty())
							lines_.insert(std::begin(lines_) + kernel::line(end) + 1, std::begin(allocatedLines), std::end(allocatedLines));

						// 4. replace first line
						Line& firstLine = *lines_[kernel::line(beginning)];
						const Index erasedLength = firstLine.text().length() - offsetInLine(beginning);
						try {
							if(!allocatedLines.empty())
								firstLine.text_.replace(offsetInLine(beginning), erasedLength, text.cbegin(), insertedLength);
							else {
								// join the first line, inserted string and the last line
								String temp(text.cbegin(), insertedLength);
								const Line& lastLine = *lines_[kernel::line(end)];
								temp.append(lastLine.text(), offsetInLine(end), lastLine.text().length() - offsetInLine(end));
								firstLine.text_.replace(offsetInLine(beginning), erasedLength, temp);
							}
						} catch(...) {
							const ascension::detail::GapVector<Line*>::const_iterator b(std::begin(lines_) + kernel::line(end) + 1);
							const ascension::detail::GapVector<Line*>::const_iterator e(b + allocatedLines.size());
							std::for_each(b, e, std::default_delete<Line>());
							lines_.erase(b, e);
							throw;
						}
						firstLine.newline_ = (firstNewline != text.cend()) ?
							*text::eatNewline(firstNewline, text.cend()) : lines_[kernel::line(end)]->newline();
						erasedStringLength += erasedLength;
						insertedStringLength += insertedLength;
					} catch(...) {
						BOOST_FOREACH(Line* line, allocatedLines)
							delete line;
						throw;
					}

					// 5. remove lines to erase
					if(!boost::empty(region)) {
						const auto b(std::begin(lines_) + kernel::line(beginning) + 1), e(std::begin(lines_) + kernel::line(end) + 1);
						std::for_each(b, e, std::default_delete<Line>());
						lines_.erase(b, e);
					}
				}
			} catch(...) {
				// fire event even if change failed
				const auto empty(Region::makeEmpty(beginning));
				fireDocumentChanged(DocumentChange(empty, empty));
				throw;
			}

			if(isRecordingChanges()) {
				std::unique_ptr<AtomicChange> c;
				if(boost::empty(region))
					c.reset(new DeletionChange(insertedRegion));
				else if(text.cbegin() == nullptr || text.empty())
					c.reset(new InsertionChange(beginning, erasedString.str()));
				else
					c.reset(new ReplacementChange(insertedRegion, erasedString.str()));
				undoManager_->addUndoableChange(std::move(c));
			}
			const bool modified = isModified();
			++revisionNumber_;
			length_ += insertedStringLength;
			length_ -= erasedStringLength;

			const DocumentChange change(region, insertedRegion);
			fireDocumentChanged(change);
			if(!rollbacking_ && !modified)
				modificationSignChangedSignal_(*this);

			return *boost::const_end(insertedRegion);
		}

		/**
		 * Performs the undo. Does nothing if the target region is inaccessible.
		 * @param n The repeat count
		 * @return @c false if the undo was not completely performed
		 * @throw ReadOnlyDocumentException The document is read only
		 * @throw IDocumentInput#ChangeRejectedException The change was rejected
		 * @throw std#invalid_argument @a n &gt; #numberOfUndoableChanges()
		 * @see redo
		 */
		bool Document::undo(std::size_t n /* = 1 */) {
			if(n == 0)
				return true;
			else if(isReadOnly())
				throw ReadOnlyDocumentException();
			else if(n > numberOfUndoableChanges())
				throw std::invalid_argument("n");
			ASCENSION_PREPARE_FIRST_CHANGE(false);

			const bool modified = isModified();
			const std::size_t oldRevisionNumber = revisionNumber_;
			UndoableChange::Result result;
			result.completed = true;
			rollbackListeners_.notify<const Document&>(&DocumentRollbackListener::documentUndoSequenceStarted, *this);

			for(; n > 0 && result.completed; --n) {
				beginCompoundChange();
				rollbacking_ = true;
				undoManager_->undo(result);	// this shouldn't throw
				rollbacking_ = false;
				endCompoundChange();
				revisionNumber_ = oldRevisionNumber - result.numberOfRevisions;
			}
			assert(n == 0 || !result.completed);

			rollbackListeners_.notify<const Document&, const Position&>(
				&DocumentRollbackListener::documentUndoSequenceStopped, *this, result.endOfChange);
			if(isModified() != modified)
				modificationSignChangedSignal_(*this);
			return result.completed;
		}
	}
}
