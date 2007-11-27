/**
 * @file point.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ASCENSION_POINT_HPP
#define ASCENSION_POINT_HPP
#include "layout.hpp"	// layout.IVisualLinesListener
#include "unicode.hpp"
#include "encoder.hpp"
#include <functional>	// std.mem_fun_t


namespace ascension {

	namespace viewers {
		class TextViewer;
		class VirtualBox;
		class VisualPoint;
	}

	namespace kernel {
		class EditPoint;

		/**
		 * A listener for @c EditPoint and @c viewers#VisualPoint.
		 * @see IPointLifeCycleListener, viewers#ICaretListener
		 */
		class IPointListener {
		private:
			/**
			 * The point was moved.
			 * @param self the point
			 * @param oldPosition the position from which the point moved
			 */
			virtual void pointMoved(const EditPoint& self, const Position& oldPosition) = 0;
			friend class EditPoint;
			friend class ascension::viewers::VisualPoint;
		};

		/**
		 * Extension of @c Point. Editable and movable in the document.
		 *
		 * @c Viewer のクライアントは選択範囲やキャレットを位置情報としてテキストを編集できるが、
		 * このクラスにより任意の場所の編集が可能となる。クライアントは点を編集箇所に移動させ、
		 * @c Viewer の操作と似た方法で編集を行う
		 *
		 * 編集点は他の編集操作でその位置が変更される。親クラスの @c Point を見よ
		 *
		 * 文字単位、単語単位で編集点の移動を行うメソッドのうち、名前が @c Left 及び @c Right
		 * で終わっているものは、論理順ではなく、視覚上の方向を指定する。
		 * 具体的にはビューのテキスト方向が左から右であれば @c xxxxLeft は @c xxxxPrev に、方向が右から左であれば
		 * @c xxxxLeft は @c xxxxNext にマップされる。これら視覚上の方向をベースにしたメソッドは、
		 * キーボードなどのユーザインターフェイスから移動を行うことを考えて提供されている
		 *
		 * EditPoint <strong>never</strong> uses sequential edit of the document and freeze of the
		 * viewer. Client is responsible for the usage of these features.
		 *
		 * @see Point, Document, IPointListener, DisposedDocumentException
		 */
		class EditPoint : public Point {
		public:
			/// Character unit defines what is one character.
			enum CharacterUnit {
				UTF16_CODE_UNIT,	///< UTF-16 code unit.
				UTF32_CODE_UNIT,	///< UTF-32 code unit. A surrogate pair is treated as one character.
				GRAPHEME_CLUSTER,	///< A grapheme cluster is a character.
				GLYPH_CLUSTER,		///< A glyph is a character. (not implemented).
				DEFAULT_UNIT,		///< Default behavior (used by only @c EditPoint#erase method).
			};
			// constructors
			explicit EditPoint(Document& document, const Position& position = Position(), IPointListener* listener = 0);
			EditPoint(const EditPoint& rhs);
			virtual ~EditPoint() throw();
			// attributes
			CharacterUnit	characterUnit() const throw();
			CodePoint		getCodePoint(bool useLineFeed = false) const;
			bool			isBeginningOfDocument() const;
			bool			isBeginningOfLine() const;
			bool			isEndOfDocument() const;
			bool			isEndOfLine() const;
			void			setCharacterUnit(CharacterUnit unit) throw();
			// movement
			void	backwardCharacter(length_t offset = 1);
			void	beginningOfDocument();
			void	beginningOfLine();
			void	endOfDocument();
			void	endOfLine();
			void	forwardCharacter(length_t offset = 1);
			void	moveToAbsoluteCharacterOffset(length_t offset);
			bool	nextBookmark();
			void	nextLine(length_t offset = 1);
			bool	previousBookmark();
			void	previousLine(length_t offset = 1);
			// text manipulations
			void			destructiveInsert(const String& text);
			void			destructiveInsert(const Char* first, const Char* last);
			void			erase(signed_length_t length = 1, CharacterUnit cu = DEFAULT_UNIT);
			void			erase(const Position& other);
			void			insert(const String& text);
			void			insert(const Char* first, const Char* last);
			virtual void	newLine();

		protected:
			virtual void	doMoveTo(const Position& to);
			IPointListener*	getListener() const throw();
			String			getText(signed_length_t length, NewlineRepresentation nlr = NLR_PHYSICAL_DATA) const;
			String			getText(const Position& other, NewlineRepresentation nlr = NLR_PHYSICAL_DATA) const;
		private:
			IPointListener* listener_;
			CharacterUnit characterUnit_;
		};
	}

	namespace viewers {

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
			static ::UINT		canPaste() throw();
			encoding::MIBenum	clipboardNativeEncoding() const throw();
			bool				isEndOfVisualLine() const;
			bool				isFirstPrintableCharacterOfLine() const;
			bool				isFirstPrintableCharacterOfVisualLine() const;
			bool				isLastPrintableCharacterOfLine() const;
			bool				isLastPrintableCharacterOfVisualLine() const;
			bool				isBeginningOfVisualLine() const;
			void				setClipboardNativeEncoding(encoding::MIBenum mib);
			TextViewer&			textViewer();
			const TextViewer&	textViewer() const;
			length_t			visualColumnNumber() const;
			// movement
			void	beginningOfVisualLine();
			void	endOfVisualLine();
			void	firstPrintableCharacterOfLine();
			void	firstPrintableCharacterOfVisualLine();
			void	lastPrintableCharacterOfLine();
			void	lastPrintableCharacterOfVisualLine();
			void	leftCharacter(length_t offset = 1);
			void	leftWord(length_t offset = 1);
			void	leftWordEnd(length_t offset = 1);
			void	nextPage(length_t offset = 1);
			void	nextVisualLine(length_t offset = 1);
			void	nextWord(length_t offset = 1);
			void	nextWordEnd(length_t offset = 1);
			void	previousPage(length_t offset = 1);
			void	previousVisualLine(length_t offset = 1);
			void	previousWord(length_t offset = 1);
			void	previousWordEnd(length_t offset = 1);
			void	rightCharacter(length_t offset = 1);
			void	rightWord(length_t offset = 1);
			void	rightWordEnd(length_t offset = 1);
			// scroll
			bool	recenter(signed_length_t length = 0);
			bool	recenter(const kernel::Position& other);
			bool	show(signed_length_t length = 0);
			bool	show(const kernel::Position& other);
			// text manipulations
			void				copy(signed_length_t length);
			void				copy(const kernel::Position& other);
			void				cut(signed_length_t length);
			void				cut(const kernel::Position& other);
			void				insertBox(const String& text);
			void				insertBox(const Char* first, const Char* last);
			void				newLine(bool inheritIndent);
			void				paste(signed_length_t length = 0);
			void				paste(const kernel::Position& other);
			kernel::Position	spaceIndent(const kernel::Position& other, bool box, long level = 1);
			kernel::Position	tabIndent(const kernel::Position& other, bool box, long level = 1);
			bool				transposeCharacters();
			bool				transposeLines();
//			bool				transposeParagraphs();
//			bool				transposeSentences();
			bool				transposeWords();

		protected:
			virtual void					doMoveTo(const kernel::Position& to);
			const text::IdentifierSyntax&	identifierSyntax() const throw();
			void							verifyViewer() const;
		private:
			using kernel::EditPoint::newLine;	// 明示的な隠蔽
			kernel::Position	doIndent(const kernel::Position& other, Char character, bool box, long level);
			void				updateLastX();
			void				viewerDisposed() throw();
			// layout.IVisualLinesListener
			void	visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) throw();
			void	visualLinesInserted(length_t first, length_t last) throw();
			void	visualLinesModified(length_t first, length_t last,
						signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) throw();

		private:
			TextViewer* viewer_;
			encoding::MIBenum clipboardNativeEncoding_;
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

		/**
		 * @c Caret is an extension of @c VisualPoint. A caret has a selection on the text viewer.
		 * And supports line selection, word selection, rectangle (box) selection, tracking match
		 * brackets, and clipboard enhancement.
		 *
		 * A caret has one another point called "anchor" (or "mark"). The selection is a region
		 * between the caret and the anchor. Anchor is @c VisualPoint but client can't operate
		 * this directly.
		 *
		 * Usually, the anchor will move adapting to the caret automatically. If you want to move
		 * the anchor isolately, create the selection by using @c #select method or call
		 * @c #extendSelection method.
		 *
		 * When the caret moves, the text viewer will scroll automatically to show the caret. See
		 * the description of @c #enableAutoShow and @c #isAutoShowEnabled.
		 *
		 * このクラスの編集用のメソッドは @c EditPoint 、@c VisualPoint の編集用メソッドと異なり、
		 * 積極的に連続編集とビューの凍結を使用する
		 *
		 * 行選択および単語選択は、選択の作成および拡張時にアンカーとキャレットを行境界や単語境界に束縛する機能で、
		 * @c #extendSelection メソッドで実際にこれらの点が移動する位置を制限する。
		 * また、この場合 @c #extendSelection を呼び出すとアンカーが自動的に移動する。
		 * @c #beginLineSelection 、@c #beginWordSelection でこれらのモードに入ることができ、
		 * @c #restoreSelectionMode で通常状態に戻ることができる。
		 * また、これらのモードで @c #moveTo か @c #select を使っても通常状態に戻る
		 *
		 * 対括弧の検索はプログラムを編集しているときに役立つ機能で、キャレット位置に括弧があれば対応する括弧を検索する。
		 * 括弧のペアを強調表示するのは、現時点ではビューの責任である
		 *
		 * To enter rectangle selection mode, call @c #beginBoxSelection method. To exit, call
		 * @c #endBoxSelection method. You can get the information of the current rectangle
		 * selection by using @c #getBoxForRectangleSelection method.
		 *
		 * This class does not accept @c IPointListener. Use @c ICaretListener interface instead.
		 *
		 * @note This class is not intended to subclass.
		 */
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
			void	addListener(ICaretListener& listener);
			void	addCharacterInputListener(ICharacterInputListener& listener);
			void	addStateListener(ICaretStateListener& listener);
			void	removeListener(ICaretListener& listener);
			void	removeCharacterInputListener(ICharacterInputListener& listener);
			void	removeStateListener(ICaretStateListener& listener);
			// attributes : the anchor and the caret
			const VisualPoint&	anchor() const throw();
			const VisualPoint&	beginning() const throw();
			void				enableAutoShow(bool enable = true) throw();
			const VisualPoint&	end() const throw();
			bool				isAutoShowEnabled() const throw();
			// attributes : selection
			const VirtualBox&	boxForRectangleSelection() const;
			bool				isPointOverSelection(const ::POINT& pt) const;
			bool				isSelectionEmpty() const throw();
			bool				isSelectionRectangle() const throw();
			bool				selectedRangeOnLine(length_t line, length_t& first, length_t& last) const;
			bool				selectedRangeOnVisualLine(length_t line, length_t subline, length_t& first, length_t& last) const;
			SelectionMode		selectionMode() const throw();
			kernel::Region		selectionRegion() const throw();
			String				selectionText(kernel::NewlineRepresentation nlr = kernel::NLR_PHYSICAL_DATA) const;
			// attributes : character input
			bool	isOvertypeMode() const throw();
			void	setOvertypeMode(bool overtype) throw();
			// attributes : matched braces
			const std::pair<kernel::Position, kernel::Position>&	matchBrackets() const;
			MatchBracketsTrackingMode								matchBracketsTrackingMode() const throw();
			void													trackMatchBrackets(MatchBracketsTrackingMode mode);
			// selection manipulations
			void	beginBoxSelection();
			void	beginLineSelection();
			void	beginWordSelection();
			void	clearSelection();
			void	copySelection(bool alsoSendToClipboardRing);
			void	cutSelection(bool alsoSendToClipboardRing);
			void	endBoxSelection();
			void	extendSelection(const kernel::Position& to);
			void	extendSelection(std::mem_fun_t<void, kernel::EditPoint>& algorithm);
			void	extendSelection(std::mem_fun_t<void, VisualPoint>& algorithm);
			void	extendSelection(std::mem_fun1_t<void, kernel::EditPoint, length_t>& algorithm, length_t offset);
			void	extendSelection(std::mem_fun1_t<void, VisualPoint, length_t>& algorithm, length_t offset);
			void	eraseSelection();
			void	pasteToSelection(bool fromClipboardRing);
			void	replaceSelection(const Char* first, const Char* last, bool rectangleInsertion = false);
			void	replaceSelection(const String& text, bool rectangleInsertion = false);
			void	restoreSelectionMode();
			void	select(const kernel::Region& region);
			void	select(const kernel::Position& anchor, const kernel::Position& caret);
			void	selectWord();
			// text manipulation
			bool	inputCharacter(CodePoint cp, bool validateSequence = true, bool blockControls = true);

		private:
			void	checkMatchBrackets();
			void	internalExtendSelection(void (*algorithm)(void));
			void	update(const kernel::DocumentChange& change);
			void	updateVisualAttributes();
			// VisualPoint
			void	doMoveTo(const kernel::Position& position);
			// kernel.IPointListener
			void	pointMoved(const kernel::EditPoint& self, const kernel::Position& oldPosition);
			// kernel.IDocumentListener
			bool	documentAboutToBeChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			void	documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
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
			length_t modeInitialAnchorLine_;	// 選択モードに入ったときのアンカーの行
			length_t wordSelectionChars_[2];	// 単語選択モードで最初に選択されていた単語の前後の文字位置
			ascension::internal::Listeners<ICaretListener> listeners_;
			ascension::internal::Listeners<ICharacterInputListener> characterInputListeners_;
			ascension::internal::Listeners<ICaretStateListener> stateListeners_;
			bool pastingFromClipboardRing_;		// クリップボードリングから貼り付けた直後でリング循環のため待機中
			bool leaveAnchorNext_;				// true if should leave the anchor at the next movement
			bool leadingAnchor_;				// anchor_->moveTo 呼び出し中なので pointMoved を無視
			bool autoShow_;						// true if show itself when movements
			VirtualBox* box_;					// for rectangular selection. null when the selection is linear
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			bool overtypeMode_;
			bool editingByThis_;				// このインスタンスが編集操作中
			bool othersEditedFromLastInputChar_;	// このインスタンスが文字を入力して以降他の編集操作が行われたか?
			kernel::Region regionBeforeMoved_;
			std::pair<kernel::Position, kernel::Position> matchBrackets_;	// 強調表示する対括弧の位置 (無い場合 Position.INVALID_POSITION)
		};

	} // namespace viewers


