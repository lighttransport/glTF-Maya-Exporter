#ifndef HOT_RELOAD_EXPORTER_H_
#define HOT_RELOAD_EXPORTER_H_

#include <maya/MPxFileTranslator.h>
#include <maya/MItGeometry.h>
#include <maya/MGlobal.h>

#include "XGenHairProcessInputOutput.h"


struct HotReloadableExporter
{
	static void *creator();

	void postConstructor();

	static MStatus initialize();

  // Process one dag path
	MStatus export_func(const XGenHairProcessInput &input, XGenHairProcessOutput *output);
};

#endif /* HOT_RELOAD_EXPORTER_H_ */
