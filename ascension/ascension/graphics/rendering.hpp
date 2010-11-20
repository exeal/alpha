/**
 * @file layout.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 separated from ascension/layout.hpp
 */

#ifndef ASCENSION_RENDERING_HPP
#define ASCENSION_RENDERING_HPP

//#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
//#include <ascension/kernel/document.hpp>
//#include <ascension/presentation.hpp>
#include <ascension/graphics/text-layout.hpp>
#include <list>

namespace ascension {

	namespace presentation {class Presentation;}
	namespace viewers {class Caret;}

	namespace graphics {
		namespace font {

			class TextRenderer;

			/*
			 * Interface for objects which are interested in change of the default font of
			 * @c TextRenderer.
			 * @see TextRenderer#addDefaultFontListener, TextRenderer#removeDefaultFontListener
			 */
			class IDefaultFontListener {
			private:
				/// The font settings was changed.
				virtual void defaultFontChanged() = 0;
				friend class TextRenderer;
			};

			/**
			 * Interface for objects which are interested in getting informed about change of visual
			 * lines of @c TextRenderer.
			 * @see LineLayoutBuffer#addVisualLinesListener, LineLayoutBuffer#removeVisualLinesListener
			 */
			class IVisualLinesListener {
			private:
				/**
				 * Several visual lines were deleted.
				 * @param first the first of created lines
				 * @param last the last of created lines (exclusive)
				 * @param sublines the total number of sublines of created lines
				 * @param longestLineChanged set @c true if the longest line is changed
				 */
				virtual void visualLinesDeleted(length_t first, length_t last,
					length_t sublines, bool longestLineChanged) /*throw()*/ = 0;
				/**
				 * Several visual lines were inserted.
				 * @param first the first of inserted lines
				 * @param last the last of inserted lines (exclusive)
				 */
				virtual void visualLinesInserted(length_t first, length_t last) /*throw()*/ = 0;
				/**
				 * A visual lines were modified.
				 * @param first the first of modified lines
				 * @param last the last of modified lines (exclusive)
				 * @param sublinesDifference the difference of the number of sublines between before and after the modification
				 * @param documentChanged set @c true if the layouts were modified for the document change
				 * @param longestLineChanged set @c true if the longest line is changed
				 */
				virtual void visualLinesModified(length_t first, length_t last,
					signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ = 0;
				friend class LineLayoutBuffer;
			};

			/**
			 * Manages a buffer of layout (@c LineLayout) and holds the longest line and the number of
			 * the visual lines.
			 * @see LineLayout, TextRenderer
			 */
			class LineLayoutBuffer : public kernel::IDocumentListener/*, public presentation::IPresentationStylistListener*/ {
				ASCENSION_NONCOPYABLE_TAG(LineLayoutBuffer);
			public:
				// constructors
				LineLayoutBuffer(kernel::Document& document, length_t bufferSize, bool autoRepair);
				virtual ~LineLayoutBuffer() /*throw()*/;
				// attributes
				const kernel::Document& document() const /*throw()*/;
				const TextLayout& lineLayout(length_t line) const;
				const TextLayout* lineLayoutIfCached(length_t line) const /*throw()*/;
				int longestLineWidth() const /*throw()*/;
				length_t numberOfSublinesOfLine(length_t) const;
				length_t numberOfVisualLines() const /*throw()*/;
				// listeners
				void addVisualLinesListener(IVisualLinesListener& listener);
				void removeVisualLinesListener(IVisualLinesListener& listener);
				// strategy
				void setLayoutInformation(const ILayoutInformationProvider* newProvider, bool delegateOwnership);
				// position translations
				length_t mapLogicalLineToVisualLine(length_t line) const;
				length_t mapLogicalPositionToVisualPosition(const kernel::Position& position, length_t* column) const;
//				length_t mapVisualLineToLogicalLine(length_t line, length_t* subline) const;
//				kernel::Position mapVisualPositionToLogicalPosition(const kernel::Position& position) const;
				void offsetVisualLine(length_t& line, length_t& subline,
					signed_length_t offset, bool* overflowedOrUnderflowed = 0) const /*throw()*/;
				// operations
				void invalidate() /*throw()*/;
				void invalidate(length_t first, length_t last);
			protected:
				void invalidate(length_t line);
				// enumeration
				typedef std::list<TextLayout*>::const_iterator Iterator;
				Iterator firstCachedLine() const /*throw()*/;
				Iterator lastCachedLine() const /*throw()*/;
				// abstract
				virtual std::auto_ptr<Context> renderingContext() const = 0;
			private:
				void clearCaches(length_t first, length_t last, bool repair);
				void createLineLayout(length_t line) /*throw()*/;
				void deleteLineLayout(length_t line, TextLayout* newLayout = 0) /*throw()*/;
				void fireVisualLinesDeleted(length_t first, length_t last, length_t sublines);
				void fireVisualLinesInserted(length_t first, length_t last);
				void fireVisualLinesModified(length_t first, length_t last,
					length_t newSublines, length_t oldSublines, bool documentChanged);
				void presentationStylistChanged();
				void updateLongestLine(length_t line, int width) /*throw()*/;
				// kernel.IDocumentListener
				void documentAboutToBeChanged(const kernel::Document& document);
				void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			private:
				struct CachedLineComparer {
					bool operator()(const TextLayout*& lhs, length_t rhs) const /*throw()*/ {return lhs->lineNumber() < rhs;}
					bool operator()(length_t lhs, const TextLayout*& rhs) const /*throw()*/ {return lhs < rhs->lineNumber();}
				};
				kernel::Document& document_;
				ascension::internal::StrategyPointer<const ILayoutInformationProvider> lip_;
				std::list<TextLayout*> layouts_;
				const std::size_t bufferSize_;
				const bool autoRepair_;
				enum {ABOUT_CHANGE, CHANGING, NONE} documentChangePhase_;
				struct {
					length_t first, last;
				} pendingCacheClearance_;	// ドキュメント変更中に呼び出された clearCaches の引数
				int longestLineWidth_;
				length_t longestLine_, numberOfVisualLines_;
				ascension::internal::Listeners<IVisualLinesListener> listeners_;
			};

