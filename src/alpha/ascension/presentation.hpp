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

		/// Visual attributes of a text segment.
		struct TextStyle : public manah::FastArenaObject<TextStyle> {
			layout::Colors color;				///< Color of the text.
			bool bold;							///< True if the font is bold.
			bool italic;						///< True if the font is italic.
			bool strikeout;						///< True if the font is strokeout.
			layout::UnderlineStyle underlineStyle;	///< Style of the underline.
			COLORREF underlineColor;				///< Color of the underline. @c STANDARD_COLOR indicates @c color.background.
			layout::BorderStyle borderStyle;		///< Style of the border.
			COLORREF borderColor;					///< Color of the border. @c STANDARD_COLOR indicates @c color.background.
			/// Constructor.
			explicit TextStyle(const layout::Colors& textColor = layout::Colors(),
				bool boldFont = false, bool italicFont = false, bool strikeoutFont = false,
				layout::UnderlineStyle styleOfUnderline = layout::NO_UNDERLINE, COLORREF colorOfUnderline = layout::STANDARD_COLOR,
				layout::BorderStyle styleOfBorder = layout::NO_BORDER, COLORREF colorOfBorder = layout::STANDARD_COLOR) throw()
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
			virtual Priority queryLineColor(length_t line, layout::Colors& color) const = 0;
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
		class Presentation : private manah::Noncopyable,
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
			layout::Colors			getLineColor(length_t line) const;
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
		 * Creates (reconstructs) styles of the document region. This is used by
		 * @c PresentationReconstructor class to manage the styles in the specified content type.
		 * @see PresentationReconstructor#setPartitionReconstructor
		 */
		class IPartitionPresentationReconstructor {
		public:
			/// Destructor.
			virtual ~IPartitionPresentationReconstructor() throw() {}
		private:
			/**
			 * Returns the styled text segments for the specified document region.
			 * @param region the region to reconstruct the new presentation
			 * @return the presentation. the ownership will be transferred to the caller
			 */
			virtual std::auto_ptr<LineStyle> getPresentation(const text::Region& region) const throw() = 0;
			friend class PresentationReconstructor;
		};

		/// Reconstructs document presentation with single text style.
		class SingleStyledPartitionPresentationReconstructor :
			virtual public IPartitionPresentationReconstructor, private manah::Unassignable {
		public:
			explicit SingleStyledPartitionPresentationReconstructor(const TextStyle& style) throw();
		private:
			// IPartitionPresentationReconstructor
			std::auto_ptr<LineStyle>	getPresentation(const text::Region& region) const throw();
		private:
			const TextStyle style_;
		};

		/**
		 * 
		 */
		class PresentationReconstructor : virtual public ILineStyleDirector,
			virtual public text::IDocumentPartitioningListener, private manah::Unassignable {
		public:
			// constructors
			explicit PresentationReconstructor(Presentation& presentation) throw();
			~PresentationReconstructor() throw();
			// attribute
			void	setPartitionReconstructor(text::ContentType contentType,
						std::auto_ptr<IPartitionPresentationReconstructor> reconstructor);
		private:
			// ILineStyleDirector
		const LineStyle&	queryLineStyle(length_t line, bool& delegates) const;
			// IDocumentPartitioningListener
			void	documentPartitioningChanged(const text::Region& changedRegion);
		private:
			Presentation& presentation_;
			std::map<text::ContentType, IPartitionPresentationReconstructor*> reconstructors_;
		};


		/**
		 * Registers the line color director.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param director the director to register
		 * @throw NullPointerException @a director is @c null
		 */
		inline void Presentation::addLineColorDirector(ASCENSION_SHARED_POINTER<ILineColorDirector> director) {
			if(director.get() == 0) throw NullPointerException("director"); lineColorDirectors_.push_back(director);}

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
