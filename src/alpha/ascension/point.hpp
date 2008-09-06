/**
 * @file point.hpp
 * @author exeal
 * @date 2003-2008
 */

#ifndef ASCENSION_POINT_HPP
#define ASCENSION_POINT_HPP
#include "document.hpp"

namespace ascension {

	namespace kernel {

		/**
		 * Interface for objects which are interested in lifecycle of the point.
		 * @see Point#addLifeCycleListener, Point#removeLifeCycleListener, IPointListener
		 */
		class IPointLifeCycleListener {
		protected:
			/// Destructor.
			virtual ~IPointLifeCycleListener() throw() {}
		private:
			/// The point was destroyed. After this, don't call @c Point#addLifeCycleListener.
			virtual void pointDestroyed() = 0;
			friend class Point;
		};

		/**
		 * A point represents a document position and adapts to the document change.
		 *
		 * When the document change occured, @c Point moves automatically as follows:
		 *
		 * - If text was inserted or deleted before the point, the point will move accordingly.
		 * - If text was inserted or deleted after the point, the point will not move.
		 * - If region includes the point was deleted, the point will move to the start (= end) of
		 *   the region.
		 * - If text was inserted at the point, the point will or will not move according to the
		 *   gravity.
		 *
		 * For details of gravity, see the description of @c updatePosition function.
		 *
		 * When the document was reset (by @c Document#resetContent), the all points move to the
		 * start of the document.
		 *
		 * Almost all methods of this or derived classes will throw @c DisposedDocumentException if
		 * the document is already disposed. Call @c #isDocumentDisposed to check if the document
		 * is exist or not.
		 *
		 * @see Position, Document, EditPoint, viewers#VisualPoint, viewers#Caret
		 */
		class Point {
			MANAH_UNASSIGNABLE_TAG(Point);
		public:
			// constructors
			explicit Point(Document& document, const Position& position = Position());
			Point(const Point& rhs);
			virtual ~Point() throw();

			// operators
			operator Position() throw();
			operator const Position() const throw();
			bool operator==(const Point& rhs) const throw();
			bool operator!=(const Point& rhs) const throw();
			bool operator<(const Point& rhs) const throw();
			bool operator<=(const Point& rhs) const throw();
			bool operator>(const Point& rhs) const throw();
			bool operator>=(const Point& rhs) const throw();

			// core attributes
			Document* document() throw();
			const Document* document() const throw();
			bool isDocumentDisposed() const throw();
			const Position& position() const throw();

			// behaviors
			bool adaptsToDocument() const throw();
			void adaptToDocument(bool adapt) throw();
			void excludeFromRestriction(bool exclude);
			Direction gravity() const throw();
			bool isExcludedFromRestriction() const throw();
			void setGravity(Direction gravity) throw();

			// listeners
			void addLifeCycleListener(IPointLifeCycleListener& listener);
			void removeLifeCycleListener(IPointLifeCycleListener& listener);

			// short-circuits
			length_t columnNumber() const throw();
			ContentType getContentType() const;
			length_t lineNumber() const throw();

			// operations
			void moveTo(const Position& to);
			void moveTo(length_t line, length_t column);

		protected:
			Point& operator=(const Position& rhs) throw();
			void documentDisposed() throw();
			virtual void doMoveTo(const Position& to);
			virtual void normalize() const;
			virtual void update(const DocumentChange& change);
			void verifyDocument() const;

		private:
			Document* document_;
			Position position_;
			bool adapting_;
			bool excludedFromRestriction_;
			Direction gravity_;
			ascension::internal::Listeners<IPointLifeCycleListener> lifeCycleListeners_;
			friend class Document;
		};
	}

	namespace viewers {class VisualPoint;}

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
			CharacterUnit characterUnit() const throw();
			CodePoint getCodePoint(bool useLineFeed = false) const;
			bool isBeginningOfDocument() const;
			bool isBeginningOfLine() const;
			bool isEndOfDocument() const;
			bool isEndOfLine() const;
			void setCharacterUnit(CharacterUnit unit) throw();

			// movement
			void backwardCharacter(length_t offset = 1);
			void beginningOfDocument();
			void beginningOfLine();
			void endOfDocument();
			void endOfLine();
			void forwardCharacter(length_t offset = 1);
			void moveToAbsoluteCharacterOffset(length_t offset);
			bool nextBookmark();
			void nextLine(length_t offset = 1);
			bool previousBookmark();
			void previousLine(length_t offset = 1);

