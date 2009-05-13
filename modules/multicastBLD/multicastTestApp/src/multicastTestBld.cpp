#include "../../multicastBLDLib/src/multicastBLDLib.h"

namespace EpicsBld
{
	
extern "C" int testMulticast( char* sInterfaceIp)
{
	return testEpicsBld( sInterfaceIp );
}

} // namespace EpicsBld
