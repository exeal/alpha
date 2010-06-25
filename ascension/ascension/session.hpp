/**
 * @file session.hpp
 * @author exeal
 * @date 2006-2009
 */

#ifndef ASCENSION_SESSION_HPP
#define ASCENSION_SESSION_HPP
#include "internal.hpp"
#include <list>
#include <vector>

namespace ascension {

	namespace kernel {
		class Region;
		class Document;
	}

	namespace searcher {
		class IncrementalSearcher;
		class TextSearcher;
	}

	namespace texteditor {

		/**
		 * Interface for objects which are interested in changes of the kill ring.
		 * @see KillRing
		 */
		class IKillRingListener {
		private:
			/// The content of the kill ring was changed.
			virtual void killRingChanged() = 0;
			friend class KillRing;
		};

		// documentation is session.cpp
		class KillRing {
		public:
			// constructor
			explicit KillRing(std::size_t maximumNumberOfKills = ASCENSION_DEFAULT_MAXIMUM_KILLS) /*throw()*/;
			// listeners
			void addListener(IKillRingListener& listener);
			void removeListener(IKillRingListener& listener);
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
			std::size_t maximumNumberOfKills() const /*throw()*/;
			std::size_t numberOfKills() const /*throw()*/;
//			void setMaximumNumberOfKills(std::size_t capacity) /*throw()*/;
		private:
			typedef std::list<std::pair<String, bool> > Contents;
			Contents::iterator at(ptrdiff_t index) const;
			void interprogramCopy(const String& text, bool rectangle);
			std::pair<String, bool> interprogramPaste();
		private:
			Contents contents_;	// plain-text vs. rectangle-flag
			Contents::iterator yankPointer_;
			const std::size_t maximumNumberOfKills_;
			ascension::internal::Listeners<IKillRingListener> listeners_;
		};

#ifdef ASCENSION_WINDOWS
		/**
		 * Base class for input sequence checkers.
		 * @see isc
		 */
		class InputSequenceChecker {
		public:
			/// Destructor.
			virtual ~InputSequenceChecker() /*throw()*/ {}
			/**
			 * Checks the sequence.
			 * @param keyboardLayout the active keyboard layout
			 * @param first the start of the string preceding to the input
			 * @param last the end of the string preceding to the input
			 * @param cp the code point of the character to be input
			 * @return true if the input is acceptable
			 */
			virtual bool check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const = 0;
		};

		/**
		 * Collection of input sequence checkers.
		 * @see InputSequenceChecker, Session#getInputSequenceCheckers
		 */
		class InputSequenceCheckers {
			MANAH_NONCOPYABLE_TAG(InputSequenceCheckers);
		public:
			~InputSequenceCheckers();
			void add(std::auto_ptr<InputSequenceChecker> checker);
			bool check(const Char* first, const Char* last, CodePoint cp) const;
			void clear();
			bool isEmpty() const /*throw()*/;
			void setKeyboardLayout(HKL keyboardLayout) /*throw()*/;
		private:
			std::list<InputSequenceChecker*> strategies_;
			HKL keyboardLayout_;
		};
#endif // ASCENSION_WINDOWS

		/**
		 * @note This class is not derivable.
		 */
		class Session {
			MANAH_NONCOPYABLE_TAG(Session);
		public:
			// constructor
			Session() /*throw()*/;
			~Session() /*throw()*/;
			// attributes
			const std::vector<kernel::Document*> documents() const /*throw()*/;
			searcher::IncrementalSearcher& incrementalSearcher() /*throw()*/;
			const searcher::IncrementalSearcher& incrementalSearcher() const /*throw()*/;
			InputSequenceCheckers* inputSequenceCheckers() /*throw()*/;
			const InputSequenceCheckers* inputSequenceCheckers() const /*throw()*/;
			KillRing& killRing() /*throw()*/;
			const KillRing& killRing() const /*throw()*/;
#ifndef ASCENSION_NO_MIGEMO
			const kernel::fileio::PathCharacter* migemoPathName(bool runtime) /*throw()*/;
#endif // !ASCENSION_NO_MIGEMO
			searcher::TextSearcher& textSearcher() /*throw()*/;
			const searcher::TextSearcher& textSearcher() const /*throw()*/;
			void setInputSequenceCheckers(std::auto_ptr<InputSequenceCheckers> isc) /*throw()*/;
#ifndef ASCENSION_NO_MIGEMO
			void setMigemoPathName(const kernel::fileio::PathCharacter* pathName, bool runtime);
#endif // !ASCENSION_NO_MIGEMO
			// operations
			void addDocument(kernel::Document& document);
			void removeDocument(kernel::Document& document);

		private:
			std::vector<kernel::Document*> documents_;
			KillRing killRing_;
			searcher::IncrementalSearcher* isearch_;
			searcher::TextSearcher* textSearcher_;
			std::auto_ptr<InputSequenceCheckers> inputSequenceCheckers_;
#ifndef ASCENSION_NO_MIGEMO
			kernel::fileio::PathCharacter migemoRuntimePathName_[MAX_PATH], migemoDictionaryPathName_[MAX_PATH];
#endif // !ASCENSION_NO_MIGEMO
		};


		/// Returns the input sequence checkers.
		inline InputSequenceCheckers* Session::inputSequenceCheckers() /*throw()*/ {return inputSequenceCheckers_.get();}

		/// Returns the input sequence checkers.
		inline const InputSequenceCheckers* Session::inputSequenceCheckers() const /*throw()*/ {return inputSequenceCheckers_.get();}

	} // namespace texteditor

} // namespace ascension

#endif // !ASCENSION_SESSION_HPP
