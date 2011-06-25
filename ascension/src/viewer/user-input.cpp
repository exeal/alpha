/**
 * @file user-input.cpp Implements the members of the namespace @c ascension#viewers related to the
 * user input (keyboard and mouse).
 * @author exeal
 * @date 2009 separated from viewer.cpp
 * @see viewer.cpp
 */

#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/base/cursor.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/text-editor/command.hpp>	// texteditor.commands.*
#include <ascension/win32/ui/menu.hpp>
#include <zmouse.h>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace ascension::viewers;
using namespace ascension::viewers::base;
using namespace std;
namespace k = ascension::kernel;