// inline implementations ///////////////////////////////////////////////////

namespace kernel {

/// Returns the character unit.
inline EditPoint::CharacterUnit EditPoint::characterUnit() const throw() {return characterUnit_;}
/**
 * Deletes the current character and inserts the specified text.
 * @param text the text to be inserted
 */
inline void EditPoint::destructiveInsert(const String& text) {destructiveInsert(text.data(), text.data() + text.length());}
/// Returns the listener.
inline IPointListener* EditPoint::getListener() const throw() {return const_cast<EditPoint*>(this)->listener_;}
/**
 * Inserts the specified text at the current position.
 * @param text the text to be inserted
 */
inline void EditPoint::insert(const String& text) {insert(text.data(), text.data() + text.length());}
/// Sets the new character unit.
inline void EditPoint::setCharacterUnit(EditPoint::CharacterUnit unit) throw() {assert(unit != DEFAULT_UNIT); characterUnit_ = unit;}

} // namespace kernel

namespace viewers {

/**
 * Inserts the specified text at the current position as a rectangle.
 * @param text the text to insert
 * @see EditPoint#insert(const String&)
 */
inline void VisualPoint::insertBox(const String& text) {insertBox(text.data(), text.data() + text.length());}
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

}} // namespace ascension.viewers

#endif /* !ASCENSION_POINT_HPP */
