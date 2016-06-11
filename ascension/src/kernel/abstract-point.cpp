/**
 * @file abstract-point.cpp
 * @author exeal
 * @date 2003-2014, 2016 Was point.cpp.
 * @date 2016-05-15 Separated from point.cpp.
 */

#include <ascension/kernel/abstract-point.hpp>
#include <ascension/kernel/document.hpp>


namespace ascension {
	namespace kernel {
		/**
		 * Constructor.
		 * @param document The document to which the point attaches
		 * @post &amp;#document() == &amp;document
		 * @post #adaptsToDocument() == true
		 * @post #gravity() == Direction#FORWARD
		 */
		AbstractPoint::AbstractPoint(Document& document) : document_(&document), adapting_(true), gravity_(Direction::FORWARD) {
			static_cast<detail::PointCollection<AbstractPoint>*>(document_)->addNewPoint(*this);
		}

		/**
		 * Copy-constructor.
		 * @param other The source object
		 * @throw DocumentDisposedException The document to which @a other belongs had been disposed
		 * @post &amp;#document() == &amp;other.document
		 * @post #adaptsToDocument() == other.adaptsToDocument()
		 * @post #gravity() == other.gravity()
		 */
		AbstractPoint::AbstractPoint(const AbstractPoint& other) : document_(other.document_), adapting_(other.adapting_), gravity_(other.gravity_) {
			if(document_ == nullptr)
				throw DocumentDisposedException();
			static_cast<detail::PointCollection<AbstractPoint>*>(document_)->addNewPoint(*this);
		}

		/// Destructor. @c AbstractPoint#DestructionSignal is invoked.
		AbstractPoint::~AbstractPoint() BOOST_NOEXCEPT {
			destructionSignal_(this);
			if(document_ != nullptr)
				static_cast<detail::PointCollection<AbstractPoint>*>(document_)->removePoint(*this);
		}

		/**
		 * @fn ascension::kernel::AbstractPoint::contentReset
		 * @c Document#resetContent method was called.
		 */

		/**
		 * @fn ascension::kernel::AbstractPoint::documentChanged
		 * Called when the document was changed.
		 * @param change The content of the document change
		 */

		/**
		 * Sets the gravity.
		 * @param gravity The new gravity value
		 * @return This object
		 * @post #gravity() == gravity
		 */
		AbstractPoint& AbstractPoint::setGravity(Direction gravity) BOOST_NOEXCEPT {
			if(isDocumentDisposed())
				throw DocumentDisposedException();
			gravity_ = gravity;
			return *this;
		}
	}
}