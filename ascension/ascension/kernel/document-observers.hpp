/**
 * @file document-observers.hpp
 * @author exeal
 * @date 2011-03-30 separated from document.hpp
 * @date 2011-2013
 */

#ifndef ASCENSION_DOCUMENT_OBSERVERS_HPP
#define ASCENSION_DOCUMENT_OBSERVERS_HPP
#include <ascension/corelib/basic-types.hpp>	// Index

namespace ascension {
	namespace kernel {
		class Position;
		class Document;
		class DocumentChange;
		class DocumentPropertyKey;

		/**
		 * A listener notified about the document change.
		 * @see DocumentChange, Document#addListener, Document#addPrenotifiedListener, Document#removeListener,
		 *      Document#removePrenotifiedListener
		 */
		class DocumentListener {
		public:
			/// Destructor.
			virtual ~DocumentListener() BOOST_NOEXCEPT {}
		private:
			/**
			 * The document is about to be changed.
			 * @param document The document
			 * @param change The modification content. Both @c change.erasedRegion() and @c change.insertedRegion() may
			 *               return an empty
			 */
			virtual void documentAboutToBeChanged(const Document& document, const DocumentChange& change) = 0;
			/**
			 * The text was deleted or inserted.
			 * @param document The document
			 * @param change The modification content. Both @c change.erasedRegion() and @c change.insertedRegion() may
			 *               return an empty
			 */
			virtual void documentChanged(const Document& document, const DocumentChange& change) = 0;
			friend class Document;
		};

#if 0
		/**
		 * Interface for objects which are interested in getting informed about changes of a document's compound
		 * change.
		 * @see Document#beginCompoundChange
		 */
		class CompoundChangeListener {
		private:
			/**
			 * The compound change started.
			 * @param document the document
			 */
			virtual void documentCompoundChangeStarted(const Document& document) = 0;
			/**
			 * The compound change stopped.
			 * @param document the document
			 */
			virtual void documentCompoundChangeStopped(const Document& document) = 0;
			friend class Document;
		};
#endif
		/**
		 * Interface for objects which are interested in getting informed about undo/redo operation invocation of
		 * document.
		 * @see Document#beginCompoundChange, Document#undo
		 */
		class DocumentRollbackListener {
		private:
			/**
			 * The undo/redo operation started.
			 * @param document The document
			 */
			virtual void documentUndoSequenceStarted(const Document& document) = 0;
			/**
			 * The undo/redo operation stopped.
			 * @param document The document
			 * @param resultPosition Th preferable position to put the caret
			 */
			virtual void documentUndoSequenceStopped(
				const Document& document, const Position& resultPosition) = 0;
			friend class Document;
		};
	}
}

#endif // !ASCENSION_DOCUMENT_OBSERVERS_HPP
