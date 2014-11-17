/**
 * @file lexical-partition-presentation-reconstructor.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Renamed from lexical-partitioning.cpp
 */

#include <ascension/rules/lexical-partition-presentation-reconstructor.hpp>
#include <ascension/corelib/ustring.hpp>
#include <ascension/corelib/utility.hpp>	// detail.searchBound
#include <boost/foreach.hpp>
#if defined(_DEBUG)
#	include <boost/log/trivial.hpp>
#endif


namespace ascension {
	namespace rules {
		/**
		 * Constructor.
		 * @param presentation The presentation which gives the document and default text run style
		 * @param tokenScanner The token scanner to use for tokenization
		 * @param styles Token identifier to its text style map
		 * @param defaultStyle The style for the regions out of tokens. can be @c null
		 * @throw NullPointerException @a tokenScanner is @c null
		 */
		LexicalPartitionPresentationReconstructor::LexicalPartitionPresentationReconstructor(
				const presentation::Presentation& presentation, std::unique_ptr<TokenScanner> tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>& styles,
				std::shared_ptr<const presentation::TextRunStyle> defaultStyle /* = shared_ptr<const presentation::TextRunStyle>() */)
				: presentation_(presentation), tokenScanner_(std::move(tokenScanner)), styles_(styles) {
			if(tokenScanner_.get() == nullptr)
				throw NullPointerException("tokenScanner");
		}
	}
}
