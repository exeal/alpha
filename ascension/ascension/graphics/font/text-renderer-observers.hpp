/**
 * @file text-renderer-observers.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2013
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 * @date 2013-11-21 separated from text-renderer.hpp
 * @see text-renderer.hpp, TextRenderer
 */

#ifndef ASCENSION_TEXT_RENDERER_OBSERVERS_HPP
#define ASCENSION_TEXT_RENDERER_OBSERVERS_HPP

#include <ascension/corelib/basic-types.hpp>		// Index
#include <ascension/presentation/writing-mode.hpp>	// presentation.BlockFlowDirection

namespace ascension {
	namespace graphics {
		namespace font {

			class TextRenderer;

			/**
			 * @see TextRenderer#addComputedBlockFlowDirectionListener,
			 *      TextRenderer#removeComputedBlockFlowDirectionListener,
			 *      presentation#TextToplevelStyleListener
			 */
			class ComputedBlockFlowDirectionListener {
			private:
				/**
				 * The computed block flow direction of the text renderer was changed.
				 * @param used The block flow direction used
				 */
				virtual void computedBlockFlowDirectionChanged(
					presentation::BlockFlowDirection used) = 0;
				friend class TextRenderer;
			};

			/**
			 * Interface for objects which are interested in change of the default font of
			 * @c TextRenderer.
			 * @see TextRenderer#addDefaultFontListener, TextRenderer#removeDefaultFontListener
			 */
			class DefaultFontListener {
			private:
				/// The font settings was changed.
				virtual void defaultFontChanged() = 0;
				friend class TextRenderer;
			};

			class InlineObject;
			class TextPaintOverride;

			/**
			 * Options for line rendering of @c TextRenderer object.
			 * @see TextRenderer#setLineRenderingOptions
			 */
			class LineRenderingOptions {
			private:
				/**
				 * Returns the inline object renders the end of line.
				 * @param line The line to render
				 * @return The inline object renders the end of line, or @c null
				 */
				virtual const InlineObject* endOfLine(Index line) const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the object overrides text paint properties for line rendering. For the
				 * detail semantics of paint override, see the documentation of
				 * @c TextPaintOverride class.
				 * @param line The line to render
				 * @return The object overrides text paint properties for line rendering, or @c null
				 */
				virtual const TextPaintOverride* textPaintOverride(Index line) const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the inline object renders the mark of text wrapping.
				 * @param line The line to render
				 * @return The inline object renders the mark of text wrapping, or @c null
				 */
				virtual const InlineObject* textWrappingMark(Index line) const BOOST_NOEXCEPT = 0;
				friend class TextRenderer;
			};
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_RENDERER_OBSERVERS_HPP