			// text manipulations
			void destructiveInsert(const String& text);
			void destructiveInsert(const Char* first, const Char* last);
			void erase(signed_length_t length = 1, CharacterUnit cu = DEFAULT_UNIT);
			void erase(const Position& other);
			void insert(const String& text);
			void insert(const Char* first, const Char* last);
			virtual void newLine(std::size_t newlines = 1);

		protected:
			virtual void doMoveTo(const Position& to);
			static Position getBackwardCharacterPosition(
				const Document& document, const Position& position, CharacterUnit cu, length_t offset = 1);
			static Position getForwardCharacterPosition(
				const Document& document, const Position& position, CharacterUnit cu, length_t offset = 1);
			IPointListener* getListener() const throw();
			String getText(signed_length_t length, Newline newline = NLF_RAW_VALUE) const;
			String getText(const Position& other, Newline newline = NLF_RAW_VALUE) const;
		private:
			IPointListener* listener_;
			CharacterUnit characterUnit_;
		};


		// inline implementations ///////////////////////////////////////////

		/// Conversion operator for convenience.
		inline Point::operator Position() throw() {return position_;}
		/// Conversion operator for convenience.
		inline Point::operator const Position() const throw() {return position_;}
		/**
		 * Protected assignment operator moves the point to @a rhs.
		 * @see #moveTo
		 */
		inline Point& Point::operator=(const Position& rhs) throw() {position_ = rhs; return *this;}
		/// Equality operator.
		inline bool Point::operator==(const Point& rhs) const throw() {return position() == rhs.position();}
		/// Unequality operator.
		inline bool Point::operator!=(const Point& rhs) const throw() {return !(*this == rhs);}
		/// Less-than operator.
		inline bool Point::operator<(const Point& rhs) const throw() {return position() < rhs.position();}
		/// Less-than-or-equal-to operator.
		inline bool Point::operator<=(const Point& rhs) const throw() {return *this < rhs || *this == rhs;}
		/// Greater-than operator.
		inline bool Point::operator>(const Point& rhs) const throw() {return !(*this >= rhs);}
		/// Greater-than-or-equal-to operator.
		inline bool Point::operator>=(const Point& rhs) const throw() {return !(*this > rhs);}
		/// Returns true if the point is adapting to the document change.
		inline bool Point::adaptsToDocument() const throw() {return adapting_;}
		/// Adapts the point to the document change.
		inline void Point::adaptToDocument(bool adapt) throw() {adapting_ = adapt;}
		/// Returns the column.
		inline length_t Point::columnNumber() const throw() {return position_.column;}
		/// Returns the document or @c null if the document is already disposed.
		inline Document* Point::document() throw() {return document_;}
		/// Returns the document or @c null if the document is already disposed.
		inline const Document* Point::document() const throw() {return document_;}
		/// Called when the document is disposed.
		inline void Point::documentDisposed() throw() {document_ = 0;}
		/// ...
		inline void Point::excludeFromRestriction(bool exclude) {verifyDocument(); if(excludedFromRestriction_ = exclude) normalize();}
		/// Returns the content type of the document partition contains the point.
		inline ContentType Point::getContentType() const {verifyDocument(); return document_->partitioner().contentType(*this);}
		/// Returns the gravity.
		inline Direction Point::gravity() const throw() {return gravity_;}
		/// Returns true if the document is already disposed.
		inline bool Point::isDocumentDisposed() const throw() {return document_ == 0;}
		/// Returns true if the point can't enter the inaccessible area of the document.
		inline bool Point::isExcludedFromRestriction() const throw() {return excludedFromRestriction_;}
		/// Returns the line number.
		inline length_t Point::lineNumber() const throw() {return position_.line;}
		/// Moves to the specified position.
		inline void Point::moveTo(length_t line, length_t column) {moveTo(Position(line, column));}
		/// Returns the position.
		inline const Position& Point::position() const throw() {return position_;}
		/// Sets the gravity.
		inline void Point::setGravity(Direction gravity) throw() {verifyDocument(); gravity_ = gravity;}
		/// Throws @c DisposedDocumentException if the document is already disposed.
		inline void Point::verifyDocument() const {if(isDocumentDisposed()) throw DisposedDocumentException();}

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
} // namespace ascension

#endif // !ASCENSION_POINT_HPP
