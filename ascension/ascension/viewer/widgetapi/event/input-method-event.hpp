/**
 * @file input-method-event.hpp
 * Defines @c InputMethodEvent and @c InputMethodQueryEvent classes.
 * @author exeal
 * @date 2017-01-09 Created.
 */

#ifndef ASCENSION_INPUT_METHOD_EVENT_HPP
#define ASCENSION_INPUT_METHOD_EVENT_HPP
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/corelib/text/character.hpp>	// String
#include <ascension/viewer/widgetapi/event/event.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
				/**
				 * Provides parameters for input method events.
				 * <table>
				 *   <tr><th>Event</th><th>@c #commitString()</th><th>@c #preeditString()</th></tr>
				 *   <tr><td>Composition started</td><td>@c boost#none</td><td>An empty string</td></tr>
				 *   <tr><td>Composition changed</td><td>@c boost#none</td><td>The preeditting string</td></tr>
				 *   <tr><td>Composition completed</td><td>The commit string</td><td>@c boost#none</td></tr>
				 *   <tr><td>Composition canceled</td><td>An empty string</td><td>@c boost#none</td></tr>
				 *  </table>
				 * @see InputMethodQueryEvent
				 */
				class InputMethodEvent : public Event {
				public:
					/// Destructor.
					virtual ~InputMethodEvent() BOOST_NOEXCEPT {}

					/// @name Attributes
					/// @{
					/**
					 * Returns the commit text string.
					 * @retval String() The composition canceled
					 * @retval boost#none The composition is still running
					 * @see #preeditString, #replacementInlineRange
					 */
					virtual boost::optional<String> commitString() const BOOST_NOEXCEPT = 0;
					/**
					 * Returns the preedit text string.
					 * @retval boost#none The composition is completed
					 * @see #commitString, #replacementInlineRange
					 */
					virtual boost::optional<String> preeditString() const BOOST_NOEXCEPT = 0;
					/**
					 * Returns the character range to be replaced in the preedit string.
					 * @see #commitString, #preeditString
					 */
					virtual boost::optional<NumericRange<Index>> replacementInlineRange() const BOOST_NOEXCEPT = 0;
					/// @}
				};

				/// Simple implementation of @c InputMethodEvent interface.
				class ConstantInputMethodEvent : public InputMethodEvent {
				public:
					/// @see InputMethodEvent#commitString
					boost::optional<String> commitString() const BOOST_NOEXCEPT override {return commitString_;}
					/// @see InputMethodEvent#preeditString
					boost::optional<String> preeditString() const BOOST_NOEXCEPT override {return preeditString_;}
					/// @see InputMethodEvent#replacementInlineRange
					boost::optional<NumericRange<Index>> replacementInlineRange() const BOOST_NOEXCEPT {return replacementInlineRange_;}

					/// @name Factories
					/// @{
					/**
					 * Creates a @c ConstantInputMethodEvent instance which means the composition canceled.
					 */
					static ConstantInputMethodEvent createCanceledInstance() {
						return ConstantInputMethodEvent(String(), boost::none, boost::none);
					}
					/**
					 * Creates a @c ConstantInputMethodEvent instance which means the composition changed.
					 * @param preeditString
					 * @param replacementInlineRange
					 */
					static ConstantInputMethodEvent createChangedInstance(const String& preeditString, boost::optional<NumericRange<Index>> replacementInlineRange = boost::none) {
						return ConstantInputMethodEvent(boost::none, preeditString, replacementInlineRange);
					}
					/**
					 * Creates a @c ConstantInputMethodEvent instance which means the composition completed.
					 * @param commitString
					 * @param replacementInlineRange
					 */
					static ConstantInputMethodEvent createCompletedInstance(const String& commitString, boost::optional<NumericRange<Index>> replacementInlineRange = boost::none) {
						return ConstantInputMethodEvent(commitString, boost::none, replacementInlineRange);
					}
					/**
					 * Creates a @c ConstantInputMethodEvent which means the composition started.
					 */
					static ConstantInputMethodEvent createStartedInstance() {
						return ConstantInputMethodEvent(boost::none, String(), boost::none);
					}
					/// @}

				private:
					/**
					 * Creates @c ConstantInputMethodEvent instance.
					 * @param commitString The value returned by @c #commitString method
					 * @param preeditString The value returned by @c #preeditString method
					 * @param replacementInlineRange The value returned by @c #replacementInlineRange method
					 */
					ConstantInputMethodEvent(boost::optional<String> commitString, boost::optional<String> preeditString, boost::optional<NumericRange<Index>> replacementInlineRange)
						: commitString_(commitString), preeditString_(preeditString), replacementInlineRange_(replacementInlineRange_) {}
					const boost::optional<String> commitString_, preeditString_;
					const boost::optional<NumericRange<Index>> replacementInlineRange_;
				};

				class InputMethodQueryEvent : public Event {
				};
			}
		}
	}
}

#endif // !ASCENSION_INPUT_METHOD_EVENT_HPP
