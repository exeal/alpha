/**
 * @file kill-ring.hpp
 * @author exeal
 * @date 2006-2011 was session.hpp
 * @date 2011-05-06 separated from session.hpp
 */

#ifndef ASCENSION_KILL_RING_HPP
#define ASCENSION_KILL_RING_HPP

#include <ascension/corelib/detail/listeners.hpp>	// detail.Listeners
#include <ascension/corelib/string-piece.hpp>
#include <list>

namespace ascension {

//	namespace kernel {
//		class Region;
//		class Document;
//	}

	namespace texteditor {
		/**
		 * Interface for objects which are interested in changes of the kill ring.
		 * @see KillRing
		 */
		class KillRingListener {
		private:
			/// The content of the kill ring was changed.
			virtual void killRingChanged() = 0;
			friend class KillRing;
		};

		// documentation is session.cpp
		class KillRing {
		public:
			// constructor
			explicit KillRing(std::size_t maximumNumberOfKills = ASCENSION_DEFAULT_MAXIMUM_KILLS) BOOST_NOEXCEPT;
			// listeners
			void addListener(KillRingListener& listener);
			void removeListener(KillRingListener& listener);
/*			// kill
			void copyRegion(const kernel::Document& document, const kernel::Region& region);
			void killRegion(kernel::Document& document, const kernel::Region& region);
			// yank
			void yank(std::size_t index = 0);
			void yankPop(std::size_t index = -1);
*/			// low level accesses
			void addNew(const String& text, bool rectangle, bool replace = false);
			void append(const String& text, bool prepend);
			const std::pair<String, bool>& get(std::ptrdiff_t places = 0) const;
			const std::pair<String, bool>& setCurrent(std::ptrdiff_t places);
			// number
			std::size_t maximumNumberOfKills() const BOOST_NOEXCEPT;
			std::size_t numberOfKills() const BOOST_NOEXCEPT;
//			void setMaximumNumberOfKills(std::size_t capacity) BOOST_NOEXCEPT;
		private:
			typedef std::list<std::pair<String, bool>> Contents;
			Contents::iterator at(ptrdiff_t index) const;
			void interprogramCopy(const String& text, bool rectangle);
			std::pair<String, bool> interprogramPaste();
		private:
			Contents contents_;	// plain-text vs. rectangle-flag
			Contents::iterator yankPointer_;
			const std::size_t maximumNumberOfKills_;
			ascension::detail::Listeners<KillRingListener> listeners_;
		};
	} // namespace texteditor
} // namespace ascension

#endif // !ASCENSION_KILL_RING_HPP
