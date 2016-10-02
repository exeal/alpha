/**
 * @file document-input.hpp
 * Defines @c DocumentInput interface.
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2012, 2014-2016
 * @date 2016-07-20 Separated from document.hpp.
 */

#ifndef ASCENSION_DOCUMENT_INPUT_HPP
#define ASCENSION_DOCUMENT_INPUT_HPP
#include <ascension/config.hpp>				// ASCENSION_DEFAULT_NEWLINE
#include <ascension/corelib/text/newline.hpp>
#include <string>

namespace ascension {
	namespace kernel {
		class Document;

		/**
		 * Provides information about a document input.
		 * @see Document
		 */
		class DocumentInput {
		public:
			typedef
#if BOOST_OS_WINDOWS
				std::wstring
#else // ASCENSION_OS_POSIX
				std::string
#endif
				LocationType;
			/**
			 * Thrown if @c DocumentInput rejected the change of the document. For details, see the documentation of
			 * @c Document class.
			 * @see Document#redo, Document#replace, Document#resetContent, Document#undo,
			 *      DocumentInput#documentAboutToBeChanged
			 */
			class ChangeRejectedException : public DocumentCantChangeException {
			public:
				ChangeRejectedException();
			};
		public:
			/// Destructor.
			virtual ~DocumentInput() BOOST_NOEXCEPT {}
			/// Returns the character encoding of the document input.
			virtual std::string encoding() const BOOST_NOEXCEPT = 0;
			/// Returns a string represents the location of the document input or an empty string.
			virtual LocationType location() const BOOST_NOEXCEPT = 0;
			/// Returns the default newline of the document. The returned value can be neighter
			/// @c text#Newline#USE_INTRINSIC_VALUE nor @c text#Newline#USE_DOCUMENT_INPUT.
			virtual text::Newline newline() const BOOST_NOEXCEPT = 0;
			/// Returns @c true if the input has Unicode byte order mark. This attribute does not bother if the
			/// @c #encoding is not Unicode.
			virtual bool unicodeByteOrderMark() const BOOST_NOEXCEPT = 0;

		private:
			virtual bool isChangeable(const Document& document) const BOOST_NOEXCEPT = 0;
			virtual void postFirstDocumentChange(const Document& document) BOOST_NOEXCEPT = 0;
			friend class Document;
		};
	}
} // namespace ascension.kernel

#endif // !ASCENSION_DOCUMENT_INPUT_HPP
