/**
 * @file presentation.hpp
 * Provides classes define appearance and presentation of a text editor user interface.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2011
 */

#ifndef ASCENSION_PRESENTATION_HPP
#define ASCENSION_PRESENTATION_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/kernel/document.hpp>

namespace ascension {

	namespace graphics {class Color;}

	namespace rules {class URIDetector;}

	namespace presentation {

		struct TextLineStyle;

		/**
		 * Interface for objects which direct style of a text line.
		 * @see Presentation#setTextLineStyleDirector
		 */
		class TextLineStyleDirector {
		public:
			/// Destructor.
			virtual ~TextLineStyleDirector() /*throw()*/ {}
		private:
			/**
			 * Queries the style of the text line.
			 * @param line The line to be queried
			 * @return The style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::tr1::shared_ptr<const TextLineStyle> queryTextLineStyle(length_t line) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which direct color of a text line.
		 * @see Presentation#addTextLineColorDirector
		 */
		class TextLineColorDirector {
		public:
			/// Priority.
			typedef uint8_t Priority;
			/// Destructor.
			virtual ~TextLineColorDirector() /*throw()*/ {}
		private:
			/**
			 * Returns the foreground and background colors of the text line.
			 * @param line The line to be queried
			 * @param[out] foreground The foreground color of the text line. If this is invalid
			 *                        color, line color is not set
			 * @param[out] background The background color of the text line. If this is invalid
			 *                        color, line color is not set
			 * @return the priority
			 */
			virtual Priority queryTextLineColors(length_t line,
				graphics::Color& foreground, graphics::Color& background) const = 0;
			friend class Presentation;
		};

		struct TextRunStyle;

		/**
		 * Interface for objects which are interested in change of the default style of
		 * @c Presentation.
		 * @see Presentation#addDefaultStyleListener, Presentation#removeDefaultStyleListener
		 */
		class DefaultTextStyleListener {
		public:
			/// Destructor.
			virtual ~DefaultTextStyleListener() /*throw()*/ {}
			/**
			 * The default text line style of @c Presentation was changed.
			 * @param used The old style used previously
			 * @see Presentation#defaultTextLineStyle, Presentation#setDefaultTextLineStyle
			 */
			virtual void defaultTextLineStyleChanged(std::tr1::shared_ptr<const TextLineStyle> used) = 0;
			/**
			 * The default text run style of @c Presentation was changed.
			 * @param used The old style used previously
			 * @see Presentation#defaultTextRunStyle, Presentation#setDefaultTextRunStyle
			 */
			virtual void defaultTextRunStyleChanged(std::tr1::shared_ptr<const TextRunStyle> used) = 0;
		};

		/**
		 * Provides support for detecting and presenting hyperlinks in text editors. "Hyperlink" is
		 * invokable text segment in the document.
		 * @see Presentation#getHyperlinks, Presentation#setHyperlinkDetector
		 */
		namespace hyperlink {
			/// Represents a hyperlink.
			class Hyperlink {
			public:
				/// Destructor.
				virtual ~Hyperlink() /*throw()*/ {}
				/// Returns the descriptive text of the hyperlink.
				virtual String description() const /*throw()*/ = 0;
				/// Invokes the hyperlink.
				virtual void invoke() const /*throw()*/ = 0;
				/// Returns the columns of the region of the hyperlink.
				const Range<length_t>& region() const /*throw()*/ {return region_;}
			protected:
				/// Protected constructor takes the region of the hyperlink.
				explicit Hyperlink(const Range<length_t>& region) /*throw()*/ : region_(region) {}
			private:
				const Range<length_t> region_;
			};

			/// A @c HyperlinkDetector finds the hyperlinks in the document.
			class HyperlinkDetector {
			public:
				/// Destructor.
				virtual ~HyperlinkDetector() /*throw()*/ {}
				/**
				 * Returns the next hyperlink in the specified text line.
				 * @param document The document
				 * @param line The line number
				 * @param range The column range in the line to search. @a range.beginning() can be
				 *              the beginning of the found hyperlink
				 * @return The found hyperlink, or @c null if not found
				 */
				virtual std::auto_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/ = 0;
			};

			/**
			 * URI hyperlink detector.
			 * @see rules#URIDetector, rules#URIRule
			 * @note This class is not intended to be subclassed.
			 */
			class URIHyperlinkDetector : public HyperlinkDetector {
			public:
				URIHyperlinkDetector(const rules::URIDetector& uriDetector, bool delegateOwnership) /*throw()*/;
				~URIHyperlinkDetector() /*throw()*/;
				// HyperlinkDetector
				std::auto_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/;
			private:
				detail::StrategyPointer<const rules::URIDetector> uriDetector_;
			};

			/**
			 * @note This class is not intended to be subclassed.
			 */
			class CompositeHyperlinkDetector : public hyperlink::HyperlinkDetector {
			public:
				~CompositeHyperlinkDetector() /*throw()*/;
				void setDetector(kernel::ContentType contentType, std::auto_ptr<hyperlink::HyperlinkDetector> detector);
				// hyperlink.HyperlinkDetector
				std::auto_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/;
			private:
				std::map<kernel::ContentType, hyperlink::HyperlinkDetector*> composites_;
			};
		} // namespace hyperlink

