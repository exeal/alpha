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
#include <functional>	// std.mem_fun_t
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

		/**
		 * Extension of @c EditPoint for viewer and layout.
		 * @see kernel#EditPoint, kernel#IPointListener, kernel#DisposedViewException
		 */
		class VisualPoint : public kernel::EditPoint, virtual public layout::IVisualLinesListener {
			MANAH_UNASSIGNABLE_TAG(VisualPoint);
		public:
			// constructors
			explicit VisualPoint(TextViewer& viewer,
				const kernel::Position& position = kernel::Position(), kernel::IPointListener* listener = 0);
			VisualPoint(const VisualPoint& rhs);
			virtual ~VisualPoint() throw();

			// attributes
			static bool canPaste() throw();
			bool isEndOfVisualLine() const;
			bool isFirstPrintableCharacterOfLine() const;
			bool isFirstPrintableCharacterOfVisualLine() const;
			bool isLastPrintableCharacterOfLine() const;
			bool isLastPrintableCharacterOfVisualLine() const;
			bool isBeginningOfVisualLine() const;
			TextViewer& textViewer();
			const TextViewer& textViewer() const;
			length_t visualColumnNumber() const;

			// movement
			void beginningOfVisualLine();
			void endOfVisualLine();
			void firstPrintableCharacterOfLine();
			void firstPrintableCharacterOfVisualLine();
			void lastPrintableCharacterOfLine();
			void lastPrintableCharacterOfVisualLine();
			void leftCharacter(length_t offset = 1);
			void leftWord(length_t offset = 1);
			void leftWordEnd(length_t offset = 1);
			void nextPage(length_t offset = 1);
			void nextVisualLine(length_t offset = 1);
			void nextWord(length_t offset = 1);
			void nextWordEnd(length_t offset = 1);
			void previousPage(length_t offset = 1);
			void previousVisualLine(length_t offset = 1);
			void previousWord(length_t offset = 1);
			void previousWordEnd(length_t offset = 1);
			void rightCharacter(length_t offset = 1);
			void rightWord(length_t offset = 1);
			void rightWordEnd(length_t offset = 1);

			// scroll
			bool recenter(signed_length_t length = 0);
			bool recenter(const kernel::Position& other);
			bool show(signed_length_t length = 0);
			bool show(const kernel::Position& other);

			// text manipulations
			void insertRectangle(const String& text);
			void insertRectangle(const Char* first, const Char* last);
			void newLine(bool inheritIndent);
			void paste(signed_length_t length = 0);
			void paste(const kernel::Position& other);
			kernel::Position spaceIndent(const kernel::Position& other, bool rectangle, long level = 1);
			kernel::Position tabIndent(const kernel::Position& other, bool rectangle, long level = 1);
			bool transposeCharacters();
			bool transposeLines();
//			bool transposeParagraphs();
//			bool transposeSentences();
			bool transposeWords();

		protected:
			virtual void doMoveTo(const kernel::Position& to);
			const text::IdentifierSyntax& identifierSyntax() const throw();
			kernel::Position offsetPosition(signed_length_t offset) const;
			void verifyViewer() const;
		private:
			using kernel::EditPoint::newLine;	// 明示的な隠蔽
			kernel::Position doIndent(const kernel::Position& other, Char character, bool rectangle, long level);
			void updateLastX();
			void viewerDisposed() throw();
			// layout.IVisualLinesListener
			void visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) throw();
			void visualLinesInserted(length_t first, length_t last) throw();
			void visualLinesModified(length_t first, length_t last,
				signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) throw();

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
		class Caret : public VisualPoint, virtual public kernel::IPointListener, virtual public kernel::IDocumentListener {
		public:
			/// Mode of selection.
			enum SelectionMode {
				CHARACTER,	///< Character selection mode. This is default.
				LINE,		///< Line selection mode.
				WORD		///< Word selection mode.
			};

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
			const VisualPoint& anchor() const throw();
			const VisualPoint& beginning() const throw();
			void enableAutoShow(bool enable = true) throw();
			const VisualPoint& end() const throw();
			bool isAutoShowEnabled() const throw();
			// attributes : selection
			const VirtualBox& boxForRectangleSelection() const;
			HRESULT createTextObject(bool rtf, IDataObject*& content) const;
			bool isPointOverSelection(const ::POINT& pt) const;
			bool isSelectionEmpty() const throw();
			bool isSelectionRectangle() const throw();
			bool selectedRangeOnLine(length_t line, length_t& first, length_t& last) const;
			bool selectedRangeOnVisualLine(length_t line, length_t subline, length_t& first, length_t& last) const;
			SelectionMode selectionMode() const throw();
			kernel::Region selectionRegion() const throw();
			String selectionText(kernel::Newline newline = kernel::NLF_RAW_VALUE) const;
			// attributes : character input
			bool isOvertypeMode() const throw();
			void setOvertypeMode(bool overtype) throw();
			// attributes : clipboard
			LCID clipboardLocale() const throw();
			LCID setClipboardLocale(LCID newLocale);
			// attributes : matched braces
			const std::pair<kernel::Position, kernel::Position>& matchBrackets() const;
			MatchBracketsTrackingMode matchBracketsTrackingMode() const throw();
			void trackMatchBrackets(MatchBracketsTrackingMode mode);

			// selection manipulations
			void beginLineSelection();
			void beginRectangleSelection();
			void beginWordSelection();
			void clearSelection();
			void copySelection(bool alsoSendToClipboardRing);
			void cutSelection(bool alsoSendToClipboardRing);
			void endRectangleSelection();
			void extendSelection(const kernel::Position& to);
			void extendSelection(std::mem_fun_t<void, kernel::EditPoint>& algorithm);
			void extendSelection(std::mem_fun_t<void, VisualPoint>& algorithm);
			void extendSelection(std::mem_fun1_t<void, kernel::EditPoint, length_t>& algorithm, length_t offset);
			void extendSelection(std::mem_fun1_t<void, VisualPoint, length_t>& algorithm, length_t offset);
			void eraseSelection();
			void pasteToSelection(bool fromClipboardRing);
			void replaceSelection(const Char* first, const Char* last, bool rectangleInsertion = false);
			void replaceSelection(const String& text, bool rectangleInsertion = false);
			void restoreSelectionMode();
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
			bool documentAboutToBeChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			using kernel::EditPoint::getListener;
		private:
			class SelectionAnchor : public VisualPoint {
			public:
				SelectionAnchor(TextViewer& viewer) throw() :
					VisualPoint(viewer), posBeforeUpdate_(kernel::Position::INVALID_POSITION) {adaptToDocument(false);}
				void beginInternalUpdate(const kernel::DocumentChange& change) throw() {
					assert(!isInternalUpdating()); posBeforeUpdate_ = position();
					adaptToDocument(true); update(change); adaptToDocument(false);}
				void endInternalUpdate() throw() {assert(isInternalUpdating()); posBeforeUpdate_ = kernel::Position::INVALID_POSITION;}
				bool isInternalUpdating() const throw() {return posBeforeUpdate_ != kernel::Position::INVALID_POSITION;}
				const kernel::Position& positionBeforeInternalUpdate() const throw() {assert(isInternalUpdating()); return posBeforeUpdate_;}
			private:
				using Point::adaptToDocument;
				kernel::Position posBeforeUpdate_;
			} * anchor_;
			SelectionMode selectionMode_;
			length_t modeInitialAnchorLine_;	// line number of the anchor when entered the selection mode
			length_t wordSelectionChars_[2];	// 単語選択モードで最初に選択されていた単語の前後の文字位置
			LCID clipboardLocale_;
			ascension::internal::Listeners<ICaretListener> listeners_;
			ascension::internal::Listeners<ICharacterInputListener> characterInputListeners_;
			ascension::internal::Listeners<ICaretStateListener> stateListeners_;
			bool pastingFromClipboardRing_;		// クリップボードリングから貼り付けた直後でリング循環のため待機中
			bool leaveAnchorNext_;				// true if should leave the anchor at the next movement
			bool leadingAnchor_;				// true if in anchor_->moveTo calling, and ignore pointMoved
			bool autoShow_;						// true if show itself when movements
			VirtualBox* box_;					// for rectangular selection. null when the selection is linear
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			bool overtypeMode_;
			bool editingByThis_;				// true if this instance is editing
			bool othersEditedFromLastInputChar_;	// このインスタンスが文字を入力して以降他の編集操作が行われたか?
			kernel::Region regionBeforeMoved_;
			std::pair<kernel::Position, kernel::Position> matchBrackets_;	// 強調表示する対括弧の位置 (無い場合 Position.INVALID_POSITION)
		};


		// inline implementations ///////////////////////////////////////////

		/**
		 * Inserts the specified text at the current position as a rectangle.
		 * @param text the text to insert
		 * @see EditPoint#insert(const String&)
		 */
		inline void VisualPoint::insertRectangle(const String& text) {insertRectangle(text.data(), text.data() + text.length());}
		/// Returns the text viewer.
		inline TextViewer& VisualPoint::textViewer() {verifyViewer(); return *viewer_;}
		/// Returns the text viewer.
		inline const TextViewer& VisualPoint::textViewer() const {verifyViewer(); return *viewer_;}
		/// Throws @c DisposedViewerException if the text viewer is already disposed.
		inline void VisualPoint::verifyViewer() const {verifyDocument(); if(viewer_ == 0) throw DisposedViewerException();}
		/// Called when the text viewer is disposed.
		inline void VisualPoint::viewerDisposed() throw() {viewer_ = 0;}
		/**
		 * Registers the listener.
		 * @param listener the listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		inline void Caret::addListener(ICaretListener& listener) {listeners_.add(listener);}
		/**
		 * Registers the character input listener.
		 * @param listener the listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		inline void Caret::addCharacterInputListener(ICharacterInputListener& listener) {characterInputListeners_.add(listener);}
		/**
		 * Registers the state listener.
		 * @param listener the listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		inline void Caret::addStateListener(ICaretStateListener& listener) {stateListeners_.add(listener);}
		/// Returns the anchor of the selection.
		inline const VisualPoint& Caret::anchor() const throw() {return *anchor_;}
		/// アンカーと自身の内、ドキュメントの先頭に近い方を返す
		inline const VisualPoint& Caret::beginning() const throw() {
			return std::min(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));}
		/**
		 * Returns the rectangle selection.
		 * @return the virtual box represents the rectangle selection
		 * @throw IllegalStateException the selection is not rectangle.
		 */
		inline const VirtualBox& Caret::boxForRectangleSelection() const {
			if(!isSelectionRectangle()) throw IllegalStateException("The selection is not rectangle.") ; return *box_;}
		/// Returns the locale identifier used to convert non-Unicode text.
		inline LCID Caret::clipboardLocale() const throw() {return clipboardLocale_;}
		/**
		 * Sets the new auto-show mode.
		 * @param enable set true to enable the mode
		 * @see #isAutoShowEnabled
		 */
		inline void Caret::enableAutoShow(bool enable /* = true */) throw() {autoShow_ = enable;}
		/// アンカーと自身の内、ドキュメントの終端に近い方を返す
		inline const VisualPoint& Caret::end() const throw() {
			return std::max(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));}
		/// Returns true if the point will be shown automatically when moved. Default is true.
		inline bool Caret::isAutoShowEnabled() const throw() {return autoShow_;}
		/// Returns true if the caret is in overtype mode.
		inline bool Caret::isOvertypeMode() const throw() {return overtypeMode_;}
		/// Returns true if the selection is empty.
		inline bool Caret::isSelectionEmpty() const throw() {return anchor_->position() == position();}
		/// Returns true if the selection is rectangle.
		inline bool Caret::isSelectionRectangle() const throw() {return box_ != 0;}
		/// キャレット位置の括弧と対応する括弧の位置を返す (@a first が対括弧、@a second がキャレット周辺の括弧)
		inline const std::pair<kernel::Position, kernel::Position>& Caret::matchBrackets() const throw() {return matchBrackets_;}
		/// Returns the matched braces tracking mode.
		inline Caret::MatchBracketsTrackingMode Caret::matchBracketsTrackingMode() const throw() {return matchBracketsTrackingMode_;}
		/**
		 * Removes the listener
		 * @param listener the listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		inline void Caret::removeListener(ICaretListener& listener) {listeners_.remove(listener);}
		/**
		 * Removes the character input listener
		 * @param listener the listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		inline void Caret::removeCharacterInputListener(ICharacterInputListener& listener) {characterInputListeners_.remove(listener);}
		/**
		 * Removes the state listener
		 * @param listener the listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		inline void Caret::removeStateListener(ICaretStateListener& listener) {stateListeners_.remove(listener);}
		/**
		 * Replaces the selected region with the specified text.
		 * @param text the text
		 * @param rectangleInsertion if set to true, @a text is inserted as rectangle
		 */
		inline void Caret::replaceSelection(const String& text, bool rectangleInsertion /* = false */) {
			replaceSelection(text.data(), text.data() + text.length(), rectangleInsertion);}
		/**
		 * Selects the specified region.
		 * @param region the region. @a pos1 member is the anchor, @a pos2 member is the caret
		 */
		inline void Caret::select(const kernel::Region& region) {select(region.first, region.second);}
		/// Returns the selection mode.
		inline Caret::SelectionMode Caret::selectionMode() const throw() {return selectionMode_;}
		/// Returns the selected region.
		inline kernel::Region Caret::selectionRegion() const throw() {return kernel::Region(*anchor_, position());}
		/**
		 * Tracks the match bracket.
		 * @param mode the tracking mode
		 */
		inline void Caret::trackMatchBrackets(MatchBracketsTrackingMode mode) throw() {
			if(mode != matchBracketsTrackingMode_) {matchBracketsTrackingMode_ = mode; checkMatchBrackets();}}

	} // namespace viewers
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
