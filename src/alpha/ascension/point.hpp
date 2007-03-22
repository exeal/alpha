/**
 * @file point.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ASCENSION_POINT_HPP
#define ASCENSION_POINT_HPP
#include "layout.hpp"	// viewers.IVisualLineListener
#include "unicode.hpp"
#include "encoder.hpp"
#include <functional>	// std.mem_fun_t


namespace ascension {

	namespace viewers {
		class TextViewer;
		class VirtualBox;
		class VisualPoint;
	}

	namespace text {
		class EditPoint;

		/// A listener for @c EditPoint and @c viewers#VisualPoint.
		class IPointListener {
		private:
			/// The point is destroyed.
			virtual void pointDestroyed() = 0;
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
				CU_UTF16,				///< UTF-16 code unit.
				CU_UTF32,				///< UTF-32 code unit. A surrogate pair is treated as one character.
				CU_GRAPHEME_CLUSTER,	///< A grapheme cluster is a character.
				CU_GLYPH_CLUSTER,		///< A glyph is a character. (not implemented).
				CU_DEFAULT,				///< Default behavior (used by only @c EditPoint#erase method).
			};
			// constructors
			explicit EditPoint(Document& document, const Position& position = Position(), IPointListener* listener = 0);
			EditPoint(const EditPoint& rhs);
			virtual ~EditPoint() throw();
			// attributes
			CharacterUnit	getCharacterUnit() const throw();
			CodePoint		getCodePoint(bool useLineFeed = false) const;
			length_t		getLineLength() const throw();
			String			getText(signed_length_t length, LineBreakRepresentation lbr = LBR_PHYSICAL_DATA) const;
			String			getText(const Position& other, LineBreakRepresentation lbr = LBR_PHYSICAL_DATA) const;
			bool			isEndOfDocument() const;
			bool			isEndOfLine() const;
			bool			isStartOfDocument() const;
			bool			isStartOfLine() const;
			void			setCharacterUnit(CharacterUnit unit) throw();
			// movement
			void	charNext(length_t offset = 1);
			void	charPrev(length_t offset = 1);
			void	lineDown(length_t offset = 1);
			void	lineUp(length_t offset = 1);
			void	moveToAbsoluteCharOffset(length_t offset);
			void	moveToEndOfDocument();
			void	moveToEndOfLine();
			bool	moveToNextBookmark();
			bool	moveToPrevBookmark();
			void	moveToStartOfDocument();
			void	moveToStartOfLine();
			// text manipulations
			void			destructiveInsert(const String& text);
			void			destructiveInsert(const Char* first, const Char* last);
			void			erase(signed_length_t length = 1, CharacterUnit cu = CU_DEFAULT);
			void			erase(const Position& other);
			void			insert(const String& text);
			void			insert(const Char* first, const Char* last);
			virtual void	newLine();

		protected:
			virtual void	doMoveTo(const Position& to);
			IPointListener*	getListener() const throw();
			static Position	getNextCharPos(const EditPoint& pt, length_t length, CharacterUnit cu = CU_DEFAULT);
			static Position	getPrevCharPos(const EditPoint& pt, length_t length, CharacterUnit cu = CU_DEFAULT);
		private:
			IPointListener* listener_;
			CharacterUnit characterUnit_;	// 文字数の計算方法 (型定義参照)
		};
	}

	namespace viewers {

		/// Target @a TextViewer of @c VisualPoint has been disposed.
		class DisposedViewerException : public std::runtime_error {
		public:
			DisposedViewerException() :
				std::runtime_error("Target viewer is already inavailable. This object is no longer able to be used anyway.") {}
		};

		/**
		 * Extension of @c EditPoint for viewer and layout.
		 * @note Inherited @c LineLayoutBuffer#getLineLayout and @c LineLayoutBuffer#invalidate
		 * methods are protected in this class for internal semantics.
		 * @see text#EditPoint, text#IPointListener, text#DisposedViewException
		 */
		class VisualPoint : public text::EditPoint, public LineLayoutBuffer {
		public:
			// constructors
			explicit VisualPoint(TextViewer& viewer,
				const text::Position& position = text::Position(), text::IPointListener* listener = 0);
			VisualPoint(const VisualPoint& rhs);
			virtual ~VisualPoint() throw();
			// attributes
			static UINT			canPaste() throw();
			encodings::CodePage	getClipboardNativeEncoding() const throw();
			TextViewer&			getTextViewer();
			const TextViewer&	getTextViewer() const;
			length_t			getVisualColumnNumber() const;
			bool				isEndOfVisualLine() const;
			bool				isFirstCharOfLine() const;
			bool				isLastCharOfLine() const;
			bool				isStartOfVisualLine() const;
			void				setClipboardNativeEncoding(encodings::CodePage cp);
			// movement
			void	charLeft(length_t offset = 1);
			void	charRight(length_t offset = 1);
			void	moveToEndOfVisualLine();
			void	moveToFirstCharOfLine();
			void	moveToLastCharOfLine();
			void	moveToStartOfVisualLine();
			void	pageDown(length_t offset = 1);
			void	pageUp(length_t offset = 1);
			void	visualLineDown(length_t offset = 1);
			void	visualLineUp(length_t offset = 1);
			void	wordEndLeft(length_t offset = 1);
			void	wordEndNext(length_t offset = 1);
			void	wordEndPrev(length_t offset = 1);
			void	wordEndRight(length_t offset = 1);
			void	wordNext(length_t offset = 1);
			void	wordPrev(length_t offset = 1);
			void	wordLeft(length_t offset = 1);
			void	wordRight(length_t offset = 1);
			// scroll
			bool	recenter(signed_length_t length = 0);
			bool	recenter(const text::Position& other);
			bool	show(signed_length_t length = 0);
			bool	show(const text::Position& other);
			// text manipulations
			void			copy(signed_length_t length);
			void			copy(const text::Position& other);
			void			cut(signed_length_t length);
			void			cut(const text::Position& other);
			void			insertBox(const String& text);
			void			insertBox(const Char* first, const Char* last);
			void			newLine(bool inheritIndent);
			void			paste(signed_length_t length = 0);
			void			paste(const text::Position& other);
			text::Position	spaceIndent(const text::Position& other, bool box, long level = 1);
			text::Position	tabIndent(const text::Position& other, bool box, long level = 1);
			bool			transposeChars();
			bool			transposeLines();
//			bool			transposeParagraphs();
//			bool			transposeSentences();
			bool			transposeWords();

		protected:
			using LineLayoutBuffer::getLineLayout;	// 限定公開
			using LineLayoutBuffer::invalidate;		// 限定公開
			virtual void						doMoveTo(const text::Position& to);
			const unicode::IdentifierSyntax&	getIdentifierSyntax() const throw();
			const LineLayout&					getLayout(length_t line = -1) const;
			void								verifyViewer() const;
		private:
			using text::EditPoint::newLine;	// 明示的な隠蔽
			text::Position	doIndent(const text::Position& other, Char character, bool box, long level);
			void			layoutDeleted(length_t first, length_t last, length_t sublines) throw();
			void			layoutInserted(length_t first, length_t last) throw();
			void			layoutModified(length_t first, length_t last,
								length_t newSublines, length_t oldSublines, bool documentChanged) throw();
			void			updateLastX();
			void			viewerDisposed() throw();

		private:
			TextViewer* viewer_;
			encodings::CodePage clipboardNativeEncoding_;
			int lastX_;				// 点の、行表示領域端からの距離。行間移動時に保持しておく。-1 だと未計算
			bool crossingLines_;	// 行間移動中
			length_t visualLine_, visualSubline_;	// 点の表示行
			friend class TextViewer;
		};

		/**
		 * Interface for objects which are interested in getting informed about changes of a caret.
		 * @see IPointListener, Caret, Caret#addListener, Caret#removeListener
		 */
		class ICaretListener {
		private:
			/**
			 * The caret was moved.
			 * @param self the caret
			 * @param oldRegion the region which the caret had before. @c first is the anchor, and @c second is the caret
			 */
			virtual void caretMoved(const class Caret& self, const text::Region& oldRegion) = 0;
			/**
			 * The matched brackets are changed.
			 * @param self the caret
			 * @param oldPair the pair of the brackets previously matched
			 * @param outsideOfView the brackets newly matched are outside of the view
			 */
			virtual void matchBracketsChanged(const Caret& self,
				const std::pair<text::Position, text::Position>& oldPair, bool outsideOfView) = 0;
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
		class Caret : public VisualPoint, virtual public text::IPointListener {
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
			explicit Caret(TextViewer& viewer, const text::Position& position = text::Position());
			~Caret();
			// listeners
			void	addListener(ICaretListener& listener);
			void	removeListener(ICaretListener& listener);
			// attributes : the anchor and the caret
			void				enableAutoShow(bool enable = true) throw();
			const VisualPoint&	getAnchor() const throw();
			const VisualPoint&	getBottomPoint() const throw();
			const VisualPoint&	getTopPoint() const throw();
			bool				isAutoShowEnabled() const throw();
			// attributes : selection
			const VirtualBox&	getBoxForRectangleSelection() const;
			bool				getSelectedRangeOnLine(length_t line, length_t& first, length_t& last) const;
			bool				getSelectedRangeOnVisualLine(length_t line, length_t subline, length_t& first, length_t& last) const;
			SelectionMode		getSelectionMode() const throw();
			text::Region		getSelectionRegion() const throw();
			String				getSelectionText(text::LineBreakRepresentation lbr = text::LBR_PHYSICAL_DATA) const;
			bool				isPointOverSelection(const ::POINT& pt) const;
			bool				isSelectionEmpty() const throw();
			bool				isSelectionRectangle() const throw();
			// attributes : character input
			bool	isOvertypeMode() const throw();
			void	setOvertypeMode(bool overtype) throw();
			// attributes : matched braces
			const std::pair<text::Position, text::Position>&	getMatchBrackets() const;
			MatchBracketsTrackingMode							getMatchBracketsTrackingMode() const throw();
			void												trackMatchBrackets(MatchBracketsTrackingMode mode);
			// selection manipulations
			void	beginBoxSelection();
			void	beginLineSelection();
			void	beginWordSelection();
			void	clearSelection();
			void	copySelection(bool alsoSendToClipboardRing);
			void	cutSelection(bool alsoSendToClipboardRing);
			void	endBoxSelection();
			void	extendSelection(const text::Position& to);
			void	extendSelection(std::mem_fun_t<void, text::EditPoint>& algorithm);
			void	extendSelection(std::mem_fun_t<void, VisualPoint>& algorithm);
			void	extendSelection(std::mem_fun1_t<void, text::EditPoint, length_t>& algorithm, length_t offset);
			void	extendSelection(std::mem_fun1_t<void, VisualPoint, length_t>& algorithm, length_t offset);
			void	eraseSelection();
			void	pasteToSelection(bool fromClipboardRing);
			void	replaceSelection(const Char* first, const Char* last, bool rectangleInsertion = false);
			void	replaceSelection(const String& text, bool rectangleInsertion = false);
			void	restoreSelectionMode();
			void	select(const text::Region& region);
			void	select(const text::Position& anchor, const text::Position& caret);
			void	selectWord();
			// text manipulation
			bool	inputCharacter(CodePoint cp, bool validateSequence = true, bool blockControls = true);
			// utility
			String	getPrecedingIdentifier(length_t maxLength) const;

		private:
			void	checkMatchBrackets();
			void	doMoveTo(const text::Position& position);
			void	internalExtendSelection(void (*algorithm)(void));
			void	pointDestroyed();
			void	pointMoved(const text::EditPoint& self, const text::Position& oldPosition);
			void	update(const text::DocumentChange& change);
			using text::EditPoint::getListener;
		private:
			class SelectionAnchor : public VisualPoint {
			public:
				SelectionAnchor(TextViewer& viewer) throw() :
					VisualPoint(viewer), posBeforeUpdate_(text::Position::INVALID_POSITION) {adaptToDocument(false);}
				void beginInternalUpdate(const text::DocumentChange& change) throw() {
					assert(!isInternalUpdating()); posBeforeUpdate_ = getPosition();
					adaptToDocument(true); update(change); adaptToDocument(false);}
				void endInternalUpdate() throw() {assert(isInternalUpdating()); posBeforeUpdate_ = text::Position::INVALID_POSITION;}
				const text::Position& getPositionBeforeInternalUpdate() const throw() {assert(isInternalUpdating()); return posBeforeUpdate_;}
				bool isInternalUpdating() const throw() {return posBeforeUpdate_ != text::Position::INVALID_POSITION;}
			private:
				using Point::adaptToDocument;
				text::Position posBeforeUpdate_;
			} * anchor_;
			SelectionMode selectionMode_;
			length_t modeInitialAnchorLine_;	// 選択モードに入ったときのアンカーの行
			length_t wordSelectionChars_[2];	// 単語選択モードで最初に選択されていた単語の前後の文字位置
			ascension::internal::Listeners<ICaretListener> listeners_;
			bool pastingFromClipboardRing_;		// クリップボードリングから貼り付けた直後でリング循環のため待機中
			bool leaveAnchorNext_;				// 次の移動時にアンカーを放置
			bool leadingAnchor_;				// anchor_->moveTo 呼び出し中なので pointMoved を無視
			bool autoShow_;						// 移動時に自動的に可視化する
			VirtualBox* box_;					// 矩形選択の実装に使う。線形選択時は null
			MatchBracketsTrackingMode matchBracketsTrackingMode_;
			bool overtypeMode_;
			bool editingByThis_;				// このインスタンスが編集操作中
			bool othersEditedFromLastInputChar_;	// このインスタンスが文字を入力して以降他の編集操作が行われたか?
			std::pair<text::Position, text::Position> matchBrackets_;	// 強調表示する対括弧の位置 (無い場合 Position::INVALID_POSITION)
		};

	} // namespace viewers


