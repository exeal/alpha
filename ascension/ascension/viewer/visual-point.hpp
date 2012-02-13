/**
 * @file visual-point.hpp
 * Defines @c VisualPoint class and related stuffs.
 * @author exeal
 * @date 2003-2008 (was point.hpp)
 * @date 2008 (separated from point.hpp)
 * @date 2009-2011 was caret.hpp
 * @date 2011-10-02 separated from caret.hpp
 * @date 2011-2012
 */

#ifndef ASCENSION_VISUAL_POINT_HPP
#define ASCENSION_VISUAL_POINT_HPP
#include <ascension/kernel/point.hpp>

namespace ascension {

	namespace viewers {

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
		class BlockProgressionDestinationProxy : private kernel::Position {
		public:
			const kernel::Position& position() const {
				return static_cast<const kernel::Position&>(*this);
			}
		private:
			explicit BlockProgressionDestinationProxy(const kernel::Position& p) : Position(p) {}
			friend class VisualPoint;
		};
	}

	namespace kernel {
		namespace locations {
			bool isEndOfVisualLine(const viewers::VisualPoint& p);
			bool isFirstPrintableCharacterOfLine(const viewers::VisualPoint& p);
			bool isFirstPrintableCharacterOfVisualLine(const viewers::VisualPoint& p);
			bool isLastPrintableCharacterOfLine(const viewers::VisualPoint& p);
			bool isLastPrintableCharacterOfVisualLine(const viewers::VisualPoint& p);
			bool isBeginningOfVisualLine(const viewers::VisualPoint& p);
			viewers::BlockProgressionDestinationProxy backwardPage(const viewers::VisualPoint& p, Index pages = 1);
			viewers::BlockProgressionDestinationProxy backwardVisualLine(const viewers::VisualPoint& p, Index lines = 1);
			kernel::Position beginningOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position contextualBeginningOfLine(const viewers::VisualPoint& p);
			kernel::Position contextualBeginningOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position contextualEndOfLine(const viewers::VisualPoint& p);
			kernel::Position contextualEndOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position endOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfLine(const viewers::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfVisualLine(const viewers::VisualPoint& p);
			viewers::BlockProgressionDestinationProxy forwardPage(const viewers::VisualPoint& p, Index pages = 1);
			viewers::BlockProgressionDestinationProxy forwardVisualLine(const viewers::VisualPoint& p, Index lines = 1);
			kernel::Position lastPrintableCharacterOfLine(const viewers::VisualPoint& p);
			kernel::Position lastPrintableCharacterOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position leftCharacter(const viewers::VisualPoint& p, CharacterUnit unit, Index characters = 1);
			kernel::Position leftWord(const viewers::VisualPoint& p, Index words = 1);
			kernel::Position leftWordEnd(const viewers::VisualPoint& p, Index words = 1);
			kernel::Position rightCharacter(const viewers::VisualPoint& p, CharacterUnit unit, Index characters = 1);
			kernel::Position rightWord(const viewers::VisualPoint& p, Index words = 1);
			kernel::Position rightWordEnd(const viewers::VisualPoint& p, Index words = 1);
		} // namespace locations
	} // namespace kernel

	namespace viewers {

		/**
		 * Extension of @c kernel#Point class for viewer and layout.
		 * @see kernel#Point, kernel#PointListener, kernel#DisposedViewException
		 */
		class VisualPoint : public kernel::Point, public graphics::font::VisualLinesListener {
			ASCENSION_UNASSIGNABLE_TAG(VisualPoint);
		public:
			// constructors
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			explicit VisualPoint(TextViewer& viewer, kernel::PointListener* listener = nullptr);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			VisualPoint(TextViewer& viewer, const kernel::Position& position, kernel::PointListener* listener = nullptr);
			VisualPoint(const VisualPoint& other);
			virtual ~VisualPoint() /*throw()*/;
			// attributes
			bool isTextViewerDisposed() const /*throw()*/;
			TextViewer& textViewer();
			const TextViewer& textViewer() const;
			// visual positions
			Index offsetInVisualLine() const;
			Index visualLine() const;
			Index visualSubline() const;
			// movement
			using kernel::Point::moveTo;
			void moveTo(const BlockProgressionDestinationProxy& to);

		protected:
			static BlockProgressionDestinationProxy makeBlockProgressionDestinationProxy(const kernel::Position& source);
			// kernel.Point
			virtual void aboutToMove(kernel::Position& to);
			virtual void moved(const kernel::Position& from) /*throw()*/;
		private:
			void rememberPositionInVisualLine();
			void viewerDisposed() /*throw()*/;
			// layout.VisualLinesListener
			void visualLinesDeleted(const Range<Index>& lines, Index sublines, bool longestLineChanged) /*throw()*/;
			void visualLinesInserted(const Range<Index>& lines) /*throw()*/;
			void visualLinesModified(const Range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/;

		private:
			TextViewer* viewer_;
			boost::optional<graphics::Scalar> positionInVisualLine_;	// see rememberPositionInVisualLine
			bool crossingLines_;	// true only when the point is moving across the different lines
			struct LineNumberCaches {
				Index visualLine, visualSubline;
			};
			boost::optional<LineNumberCaches> lineNumberCaches_;	// caches
			friend class TextViewer;
			friend BlockProgressionDestinationProxy kernel::locations::backwardVisualLine(const VisualPoint& p, Index lines);
			friend BlockProgressionDestinationProxy kernel::locations::forwardVisualLine(const VisualPoint& p, Index lines);
		};

		namespace utils {
			// scroll
			void recenter(VisualPoint& p);
			void show(VisualPoint& p);
		}	// namespace utils


		/// Returns @c true if the text viewer the point connecting to has been disposed.
		inline bool VisualPoint::isTextViewerDisposed() const /*throw()*/ {return viewer_ == nullptr;}

		/// @internal
		inline BlockProgressionDestinationProxy VisualPoint::makeBlockProgressionDestinationProxy(
			const kernel::Position& source) {return BlockProgressionDestinationProxy(source);}

		/// Returns the text viewer or throw @c TextViewerDisposedException if the text viewer the
		/// point connecting to has been disposed.
		inline TextViewer& VisualPoint::textViewer() {
			if(viewer_ == nullptr) throw TextViewerDisposedException(); return *viewer_;}

		/// Returns the text viewer or throw @c TextViewerDisposedException if the text viewer the
		/// point connecting to has been disposed.
		inline const TextViewer& VisualPoint::textViewer() const {
			if(viewer_ == nullptr) throw TextViewerDisposedException(); return *viewer_;}

		/// Called when the text viewer is disposed.
		inline void VisualPoint::viewerDisposed() /*throw()*/ {viewer_ = nullptr;}

		/// Returns the visual subline number.
		inline Index VisualPoint::visualSubline() const {
			if(!lineNumberCaches_)
				visualLine();
			return lineNumberCaches_->visualSubline;
		}

	} // namespace viewers
} // namespace ascension

#endif // !ASCENSION_VISUAL_POINT_HPP
