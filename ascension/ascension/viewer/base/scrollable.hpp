/**
 * @file scrollable.hpp
 * @author exeal
 * @date 2011-09-27
 */

#ifndef ASCENSION_SCROLLABLE_HPP
#define ASCENSION_SCROLLABLE_HPP
#include <ascension/corelib/range.hpp>
#include <ascension/viewer/base/widget.hpp>

namespace ascension {
	namespace viewers {
		namespace base {

			struct ScrollCommands {
				enum Value {
					SINGLE_STEP_INCREMENT,
					SINGLE_STEP_DECREMENT,
					PAGE_STEP_INCREMENT,
					PAGE_STEP_DECREMENT,
					MAXIMIZE,
					MINIMIZE
				};
			};

			template<typename Position>
			class ScrollProperties {
			public:
				virtual void command(ScrollCommands::Value ) = 0;
				virtual Range<Position> range() const = 0;
				virtual Position pageStep() const = 0;
				virtual Position position() const = 0;
				virtual void setRange(const Range<Position>& newRange) = 0;
				virtual void setPageStep(Position newPageStep) = 0;
				virtual void setPosition(Position newPosition) = 0;
				virtual void setSingleStep(Position newSingleStep) = 0;
				virtual Position singleStep() const = 0;
			};

			class ScrollableWidget : public ScrollProperties<int>, public Widget {
			public:
				enum ScrollBarPolicy {
					ALWAYS_HIDDEN,
					ALWAYS_VISIBLE,
					VISIBLE_AS_NEEDED
				};
			public:
				virtual ScrollProperties<int>& horizontalScrollBar() const = 0;
				virtual ScrollBarPolicy horizontalScrollBarPolicy() const = 0;
				virtual void setHorizontalScrollBarPolicy(ScrollBarPolicy policy) = 0;
				virtual void setVerticalScrollBarPolicy(ScrollBarPolicy policy) = 0;
				virtual ScrollProperties<int>& verticalScrollBar() const = 0;
				virtual ScrollBarPolicy verticalScrollBarPolicy() const = 0;
			};

		}
	}
}

#endif // !ASCENSION_SCROLLABLE_HPP
