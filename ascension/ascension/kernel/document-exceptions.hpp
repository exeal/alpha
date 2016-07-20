/**
 * @file document-exceptions.hpp
 * Defines document-related exception classes.
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2012, 2014-2016
 * @date 2016-07-20 Separated from document.hpp.
 */

#ifndef ASCENSION_DOCUMENT_EXCEPTIONS_HPP
#define ASCENSION_DOCUMENT_EXCEPTIONS_HPP
#include <ascension/corelib/basic-exceptions.hpp>

namespace ascension {
	namespace kernel {
		/**
		 * Base class of the exceptions represent @c Document#replace could not change the document because of its
		 * property.
		 * @see ReadOnlyDocumentException, DocumentInput#ChangeRejectedException
		 */
		class DocumentCantChangeException {
		public:
			virtual ~DocumentCantChangeException();
		protected:
			DocumentCantChangeException();
		};

		/// Thrown when the read only document is about to be modified.
		class ReadOnlyDocumentException : public DocumentCantChangeException, public IllegalStateException {
		public:
			ReadOnlyDocumentException();
			~ReadOnlyDocumentException() BOOST_NOEXCEPT;
		};

		/**
		 * Thrown when the caller accessed inaccessible region of the document.
		 * Document#accessibleRegion, Document#erase, Document#insert
		 */
		class DocumentAccessViolationException : public DocumentCantChangeException, public std::invalid_argument {
		public:
			DocumentAccessViolationException();
			~DocumentAccessViolationException() BOOST_NOEXCEPT;
		};
	}
} // namespace ascension.kernel

#endif // !ASCENSION_DOCUMENT_EXCEPTIONS_HPP
