/**
 * @file abstract-point.hpp
 * @author exeal
 * @date 2003-2015 Was point.hpp
 * @date 2016-05-15 Separated from point.hpp.
 */

#ifndef ASCENSION_ABSTRACT_POINT_HPP
#define ASCENSION_ABSTRACT_POINT_HPP
#include <ascension/corelib/signals.hpp>
#include <ascension/direction.hpp>

namespace ascension {
	namespace kernel {
		/**
		 * Tried to use some object but the document used by the object had already been disposed.
		 * @see Point
		 */
		class DocumentDisposedException : public IllegalStateException {
		public:
			DocumentDisposedException();
		};

		class Document;
		class DocumentChange;

		/// Base class of @c Point and @c viewer#VisualPoint.
		class AbstractPoint {
		public:
			explicit AbstractPoint(Document& document);
			AbstractPoint(const AbstractPoint& other);
			virtual ~AbstractPoint() BOOST_NOEXCEPT;

			/// @name Document
			/// @{
			bool adaptsToDocument() const BOOST_NOEXCEPT;
			AbstractPoint& adaptToDocument(bool adapt) BOOST_NOEXCEPT;
			Document& document();
			const Document& document() const;
			bool isDocumentDisposed() const BOOST_NOEXCEPT;
			/// @}

			/// @name Behaviors
			/// @{
			Direction gravity() const BOOST_NOEXCEPT;
			AbstractPoint& setGravity(Direction gravity) BOOST_NOEXCEPT;
			/// @}

			/// @name Signal
			/// @{
			typedef boost::signals2::signal<void(const AbstractPoint*)> DestructionSignal;
			SignalConnector<DestructionSignal> destructionSignal() BOOST_NOEXCEPT;
			/// @}

		private:
			/// Called when @c Document#resetContent of the document was called.
			virtual void contentReset() = 0;
			/**
				* Called when the content of the document was changed.
				* @param change The change
				*/
			virtual void documentChanged(const DocumentChange& change) = 0;
			/// Called when the document is disposed.
			void documentDisposed() BOOST_NOEXCEPT;
			friend class Document;
		private:
			Document* document_;	// weak reference
			bool adapting_;
			Direction gravity_;
			DestructionSignal destructionSignal_;
		};

		/// Returns @c true if the point is adapting to the document change.
		inline bool AbstractPoint::adaptsToDocument() const BOOST_NOEXCEPT {
			return adapting_;
		}

		/**
		 * Adapts the point to the document change.
		 * @param adapt
		 * @post #adaptsToDocument() == adapt
		 */
		inline AbstractPoint& AbstractPoint::adaptToDocument(bool adapt) BOOST_NOEXCEPT {
			adapting_ = adapt;
			return *this;
		}

		/// Returns the @c AbstractPoint#DestructionSignal signal connector.
		inline SignalConnector<AbstractPoint::DestructionSignal> AbstractPoint::destructionSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(destructionSignal_);
		}

		/**
		 * Returns the document.
		 * @throw DocumentDisposedException The document is already disposed
		 */
		inline Document& AbstractPoint::document() {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			return *document_;
		}

		/**
		 * Returns the document.
		 * @throw DocumentDisposedException The document is already disposed
		 */
		inline const Document& AbstractPoint::document() const {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			return *document_;
		}

		/**
		 * @internal The document is in destruction.
		 * @post #isDocumentDisposed() == true
		 */
		inline void AbstractPoint::documentDisposed() BOOST_NOEXCEPT {
			document_ = nullptr;
		}

		/// Returns the gravity.
		inline Direction AbstractPoint::gravity() const BOOST_NOEXCEPT {
			return gravity_;
		}

		/// Returns @c true if the document is already disposed.
		inline bool AbstractPoint::isDocumentDisposed() const BOOST_NOEXCEPT {
			return document_ == nullptr;
		}
	}
}

#endif // !ASCENSION_ABSTRACT_POINT_HPP
