/**
 * @file conversions.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 * @date 2016-11-04 Separated from command.hpp.
 */

#ifndef ASCENSION_CONVERSIONS_HPP
#define ASCENSION_CONVERSIONS_HPP
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/// Converts a character into the text represents the code value of the character.
			class CharacterToCodePointConversionCommand : public Command {
			public:
				CharacterToCodePointConversionCommand(viewer::TextViewer& viewer) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/// Converts a text represents a code value into the character has the code value.
			class CodePointToCharacterConversionCommand : public Command {
			public:
				CodePointToCharacterConversionCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/// Reconverts by using the input method editor.
			class ReconversionCommand : public Command {
			public:
				explicit ReconversionCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/// Tabifies (exchanges tabs and spaces).
			class TabifyCommand : public Command {
			public:
				TabifyCommand(viewer::TextViewer& view, bool untabify) BOOST_NOEXCEPT;
			private:
				bool perform();
				bool untabify_;
			};
		}
	}
}

#endif // !ASCENSION_CONVERSIONS_HPP
