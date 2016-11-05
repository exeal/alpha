/**
 * @file command.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 */

#ifndef ASCENSION_COMMAND_HPP
#define ASCENSION_COMMAND_HPP
#include <boost/config.hpp>

namespace ascension {
	namespace viewer {
		class TextViewer;
	}

	namespace texteditor {
		/**
		 * Abstract class for the editor commands.
		 * @see ascension#texteditor#commands
		 */
		class Command {
		public:
			typedef unsigned long NumericPrefix;	///< Type of numeric prefixes.
		public:
			virtual ~Command() BOOST_NOEXCEPT;
			bool operator()();
			NumericPrefix numericPrefix() const BOOST_NOEXCEPT;
			Command& retarget(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			Command& setNumericPrefix(NumericPrefix number) BOOST_NOEXCEPT;
		protected:
			explicit Command(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			bool abortModes();
			/// Returns the text viewer which is the target of this command.
			viewer::TextViewer& target() const BOOST_NOEXCEPT {return *viewer_;}
			void throwIfTargetIsReadOnly() const;
			void throwIfTargetHasNoWindow() const;
		private:
			/// Called by @c operator(). For semantics, see @c operator().
			virtual bool perform() = 0;
		private:
			viewer::TextViewer* viewer_;
			NumericPrefix numericPrefix_;
		};


		/**
		 * Performs the command. The command can return the command-specific result value. If the
		 * command didn't throw an exception, this resets the numeric prefix to 1.
		 * @retval true The command succeeded
		 * @retval false An ignorable or easily recoverable error occurred. Or the command tried to
		 *               change the read-only document or the document's input rejected the change
		 * @throw ... A fatal error occurred. The type of exception(s) is defined by the derived
		 *            class. See the documentation of @c #perform methods of the implementation
		 * @see kernel#DocumentCantChangeException, kernel#ReadOnlyDocumentException
		 */
		inline bool Command::operator()() {
			const bool result = perform();
			setNumericPrefix(1);
			return result;
		}

		/// Returns the numeric prefix for the next execution.
		inline Command::NumericPrefix Command::numericPrefix() const BOOST_NOEXCEPT {
			return numericPrefix_;
		}

		/**
		 * Changes the command target.
		 * @param viewer The text viewer as the new target to set
		 * @return This command
		 */
		inline Command& Command::retarget(viewer::TextViewer& viewer) BOOST_NOEXCEPT {
			viewer_ = &viewer;
			return *this;
		}

		/**
		 * Sets the numeric prefix for the next performance.
		 * @param number The new numeric prefix
		 * @return This command
		 */
		inline Command& Command::setNumericPrefix(NumericPrefix number) BOOST_NOEXCEPT {
			numericPrefix_ = number;
			return *this;
		}

		/**
		 * Implementations of the standard commands. These classes extends @c Command class.
		 *
		 * These commands are very common for text editors, but somewhat complex to implement. Use
		 * these classes, rather than write your own code implements same feature.
		 */
		namespace commands {}
	}
}

#endif // !ASCENSION_COMMAND_HPP
