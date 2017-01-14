/**
 * @file event.hpp
 * Defines @c Event class.
 * @author exeal
 * @date 2010-11-10 Created (as user-input.hpp)
 * @date 2014-09-07 Separated from user-input.hpp
 */

#ifndef ASCENSION_EVENT_HPP
#define ASCENSION_EVENT_HPP
#include <boost/config.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
				/// Base class of all event types.
				class Event {
				public:
					/// Default constructor creates an event not sonsumed.
					Event() BOOST_NOEXCEPT : consumed_(false) {}
					/// Destructor.
					virtual ~Event() BOOST_NOEXCEPT {}
					/**
					 * Marks this event was consumed.
					 * @see #ignore, #isConsumed
					 */
					void consume() BOOST_NOEXCEPT {consumed_ = true;}
					/**
					 * Marks this event was not consumed.
					 * @see #consume, #isConsumed
					 */
					void ignore() BOOST_NOEXCEPT {consumed_ = false;}
					/// Returns if this event was consumed.
					bool isConsumed() const BOOST_NOEXCEPT {return consumed_;}

				private:
					bool consumed_;
				};
			}
		}
	}
}

#endif // !ASCENSION_EVENT_HPP
