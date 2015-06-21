/**
 * @file visual-point.hpp
 * Defines @c VisualPoint class and related stuffs.
 * @author exeal
 * @date 2003-2008 was point.hpp
 * @date 2008 separated from point.hpp
 * @date 2009-2011 was caret.hpp
 * @date 2011-10-02 separated from caret.hpp
 * @date 2011-2014
 */

#ifndef ASCENSION_VISUAL_POINT_HPP
#define ASCENSION_VISUAL_POINT_HPP
#include <ascension/kernel/point.hpp>
#include <ascension/graphics/font/visual-lines-listener.hpp>
#include <ascension/viewer/detail/weak-reference-for-points.hpp>

namespace ascension {
	namespace viewer {
		namespace detail {
			class VisualDestinationProxyMaker;
		}

		class TextViewer;
		class VisualPoint;

		/**
		 * The text viewer the object connecting to had been disposed.
		 * @see kernel#DocumentDisposedException, VisualPoint
		 */
		class TextViewerDisposedException : public std::logic_error {
		public:
			TextViewerDisposedException();
		};

		/// See the documentation of @c kernel#locations namespace.
		class VisualDestinationProxy : private kernel::Position {
		public:
			bool crossesVisualLines() const BOOST_NOEXCEPT {return crossesVisualLines_;}
			const kernel::Position& position() const BOOST_NOEXCEPT {
				return static_cast<const kernel::Position&>(*this);
			}
		private:
			explicit VisualDestinationProxy(const Position& p, bool crossesVisualLines)
				: Position(p), crossesVisualLines_(crossesVisualLines) {}
			const bool crossesVisualLines_;
			friend class detail::VisualDestinationProxyMaker;
		};
	}
}

namespace ascension {
	namespace kernel {
		namespace locations {
			bool isEndOfVisualLine(const viewer::VisualPoint& p);
			bool isFirstPrintableCharacterOfLine(const viewer::VisualPoint& p);
			bool isFirstPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
			bool isLastPrintableCharacterOfLine(const viewer::VisualPoint& p);
			bool isLastPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
			bool isBeginningOfVisualLine(const viewer::VisualPoint& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy backwardPage(const viewer::VisualPoint& p, Index pages = 1);
			viewer::VisualDestinationProxy backwardVisualLine(const viewer::VisualPoint& p, Index lines = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			kernel::Position beginningOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position contextualBeginningOfLine(const viewer::VisualPoint& p);
			kernel::Position contextualBeginningOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position contextualEndOfLine(const viewer::VisualPoint& p);
			kernel::Position contextualEndOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position endOfVisualLine(const viewer::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfLine(const viewer::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy forwardPage(const viewer::VisualPoint& p, Index pages = 1);
			viewer::VisualDestinationProxy forwardVisualLine(const viewer::VisualPoint& p, Index lines = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			kernel::Position lastPrintableCharacterOfLine(const viewer::VisualPoint& p);
			kernel::Position lastPrintableCharacterOfVisualLine(const viewer::VisualPoint& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy leftCharacter(
				const viewer::VisualPoint& p, CharacterUnit unit, Index characters = 1);
			boost::optional<kernel::Position> leftWord(const viewer::VisualPoint& p, Index words = 1);
			boost::optional<kernel::Position> leftWordEnd(const viewer::VisualPoint& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy nextPage(
				const viewer::VisualPoint& p, Direction direction, Index pages = 1);
			viewer::VisualDestinationProxy nextVisualLine(
				const viewer::VisualPoint& p, Direction direction, Index lines = 1);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy rightCharacter(
				const viewer::VisualPoint& p, CharacterUnit unit, Index characters = 1);
			boost::optional<Position> rightWord(const viewer::VisualPoint& p, Index words = 1);
			boost::optional<Position> rightWordEnd(const viewer::VisualPoint& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
		} // namespace locations
	} // namespace kernel

	namespace viewer {
		/**
		 * Extension of @c kernel#Point class for viewer and layout.
		 * @see kernel#Point, kernel#DisposedViewException
		 */
		class VisualPoint : public kernel::Point, public graphics::font::VisualLinesListener {
		public:
			VisualPoint(TextViewer& viewer, const kernel::Position& position);
			VisualPoint(const VisualPoint& other);
			virtual ~VisualPoint() BOOST_NOEXCEPT;
			/// @name Attributes
			/// @{
			bool isTextViewerDisposed() const BOOST_NOEXCEPT;
			TextViewer& textViewer();
			const TextViewer& textViewer() const;
			/// @}

			/// @name Visual Positions
			/// @{
			Index offsetInVisualLine() const;
			Index visualLine() const;
			Index visualSubline() const;
			/// @}

			/// @name Movement
			/// @{
			using kernel::Point::moveTo;
			void moveTo(const VisualDestinationProxy& to);
			/// @}

		protected:
			// kernel.Point
			virtual void aboutToMove(kernel::Position& to) override;
			virtual void moved(const kernel::Position& from) override BOOST_NOEXCEPT;
		private:
			void rememberPositionInVisualLine();
			// layout.VisualLinesListener
			void visualLinesDeleted(const boost::integer_range<Index>& lines,
				Index sublines, bool longestLineChanged) override BOOST_NOEXCEPT;
			void visualLinesInserted(const boost::integer_range<Index>& lines) override BOOST_NOEXCEPT;
			void visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) override BOOST_NOEXCEPT;

		private:
			std::shared_ptr<const detail::WeakReferenceForPoints<TextViewer>::Proxy> viewerProxy_;
			boost::optional<graphics::Scalar> positionInVisualLine_;	// see rememberPositionInVisualLine
			bool crossingLines_;	// true only when the point is moving across the different lines
			struct LineNumberCaches {
				Index visualLine, visualSubline;
			};
			boost::optional<LineNumberCaches> lineNumberCaches_;	// caches
			friend class TextViewer;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			friend VisualDestinationProxy kernel::locations::backwardVisualLine(const VisualPoint& p, Index lines);
			friend VisualDestinationProxy kernel::locations::forwardVisualLine(const VisualPoint& p, Index lines);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			friend VisualDestinationProxy kernel::locations::nextVisualLine(const VisualPoint& p, Direction direction, Index lines);
		};

		namespace utils {
			// scroll
			void recenter(VisualPoint& p);
			void show(VisualPoint& p);
		}	// namespace utils

		namespace detail {
			using kernel::detail::identifierSyntax;
		}


		/// Returns @c true if the text viewer the point connecting to has been disposed.
		inline bool VisualPoint::isTextViewerDisposed() const BOOST_NOEXCEPT {
			return viewerProxy_.get() == nullptr && viewerProxy_->get() != nullptr;
		}

		/// Returns the text viewer or throw @c TextViewerDisposedException if the text viewer the
		/// point connecting to has been disposed.
		inline TextViewer& VisualPoint::textViewer() {
			if(isTextViewerDisposed())
				throw TextViewerDisposedException();
			return *viewerProxy_->get();
		}

		/// Returns the text viewer or throw @c TextViewerDisposedException if the text viewer the
		/// point connecting to has been disposed.
		inline const TextViewer& VisualPoint::textViewer() const {
			if(isTextViewerDisposed())
				throw TextViewerDisposedException();
			return *viewerProxy_->get();
		}

		/// Returns the visual subline number.
		inline Index VisualPoint::visualSubline() const {
			if(!lineNumberCaches_)
				visualLine();
			return lineNumberCaches_->visualSubline;
		}

	} // namespace viewer
} // namespace ascension

#endif // !ASCENSION_VISUAL_POINT_HPP
