/**
 * @file graphics.hpp
 * @author exeal
 * @date 2010-2011
 */

#ifndef ASCENSION_GRAPHICS_HPP
#define ASCENSION_GRAPHICS_HPP

#include <ascension/graphics/rendering-context.hpp>

namespace ascension {
	
	namespace graphics {

		class Device {
		public:
			/// Destructor.
			virtual ~Device() /*throw()*/ {}
			virtual std::auto_ptr<RenderingContext2D> createGraphicContext() const = 0;
		};

		class Screen : public Device {
		public:
			static Screen& instance();
			std::auto_ptr<RenderingContext2D> createGraphicContext() const;
		};

	}
}

#endif // !ASCENSION_GRAPHICS_HPP
