/**
 * @file ruler-allocation-width-sink.hpp
 * Defines @c RulerAllocationWidthSink interface.
 * @author exeal
 * @date 2015-03-02 Created.
 */

#ifndef ASCENSION_RULER_ALLOCATION_WIDTH_SINK_HPP
#define ASCENSION_RULER_ALLOCATION_WIDTH_SINK_HPP

namespace ascension {
	namespace viewer {
		namespace source {
			class Ruler;

			/**
			 * Receives the change of the ruler's width
			 * @see Ruler, SourceViewer
			 */
			struct RulerAllocationWidthSink {
				/**
				 * Informs that the width of the specified ruler was changed.
				 * @param ruler The source ruler
				 */
				virtual void updateRulerAllocationWidth(const Ruler& ruler) = 0;
			};
		}
	}
}

#endif // !ASCENSION_RULER_ALLOCATION_WIDTH_SINK_HPP
