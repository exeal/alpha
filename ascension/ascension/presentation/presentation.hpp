/**
 * @file presentation.hpp
 * Provides classes define appearance and presentation of a text editor user interface.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2016
 */

#ifndef ASCENSION_PRESENTATION_HPP
#define ASCENSION_PRESENTATION_HPP
#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/kernel/document.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <boost/core/noncopyable.hpp>
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
		struct TextOverrideStyle;

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
#ifdef ASCENSION_ENABLE_TEXT_LINE_COLOR_SPECIFIER
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
#endif
		struct ComputedStyledTextRunIterator;
		struct ComputedTextLineStyle;
		struct ComputedTextRunStyle;
		struct ComputedTextToplevelStyle;
		class DeclaredTextToplevelStyle;
		class GlobalTextStyleSwitch;
		class TextRunStyleDeclarator;

		namespace hyperlink {
			class Hyperlink;
			class HyperlinkDetector;
		}

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
			const DeclaredTextToplevelStyle& declaredTextToplevelStyle() const BOOST_NOEXCEPT;
			void setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle> newStyle);
			void setTextLineStyleDeclarator(std::shared_ptr<TextLineStyleDeclarator> newDeclarator) BOOST_NOEXCEPT;
			void setTextRunStyleDeclarator(std::shared_ptr<TextRunStyleDeclarator> newDeclarator) BOOST_NOEXCEPT;
#ifdef ASCENSION_ENABLE_TEXT_LINE_COLOR_SPECIFIER
			void textLineColors(Index line,
				boost::optional<graphics::Color>& foreground, boost::optional<graphics::Color>& background) const;
#endif
			/// @}
#if 0
			/// @name Default Writing Modes
			/// @{
			ReadingDirection defaultDirection() const BOOST_NOEXCEPT override;
			void setDefaultDirection(ReadingDirection direction) BOOST_NOEXCEPT override;
			/// @}
#endif
			/// @name Computed Styles
			/// @{
			const ComputedTextLineStyle& computedTextLineStyle() const BOOST_NOEXCEPT;
			const ComputedTextRunStyle& computedTextRunStyle() const BOOST_NOEXCEPT;
			const ComputedTextToplevelStyle& computedTextToplevelStyle() const BOOST_NOEXCEPT;
			typedef boost::signals2::signal<
				void(const Presentation&, const DeclaredTextToplevelStyle&, const ComputedTextToplevelStyle&)
			> ComputedTextToplevelStyleChanged;
			SignalConnector<ComputedTextToplevelStyleChanged> computedTextToplevelStyleChangedSignal() BOOST_NOEXCEPT;
			/// @}

			/// @name Styles Computation
			/// @{
			const ComputedTextLineStyle& computeTextLineStyle(Index line) const;
			const ComputedTextRunStyle& computeTextRunStyleForLine(Index line) const;
			std::unique_ptr<ComputedStyledTextRunIterator> computeTextRunStyles(Index line) const;
			WritingMode computeWritingMode() const BOOST_NOEXCEPT;
			WritingMode computeWritingMode(Index line) const;
			/// @}

			/// @name Override Styles
			/// @{
			const TextOverrideStyle& overrideStyle() const BOOST_NOEXCEPT;
			void setOverrideStyle(std::shared_ptr<const TextOverrideStyle> newStyle) BOOST_NOEXCEPT;
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
#ifdef ASCENSION_ENABLE_TEXT_LINE_COLOR_SPECIFIER
			void addTextLineColorSpecifier(std::shared_ptr<TextLineColorSpecifier> specifier);
			void removeTextLineColorSpecifier(TextLineColorSpecifier& specifier) BOOST_NOEXCEPT;
#endif
			/// @}

		private:
			void clearHyperlinksCache() BOOST_NOEXCEPT;
			std::shared_ptr<const DeclaredTextLineStyle> declaredTextLineStyle(Index line) const;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
		private:
			kernel::Document& document_;
			std::shared_ptr<const DeclaredTextToplevelStyle> declaredTextToplevelStyle_;
			std::shared_ptr<TextLineStyleDeclarator> textLineStyleDeclarator_;
			std::shared_ptr<TextRunStyleDeclarator> textRunStyleDeclarator_;
#ifdef ASCENSION_ENABLE_TEXT_LINE_COLOR_SPECIFIER
			std::list<std::shared_ptr<TextLineColorSpecifier>> textLineColorSpecifiers_;
#endif
#if 0
			ReadingDirection defaultDirection_;
#endif
			struct ComputedStyles;
			std::unique_ptr<ComputedStyles> computedStyles_;
			std::shared_ptr<const TextOverrideStyle> textOverrideStyle_;
			ComputedTextToplevelStyleChanged computedTextToplevelStyleChangedSignal_;
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
#if 0
		/// Returns the default direction.
		inline ReadingDirection Presentation::defaultDirection() const BOOST_NOEXCEPT {
			return defaultDirection_;
		}
#endif
		/// Returns the registered @c TextOverrideStyle.
		inline const TextOverrideStyle& Presentation::overrideStyle() const BOOST_NOEXCEPT {
			assert(textOverrideStyle_.get() != nullptr);
			return *textOverrideStyle_;
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_HPP
