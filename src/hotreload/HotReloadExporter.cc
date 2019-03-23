#define PLATFORM_LEAN
#include <ssmath/common_math.h>
#include <ssmath/platform.h>

#include "HotReloadExporter.h"
#include "HotReloadExporterPlatform.h"
#include "XGenSplineProcessInputOutput.h"
//#include "HotReloadExporterLogic.h"

#include <maya/MGlobal.h>
#include <maya/MPoint.h>

void* HotReloadableExporter::creator()
{
    return new HotReloadableExporter;
}

void HotReloadableExporter::postConstructor()
{
    fprintf(stderr, "Trying to load logic plugin dll\n");
    LibraryStatus result = loadExporterLogicDLL(kLogicLibrary);
    if (result != LibraryStatus_Success)
    {
        MGlobal::displayError("Failed to load shared library!");
        return;
    }
    MGlobal::displayInfo("Loaded logic dll.");

    return;
}

MStatus HotReloadableExporter::initialize()
{
    MStatus result;

    //attributeAffects(envelope, outputGeom);

    return result;
}

MStatus HotReloadableExporter::export_func(const XGenSplineProcessInput &input, XGenSplineProcessOutput *output)
{
    LibraryStatus status;


    if (!kLogicLibrary.isValid)
    {
#ifdef _DEBUG_MODE
        MGlobal::displayError("The logic DLL is not valid, attempting reload!");
#endif
        // NOTE: (sonictk) Just in case, we make sure the library is unloaded
        unloadExporterLogicDLL(kLogicLibrary);
        status = loadExporterLogicDLL(kLogicLibrary);
        if (status != LibraryStatus_Success)
        {
            return MStatus::kFailure;
        }
    }

    // NOTE: (yliangsiew) Find the last modified time of the DLL and check if
    // there is a newer version; if so, unload the existing DLL and load the new
    // one, then fix up the function pointers again
    if (kPluginLogicLibraryPath.numChars() == 0)
    {
        return MStatus::kFailure;
    }

    // NOTE: (sonictk) We only reload the DLL *if* the DLL actually exists; this
    // is so we can rename the DLL on Windows to avoid having the DLL handle be locked.
    FileTime lastModified = getLastWriteTime(kPluginLogicLibraryPath.asChar());
    if (lastModified >= 0 && lastModified != kLogicLibrary.lastModified)
    {
#ifdef _DEBUG_MODE
        MGlobal::displayInfo("DEBUG: Reloading logic DLL...");
#endif // _DEBUG_MODE
        status = unloadExporterLogicDLL(kLogicLibrary);
        if (status != LibraryStatus_Success)
        {
#ifdef _DEBUG_MODE
            MGlobal::displayError("Unable to unload logic library!");
#endif
            return MStatus::kFailure;
        }
        status = loadExporterLogicDLL(kLogicLibrary);
        if (status != LibraryStatus_Success)
        {
#ifdef _DEBUG_MODE
            MGlobal::displayError("Unable to load logic library!");
#endif
            return MStatus::kFailure;
        }
    }

    MStatus result;

    // NOTE: (yliangsiew) Simple example function code here
    //MDataHandle envelopeHandle = block.inputValue(envelope, &result);
    //CHECK_MSTATUS_AND_RETURN_IT(result);
    //float envelope = envelopeHandle.asFloat();

    // Call a method defined in dll.
    const void *in_arg = reinterpret_cast<const void*>(&input);
    void *out_arg = reinterpret_cast<void*>(output);

    try {
      kLogicLibrary.exportCB(in_arg, out_arg);
    } catch (std::exception &e) {
      std::cerr << "[HotReload] Exception happened inside logic dll. what = " << e.what() << std::endl;
      // TODO(LTE): recover previous state?
    }

    return result;
}