// inline implementations ///////////////////////////////////////////////////

namespace text {

/**
 * Deletes the current character and inserts the specified text.
 * @param text the text to be inserted
 */
inline void EditPoint::destructiveInsert(const String& text) {destructiveInsert(text.data(), text.data() + text.length());}
/// Returns the character unit.
inline EditPoint::CharacterUnit EditPoint::getCharacterUnit() const throw() {return characterUnit_;}
/// Returns the listener.
inline IPointListener* EditPoint::getListener() const throw() {return const_cast<EditPoint*>(this)->listener_;}
/**
 * Inserts the specified text at the current position.
 * @param text the text to be inserted
 */
inline void EditPoint::insert(const String& text) {insert(text.data(), text.data() + text.length());}
/// Sets the new character unit.
inline void EditPoint::setCharacterUnit(EditPoint::CharacterUnit unit) throw() {assert(unit != CU_DEFAULT); characterUnit_ = unit;}

} // namespace text

namespace viewers {

/// Returns the text viewer.
inline viewers::TextViewer& viewers::VisualPoint::getTextViewer() {verifyViewer(); return *viewer_;}
/// Returns the text viewer.
inline const viewers::TextViewer& viewers::VisualPoint::getTextViewer() const {verifyViewer(); return *viewer_;}
/**
 * Inserts the specified text at the current position as a rectangle.
 * @param text the text to insert
 * @see EditPoint#insert(const String&)
 */
inline void viewers::VisualPoint::insertBox(const String& text) {insertBox(text.data(), text.data() + text.length());}
/// Throws @c DisposedViewerException if the text viewer is already disposed.
inline void viewers::VisualPoint::verifyViewer() const {verifyDocument(); if(viewer_ == 0) throw DisposedViewerException();}
/// Called when the text viewer is disposed.
inline void viewers::VisualPoint::viewerDisposed() throw() {viewer_ = 0;}
/**
 * Registers the listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @p listener is already registered
 */
inline void viewers::Caret::addListener(ICaretListener& listener) {listeners_.add(listener);}
/**
 * Sets the new auto-show mode.
 * @param enable set true to enable the mode
 * @see #isAutoShowEnabled
 */
inline void viewers::Caret::enableAutoShow(bool enable /* = true */) throw() {autoShow_ = enable;}
/// Returns the anchor of the selection.
inline const viewers::VisualPoint& viewers::Caret::getAnchor() const throw() {return *anchor_;}
/// アンカーと自身の内、ドキュメントの終端に近い方を返す
inline const viewers::VisualPoint& viewers::Caret::getBottomPoint() const throw() {
	return std::max(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));}
