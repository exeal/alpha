/**
 * @file caret.hpp
 * Defines the classes in the point-hierarchy related to visual presentation.
 * @author exeal
 * @date 2003-2008 (was point.hpp)
 * @date 2008 (separated from point.hpp)
 * @date 2009
 */

#ifndef ASCENSION_CARET_HPP
#define ASCENSION_CARET_HPP
#include "point.hpp"
#include "layout.hpp"	// layout.IVisualLinesListener
#include "unicode.hpp"	// text.IdentifierSyntax
#ifdef ASCENSION_GCC
#	include <unknwn.h>	// IUnknown, OLESTR, ...
#endif // ASCENSION_GCC
#include <objidl.h>		// IDataObject

namespace ascension {

	namespace viewers {

		namespace utils {
			HRESULT createTextObjectForSelectedString(const Caret& caret, bool rtf, IDataObject*& content);
			std::pair<HRESULT, String> getTextFromDataObject(IDataObject& data, bool* rectangle = 0);
		}

		class TextViewer;
		class VirtualBox;
		class VisualPoint;

		/**
		 * Interface for objects which are interested in change of scroll positions of a @c TextViewer.
		 * @see TextViewer#addViewportListener, TextViewer#removeViewportListener
		 */
		class IViewportListener {
		private:
			/**
			 * The scroll positions of the viewer were changed.
			 * @param horizontal true if the vertical scroll position is changed
			 * @param vertical true if the vertical scroll position is changed
			 * @see TextViewer#getFirstVisibleLine
			 */
			virtual void viewportChanged(bool horizontal, bool vertical) = 0;
			friend class TextViewer;
		};

		/**
		 * The text viewer the object connecting to had been disposed.
		 * @see kernel#DocumentDisposedException, VisualPoint
		 */
		class TextViewerDisposedException : public std::logic_error {
		public:
			TextViewerDisposedException();
		};

		/// Clipboard Win32 API was failed.
		class ClipboardException : public std::runtime_error {
		public:
			explicit ClipboardException(HRESULT hr);
		};

		/// See the documentation of @c kernel#locations namespace.
		class VerticalDestinationProxy : private kernel::Position {
		public:
			const kernel::Position& position() const {return static_cast<const kernel::Position&>(*this);}
		private:
			explicit VerticalDestinationProxy(const kernel::Position& p) : Position(p) {}
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
			viewers::VerticalDestinationProxy backwardPage(const viewers::VisualPoint& p, length_t pages = 1);
			viewers::VerticalDestinationProxy backwardVisualLine(const viewers::VisualPoint& p, length_t lines = 1);
			kernel::Position beginningOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position contextualBeginningOfLine(const viewers::VisualPoint& p);
			kernel::Position contextualBeginningOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position contextualEndOfLine(const viewers::VisualPoint& p);
			kernel::Position contextualEndOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position endOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfLine(const viewers::VisualPoint& p);
			kernel::Position firstPrintableCharacterOfVisualLine(const viewers::VisualPoint& p);
			viewers::VerticalDestinationProxy forwardPage(const viewers::VisualPoint& p, length_t pages = 1);
			viewers::VerticalDestinationProxy forwardVisualLine(const viewers::VisualPoint& p, length_t lines = 1);
			kernel::Position lastPrintableCharacterOfLine(const viewers::VisualPoint& p);
			kernel::Position lastPrintableCharacterOfVisualLine(const viewers::VisualPoint& p);
			kernel::Position leftCharacter(const viewers::VisualPoint& p, CharacterUnit unit, length_t characters = 1);
			kernel::Position leftWord(const viewers::VisualPoint& p, length_t words = 1);
			kernel::Position leftWordEnd(const viewers::VisualPoint& p, length_t words = 1);
			kernel::Position rightCharacter(const viewers::VisualPoint& p, CharacterUnit unit, length_t characters = 1);
			kernel::Position rightWord(const viewers::VisualPoint& p, length_t words = 1);
			kernel::Position rightWordEnd(const viewers::VisualPoint& p, length_t words = 1);
		} // namespace locations
	} // namespace kernel

