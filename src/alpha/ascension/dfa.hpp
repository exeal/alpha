/**
 * @file dfa.hpp
 * @author exeal
 * @date 2007
 */

#include "common.hpp"

namespace ascension {
	namespace regex {
		namespace dfa {
			/// Thrown when the given regular expression is invalid.
			class PatternSyntaxException : public std::invalid_argument {
			public:
				PatternSyntaxException() : std::invalid_argument("") {}
			};

			/**
			 * Represents a regular expression pattern.
			 * @see regex#Pattern
			 */
			class Pattern {
			public:
				enum SyntaxOption {NORMAL = 0};
				typedef manah::Flags<SyntaxOption> SyntaxOptions;
			public:
				Pattern(const Char* first, const Char* last, const SyntaxOptions& options = NORMAL);
				explicit Pattern(const String& pattern, const SyntaxOptions& options = NORMAL);
				~Pattern() throw();
				template<typename InputIterator>
				bool matches(InputIterator first, InputIterator last) const;
				bool matches(const String& target) const {return matches(target.begin(), target.end());}
			private:
				class DFA;
				DFA* impl_;
			};
		}
	}
} // namespace ascension.regex.dfa
