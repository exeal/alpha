/**
 * @file abstract-point.hpp
 * @author exeal
 * @date 2003-2015 Was point.hpp
 * @date 2016-05-15 Separated from point.hpp.
 */

#ifndef ASCENSION_ABSTRACT_POINT_HPP
#define ASCENSION_ABSTRACT_POINT_HPP
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/signals.hpp>
#include <ascension/direction.hpp>
#include <ascension/kernel/access.hpp>

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
			/**
			 * Adaptation levels.
			 * @see #adaptationLevel, #setAdaptationLevel
			 */
			enum AdaptationLevel {
				/// The point is moved automatically according to the document change.
				ADAPT_TO_DOCUMENT,
				/// @c ADAPT_TO_DOCUMENT and the point is shrunk to the accessible region of the document.
				ADAPT_TO_DOCUMENT_ACCESSIBLE_REGION
			};

			explicit AbstractPoint(Document& document);
			AbstractPoint(const AbstractPoint& other);
			virtual ~AbstractPoint() BOOST_NOEXCEPT;

			/// @name Document
			/// @{
			Document& document();
			const Document& document() const;
			bool isDocumentDisposed() const BOOST_NOEXCEPT;
			/// @}

			/// @name Behaviors
			/// @{
			const boost::optional<AdaptationLevel>& adaptationLevel() const BOOST_NOEXCEPT;
			Direction gravity() const BOOST_NOEXCEPT;
			AbstractPoint& setAdaptationLevel(const boost::optional<AdaptationLevel>& level);
			AbstractPoint& setGravity(Direction gravity);
			/// @}

			/// @name Signal
			/// @{
			typedef boost::signals2::signal<void(const AbstractPoint*)> DestructionSignal;
			SignalConnector<DestructionSignal> destructionSignal() BOOST_NOEXCEPT;
			/// @}

		private:
			virtual void adaptationLevelChanged() BOOST_NOEXCEPT;
			virtual void contentReset() = 0;
			virtual void documentAboutToBeChanged(const DocumentChange& change) = 0;
			virtual void documentChanged(const DocumentChange& change) = 0;
			void documentDisposed() BOOST_NOEXCEPT;
			friend class Document;
		private:
			Document* document_;	// weak reference
			boost::optional<AdaptationLevel> adaptationLevel_;
			Direction gravity_;
			DestructionSignal destructionSignal_;
		};

		/// Returns @c true if the point is adapting to the document change.
		inline const boost::optional<AbstractPoint::AdaptationLevel>& AbstractPoint::adaptationLevel() const BOOST_NOEXCEPT {
			return adaptationLevel_;
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
		 * Returns the gravity.
		 * @see #setGravity
		 */
		inline Direction AbstractPoint::gravity() const BOOST_NOEXCEPT {
			return gravity_;
		}

		/**
		 * Returns @c true if the document is already disposed.
		 * @see #DestructionSignal
		 */
		inline bool AbstractPoint::isDocumentDisposed() const BOOST_NOEXCEPT {
			return document_ == nullptr;
		}
	}
}

#endif // !ASCENSION_ABSTRACT_POINT_HPP