			// documentation is layout.cpp
			class TextRenderer : public LineLayoutBuffer, public ILayoutInformationProvider {
			public:
				// constructors
				TextRenderer(presentation::Presentation& presentation,
					const FontCollection& fontCollection, bool enableDoubleBuffering);
				TextRenderer(const TextRenderer& other);
				virtual ~TextRenderer() /*throw()*/;
				// text metrics
				std::tr1::shared_ptr<const Font> primaryFont() const /*throw()*/;
				int lineIndent(length_t line, length_t subline = 0) const;
				bool updateTextMetrics();
				// listener
				void addDefaultFontListener(IDefaultFontListener& listener);
				void removeDefaultFontListener(IDefaultFontListener& listener);
				// strategy
				void setSpecialCharacterRenderer(ISpecialCharacterRenderer* newRenderer, bool delegateOwnership);
				// operation
				void renderLine(length_t line, Context& context, int x, int y,
					const Rect<>& paintRect, const Rect<>& clipRect,
					const TextLayout::Selection* selection) const /*throw()*/;
				// ILayoutInformationProvider
				const FontCollection& fontCollection() const /*throw()*/;
				const presentation::Presentation& presentation() const /*throw()*/;
				ISpecialCharacterRenderer* specialCharacterRenderer() const /*throw()*/;
				const Font::Metrics& textMetrics() const /*throw()*/;
			private:
				void fireDefaultFontChanged();
			private:
				presentation::Presentation& presentation_;
				const FontCollection& fontCollection_;
				const bool enablesDoubleBuffering_;
				mutable win32::Handle<HDC> memoryDC_;
				mutable win32::Handle<HBITMAP> memoryBitmap_;
				std::tr1::shared_ptr<const Font> primaryFont_;
				ascension::internal::StrategyPointer<ISpecialCharacterRenderer> specialCharacterRenderer_;
				ascension::internal::Listeners<IDefaultFontListener> listeners_;
			};


// inlines //////////////////////////////////////////////////////////////////

/// Returns the document.
inline const kernel::Document& LineLayoutBuffer::document() const /*throw()*/ {return document_;}

/// Returns the first cached line layout.
inline LineLayoutBuffer::Iterator LineLayoutBuffer::firstCachedLine() const /*throw()*/ {return layouts_.begin();}

/// Returns the last cached line layout.
inline LineLayoutBuffer::Iterator LineLayoutBuffer::lastCachedLine() const /*throw()*/ {return layouts_.end();}

/**
 * Returns the layout of the specified line.
 * @param line the line
 * @return the layout or @c null if the layout is not cached
 */
inline const TextLayout* LineLayoutBuffer::lineLayoutIfCached(length_t line) const /*throw()*/ {
	if(pendingCacheClearance_.first != INVALID_INDEX && line >= pendingCacheClearance_.first && line < pendingCacheClearance_.last)
		return 0;
	for(std::list<TextLayout*>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
		if((*i)->lineNumber_ == line)
			return *i;
	}
	return 0;
}

/// Returns the width of the longest line.
inline int LineLayoutBuffer::longestLineWidth() const /*throw()*/ {return longestLineWidth_;}

/**
 * Returns the number of sublines of the specified line.
 * If the layout of the line is not calculated, this method returns 1.
 * @param line the line
 * @return the count of the sublines
 * @throw BadPositionException @a line is outside of the document
 * @see #lineLayout, TextLayout#numberOfLines
 */
inline length_t LineLayoutBuffer::numberOfSublinesOfLine(length_t line) const /*throw()*/ {
	const TextLayout* const layout = lineLayoutIfCached(line); return (layout != 0) ? layout->numberOfLines() : 1;}

/// Returns the number of the visual lines.
inline length_t LineLayoutBuffer::numberOfVisualLines() const /*throw()*/ {return numberOfVisualLines_;}

/**
 * Removes the visual lines listener.
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
inline void LineLayoutBuffer::removeVisualLinesListener(IVisualLinesListener& listener) {listeners_.remove(listener);}

/// Returns the primary font.
inline std::tr1::shared_ptr<const Font> TextRenderer::primaryFont() const /*throw()*/ {return primaryFont_;}

/// @see ILayoutInformationProvider#textMetrics
inline const Font::Metrics& TextRenderer::textMetrics() const /*throw()*/ {return primaryFont()->metrics();}

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_RENDERING_HPP
