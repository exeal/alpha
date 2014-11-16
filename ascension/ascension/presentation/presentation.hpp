/**
 * @file presentation.hpp
 * Provides classes define appearance and presentation of a text editor user interface.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2014
 */

#ifndef ASCENSION_PRESENTATION_HPP
#define ASCENSION_PRESENTATION_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/kernel/document.hpp>
#include <ascension/presentation/styles/length.hpp>	// Length.Context
#include <boost/core/noncopyable.hpp>
#include <boost/flyweight.hpp>
#include <boost/optional.hpp>
#include <boost/range/irange.hpp>

namespace ascension {

	namespace graphics {
		class Color;
	}

	namespace rules {
		class URIDetector;
	}

	namespace presentation {
		class DeclaredTextLineStyle;

		/**
		 * Interface for objects which declare style of a text line.
		 * @see TextRunStyleDeclarator, DeclaredTextLineStyle, Presentation#setTextLineStyleDeclarator
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
			virtual std::shared_ptr<const DeclaredTextLineStyle> declareTextLineStyle(Index line) const = 0;
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
			 * @param[out] foreground The foreground color of the text line. If @c boost#none, line color is not set
			 * @param[out] background The background color of the text line. If @c boost#none, line color is not set
			 * @return The priority
			 */
			virtual Priority specifyTextLineColors(Index line,
				boost::optional<graphics::Color>& foreground,
				boost::optional<graphics::Color>& background) const = 0;
			friend class Presentation;
		};

		struct ComputedTextToplevelStyle;

		/**
		 * Interface for objects which are interested in change of the computed toplevel text style of @c Presentation.
		 * @see Presentation#addComputedTextToplevelStyleListener,
		 *      Presentation#removeComputedTextToplevelStyleListener, font#ComputedBlockFlowDirectionListener
		 */
		class ComputedTextToplevelStyleListener {
		public:
			/// Destructor.
			virtual ~ComputedTextToplevelStyleListener() BOOST_NOEXCEPT {}
			/**
			 * The computed toplevel text style of @c Presentation was changed.
			 * @param previous The old style used previously
			 * @see Presentation#declaredTextToplevelStyle, Presentation#setDeclaredTextToplevelStyle
			 */
			virtual void computedTextToplevelStyleChanged(std::shared_ptr<const ComputedTextToplevelStyle> previous) = 0;
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
				const boost::integer_range<Index>& region() const BOOST_NOEXCEPT {return region_;}
			protected:
				/// Protected constructor takes the region of the hyperlink.
				explicit Hyperlink(const boost::integer_range<Index>& region) BOOST_NOEXCEPT : region_(region) {}
			private:
				const boost::integer_range<Index> region_;
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
				 * @param range The range of offsets in the line to search. @a range.begin() can be
				 *              the beginning of the found hyperlink
				 * @return The found hyperlink, or @c null if not found
				 */
				virtual std::unique_ptr<Hyperlink> nextHyperlink(
					const kernel::Document& document, Index line,
					const boost::integer_range<Index>& range) const BOOST_NOEXCEPT = 0;
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
					const kernel::Document& document, Index line, const boost::integer_range<Index>& range) const BOOST_NOEXCEPT;
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
					const kernel::Document& document, Index line, const boost::integer_range<Index>& range) const BOOST_NOEXCEPT;
			private:
				std::map<kernel::ContentType, hyperlink::HyperlinkDetector*> composites_;
			};
		} // namespace hyperlink

		struct ComputedStyledTextRunIterator;
		struct ComputedTextLineStyle;
		struct ComputedTextRunStyle;
		class DeclaredTextToplevelStyle;
		class GlobalTextStyleSwitch;
		class TextRunStyleDeclarator;
		struct WritingMode;

		/**
		 * A bridge between the document and visual styled text.
		 * @note This class is not intended to be subclassed.
		 * @see kernel#Document, kernel#DocumentPartitioner
		 */
		class Presentation : public kernel::DocumentListener, private boost::noncopyable {
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
			void addComputedTextToplevelStyleListener(ComputedTextToplevelStyleListener& listener);
			const DeclaredTextToplevelStyle& declaredTextToplevelStyle() const BOOST_NOEXCEPT;
			void removeComputedTextToplevelStyleListener(ComputedTextToplevelStyleListener& listener);
			void setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle> newStyle);
			void setTextLineStyleDeclarator(std::shared_ptr<TextLineStyleDeclarator> newDeclarator) BOOST_NOEXCEPT;
			void setTextRunStyleDeclarator(std::shared_ptr<TextRunStyleDeclarator> newDeclarator) BOOST_NOEXCEPT;
			void textLineColors(Index line,
				boost::optional<graphics::Color>& foreground, boost::optional<graphics::Color>& background) const;
			/// @}

			/// @name Computed Toplevel Styles
			/// @{
			const ComputedTextLineStyle& computedTextLineStyle() const BOOST_NOEXCEPT;
			const ComputedTextRunStyle& computedTextRunStyle() const BOOST_NOEXCEPT;
			const ComputedTextToplevelStyle& computedTextToplevelStyle() const BOOST_NOEXCEPT;
			/// @}

			/// @name Styles Computation
			/// @{
			void computeTextLineStyle(Index line,
				const styles::Length::Context& lengthContext, ComputedTextLineStyle& result) const;
			std::unique_ptr<ComputedStyledTextRunIterator>
				computeTextRunStyles(Index line, const styles::Length::Context& lengthContext) const;
			WritingMode computeWritingMode(boost::optional<Index> line = boost::none) const;
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
			std::shared_ptr<const DeclaredTextToplevelStyle> declaredTextToplevelStyle_;
			std::shared_ptr<TextLineStyleDeclarator> textLineStyleDeclarator_;
			std::shared_ptr<TextRunStyleDeclarator> textRunStyleDeclarator_;
			std::list<std::shared_ptr<TextLineColorSpecifier>> textLineColorSpecifiers_;
			ascension::detail::Listeners<ComputedTextToplevelStyleListener> computedTextToplevelStyleListeners_;
			struct ComputedStyles;
			std::unique_ptr<ComputedStyles> computedStyles_;
			std::shared_ptr<hyperlink::HyperlinkDetector> hyperlinkDetector_;
			struct Hyperlinks;
			mutable std::list<Hyperlinks*> hyperlinks_;
		};


		/**
		 * Returns the declared text toplevel style this object gives.
		 * @return The declared text toplevel style
		 * @see #setDeclaredTextToplevelStyle
		 */
		inline const DeclaredTextToplevelStyle& Presentation::declaredTextToplevelStyle() const BOOST_NOEXCEPT {
			assert(declaredTextToplevelStyle_.get() != nullptr);
			return *declaredTextToplevelStyle_;
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_HPP
