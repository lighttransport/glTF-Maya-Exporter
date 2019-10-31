#ifndef HOT_RELOAD_EXPORTER_H_
#define HOT_RELOAD_EXPORTER_H_

#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MPxFileTranslator.h>

#include "XGenSplineProcessInputOutput.h"

struct HotReloadableExporter
{
    static void* creator();

    void postConstructor();

    static MStatus initialize();

    // Process one dag path
    MStatus export_func(const XGenSplineProcessInput& input, XGenSplineProcessOutput* output);
};

#endif /* HOT_RELOAD_EXPORTER_H_ */