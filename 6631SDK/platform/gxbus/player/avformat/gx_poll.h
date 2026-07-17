#ifndef __GX_POLL__
#define __GX_POLL__

#ifdef LINUX_OS
#include <poll.h>
#else
#include "gx_system.h"

#define random() rand()
#endif
#endif
