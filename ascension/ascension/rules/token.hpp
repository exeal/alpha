/**
 * @file token.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 */

#ifndef ASCENSION_TOKEN_HPP
#define ASCENSION_TOKEN_HPP
#include <ascension/corelib/memory.hpp>
#include <ascension/kernel/position.hpp>

namespace ascension {
	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {
		/// A token is a text segment with an identifier.
		struct Token : public FastArenaObject<Token> {
			typedef std::uint16_t Identifier;
			static const Identifier UNCALCULATED;
			Identifier identifier;		///< The identifier of the token.
			kernel::Position position;	///< The beginning position of the token.

			/// Default constructor initializes nothing.
			Token() BOOST_NOEXCEPT {}
			/**
			 * Creates a token with initial values.
			 * @param identifier The initial value of @c identifier data member
			 * @param position The initial value of @c position data member
			 */
			Token(Identifier identifier, kernel::Position position) BOOST_NOEXCEPT
				: identifier(identifier), position(position) {}
			/**
			 * Creates a token with initial values.
			 * @param position The initial value of @c position data member
			 * @param identifier The initial value of @c identifier data member
			 */
			Token(kernel::Position position, Identifier identifier) BOOST_NOEXCEPT
				: identifier(identifier), position(position) {}
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_TOKEN_HPP
