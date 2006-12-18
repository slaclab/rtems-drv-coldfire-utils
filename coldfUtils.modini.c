#include <rtems.h>
#include <coldfUtils.h>

int
_cexpModuleFinalize(void*unused)
{
int rval = 0;

	rval = rval || coldfQspiCleanup();

	return 0;
}
