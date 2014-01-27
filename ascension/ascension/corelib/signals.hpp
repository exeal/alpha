/**
 * @file signals.hpp
 * @author exeal
 * @date 2006-2011 was internal.hpp
 * @date 2011-04-03 separated from internal.hpp
 * @date 2014-01-27 separated from listeners.hpp
 */

#ifndef ASCENSION_SIGNALS_HPP
#define ASCENSION_SIGNALS_HPP
#include <boost/signals2.hpp>

namespace ascension {
	/**
	 * @tparam The signal type (@c boost#signals2#signal)
	 */
	template<typename Signal>
	class SignalConnector {
	public:
		SignalConnector(Signal& signal) BOOST_NOEXCEPT : signal_(signal) {
		}
		/// Calls @c signal.connect(slot, where) with internal signal object.
		boost::signals2::connection connect(
				const typename Signal::slot_type& slot,
				boost::signals2::connect_position where = boost::signals2::at_back) {
			return signal_.connect(slot, where);
		}
		/// Calls @c signal.connect(group, slot, where) with internal signal object.
		boost::signals2::connection connect(
				const typename Signal::group_type& group,
				const typename Signal::slot_type& slot,
				boost::signals2::connect_position where = boost::signals2::at_back) {
			return signal_.connect(group, slot, where);
		}
		/// Calls @c signal.connect_extended(slot, where) with internal signal object.
		boost::signals2::connection connectExtended(
				const typename Signal::extended_slot_type& slot,
				boost::signals2::connect_position where = boost::signals2::at_back) {
			return signal_.connect(slot, where);
		}
		/// Calls @c signal.connect_extended(group, slot, where) with internal signal object.
		boost::signals2::connection connectExtended(
				const typename Signal::group_type& group,
				const typename Signal::extended_slot_type& slot,
				boost::signals2::connect_position where = boost::signals2::at_back) {
			return signal_.connect(group, slot, where);
		}
		/// Calls @c signal.disconnect(group) with internal signal object.
		void disconnect(const typename Signal::group_type& group) {
			return signal_.disconnect(group);
		}
		/// Calls @c signal.disconnect(slot) with internal signal object.
		template<typename Slot> void disconnect(const Slot& slot) {
			return signal_.disconnect(slot);
		}
		/// Calls @c signal.disconnect_all_slots() with internal signal object.
		template<typename Slot> void disconnectAllSlots() {
			return signal_.disconnect_all_slots();
		}
	private:
		Signal& signal_;
	};

	/**
	 * Makes and returns @c SignalConnector instance for the given signal.
	 * @tparam Signal The type of @a signal
	 * @param signal The signal object
	 * @return A @c SignalConnector instance
	 */
	template<typename Signal>
	inline SignalConnector<Signal> makeSignalConnector(Signal& signal) BOOST_NOEXCEPT {
		return SignalConnector<Signal>(signal);
	}
}

#endif // !ASCENSION_SIGNALS_HPP
