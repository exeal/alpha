/**
 * @file caret.hpp
 * Defines the classes in the point-hierarchy related to visual presentation.
 * @author exeal
 * @date 2003-2008 (was point.hpp)
 * @date 2008 (separated from point.hpp)
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
		class TextViewer;
		class VirtualBox;
		class VisualPoint;

		std::pair<HRESULT, String> getTextFromDataObject(IDataObject& data, bool* rectangle = 0);

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

		/// Target @a TextViewer of @c VisualPoint has been disposed.
		class DisposedViewerException : public std::runtime_error {
		public:
			DisposedViewerException() :
				std::runtime_error("Target viewer is already inavailable. This object is no longer able to be used anyway.") {}
		};

		/// Clipboard Win32 API was failed.
		class ClipboardException : public std::runtime_error {
		public:
			explicit ClipboardException(HRESULT hr);
		};

		/// 
		class VerticalDestinationProxy : private kernel::Position {
		public:
			const kernel::Position& position() const {return static_cast<const kernel::Position&>(*this);}
		private:
			explicit VerticalDestinationProxy(const kernel::Position& p) : Position(p) {}
			friend class VisualPoint;
		};

		/**
		 * Extension of @c EditPoint for viewer and layout.
		 * @see kernel#EditPoint, kernel#IPointListener, kernel#DisposedViewException
		 */
		class VisualPoint : public kernel::EditPoint, public layout::IVisualLinesListener {
			MANAH_UNASSIGNABLE_TAG(VisualPoint);
		public:
			// constructors
			explicit VisualPoint(TextViewer& viewer,
				const kernel::Position& position = kernel::Position(), kernel::IPointListener* listener = 0);
			VisualPoint(const VisualPoint& rhs);
			virtual ~VisualPoint() /*throw()*/;
			// attributes
			static bool canPaste() /*throw()*/;
			bool isEndOfVisualLine() const;
			bool isFirstPrintableCharacterOfLine() const;
			bool isFirstPrintableCharacterOfVisualLine() const;
			bool isLastPrintableCharacterOfLine() const;
			bool isLastPrintableCharacterOfVisualLine() const;
			bool isBeginningOfVisualLine() const;
			TextViewer& textViewer();
			const TextViewer& textViewer() const;
			length_t visualColumnNumber() const;
			// destination calculations
			VerticalDestinationProxy backwardPage(length_t pages = 1) const;
			VerticalDestinationProxy backwardVisualLine(length_t lines = 1) const;
			kernel::Position beginningOfVisualLine() const;
			kernel::Position contextualBeginningOfLine() const;
			kernel::Position contextualBeginningOfVisualLine() const;
			kernel::Position contextualEndOfLine() const;
			kernel::Position contextualEndOfVisualLine() const;
			kernel::Position endOfVisualLine() const;
			kernel::Position firstPrintableCharacterOfLine() const;
			kernel::Position firstPrintableCharacterOfVisualLine() const;
			VerticalDestinationProxy forwardPage(length_t pages = 1) const;
			VerticalDestinationProxy forwardVisualLine(length_t lines = 1) const;
			kernel::Position lastPrintableCharacterOfLine() const;
			kernel::Position lastPrintableCharacterOfVisualLine() const;
			kernel::Position leftCharacter(length_t characters = 1) const;
			kernel::Position leftWord(length_t words = 1) const;
			kernel::Position leftWordEnd(length_t words = 1) const;
			kernel::Position rightCharacter(length_t characters = 1) const;
			kernel::Position rightWord(length_t words = 1) const;
			kernel::Position rightWordEnd(length_t words = 1) const;
			// movement
			using EditPoint::moveTo;
			void moveTo(const VerticalDestinationProxy& to);
			// scroll
			void recenter();
			void show();
			// text manipulations
			bool insertRectangle(const String& text);
			bool insertRectangle(const Char* first, const Char* last);
			bool newLine(bool inheritIndent, size_t newlines = 1);
			bool paste();
			kernel::Position spaceIndent(const kernel::Position& other, bool rectangle, long level = 1);
			kernel::Position tabIndent(const kernel::Position& other, bool rectangle, long level = 1);

		protected:
			virtual void doMoveTo(const kernel::Position& to);
			void verifyViewer() const;
		private:
			using kernel::EditPoint::newLine;	// 明示的な隠蔽
			kernel::Position doIndent(const kernel::Position& other, Char character, bool rectangle, long level);
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
			bool crossingLines_;	// 行間移動中
			length_t visualLine_, visualSubline_;	// 点の表示行
			friend class TextViewer;
		};

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
			HRESULT createTextObject(bool rtf, IDataObject*& content) const;
			bool isPointOverSelection(const POINT& pt) const;
			bool isSelectionEmpty() const /*throw()*/;
			bool isSelectionRectangle() const /*throw()*/;
			bool selectedRangeOnLine(length_t line, length_t& first, length_t& last) const;
			bool selectedRangeOnVisualLine(length_t line, length_t subline, length_t& first, length_t& last) const;
			kernel::Region selectionRegion() const /*throw()*/;
			String selectionText(kernel::Newline newline = kernel::NLF_RAW_VALUE) const;
			// attributes : character input
			bool isOvertypeMode() const /*throw()*/;
			Caret& setOvertypeMode(bool overtype) /*throw()*/;
			// attributes : clipboard
			LCID clipboardLocale() const /*throw()*/;
			LCID setClipboardLocale(LCID newLocale);
			// attributes : matched braces
			const std::pair<kernel::Position, kernel::Position>& matchBrackets() const /*throw()*/;
			MatchBracketsTrackingMode matchBracketsTrackingMode() const /*throw()*/;
			Caret& trackMatchBrackets(MatchBracketsTrackingMode mode);
			// selection manipulations
			void beginRectangleSelection();
			void clearSelection();
			void copySelection(bool useKillRing);
			bool cutSelection(bool useKillRing);
			void endRectangleSelection();
			bool eraseSelection();
			void extendSelection(const kernel::Position& to);
			void extendSelection(const VerticalDestinationProxy& to);
			bool pasteToSelection(bool useKillRing);
			bool replaceSelection(const Char* first, const Char* last, bool rectangleInsertion = false);
			bool replaceSelection(const String& text, bool rectangleInsertion = false);
			void select(const kernel::Region& region);
			void select(const kernel::Position& anchor, const kernel::Position& caret);
			void selectWord();
			// text manipulation
			bool inputCharacter(CodePoint cp, bool validateSequence = true, bool blockControls = true);

		private:
			void checkMatchBrackets();
			void internalExtendSelection(void (*algorithm)(void));
			void update(const kernel::DocumentChange& change);
			void updateVisualAttributes();
			// VisualPoint
			void doMoveTo(const kernel::Position& position);
			// kernel.IPointListener
			void pointMoved(const kernel::EditPoint& self, const kernel::Position& oldPosition);
			// kernel.IDocumentListener
			void documentAboutToBeChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			using kernel::EditPoint::getListener;
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
				using Point::adaptToDocument;
				kernel::Position posBeforeUpdate_;
			} * anchor_;
			LCID clipboardLocale_;
			ascension::internal::Listeners<ICaretListener> listeners_;
			ascension::internal::Listeners<ICharacterInputListener> characterInputListeners_;
			ascension::internal::Listeners<ICaretStateListener> stateListeners_;
			bool yanking_;			// クリップボードリングから貼り付けた直後でリング循環のため待機中
			bool leaveAnchorNext_;	// true if should leave the anchor at the next movement
			bool leadingAnchor_;	// true if in anchor_->moveTo calling, and ignore pointMoved
			bool autoShow_;			// true if show itself when movements
			VirtualBox* box_;		// for rectangular selection. null when the selection is linear
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			bool overtypeMode_;
			bool callingDocumentInsertForTyping_;	// true during call Document.insert in inputCharacter
			kernel::Position lastTypedPosition_;	// the position the caret input character previously or INVALID_POSITION
			kernel::Region regionBeforeMoved_;
			std::pair<kernel::Position, kernel::Position> matchBrackets_;	// 強調表示する対括弧の位置 (無い場合 Position.INVALID_POSITION)
		};


		// inline implementations ///////////////////////////////////////////

		/**
		 * Inserts the specified text at the current position as a rectangle.
		 * @param text the text to insert
		 * @return false if the change was interrupted
		 * @throw ReadOnlyDocumentException the document is read only
		 * @see EditPoint#insert(const String&)
		 */
		inline bool VisualPoint::insertRectangle(const String& text) {return insertRectangle(text.data(), text.data() + text.length());}
		/// Returns the text viewer.
		inline TextViewer& VisualPoint::textViewer() {verifyViewer(); return *viewer_;}
		/// Returns the text viewer.
		inline const TextViewer& VisualPoint::textViewer() const {verifyViewer(); return *viewer_;}
		/// Throws @c DisposedViewerException if the text viewer is already disposed.
		inline void VisualPoint::verifyViewer() const {verifyDocument(); if(viewer_ == 0) throw DisposedViewerException();}
		/// Called when the text viewer is disposed.
		inline void VisualPoint::viewerDisposed() /*throw()*/ {viewer_ = 0;}
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
		/// Returns true if the selection is empty.
		inline bool Caret::isSelectionEmpty() const /*throw()*/ {return anchor_->position() == position();}
		/// Returns true if the selection is rectangle.
		inline bool Caret::isSelectionRectangle() const /*throw()*/ {return box_ != 0;}
		/// キャレット位置の括弧と対応する括弧の位置を返す (@a first が対括弧、@a second がキャレット周辺の括弧)
		inline const std::pair<kernel::Position, kernel::Position>& Caret::matchBrackets() const /*throw()*/ {return matchBrackets_;}
		/// Returns the matched braces tracking mode.
		inline Caret::MatchBracketsTrackingMode Caret::matchBracketsTrackingMode() const /*throw()*/ {return matchBracketsTrackingMode_;}
		/**
		 * Replaces the selected region with the specified text.
		 * @param text the text
		 * @param rectangleInsertion if set to true, @a text is inserted as rectangle
		 * @return false if the change was interrupted
		 * @throw ReadOnlyDocumentException the document is read only
		 */
		inline bool Caret::replaceSelection(const String& text, bool rectangleInsertion /* = false */) {
			return replaceSelection(text.data(), text.data() + text.length(), rectangleInsertion);}
		/**
		 * Selects the specified region.
		 * @param region the region. @a pos1 member is the anchor, @a pos2 member is the caret
		 */
		inline void Caret::select(const kernel::Region& region) {select(region.first, region.second);}
		/// Returns the selected region.
		inline kernel::Region Caret::selectionRegion() const /*throw()*/ {return kernel::Region(*anchor_, position());}
		/**
		 * Tracks the match bracket.
		 * @param mode the tracking mode
		 * @return this caret
		 */
		inline Caret& Caret::trackMatchBrackets(MatchBracketsTrackingMode mode) /*throw()*/ {
			if(mode != matchBracketsTrackingMode_) {matchBracketsTrackingMode_ = mode; checkMatchBrackets();} return *this;}

	} // namespace viewers
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
