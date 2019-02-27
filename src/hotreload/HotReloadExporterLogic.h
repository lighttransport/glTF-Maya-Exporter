#ifndef HO_RELOAD_EXPORTER_LOGIC_H
#define HO_RELOAD_EXPORTER_LOGIC_H

#include <limits.h>

#define PLATFORM_LEAN
#include <ssmath/platform.h>
#include <ssmath/vector_math.h>


enum DeformResult
{
  DeformResult_Failure = INT_MIN,
  DeformResult_Success = 0
};


Shared
{
  /// Entry point signature of an exporter function.
  DLLExport void exportFunc(const void *arg);
}


#endif /* HOT_RELOAD_EXPORTER_LOGIC_H_ */
