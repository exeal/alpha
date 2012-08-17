/**
 * @file presentation.hpp
 * Provides classes define appearance and presentation of a text editor user interface.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2012
 */

#ifndef ASCENSION_PRESENTATION_HPP
#define ASCENSION_PRESENTATION_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/kernel/document.hpp>

namespace ascension {

	namespace graphics {
		class Color;
		namespace font {
			struct ComputedTextLineStyle;
		}
	}

	namespace rules {class URIDetector;}

	namespace presentation {

		struct TextLineStyle;
		struct TextToplevelStyle;
		template<typename T> class Inheritable;

		/**
		 * Interface for objects which direct style of a text line.
		 * @see TextLineStyle, Presentation#setTextLineStyleDirector
		 */
		class TextLineStyleDirector {
		public:
			/// Destructor.
			virtual ~TextLineStyleDirector() /*noexcept*/ {}
		private:
			/**
			 * Queries the style of the text line.
			 * @param line The line to be queried
			 * @return The style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::shared_ptr<const TextLineStyle> queryTextLineStyle(Index line) const = 0;
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
			virtual ~TextLineColorDirector() /*noexcept*/ {}
		private:
			/**
			 * Returns the foreground and background colors of the text line.
			 * @param line The line to be queried
			 * @param[out] foreground The foreground color of the text line. If @c boost#none,
			 *                        line color is not set
			 * @param[out] background The background color of the text line. If @c boost#none,
			 *                        line color is not set
			 * @return the priority
			 */
			virtual Priority queryTextLineColors(Index line,
				boost::optional<graphics::Color>& foreground,
				boost::optional<graphics::Color>& background) const = 0;
			friend class Presentation;
		};

		struct TextRunStyle;

		/**
		 * Interface for objects which are interested in change of the global text style of
		 * @c Presentation.
		 * @see Presentation#addGlobalTextStyleListener, Presentation#removeGlobalTextStyleListener
		 */
		class GlobalTextStyleListener {
		public:
			/// Destructor.
			virtual ~GlobalTextStyleListener() /*throw()*/ {}
			/**
			 * The global text style of @c Presentation was changed.
			 * @param used The old style used previously
			 * @see Presentation#globalTextStyle, Presentation#setGlobalTextStyle
			 */
			virtual void globalTextStyleChanged(std::shared_ptr<const TextToplevelStyle> used) = 0;
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
				const Range<Index>& region() const /*throw()*/ {return region_;}
			protected:
				/// Protected constructor takes the region of the hyperlink.
				explicit Hyperlink(const Range<Index>& region) /*throw()*/ : region_(region) {}
			private:
				const Range<Index> region_;
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
				 * @param range The range of offsets in the line to search. @a range.beginning()
				 *              can be the beginning of the found hyperlink
				 * @return The found hyperlink, or @c null if not found
				 */
				virtual std::unique_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, Index line, const Range<Index>& range) const /*throw()*/ = 0;
			};

			/**
			 * URI hyperlink detector.
			 * @see rules#URIDetector, rules#URIRule
			 * @note This class is not intended to be subclassed.
			 */
			class URIHyperlinkDetector : public HyperlinkDetector {
			public:
				URIHyperlinkDetector(std::shared_ptr<const rules::URIDetector> uriDetector) /*throw()*/;
				~URIHyperlinkDetector() /*throw()*/;
				// HyperlinkDetector
				std::unique_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, Index line, const Range<Index>& range) const /*throw()*/;
			private:
				std::shared_ptr<const rules::URIDetector> uriDetector_;
			};

			/**
			 * @note This class is not intended to be subclassed.
			 */
			class CompositeHyperlinkDetector : public hyperlink::HyperlinkDetector {
			public:
				~CompositeHyperlinkDetector() /*throw()*/;
				void setDetector(kernel::ContentType contentType, std::unique_ptr<hyperlink::HyperlinkDetector> detector);
				// hyperlink.HyperlinkDetector
				std::unique_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, Index line, const Range<Index>& range) const /*throw()*/;
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
			explicit Presentation(kernel::Document& document) /*noexcept*/;
			~Presentation() /*noexcept*/;
			// attributes
			kernel::Document& document() /*noexcept*/;
			const kernel::Document& document() const /*noexcept*/;
			const hyperlink::Hyperlink* const* getHyperlinks(Index line, std::size_t& numberOfHyperlinks) const;
			// styles
			void addGlobalTextStyleListener(GlobalTextStyleListener& listener);
			const TextToplevelStyle& globalTextStyle() const /*noexcept*/;
			void removeGlobalTextStyleListener(GlobalTextStyleListener& listener);
			void setGlobalTextStyle(std::shared_ptr<const TextToplevelStyle> newStyle);
			void textLineColors(Index line,
				boost::optional<graphics::Color>& foreground, boost::optional<graphics::Color>& background) const;
			TextLineStyle&& textLineStyle(Index line) const;
			std::unique_ptr<StyledTextRunIterator> textRunStyles(Index line) const;
			// strategies
			void addTextLineColorDirector(std::shared_ptr<TextLineColorDirector> director);
			void removeTextLineColorDirector(TextLineColorDirector& director) /*noexcept*/;
			void setHyperlinkDetector(std::shared_ptr<hyperlink::HyperlinkDetector> newDetector) /*noexcept*/;
			void setTextLineStyleDirector(std::shared_ptr<TextLineStyleDirector> newDirector) /*noexcept*/;
			void setTextRunStyleDirector(std::shared_ptr<TextRunStyleDirector> newDirector) /*noexcept*/;
		private:
			void clearHyperlinksCache() /*noexcept*/;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
		private:
			kernel::Document& document_;
			static std::shared_ptr<const TextToplevelStyle> DEFAULT_GLOBAL_TEXT_STYLE;
			std::shared_ptr<const TextToplevelStyle> globalTextStyle_;
			std::shared_ptr<TextLineStyleDirector> textLineStyleDirector_;
			std::shared_ptr<TextRunStyleDirector> textRunStyleDirector_;
			std::list<std::shared_ptr<TextLineColorDirector>> textLineColorDirectors_;
			detail::Listeners<GlobalTextStyleListener> globalTextStyleListeners_;
			std::shared_ptr<hyperlink::HyperlinkDetector> hyperlinkDetector_;
			struct Hyperlinks;
			mutable std::list<Hyperlinks*> hyperlinks_;
		};


		/**
		 * Registers the text line color director.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param director the director to register
		 * @throw NullPointerException @a director is @c null
		 */
		inline void Presentation::addTextLineColorDirector(std::shared_ptr<TextLineColorDirector> director) {
			if(director.get() == nullptr) throw NullPointerException("director"); textLineColorDirectors_.push_back(director);}

		/**
		 * Returns the global text style this object gives.
		 * @return The global text style
		 * @see #setGlobalTextStyle
		 */
		inline const TextToplevelStyle& Presentation::globalTextStyle() const /*throw()*/ {
			assert(globalTextStyle_ != nullptr);
			return *globalTextStyle_;
		}

		/**
		 * Removes the specified text line color director.
		 * @param director the director to remove
		 */
		inline void Presentation::removeTextLineColorDirector(TextLineColorDirector& director) /*throw()*/ {
			for(std::list<std::shared_ptr<TextLineColorDirector>>::iterator
					i(textLineColorDirectors_.begin()), e(textLineColorDirectors_.end()); i != e; ++i) {
				if(i->get() == &director) {textLineColorDirectors_.erase(i); return;}
			}
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_HPP
