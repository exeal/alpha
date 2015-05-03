/**
 * @file token-scanner.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 */

#ifndef ASCENSION_TOKEN_SCANNER_HPP
#define ASCENSION_TOKEN_SCANNER_HPP

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/kernel/position.hpp>
#include <boost/optional.hpp>
#include <memory>

namespace ascension {
	namespace kernel {
		class Document;
		class Region;
	}

	namespace text {
		class IdentifierSyntax;
	}

	namespace rules {
		struct Token;

		/**
		 * 
		 * @see TokenScanner
		 */
		class BadScannerStateException : public IllegalStateException {
		public:
			BadScannerStateException() : IllegalStateException("The scanner can't accept the requested operation in this state.") {}
		};

		/**
		 * @c TokenScanner scans a range of document and returns the tokens it finds.
		 * To start scanning, call @c #parse method with a target document region. And then call @c #nextToken method
		 * repeatedly to get tokens. When reached the end of the scanning region, the scanning is end and @c #hasNext
		 * will return @c false.
		 */
		class TokenScanner {
		public:
			/// Destructor.
			virtual ~TokenScanner() BOOST_NOEXCEPT {}
			/// Returns @c false if the scanning is done.
			virtual bool hasNext() const BOOST_NOEXCEPT = 0;
			/// Returns the identifier syntax.
			virtual const text::IdentifierSyntax& identifierSyntax() const BOOST_NOEXCEPT = 0;
			/**
			 * Moves to the next token and returns it.
			 * @return The token or @c null if the scanning was done.
			 */
			virtual std::unique_ptr<Token> nextToken() = 0;
			/**
			 * Starts the scan with the specified range. The current position of the scanner will be the top of the
			 * specified region.
			 * @param document The document
			 * @param region The region to be scanned
			 * @throw kernel#BadRegionException @a region intersects outside of the document
			 */
			virtual void parse(const kernel::Document& document, const kernel::Region& region) = 0;
			/// Returns the current position.
			virtual kernel::Position position() const = 0;
		};

		/// @c NullTokenScanner returns no tokens. @c NullTokenScanner#hasNext returns always @c false.
		class NullTokenScanner : public TokenScanner {
		public:
			bool hasNext() const BOOST_NOEXCEPT override;
			const text::IdentifierSyntax& identifierSyntax() const BOOST_NOEXCEPT override;
			std::unique_ptr<Token> nextToken() override;
			void parse(const kernel::Document& document, const kernel::Region& region) override;
			kernel::Position position() const override;
		private:
			boost::optional<kernel::Position> position_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_TOKEN_SCANNER_HPP
