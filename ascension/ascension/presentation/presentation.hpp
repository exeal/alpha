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
			class ComputedStyledTextRunIterator;
		}
	}

	namespace rules {
		class URIDetector;
	}

	namespace presentation {

		struct TextLineStyle;
		struct TextToplevelStyle;

		/**
		 * Interface for objects which declare style of a text line.
		 * @see TextRunStyleDeclarator, TextLineStyle, Presentation#setTextLineStyleDeclarator
		 */
		class TextLineStyleDeclarator {
		public:
			/// Destructor.
			virtual ~TextLineStyleDeclarator() BOOST_NOEXCEPT {}
		private:
			/**
			 * Returns the style of the specified text line in the document.
			 * @param line The line to be queried
			 * @return The style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::shared_ptr<const TextLineStyle> declareTextLineStyle(Index line) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which specify color of a text line.
		 * @see Presentation#addTextLineColorS
		 */
		class TextLineColorSpecifier {
		public:
			/// Priority.
			typedef std::uint8_t Priority;
			/// Destructor.
			virtual ~TextLineColorSpecifier() BOOST_NOEXCEPT {}
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
			virtual Priority specifyTextLineColors(Index line,
				boost::optional<graphics::Color>& foreground,
				boost::optional<graphics::Color>& background) const = 0;
			friend class Presentation;
		};

		struct TextRunStyle;

		/**
		 * Interface for objects which are interested in change of the toplevel text style of
		 * @c Presentation.
		 * @see Presentation#addTextToplevelStyleListener,
		 *      Presentation#removeTextToplevelStyleListener,
		 *      font#ComputedBlockFlowDirectionListener
		 */
		class TextToplevelStyleListener {
		public:
			/// Destructor.
			virtual ~TextToplevelStyleListener() BOOST_NOEXCEPT {}
			/**
			 * The toplevel text style of @c Presentation was changed.
			 * @param used The old style used previously
			 * @see Presentation#textToplevelStyle, Presentation#setTextToplevelStyle
			 */
			virtual void textToplevelStyleChanged(std::shared_ptr<const TextToplevelStyle> used) = 0;
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
				virtual ~Hyperlink() BOOST_NOEXCEPT {}
				/// Returns the descriptive text of the hyperlink.
				virtual String description() const BOOST_NOEXCEPT = 0;
				/// Invokes the hyperlink.
				virtual void invoke() const BOOST_NOEXCEPT = 0;
				/// Returns the columns of the region of the hyperlink.
				const Range<Index>& region() const BOOST_NOEXCEPT {return region_;}
			protected:
				/// Protected constructor takes the region of the hyperlink.
				explicit Hyperlink(const Range<Index>& region) BOOST_NOEXCEPT : region_(region) {}
			private:
				const Range<Index> region_;
			};

			/// A @c HyperlinkDetector finds the hyperlinks in the document.
			class HyperlinkDetector {
			public:
				/// Destructor.
				virtual ~HyperlinkDetector() BOOST_NOEXCEPT {}
				/**
				 * Returns the next hyperlink in the specified text line.
				 * @param document The document
				 * @param line The line number
				 * @param range The range of offsets in the line to search. @a range.beginning()
				 *              can be the beginning of the found hyperlink
				 * @return The found hyperlink, or @c null if not found
				 */
				virtual std::unique_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, Index line,
					const Range<Index>& range) const BOOST_NOEXCEPT = 0;
			};

			/**
			 * URI hyperlink detector.
			 * @see rules#URIDetector, rules#URIRule
			 * @note This class is not intended to be subclassed.
			 */
			class URIHyperlinkDetector : public HyperlinkDetector {
			public:
				URIHyperlinkDetector(std::shared_ptr<const rules::URIDetector> uriDetector) BOOST_NOEXCEPT;
				~URIHyperlinkDetector() BOOST_NOEXCEPT;
				// HyperlinkDetector
				std::unique_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, Index line, const Range<Index>& range) const BOOST_NOEXCEPT;
			private:
				std::shared_ptr<const rules::URIDetector> uriDetector_;
			};

			/**
			 * @note This class is not intended to be subclassed.
			 */
			class CompositeHyperlinkDetector : public hyperlink::HyperlinkDetector {
			public:
				~CompositeHyperlinkDetector() BOOST_NOEXCEPT;
				void setDetector(kernel::ContentType contentType, std::unique_ptr<hyperlink::HyperlinkDetector> detector);
				// hyperlink.HyperlinkDetector
				std::unique_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, Index line, const Range<Index>& range) const BOOST_NOEXCEPT;
			private:
				std::map<kernel::ContentType, hyperlink::HyperlinkDetector*> composites_;
			};
		} // namespace hyperlink

		class GlobalTextStyleSwitch;
		class TextRunStyleDeclarator;
		struct WritingMode;

		/**
		 * A bridge between the document and visual styled text.
		 * @note This class is not intended to be subclassed.
		 * @see kernel#Document, kernel#DocumentPartitioner
		 */
		class Presentation : public kernel::DocumentListener {
			ASCENSION_NONCOPYABLE_TAG(Presentation);
		public:
			explicit Presentation(kernel::Document& document) BOOST_NOEXCEPT;
			~Presentation() BOOST_NOEXCEPT;
			/// @name Attributes
			/// @{
			kernel::Document& document() BOOST_NOEXCEPT;
			const kernel::Document& document() const BOOST_NOEXCEPT;
			/// @}

			/// @name Styles Declaration
			/// @{
			void addTextToplevelStyleListener(TextToplevelStyleListener& listener);
			void removeTextToplevelStyleListener(TextToplevelStyleListener& listener);
			void setTextLineStyleDeclarator(std::shared_ptr<TextLineStyleDeclarator> newDeclarator) BOOST_NOEXCEPT;
			void setTextRunStyleDeclarator(std::shared_ptr<TextRunStyleDeclarator> newDeclarator) BOOST_NOEXCEPT;
			void setTextToplevelStyle(std::shared_ptr<const TextToplevelStyle> newStyle);
			void textLineColors(Index line,
				boost::optional<graphics::Color>& foreground, boost::optional<graphics::Color>& background) const;
			const TextToplevelStyle& textToplevelStyle() const BOOST_NOEXCEPT;
			/// @}

			/// @name Styles Computation
			/// @{
			graphics::font::ComputedTextLineStyle&& computeTextLineStyle(
				Index line, const graphics::RenderingContext2D& context,
				const graphics::NativeSize& contextSize, const GlobalTextStyleSwitch* globalSwitch) const;
			std::unique_ptr<graphics::font::ComputedStyledTextRunIterator> computeTextRunStyles(
				Index line, const graphics::RenderingContext2D& context, const graphics::NativeSize& contextSize) const;
			WritingMode&& computeWritingMode(const GlobalTextStyleSwitch* globalSwitch) const;
			/// @}

			/// @name Hyperlinks
			/// @{
			const hyperlink::Hyperlink* const* getHyperlinks(
				Index line, std::size_t& numberOfHyperlinks) const;
			void setHyperlinkDetector(
				std::shared_ptr<hyperlink::HyperlinkDetector> newDetector) BOOST_NOEXCEPT;
			/// @}

			/// @name Strategies
			/// @{
			void addTextLineColorSpecifier(std::shared_ptr<TextLineColorSpecifier> specifier);
			void removeTextLineColorSpecifier(TextLineColorSpecifier& specifier) BOOST_NOEXCEPT;
			/// @}
		private:
			void clearHyperlinksCache() BOOST_NOEXCEPT;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
		private:
			kernel::Document& document_;
			static std::shared_ptr<const TextToplevelStyle> DEFAULT_TEXT_TOPLEVEL_STYLE;
			std::shared_ptr<const TextToplevelStyle> textToplevelStyle_;
			std::shared_ptr<TextLineStyleDeclarator> textLineStyleDeclarator_;
			std::shared_ptr<TextRunStyleDeclarator> textRunStyleDeclarator_;
			std::list<std::shared_ptr<TextLineColorSpecifier>> textLineColorSpecifiers_;
			detail::Listeners<TextToplevelStyleListener> textToplevelStyleListeners_;
			std::shared_ptr<hyperlink::HyperlinkDetector> hyperlinkDetector_;
			struct Hyperlinks;
			mutable std::list<Hyperlinks*> hyperlinks_;
		};


		/**
		 * Returns the text toplevel style this object gives.
		 * @return The text toplevel style
		 * @see #setTextToplevelStyle
		 */
		inline const TextToplevelStyle& Presentation::textToplevelStyle() const BOOST_NOEXCEPT {
			assert(textToplevelStyle_ != nullptr);
			return *textToplevelStyle_;
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_HPP
