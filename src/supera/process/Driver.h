#ifndef __SUPERA_DRIVER_H__
#define __SUPERA_DRIVER_H__

#include "supera/base/SuperaEvent.h"

namespace supera {
	
	class Driver {
	public:
		Driver() {}
		Reset() {}
		
		Event& EventWriteable()              { return _data; }
		const Event& Event() const           { return _data; }
		ImageMeta3D& MetaWriteable()         { return _meta; }
		const ImageMeta3D& ImageMeta() const { return _meta; }

		void Process();

	private:
		ImageMeta3D _meta;
		Event data;
	};
}

#endif