		class StyledTextRunIterator;
		class TextRunStyleDirector;

		/**
		 * A bridge between the document and visual styled text.
		 * @note This class is not intended to be subclassed.
		 * @see kernel#Document, kernel#DocumentPartitioner
		 */
		class Presentation : public kernel::DocumentListener {
			ASCENSION_NONCOPYABLE_TAG(Presentation);
		public:
			// constructors
			explicit Presentation(kernel::Document& document) /*throw()*/;
			~Presentation() /*throw()*/;
			// attributes
			kernel::Document& document() /*throw()*/;
			const kernel::Document& document() const /*throw()*/;
			const hyperlink::Hyperlink* const* getHyperlinks(length_t line, std::size_t& numberOfHyperlinks) const;
			// styles
			void addDefaultTextStyleListener(DefaultTextStyleListener& listener);
			std::tr1::shared_ptr<const TextLineStyle> defaultTextLineStyle() const /*throw()*/;
			std::tr1::shared_ptr<const TextRunStyle> defaultTextRunStyle() const /*throw()*/;
			void removeDefaultTextStyleListener(DefaultTextStyleListener& listener);
			void setDefaultTextLineStyle(std::tr1::shared_ptr<const TextLineStyle> newStyle);
			void setDefaultTextRunStyle(std::tr1::shared_ptr<const TextRunStyle> newStyle);
			void textLineColors(length_t line, graphics::Color& foreground, graphics::Color& background) const;
			std::tr1::shared_ptr<const TextLineStyle> textLineStyle(length_t line) const;
			std::auto_ptr<StyledTextRunIterator> textRunStyles(length_t line) const;
			// strategies
			void addTextLineColorDirector(std::tr1::shared_ptr<TextLineColorDirector> director);
			void removeTextLineColorDirector(TextLineColorDirector& director) /*throw()*/;
			void setHyperlinkDetector(hyperlink::HyperlinkDetector* newDetector, bool delegateOwnership) /*throw()*/;
			void setTextLineStyleDirector(std::tr1::shared_ptr<TextLineStyleDirector> newDirector) /*throw()*/;
			void setTextRunStyleDirector(std::tr1::shared_ptr<TextRunStyleDirector> newDirector) /*throw()*/;
		private:
			void clearHyperlinksCache() /*throw()*/;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
		private:
			kernel::Document& document_;
			static std::tr1::shared_ptr<const TextLineStyle> DEFAULT_TEXT_LINE_STYLE;
			static std::tr1::shared_ptr<const TextRunStyle> DEFAULT_TEXT_RUN_STYLE;
			std::tr1::shared_ptr<const TextLineStyle> defaultTextLineStyle_;
			std::tr1::shared_ptr<const TextRunStyle> defaultTextRunStyle_;
			std::tr1::shared_ptr<TextLineStyleDirector> textLineStyleDirector_;
			std::tr1::shared_ptr<TextRunStyleDirector> textRunStyleDirector_;
			std::list<std::tr1::shared_ptr<TextLineColorDirector> > textLineColorDirectors_;
			detail::Listeners<DefaultTextStyleListener> defaultTextStyleListeners_;
			detail::StrategyPointer<hyperlink::HyperlinkDetector> hyperlinkDetector_;
			struct Hyperlinks;
			mutable std::list<Hyperlinks*> hyperlinks_;
		};


		/**
		 * Registers the text line color director.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param director the director to register
		 * @throw NullPointerException @a director is @c null
		 */
		inline void Presentation::addTextLineColorDirector(std::tr1::shared_ptr<TextLineColorDirector> director) {
			if(director.get() == 0) throw NullPointerException("director"); textLineColorDirectors_.push_back(director);}

		/// Returns the default text line style this object gives.
		inline std::tr1::shared_ptr<const TextLineStyle> Presentation::defaultTextLineStyle() const /*throw()*/ {return defaultTextLineStyle_;}

		/// Returns the default text run style this object gives.
		inline std::tr1::shared_ptr<const TextRunStyle> Presentation::defaultTextRunStyle() const /*throw()*/ {return defaultTextRunStyle_;}

		/// Returns the style of the specified text line.
		inline std::tr1::shared_ptr<const TextLineStyle> Presentation::textLineStyle(length_t line) const /*throw()*/ {
			std::tr1::shared_ptr<const TextLineStyle> style;
			if(textLineStyleDirector_.get() != 0)
				style = textLineStyleDirector_->queryTextLineStyle(line);
			return (style.get() != 0) ? style : defaultTextLineStyle();
		}

		/**
		 * Removes the specified text line color director.
		 * @param director the director to remove
		 */
		inline void Presentation::removeTextLineColorDirector(TextLineColorDirector& director) /*throw()*/ {
			for(std::list<std::tr1::shared_ptr<TextLineColorDirector> >::iterator
					i(textLineColorDirectors_.begin()), e(textLineColorDirectors_.end()); i != e; ++i) {
				if(i->get() == &director) {textLineColorDirectors_.erase(i); return;}
			}
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_HPP
