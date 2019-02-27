#include "HotReloadExporterLogic.h"

#include <ssmath/common_math.h>
#include <maya/MPoint.h>
#include <maya/MDataHandle.h>

Shared
{
  DLLExport void exportFunc(const void *arg) {
    const MDagPath *dag = reinterpret_cast<const MDagPath *>(arg);
    fprintf(stderr, "logic! ptr = 0x%08x\n", dag);
    return;
  }
}

