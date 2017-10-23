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
		 * @post #adaptationLevel() == #ADAPT_TO_DOCUMENT
		 * @post #gravity() == Direction#forward()
		 */
		AbstractPoint::AbstractPoint(Document& document) : document_(&document), adaptationLevel_(ADAPT_TO_DOCUMENT), gravity_(Direction::forward()) {
			static_cast<detail::PointCollection<AbstractPoint>*>(document_)->addNewPoint(*this);
		}

		/**
		 * Copy-constructor.
		 * @param other The source object
		 * @throw DocumentDisposedException The document to which @a other belongs had been disposed
		 * @post &amp;#document() == &amp;other.document
		 * @post #adaptationLevel() == other.adaptationLevel()
		 * @post #gravity() == other.gravity()
		 */
		AbstractPoint::AbstractPoint(const AbstractPoint& other) : document_(other.document_), adaptationLevel_(other.adaptationLevel()), gravity_(other.gravity()) {
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
		 * @internal The adaptation level was changed.
		 * @see #setAdaptationLevel
		 */
		void AbstractPoint::adaptationLevelChanged() BOOST_NOEXCEPT {
		}

		/**
		 * @fn ascension::kernel::AbstractPoint::contentReset
		 * @c Document#resetContent method was called.
		 */

		/**
		 * @fn ascension::kernel::AbstractPoint::documentAboutToBeChanged
		 * Called when the content of the document is about to be changed.
		 * @param change The content of the document change
		 */

		/**
		 * @fn ascension::kernel::AbstractPoint::documentChanged
		 * Called when the document was changed.
		 * @param change The content of the document change
		 */

		/**
		 * @internal The document is in destruction.
		 * @post #isDocumentDisposed() == true
		 * @see #DestructionSignal
		 */
		void AbstractPoint::documentDisposed() BOOST_NOEXCEPT {
			document_ = nullptr;
		}

		/**
		 * Sets the adaptation level.
		 * @param level The new adaptation level
		 * @return This object
		 * @post #adaptationLevel() == level
		 * @throw UnknownValueException @a level is unknown
		 * @note This method may invoke @c #adaptationLevelChanged method
		 * @see #adaptationLevel
		 */
		AbstractPoint& AbstractPoint::setAdaptationLevel(const boost::optional<AdaptationLevel>& level) {
			if(level != adaptationLevel_) {
				if(level != boost::none) {
					if(boost::get(level) != ADAPT_TO_DOCUMENT && boost::get(level) != ADAPT_TO_DOCUMENT_ACCESSIBLE_REGION)
						throw UnknownValueException("level");
				}
				adaptationLevel_ = level;
				adaptationLevelChanged();
			}
			return *this;
		}

		/**
		 * Sets the gravity.
		 * @param gravity The new gravity value
		 * @return This object
		 * @post #gravity() == gravity
		 * @see #gravity
		 */
		AbstractPoint& AbstractPoint::setGravity(Direction gravity) {
			if(isDocumentDisposed())
				throw DocumentDisposedException();
			gravity_ = gravity;
			return *this;
		}
	}
}