	namespace viewers {

		/**
		 * Extension of @c kernel#Point class for viewer and layout.
		 * @see kernel#Point, kernel#IPointListener, kernel#DisposedViewException
		 */
		class VisualPoint : public kernel::Point, public layout::IVisualLinesListener {
			MANAH_UNASSIGNABLE_TAG(VisualPoint);
		public:
			// constructors
			explicit VisualPoint(TextViewer& viewer,
				const kernel::Position& position = kernel::Position(), kernel::IPointListener* listener = 0);
			VisualPoint(const VisualPoint& other);
			virtual ~VisualPoint() /*throw()*/;
			// attributes
			bool isTextViewerDisposed() const /*throw()*/;
			TextViewer& textViewer();
			const TextViewer& textViewer() const;
			length_t visualColumn() const;
			length_t visualLine() const;
			length_t visualSubline() const;
			// movement
			using kernel::Point::moveTo;
			void moveTo(const VerticalDestinationProxy& to);

		protected:
			static VerticalDestinationProxy makeVerticalDestinationProxy(const kernel::Position& source);
			// kernel.Point
			virtual void aboutToMove(kernel::Position& to);
			virtual void moved(const kernel::Position& from) /*throw()*/;
		private:
			void updateLastX();
			void viewerDisposed() /*throw()*/;
			// layout.IVisualLinesListener
			void visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) /*throw()*/;
			void visualLinesInserted(length_t first, length_t last) /*throw()*/;
			void visualLinesModified(length_t first, length_t last,
				signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/;

		private:
			TextViewer* viewer_;
			int lastX_;				// 点の、行表示領域端からの距離。行間移動時に保持しておく。-1 だと未計算
			bool crossingLines_;	// true only when the point is moving across the different lines
			length_t visualLine_, visualSubline_;	// caches
			friend class TextViewer;
			friend VerticalDestinationProxy kernel::locations::backwardVisualLine(const VisualPoint& p, length_t lines);
			friend VerticalDestinationProxy kernel::locations::forwardVisualLine(const VisualPoint& p, length_t lines);
		};

		namespace utils {
			// scroll
			void recenter(VisualPoint& p);
			void show(VisualPoint& p);
		}	// namespace utils

		/**
		 * Interface for objects which are interested in getting informed about caret movement.
		 * @see Caret#addListener, Caret#removeListener
		 */
		class ICaretListener {
		private:
			/**
			 * The caret was moved.
			 * @param self the caret
			 * @param oldRegion the region which the caret had before. @c first is the anchor, and @c second is the caret
			 */
			virtual void caretMoved(const class Caret& self, const kernel::Region& oldRegion) = 0;
			friend class Caret;
		};

		/**
		 * Interface for objects which are interested in character input by a caret.
		 * @see Caret#addCharacterInputListener, Caret#removeCharacterInputListener
		 */
		class ICharacterInputListener {
		private:
			/**
			 * A character was inputted by the caret.
			 * @param self the caret
			 * @param c the code point of the inputted character
			 */
			virtual void characterInputted(const Caret& self, CodePoint c) = 0;
			friend class Caret;
		};

		/**
		 * Interface for objects which are interested in getting informed about changes of a caret.
		 * @see IPointListener, Caret#addStateListener, Caret#removeStateListener
		 */
		class ICaretStateListener {
		private:
			/**
			 * The matched brackets are changed.
			 * @param self the caret
			 * @param oldPair the pair of the brackets previously matched
			 * @param outsideOfView the brackets newly matched are outside of the view
			 */
			virtual void matchBracketsChanged(const Caret& self,
				const std::pair<kernel::Position, kernel::Position>& oldPair, bool outsideOfView) = 0;
			/// The overtype mode of the caret is changed.
			virtual void overtypeModeChanged(const Caret& self) = 0;
			/// The shape (linear or rectangle) of the selection is changed.
			virtual void selectionShapeChanged(const Caret& self) = 0;
			friend class Caret;
		};

