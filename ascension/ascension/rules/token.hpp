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
		/// A token is a text segment with identifier.
		struct Token : public FastArenaObject<Token> {
			typedef std::uint16_t Identifier;
			static const Identifier UNCALCULATED;
			Identifier id;
			kernel::Region region;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_TOKEN_HPP
