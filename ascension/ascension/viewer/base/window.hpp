/**
 * @file window.hpp
 * @author exeal
 * @date 2010-10-27 created
 */

#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/viewer/base/user-input.hpp>

namespace ascension {

	namespace graphics {class Context;}

	namespace viewers {
		namespace base {

			class Window {
			public:
				virtual ~Window() /*throw()*/;

				virtual void activate() = 0;
				virtual void deactivate() = 0;
				virtual void close() = 0;
				virtual void enableClose(bool enable) = 0;

				virtual void maximize() = 0;
				virtual void minimize() = 0;
				virtual void restore() = 0;
				virtual bool isMaximized() const = 0;
				virtual bool isMinimized() const = 0;

				virtual void setFullscreen(bool fullscreen) = 0;
				virtual bool isFullscreen() const = 0;
			};

		}
	}
}