		// documentation is caret.cpp
		class Caret : public VisualPoint, public kernel::IPointListener, public kernel::IDocumentListener {
		public:
			/// Mode of tracking match brackets.
			enum MatchBracketsTrackingMode {
				DONT_TRACK,						///< Does not track.
				TRACK_FOR_FORWARD_CHARACTER,	///< Tracks the bracket matches forward character.
				TRACK_FOR_SURROUND_CHARACTERS	///< Tracks the bracket matches backward character.
			};

			// constructor
			explicit Caret(TextViewer& viewer, const kernel::Position& position = kernel::Position());
			~Caret();
			// listeners
			void addListener(ICaretListener& listener);
			void addCharacterInputListener(ICharacterInputListener& listener);
			void addStateListener(ICaretStateListener& listener);
			void removeListener(ICaretListener& listener);
			void removeCharacterInputListener(ICharacterInputListener& listener);
			void removeStateListener(ICaretStateListener& listener);
			// attributes : the anchor and the caret
			const VisualPoint& anchor() const /*throw()*/;
			const VisualPoint& beginning() const /*throw()*/;
			Caret& enableAutoShow(bool enable = true) /*throw()*/;
			const VisualPoint& end() const /*throw()*/;
			bool isAutoShowEnabled() const /*throw()*/;
			// attributes : selection
			const VirtualBox& boxForRectangleSelection() const;
			bool isSelectionRectangle() const /*throw()*/;
			kernel::Region selectedRegion() const /*throw()*/;
			// attributes : character input
			bool isOvertypeMode() const /*throw()*/;
			Caret& setOvertypeMode(bool overtype) /*throw()*/;
			// attributes : clipboard
			bool canPaste(bool useKillRing) const /*throw()*/;
			LCID clipboardLocale() const /*throw()*/;
			LCID setClipboardLocale(LCID newLocale);
			// attributes : matched braces
			const std::pair<kernel::Position, kernel::Position>& matchBrackets() const /*throw()*/;
			MatchBracketsTrackingMode matchBracketsTrackingMode() const /*throw()*/;
			Caret& trackMatchBrackets(MatchBracketsTrackingMode mode);
			// selection manipulations
			void beginRectangleSelection();
			void clearSelection();
			void endRectangleSelection();
			void extendSelection(const kernel::Position& to);
			void extendSelection(const VerticalDestinationProxy& to);
			void paste(bool useKillRing);
			void replaceSelection(const Char* first, const Char* last, bool rectangleInsertion = false);
			void select(const kernel::Region& region);
			void select(const kernel::Position& anchor, const kernel::Position& caret);
			// text manipulation
			bool inputCharacter(CodePoint cp, bool validateSequence = true, bool blockControls = true);

