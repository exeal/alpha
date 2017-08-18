/**
 * @file visual-point.hpp
 * Defines @c VisualPoint class and related stuffs.
 * @author exeal
 * @date 2003-2008 was point.hpp
 * @date 2008 separated from point.hpp
 * @date 2009-2011 was caret.hpp
 * @date 2011-10-02 separated from caret.hpp
 * @date 2011-2015
 */

#ifndef ASCENSION_VISUAL_POINT_HPP
#define ASCENSION_VISUAL_POINT_HPP
#include <ascension/kernel/point.hpp>
#include <ascension/graphics/font/visual-line.hpp>
#include <ascension/graphics/font/visual-lines-listener.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/viewer/detail/weak-reference-for-points.hpp>
#include <ascension/viewer/visual-destination-proxy.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			class TextLayout;
			class TextViewport;
		}
	}

	namespace viewer {
		class TextArea;
		class TextViewer;
		class VisualPoint;

		kernel::Position insertionPosition(const VisualPoint& p);
		graphics::Point modelToView(const graphics::font::TextViewport& viewport, const VisualPoint& p/*, bool fullSearchBpd*/);
		graphics::Point modelToView(const TextViewer& textViewer, const VisualPoint& p/*, bool fullSearchBpd*/);
		TextHit otherHit(const VisualPoint& p);

		namespace locations {
			/// @addtogroup motions_in_text_area Motions in Text Area
			/// @{
			VisualDestinationProxy nextPage(const VisualPoint& p, Direction direction, Index pages = 1);
			VisualDestinationProxy nextVisualLine(const VisualPoint& p, Direction direction, Index lines = 1);
			/// @}
		}

		// documentation is visual-point.cpp
		class VisualPoint : public kernel::AbstractPoint, public graphics::font::VisualLinesListener, private boost::totally_ordered<VisualPoint> {
		public:
			/**
			 * The @c VisualPoint is not installed by a @c TextArea.
			 * @see VisualPoint#install, TextAreaDisposedException
			 */
			struct NotInstalledException : IllegalStateException {
				NotInstalledException();
			};
			/**
			 * The @c VisualPoint had been installed by the @c TextArea, but the text area has been disposed.
			 * @see kernel#DocumentDisposedException, VisualPoint
			 */
			struct TextAreaDisposedException : IllegalStateException {
				TextAreaDisposedException();
			};

			explicit VisualPoint(kernel::Document& document, const TextHit& position = TextHit::leading(kernel::Position::zero()));
			explicit VisualPoint(TextArea& textArea, const TextHit& position = TextHit::leading(kernel::Position::zero()));
			explicit VisualPoint(const graphics::font::TextHit<kernel::Point>& other);
			VisualPoint(const VisualPoint& other);
			virtual ~VisualPoint() BOOST_NOEXCEPT;
			operator std::pair<const kernel::Document&, kernel::Position>() const;
			operator std::pair<const TextArea&, TextHit>() const;

			/// @name Installation
			/// @{
			virtual void install(TextArea& textArea);
			bool isInstalled() const BOOST_NOEXCEPT;
			virtual void uninstall() BOOST_NOEXCEPT;
			/// @}

			/// @name Text Area
			/// @{
			bool isFullyAvailable() const BOOST_NOEXCEPT;
			bool isTextAreaDisposed() const;
			TextArea& textArea();
			const TextArea& textArea() const;
			/// @}

			/// @name Visual Positions
			/// @{
			BOOST_CONSTEXPR const TextHit& hit() const BOOST_NOEXCEPT;
			Index offsetInVisualLine() const;
			const graphics::font::VisualLine& visualLine() const;
			/// @}

			/// @name Motions
			/// @{
			typedef boost::signals2::signal<void(const VisualPoint&, const TextHit&)> MotionSignal;
			SignalConnector<MotionSignal> motionSignal() BOOST_NOEXCEPT;
			VisualPoint& moveTo(const TextHit& to);
			void moveTo(const VisualDestinationProxy& to);
			/// @}

		protected:
			virtual void aboutToMove(TextHit& to);
			virtual void moved(const TextHit& from) BOOST_NOEXCEPT;
			// kernel.AbstractPoint
			virtual void contentReset() override;
			virtual void documentChanged(const kernel::DocumentChange& change) override;
		private:
			void buildVisualLineCaches();
			void rememberPositionInVisualLine();
			void throwIfNotFullyAvailable() const;
			void updateLineNumberCaches();
			// layout.VisualLinesListener
			void visualLinesDeleted(const boost::integer_range<Index>& lines,
				Index sublines, bool longestLineChanged) BOOST_NOEXCEPT override;
			void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT override;
			void visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT override;

		private:
			std::shared_ptr<const detail::WeakReferenceForPoints<TextArea>::Proxy> textAreaProxy_;
			TextHit hit_;
			MotionSignal motionSignal_;
			boost::optional<graphics::Scalar> positionInVisualLine_;	// see rememberPositionInVisualLine
			bool crossingLines_;	// true only when the point is moving across the different lines
			boost::optional<graphics::font::VisualLine> lineNumberCaches_;	// caches
			friend class TextArea;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			friend VisualDestinationProxy locations::backwardVisualLine(const VisualPoint& p, Index lines);
			friend VisualDestinationProxy locations::forwardVisualLine(const VisualPoint& p, Index lines);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			friend VisualDestinationProxy locations::nextVisualLine(const VisualPoint& p, Direction direction, Index lines /* = 1 */);
			friend graphics::Point modelToView(const graphics::font::TextViewport& viewport, const VisualPoint& position/*, bool fullSearchBpd*/);
			friend graphics::Point modelToView(const TextViewer& textViewer, const VisualPoint& position/*, bool fullSearchBpd*/);
		};

		namespace utils {
			// scroll
			void recenter(VisualPoint& p);
			void show(VisualPoint& p);
		}	// namespace utils


		/// Equality operator for @c VisualPoint objects.
		inline bool operator==(const VisualPoint& lhs, const VisualPoint& rhs) BOOST_NOEXCEPT {
			return lhs.hit() == rhs.hit();
		}

		/// Less-than operator for @c VisualPoint objects.
		inline bool operator<(const VisualPoint& lhs, const VisualPoint& rhs) BOOST_NOEXCEPT {
			return lhs.hit() < rhs.hit();
		}

		/// Conversion operator into @c kernel#locations#PointProyx.
		inline VisualPoint::operator std::pair<const kernel::Document&, kernel::Position>() const {
			return std::make_pair(std::ref(document()), insertionPosition(*this));
		}

		/// Conversion operator into @c viewer#locations#PointProxy.
		inline VisualPoint::operator std::pair<const TextArea&, TextHit>() const {
			return std::make_pair(std::ref(textArea()), hit());
		}

		/**
		 * Returns the text hit.
		 * @see kernel#Point#position
		 */
		BOOST_CONSTEXPR inline const TextHit& VisualPoint::hit() const BOOST_NOEXCEPT {
			return hit_;
		}

		/**
		 * Returns @c true if the point has been installed and the @c TextArea is not disposed.
		 * @note This method does not check if the document has been disposed.
		 * @see #isInstalled, #isTextAreaDisposed
		 */
		inline bool VisualPoint::isFullyAvailable() const BOOST_NOEXCEPT {
			return textAreaProxy_.get() != nullptr && textAreaProxy_->get() != nullptr;
		}

		/**
		 * Returns @c true if the point has been installed by the @c TextArea.
		 * @see #install, #isFullyAvailable, #isTextAreaDisposed, #uninstall
		 */
		inline bool VisualPoint::isInstalled() const BOOST_NOEXCEPT {
			return textAreaProxy_.get() != nullptr;
		}

		/**
		 * Returns @c true if the text area which had installed the point has been disposed.
		 * @throw NotInstalledException The point is not installed
		 * @see #isFullyAvailable, #isInstalled
		 */
		inline bool VisualPoint::isTextAreaDisposed() const {
			if(!isInstalled())
				throw NotInstalledException();
			return textAreaProxy_->get() == nullptr;
		}

		/// Returns the @c MotionSignal signal connector.
		inline SignalConnector<VisualPoint::MotionSignal> VisualPoint::motionSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(motionSignal_);
		}

		/**
		 * Returns the text area which has installed this @c VisualPoint.
		 * @throw NotInstalledException
		 * @throw TextAreaDisposedException
		 */
		inline TextArea& VisualPoint::textArea() {
			throwIfNotFullyAvailable();
			return *textAreaProxy_->get();
		}

		/**
		 * Returns the text area which has installed this @c VisualPoint.
		 * @throw NotInstalledException
		 * @throw TextAreaDisposedException
		 */
		inline const TextArea& VisualPoint::textArea() const {
			throwIfNotFullyAvailable();
			return *textAreaProxy_->get();
		}

		/// @internal
		inline void VisualPoint::throwIfNotFullyAvailable() const {
			if(isTextAreaDisposed())	// may throw
				throw TextAreaDisposedException();
		}

		/**
		 * Returns the visual line numbers.
		 * @throw kernel#DocumentDisposedException
		 * @throw NotInstalledException
		 * @throw TextAreaDisposedException
		 */
		inline const graphics::font::VisualLine& VisualPoint::visualLine() const {
			throwIfNotFullyAvailable();
			if(lineNumberCaches_ == boost::none)
				const_cast<VisualPoint*>(this)->buildVisualLineCaches();
			return boost::get(lineNumberCaches_);
		}

		/// @overload
		inline kernel::Position insertionPosition(const VisualPoint& p) {
			return insertionPosition(p.document(), p.hit());
		}

		/// @overload
		inline TextHit otherHit(const VisualPoint& p) {
			return otherHit(p.document(), p.hit());
		}
	} // namespace viewer

	namespace kernel {
		/**
		 * @overload
		 * @note There is no @c offsetInLine for @c viewer#VisualPoint.
		 */
		BOOST_CONSTEXPR inline Index line(const viewer::VisualPoint& p) BOOST_NOEXCEPT {
			return line(p.hit().characterIndex());
		}
	}
} // namespace ascension

#endif // !ASCENSION_VISUAL_POINT_HPP
