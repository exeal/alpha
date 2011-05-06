/**
 * @file session.hpp
 * @author exeal
 * @date 2006-2011
 */

#ifndef ASCENSION_SESSION_HPP
#define ASCENSION_SESSION_HPP

#include <ascension/config.hpp>				// ASCENSION_NO_MIGEMO
#include <ascension/corelib/string-piece.hpp>
#ifndef ASCENSION_NO_MIGEMO
#	include <ascension/kernel/fileio.hpp>	// fileio.PathCharacter
#endif // !ASCENSION_NO_MIGEMO
#include <list>
#include <memory>
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

#ifdef ASCENSION_OS_WINDOWS
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
			 * @param keyboardLayout The active keyboard layout
			 * @param preceding The string preceding to the input
			 * @param c The code point of the character to be input
			 * @return true if the input is acceptable
			 */
			virtual bool check(HKL keyboardLayout, const StringPiece& preceding, CodePoint c) const = 0;
		};

		/**
		 * Collection of input sequence checkers.
		 * @see InputSequenceChecker, Session#getInputSequenceCheckers
		 */
		class InputSequenceCheckers {
			ASCENSION_NONCOPYABLE_TAG(InputSequenceCheckers);
		public:
			~InputSequenceCheckers();
			void add(std::auto_ptr<InputSequenceChecker> checker);
			bool check(const StringPiece& preceding, CodePoint c) const;
			void clear();
			bool isEmpty() const /*throw()*/;
			void setKeyboardLayout(HKL keyboardLayout) /*throw()*/;
		private:
			std::list<InputSequenceChecker*> strategies_;
			HKL keyboardLayout_;
		};
#endif // ASCENSION_OS_WINDOWS

		class KillRing;

		/**
		 * @note This class is not derivable.
		 */
		class Session {
			ASCENSION_NONCOPYABLE_TAG(Session);
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
		inline InputSequenceCheckers* Session::inputSequenceCheckers() /*throw()*/ {
			return inputSequenceCheckers_.get();
		}

		/// Returns the input sequence checkers.
		inline const InputSequenceCheckers* Session::inputSequenceCheckers() const /*throw()*/ {
			return inputSequenceCheckers_.get();
		}

	} // namespace texteditor
} // namespace ascension

#endif // !ASCENSION_SESSION_HPP