		private:
			void checkMatchBrackets();
			void internalExtendSelection(void (*algorithm)(void));
			void prechangeDocument();
			void update(const kernel::DocumentChange& change);
			void updateVisualAttributes();
			// VisualPoint
			void aboutToMove(kernel::Position& to);
			void moved(const kernel::Position& from) /*throw()*/;
			// kernel.IPointListener
			void pointMoved(const kernel::Point& self, const kernel::Position& oldPosition);
			// kernel.IDocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
		private:
			class SelectionAnchor : public VisualPoint {
			public:
				SelectionAnchor(TextViewer& viewer) /*throw()*/ :
					VisualPoint(viewer), posBeforeUpdate_(kernel::Position::INVALID_POSITION) {adaptToDocument(false);}
				void beginInternalUpdate(const kernel::DocumentChange& change) /*throw()*/ {
					assert(!isInternalUpdating()); posBeforeUpdate_ = position();
					adaptToDocument(true); update(change); adaptToDocument(false);}
				void endInternalUpdate() /*throw()*/ {
					assert(isInternalUpdating()); posBeforeUpdate_ = kernel::Position::INVALID_POSITION;}
				bool isInternalUpdating() const /*throw()*/ {return posBeforeUpdate_ != kernel::Position::INVALID_POSITION;}
				const kernel::Position& positionBeforeInternalUpdate() const /*throw()*/ {
					assert(isInternalUpdating()); return posBeforeUpdate_;}
			private:
				using kernel::Point::adaptToDocument;
				kernel::Position posBeforeUpdate_;
			} * anchor_;
			LCID clipboardLocale_;
			ascension::internal::Listeners<ICaretListener> listeners_;
			ascension::internal::Listeners<ICharacterInputListener> characterInputListeners_;
			ascension::internal::Listeners<ICaretStateListener> stateListeners_;
			bool yanking_;			// true when right after pasted by using clipboard ring, and waiting for next cycle of ring
			bool leaveAnchorNext_;	// true if should leave the anchor at the next movement
			bool leadingAnchor_;	// true if in anchor_->moveTo calling, and ignore pointMoved
			bool autoShow_;			// true if show itself when movements
			VirtualBox* box_;		// for rectangular selection. null when the selection is linear
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			bool overtypeMode_;
			bool typing_;			// true when inputCharacter called (see prechangeDocument)
			kernel::Position lastTypedPosition_;	// the position the caret input character previously or INVALID_POSITION
			kernel::Region regionBeforeMoved_;
			std::pair<kernel::Position, kernel::Position> matchBrackets_;	// 強調表示する対括弧の位置 (無い場合 Position.INVALID_POSITION)
		};

		// free functions related to selection of Caret class
		void copySelection(Caret& caret, bool useKillRing);
		void cutSelection(Caret& caret, bool useKillRing);
		bool isPointOverSelection(const Caret& caret, const POINT& p);
		bool isSelectionEmpty(const Caret& caret) /*throw()*/;
		bool selectedRangeOnLine(const Caret& caret, length_t line, length_t& first, length_t& last);
		bool selectedRangeOnVisualLine(const Caret& caret,
			length_t line, length_t subline, length_t& first, length_t& last);
		String selectedString(const Caret& caret, kernel::Newline newline = kernel::NLF_RAW_VALUE);
		std::basic_ostream<Char>& selectedString(const Caret& caret,
			std::basic_ostream<Char>& out, kernel::Newline newline = kernel::NLF_RAW_VALUE);
		void selectWord(Caret& caret);

		// free functions change the document by using Caret class
		void breakLine(Caret& at, bool inheritIndent, std::size_t newlines /* = 1 */);
		void eraseSelection(Caret& caret);
		void insertRectangle(Caret& caret, const Char* first, const Char* last);
		void insertRectangle(Caret& caret, const String& text);
		void indentBySpaces(Caret& caret, bool rectangle, long level = 1);
		void indentByTabs(Caret& caret, bool rectangle, long level = 1);
		void newLine(Caret& caret, std::size_t newlines = 1);
		bool transposeCharacters(Caret& caret);
		void replaceSelection(Caret& caret, const String& text, bool rectangleInsertion = false);
		bool transposeLines(Caret& caret);
//		bool transposeParagraphs(Caret& caret);
//		bool transposeSentences(Caret& caret);
		bool transposeWords(Caret& caret);


		// inline implementations ///////////////////////////////////////////

		/// Returns @c true if the text viewer the point connecting to has been disposed.
		inline bool VisualPoint::isTextViewerDisposed() const /*throw()*/ {return viewer_ == 0;}
		/// @internal
		inline VerticalDestinationProxy VisualPoint::makeVerticalDestinationProxy(
			const kernel::Position& source) {return VerticalDestinationProxy(source);}
		/// Returns the text viewer or throw @c TextViewerDisposedException if the text viewer the
		/// point connecting to has been disposed.
		inline TextViewer& VisualPoint::textViewer() {
			if(viewer_ == 0) throw TextViewerDisposedException(); return *viewer_;}
		/// Returns the text viewer or throw @c TextViewerDisposedException if the text viewer the
		/// point connecting to has been disposed.
		inline const TextViewer& VisualPoint::textViewer() const {
			if(viewer_ == 0) throw TextViewerDisposedException(); return *viewer_;}
		/// Called when the text viewer is disposed.
		inline void VisualPoint::viewerDisposed() /*throw()*/ {viewer_ = 0;}

