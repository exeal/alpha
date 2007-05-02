/**
 * @file presentation.hpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_PRESENTATION_HPP
#define ASCENSION_PRESENTATION_HPP
#include "layout.hpp"

namespace ascension {

	namespace viewers {class TextViewer;}

	namespace rules {class Scanner;}

	namespace presentation {

/*		/// 特殊な定義済みトークン
		namespace specialtokens {
			const rules::Token::ID
				NORMAL_TEXT = 10,					///< 通常のテキスト
				SELECTION = 11,						///< 選択領域 (強調設定は色のみ有効)
				INACTIVE_SELECTION = 12,			///< 非アクティブ選択領域 (強調設定は色のみ有効)
				NUMERAL = 13,						///< 数字
				NUMBER = 14,						///< 数値
				MATCH_BRACKETS = 15,				///< 括弧の一致
				END_OF_LINE = 16,					///< 行の終端
				END_OF_FILE = 17,					///< ファイルの終端
				MATCH_TEXT = 18,					///< 検索パターンに一致するテキスト
				INACCESSIBLE_REGION = 19,			///< ナローイング中のアクセス不能領域 (強調設定は色のみ有効)
				WHITE_SPACE_ALTERNATIVE = 20,		///< 空白類文字の代替表示
				ASCII_CONTROL_ALTERNATIVE = 21,		///< ASCII 制御文字の代替表示
				UNICODE_CONTROL_ALTERNATIVE = 22,	///< Unicode 制御文字の代替表示
				LAST_ITEM = 22;
		}
*/
		/// Visual attributes of a text segment.
		struct TextStyle : public manah::FastArenaObject<TextStyle> {
			viewers::Colors color;				///< Color of the text.
			bool bold;							///< True if the font is bold.
			bool italic;						///< True if the font is italic.
			bool strikeout;						///< True if the font is strokeout.
			viewers::UnderlineStyle underlineStyle;	///< Style of the underline.
			COLORREF underlineColor;				///< Color of the underline. @c STANDARD_COLOR indicates @c color.background.
			viewers::BorderStyle borderStyle;		///< Style of the border.
			COLORREF borderColor;					///< Color of the border. @c STANDARD_COLOR indicates @c color.background.
			/// Constructor.
			explicit TextStyle(const viewers::Colors& textColor = viewers::Colors(),
				bool boldFont = false, bool italicFont = false, bool strikeoutFont = false,
				viewers::UnderlineStyle styleOfUnderline = viewers::NO_UNDERLINE, COLORREF colorOfUnderline = viewers::STANDARD_COLOR,
				viewers::BorderStyle styleOfBorder = viewers::NO_BORDER, COLORREF colorOfBorder = viewers::STANDARD_COLOR) throw()
				: color(textColor), bold(boldFont), italic(italicFont), strikeout(strikeoutFont),
				underlineStyle(styleOfUnderline), underlineColor(colorOfUnderline), borderStyle(styleOfBorder), borderColor(colorOfBorder) {}
		};

		/// A styled text segment.
		struct StyledText {
			length_t column;	///< Column number from which the text starts.
			TextStyle style;	///< Style of the text.
		};

		/// An array of styled segments.
		struct LineStyle {
			StyledText* array;					///< The styled segments.
			std::size_t count;					///< The number of the styled segments.
			static const LineStyle NULL_STYLE;	///< Empty styles.
		};

		/**
		 * Interface for objects which direct style of a line.
		 * @see Presentation#setLineStyleDirector
		 */
		class ILineStyleDirector {
		public:
			/// Destructor.
			virtual ~ILineStyleDirector() throw() {}
		private:
			/**
			 * Queries the style of the line.
			 * @param line the line to be queried
			 * @param[out] delegatedOwnership true if the caller must delete the returned value
			 * @return the style of the line or @c LineStyle#NULL_STYLE
			 */
			virtual const LineStyle& queryLineStyle(length_t line, bool& delegatedOwnership) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which direct color of a line.
		 * @see Presentation#addLineColorDirector
		 */
		class ILineColorDirector {
		public:
			/// Priority.
			typedef uchar Priority;
			/// Destructor.
			virtual ~ILineColorDirector() throw() {}
		private:
			/**
			 * Queries the color of the line.
			 * @param line the line to be queried
			 * @param[out] the color of the line or @c Colors#STANDARD
			 * @return the priority
			 */
			virtual Priority queryLineColor(length_t line, viewers::Colors& color) const = 0;
			friend class Presentation;
		};

		/***/
		class ITextViewerListListener {
		private:
			/***/
			virtual void textViewerListChanged(Presentation& presentation) = 0;
			friend class Presentation;
		};

		namespace internal {
			class ITextViewerCollection {
			private:
				virtual void addTextViewer(viewers::TextViewer& viewer) throw() = 0;
				virtual void removeTextViewer(viewers::TextViewer& viewer) throw() = 0;
				friend class viewers::TextViewer;
			};
		}

		/**
		 * A bridge between the document and visual styled text.
		 * @note This class is not derivable.
		 * @see Document, DocumentPartitioner, TextViewer
		 */
		class Presentation : public manah::Noncopyable,
				virtual public text::IDocumentListener, virtual public internal::ITextViewerCollection {
		public:
			typedef std::set<viewers::TextViewer*>::iterator TextViewerIterator;
			typedef std::set<viewers::TextViewer*>::const_iterator TextViewerConstIterator;
			// constructors
			explicit Presentation(text::Document& document) throw();
			// attributes
			void					addLineColorDirector(ASCENSION_SHARED_POINTER<ILineColorDirector> director);
			void					addTextViewerListListener(ITextViewerListListener& listener);
			text::Document&			getDocument() throw();
			const text::Document&	getDocument() const throw();
			viewers::Colors			getLineColor(length_t line) const;
			const LineStyle&		getLineStyle(length_t line, bool& delegatedOwnership) const;
			void					removeLineColorDirector(ILineColorDirector& director) throw();
			void					removeTextViewerListListener(ITextViewerListListener& listener);
			void					setLineStyleDirector(ASCENSION_SHARED_POINTER<ILineStyleDirector> newDirector) throw();
			// enumeration
			TextViewerIterator		getFirstTextViewer() throw();
			TextViewerConstIterator	getFirstTextViewer() const throw();
			TextViewerIterator		getLastTextViewer() throw();
			TextViewerConstIterator	getLastTextViewer() const throw();
			std::size_t				getNumberOfTextViewers() const throw();
		private:
			// IDocumentListener
			void	documentAboutToBeChanged(const text::Document& document);
			void	documentChanged(const text::Document& document, const text::DocumentChange& change);
			// internal::ITextViewerCollection
			void	addTextViewer(viewers::TextViewer& viewer) throw();
			void	removeTextViewer(viewers::TextViewer& viewer) throw();
		private:
			text::Document& document_;
			std::set<viewers::TextViewer*> textViewers_;
			ASCENSION_SHARED_POINTER<ILineStyleDirector> lineStyleDirector_;
			std::list<ASCENSION_SHARED_POINTER<ILineColorDirector> > lineColorDirectors_;
			ascension::internal::Listeners<ITextViewerListListener> textViewerListListeners_;
		};


		/**
		 * Registers the line color director.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param director the director to register
		 * @throw std#invalid_argument @a director is @c null
		 */
		inline void Presentation::addLineColorDirector(ASCENSION_SHARED_POINTER<ILineColorDirector> director) {
			if(director.get() == 0) throw std::invalid_argument("the director is null."); lineColorDirectors_.push_back(director);}

		/**
		 * Registers the text viewer list listener.
		 * @param listener the listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		inline void Presentation::addTextViewerListListener(ITextViewerListListener& listener) {textViewerListListeners_.add(listener);}

		/// Returns the number of text viewers.
		inline std::size_t Presentation::getNumberOfTextViewers() const throw() {return textViewers_.size();}

		/**
		 * Removes the specified line color director.
		 * @param director the director to remove
		 */
		inline void Presentation::removeLineColorDirector(ILineColorDirector& director) throw() {
			for(std::list<ASCENSION_SHARED_POINTER<ILineColorDirector> >::iterator
					i(lineColorDirectors_.begin()), e(lineColorDirectors_.end()); i != e; ++i) {
				if(i->get() == &director) {lineColorDirectors_.erase(i); return;}
			}
		}

		/**
		 * Removes the text viewer list listener.
		 * @param listener the listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		inline void Presentation::removeTextViewerListListener(ITextViewerListListener& listener) {textViewerListListeners_.remove(listener);}

		/**
		 * Sets the line style director.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param newDirector the director. @c null to unregister
		 * @param delegateOwnership set true to transfer the ownership of @a newDirector to the callee
		 */
		inline void Presentation::setLineStyleDirector(ASCENSION_SHARED_POINTER<ILineStyleDirector> newDirector)
			throw() {lineStyleDirector_ = newDirector;}

}} // namespace ascension::presentation

#endif /* !ASCENSION_PRESENTATION_HPP */