/**
 * Returns the rectangle selection.
 * @return the virtual box represents the rectangle selection
 * @throw std#logic_error the selection is not rectangle.
 */
inline const viewers::VirtualBox& viewers::Caret::getBoxForRectangleSelection() const {
	if(!isSelectionRectangle()) throw std::logic_error("The selection is not rectangle.") ; return *box_;}
/// キャレット位置の括弧と対応する括弧の位置を返す (@a first が対括弧、@a second がキャレット周辺の括弧)
inline const std::pair<text::Position, text::Position>& viewers::Caret::getMatchBrackets() const throw() {return matchBrackets_;}
/// Returns the selection mode.
inline viewers::Caret::SelectionMode viewers::Caret::getSelectionMode() const throw() {return selectionMode_;}
/// Returns the selected region.
inline text::Region viewers::Caret::getSelectionRegion() const throw() {return text::Region(*anchor_, getPosition());}
/// アンカーと自身の内、ドキュメントの先頭に近い方を返す
inline const viewers::VisualPoint& viewers::Caret::getTopPoint() const throw() {
	return std::min(static_cast<const VisualPoint&>(*this), static_cast<const VisualPoint&>(*anchor_));}
/// Returns true if the point will be shown automatically when moved. Default is true.
inline bool viewers::Caret::isAutoShowEnabled() const throw() {return autoShow_;}
/// Returns true if the caret is in overtype mode.
inline bool viewers::Caret::isOvertypeMode() const throw() {return overtypeMode_;}
/// Returns true if the selection is empty.
inline bool viewers::Caret::isSelectionEmpty() const throw() {return anchor_->getPosition() == getPosition();}
/// Returns true if the selection is rectangle.
inline bool viewers::Caret::isSelectionRectangle() const throw() {return box_ != 0;}
/// Returns the matched braces tracking mode.
inline viewers::Caret::MatchBracketsTrackingMode viewers::Caret::getMatchBracketsTrackingMode() const throw() {return matchBracketsTrackingMode_;}
/**
 * Removes the listener
 * @param listener the listener to be removed
 * @throw std#invalid_argument @p listener is not registered
 */
inline void Caret::removeListener(ICaretListener& listener) {listeners_.remove(listener);}
/**
 * Replaces the selected region with the specified text.
 * @param text the text
 * @param rectangleInsertion if set to true, @p text is inserted as rectangle
 */
inline void viewers::Caret::replaceSelection(const String& text, bool rectangleInsertion /* = false */) {
	replaceSelection(text.data(), text.data() + text.length(), rectangleInsertion);}
/**
 * Selects the specified region.
 * @param region the region. @p pos1 member is the anchor, @p pos2 member is the caret
 */
inline void viewers::Caret::select(const text::Region& region) {select(region.first, region.second);}
/**
 * Tracks the match bracket.
 * @param mode the tracking mode
 */
inline void viewers::Caret::trackMatchBrackets(MatchBracketsTrackingMode mode) throw() {
	if(mode != matchBracketsTrackingMode_) {matchBracketsTrackingMode_ = mode; checkMatchBrackets();}}

}} // namespace ascension::viewers

#endif /* !ASCENSION_POINT_HPP */