		/// Returns the visual subline number.
		inline length_t VisualPoint::visualSubline() const {
			if(visualLine_ == INVALID_INDEX)
				visualLine();
			return visualSubline_;
		}

		/// Returns the anchor of the selection.
		inline const VisualPoint& Caret::anchor() const /*throw()*/ {return *anchor_;}
		/// Returns the neighborhood to the beginning of the document among the anchor and this point.
		inline const VisualPoint& Caret::beginning() const /*throw()*/ {
			return std::min(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));}
		/**
		 * Returns the rectangle selection.
		 * @return the virtual box represents the rectangle selection
		 * @throw IllegalStateException the selection is not rectangle.
		 */
		inline const VirtualBox& Caret::boxForRectangleSelection() const {
			if(!isSelectionRectangle()) throw IllegalStateException("The selection is not rectangle.") ; return *box_;}
		/// Returns the locale identifier used to convert non-Unicode text.
		inline LCID Caret::clipboardLocale() const /*throw()*/ {return clipboardLocale_;}
		/**
		 * Sets the new auto-show mode.
		 * @param enable set true to enable the mode
		 * @return this caret
		 * @see #isAutoShowEnabled
		 */
		inline Caret& Caret::enableAutoShow(bool enable /* = true */) /*throw()*/ {autoShow_ = enable; return *this;}
		/// Returns the neighborhood to the end of the document among the anchor and this point.
		inline const VisualPoint& Caret::end() const /*throw()*/ {
			return std::max(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));}
		/// Returns true if the point will be shown automatically when moved. Default is true.
		inline bool Caret::isAutoShowEnabled() const /*throw()*/ {return autoShow_;}
		/// Returns true if the caret is in overtype mode.
		inline bool Caret::isOvertypeMode() const /*throw()*/ {return overtypeMode_;}
		/// Returns true if the selection is rectangle.
		inline bool Caret::isSelectionRectangle() const /*throw()*/ {return box_ != 0;}
		/// キャレット位置の括弧と対応する括弧の位置を返す (@a first が対括弧、@a second がキャレット周辺の括弧)
		inline const std::pair<kernel::Position, kernel::Position>& Caret::matchBrackets() const /*throw()*/ {return matchBrackets_;}
		/// Returns the matched braces tracking mode.
		inline Caret::MatchBracketsTrackingMode Caret::matchBracketsTrackingMode() const /*throw()*/ {return matchBracketsTrackingMode_;}
		/**
		 * Selects the specified region.
		 * @param region the region. @a pos1 member is the anchor, @a pos2 member is the caret
		 */
		inline void Caret::select(const kernel::Region& region) {select(region.first, region.second);}
		/// Returns the selected region.
		inline kernel::Region Caret::selectedRegion() const /*throw()*/ {return kernel::Region(*anchor_, position());}
		/**
		 * Tracks the match bracket.
		 * @param mode the tracking mode
		 * @return this caret
		 */
		inline Caret& Caret::trackMatchBrackets(MatchBracketsTrackingMode mode) /*throw()*/ {
			if(mode != matchBracketsTrackingMode_) {matchBracketsTrackingMode_ = mode; checkMatchBrackets();} return *this;}

		/// Returns true if the selection of the given caret is empty.
		inline bool isSelectionEmpty(const Caret& caret) /*throw()*/ {return caret.selectedRegion().isEmpty();}
		/**
		 * Returns the selected text string.
		 * @param caret the caret gives a selection
		 * @param newline the newline representation for multiline selection. if the selection is
		 *                rectangular, this value is ignored and the document's newline is used instead
		 * @return the text string
		 */
		inline String selectedString(const Caret& caret, kernel::Newline newline /* = NLF_RAW_VALUE */) {
			std::basic_ostringstream<Char> ss; selectedString(caret, ss, newline); return ss.str();}

	} // namespace viewers
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
