/**
 * @file session.hpp
 * @author exeal
 * @date 2006-2007
 */

#ifndef ASCENSION_SESSION_HPP
#define ASCENSION_SESSION_HPP
#include "internal.hpp"
#include <list>
#include <vector>

namespace ascension {

	namespace kernel {class Document;}

	namespace searcher {
		class IncrementalSearcher;
		class TextSearcher;
	}

	namespace texteditor {

		/**
		 * Interface for objects which are interested in changes of the clipboard ring.
		 * @see ClipboardRing
		 */
		class IClipboardRingListener {
		private:
			/// The content of the clipboard ring is changed.
			virtual void clipboardRingChanged() = 0;
			/// The text which are added to the clipboard ring is denied.
			virtual void clipboardRingAddingDenied() = 0;
			friend class ClipboardRing;
		};

		/// A clipboard ring.
		class ClipboardRing {
		public:
			// constructor
			ClipboardRing() throw();
			// attributes
			std::size_t	activeItem() const;
			void		addListener(IClipboardRingListener& listener);
			std::size_t	capacity() const throw();
			bool		isEmpty() const throw();
			std::size_t	numberOfItems() const throw();
			void		removeListener(IClipboardRingListener& listener);
			void		setActiveItem(std::size_t index);
			void		setCapacity(std::size_t limit);
			void		text(std::size_t index, String& text, bool& box) const;
			// operations
			void	add(const String& text, bool box);
			void	remove(std::size_t index);
			void	removeAll();

		private:
			struct ClipText {
				String text;	// the text data
				bool rectangle;	// true if the text is rectangle
			};
			std::list<ClipText> datas_;
			std::size_t capacity_;
			ulong maximumBytes_;
			std::size_t activeItem_;
			ascension::internal::Listeners<IClipboardRingListener> listeners_;
		};

#ifdef ASCENSION_WINDOWS
		/**
		 * Base class for input sequence checkers.
		 * @see isc
		 */
		class InputSequenceChecker {
		public:
			/// Destructor.
			virtual ~InputSequenceChecker() throw() {}
			/**
			 * Checks the sequence.
			 * @param keyboardLayout the active keyboard layout
			 * @param first the start of the string preceding to the input
			 * @param last the end of the string preceding to the input
			 * @param cp the code point of the character to be input
			 * @return true if the input is acceptable
			 */
			virtual bool check(::HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const = 0;
		};

		/**
		 * Collection of input sequence checkers.
		 * @see InputSequenceChecker, Session#getInputSequenceCheckers
		 */
		class InputSequenceCheckers {
			MANAH_NONCOPYABLE_TAG(InputSequenceCheckers);
		public:
			~InputSequenceCheckers();
			void	add(std::auto_ptr<InputSequenceChecker> checker);
			bool	check(const Char* first, const Char* last, CodePoint cp) const;
			void	clear();
			bool	isEmpty() const throw();
			void	setKeyboardLayout(::HKL keyboardLayout) throw();
		private:
			std::list<InputSequenceChecker*> strategies_;
			::HKL keyboardLayout_;
		};
#endif /* ASCENSION_WINDOWS */

		/**
		 * @note This class is not derivable.
		 */
		class Session {
			MANAH_NONCOPYABLE_TAG(Session);
		public:
			// constructor
			Session() throw();
			~Session() throw();
			// attributes
			ClipboardRing&							clipboardRing() throw();
			const ClipboardRing&					clipboardRing() const throw();
			const std::vector<kernel::Document*>	documents() const throw();
			searcher::IncrementalSearcher&			incrementalSearcher() throw();
			const searcher::IncrementalSearcher&	incrementalSearcher() const throw();
			InputSequenceCheckers*					inputSequenceCheckers() throw();
			const InputSequenceCheckers*			inputSequenceCheckers() const throw();
#ifndef ASCENSION_NO_MIGEMO
			const WCHAR*							migemoPathName(bool runtime) throw();
#endif /* !ASCENSION_NO_MIGEMO */
			searcher::TextSearcher&					textSearcher() throw();
			const searcher::TextSearcher&			textSearcher() const throw();
			void									setInputSequenceCheckers(std::auto_ptr<InputSequenceCheckers> isc) throw();
#ifndef ASCENSION_NO_MIGEMO
			void									setMigemoPathName(const WCHAR* pathName, bool runtime);
#endif /* !ASCENSION_NO_MIGEMO */
			// operations
			void	addDocument(kernel::Document& document);
			void	removeDocument(kernel::Document& document);

		private:
			std::vector<kernel::Document*> documents_;
			ClipboardRing clipboardRing_;
			searcher::IncrementalSearcher* isearch_;
			searcher::TextSearcher* textSearcher_;
			std::auto_ptr<InputSequenceCheckers> inputSequenceCheckers_;
#ifndef ASCENSION_NO_MIGEMO
			::WCHAR migemoRuntimePathName_[MAX_PATH], migemoDictionaryPathName_[MAX_PATH];
#endif /* !ASCENSION_NO_MIGEMO */
		};


		/**
		 * Returns the index of the active content in the ring.
		 * @return the active index
		 * @throw IllegalStateException the ring is empty
		 */
		inline std::size_t ClipboardRing::activeItem() const {if(isEmpty()) throw IllegalStateException("the ring is empty."); return activeItem_;}

		/**
		 * Registers the listener.
		 * @param listener the listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		inline void ClipboardRing::addListener(IClipboardRingListener& listener) {listeners_.add(listener);}

		/// Returns the number of texts than the ring can contain.
		inline std::size_t ClipboardRing::capacity() const throw() {return capacity_;}

		/// Returns true if the ring is empty.
		inline bool ClipboardRing::isEmpty() const throw() {return datas_.empty();}

		/// Returns the count of the stored texts.
		inline std::size_t ClipboardRing::numberOfItems() const throw() {return datas_.size();}

		/**
		 * Removes the listener.
		 * @param listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		inline void ClipboardRing::removeListener(IClipboardRingListener& listener) {listeners_.remove(listener);}

		/**
		 * Sets the active content.
		 * @param index the index of the content to be activated
		 * @throw IndexOutOfBoundsException @a index is out of range
		 */
		inline void ClipboardRing::setActiveItem(std::size_t index) {if(index >= datas_.size()) throw IndexOutOfBoundsException(); activeItem_ = index;}

		/// Returns the input sequence checkers.
		inline InputSequenceCheckers* Session::inputSequenceCheckers() throw() {return inputSequenceCheckers_.get();}

		/// Returns the input sequence checkers.
		inline const InputSequenceCheckers* Session::inputSequenceCheckers() const throw() {return inputSequenceCheckers_.get();}

	} // namespace texteditor

} // namespace ascension

#endif /* !ASCENSION_SESSION_HPP */
