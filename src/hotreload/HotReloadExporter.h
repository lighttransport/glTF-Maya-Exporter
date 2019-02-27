#ifndef HOT_RELOAD_EXPORTER_H_
#define HOT_RELOAD_EXPORTER_H_

#include <maya/MPxFileTranslator.h>
#include <maya/MItGeometry.h>
#include <maya/MGlobal.h>


//static const MTypeId kHotReloadableExporterID = 0x0008003F;
//static const char *kHotReloadableExporterName = "hotReloadableExporter";

struct HotReloadableExporter
{
	static void *creator();

	void postConstructor();

	static MStatus initialize();

  // Process one dag path
	MStatus export_func(const MDagPath &dag);
};

#endif /* HOT_RELOAD_EXPORTER_H_ */
