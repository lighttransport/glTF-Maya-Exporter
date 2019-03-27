#ifdef _MSC_VER
#pragma warning(disable : 4819)
#endif

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MStreamUtils.h>
#include <maya/MString.h>

#include <fstream>
#include <memory>
#include <sstream>

#include <kml/Options.h>

#if defined(GLTF_EXPORTER_ENABLE_HOT_RELOAD)
#include "hotreload/HotReloadExporterPlatform.h"
#endif

#include "glTFTranslator.h"

#define VENDOR_NAME "Light Transport Entertainment, Inc."
#define PLUGIN_NAME "glTF-Maya-Exporter-LTE"
#define PLUGIN_VERSION "1.6.0"
#define EXPOTER_NAME_GLTF "GLTF Export"
#define EXPOTER_NAME_GLB "GLB Export"

const char* const gltfOptionScript = "glTFExporterOptions";
const char* const gltfDefaultOptions =
    "recalc_normals=0;"
    "output_onefile=1;"
    "make_preload_texture=0;"
    "output_buffer=1;"
    "convert_texture_format=1;"
    "output_animations=1;";

static void PrintTextLn(const std::string& str)
{
#if MAYA_API_VERSION >= 20180000
    MStreamUtils::stdOutStream() << str << std::endl;
#else
    std::cerr << str << std::endl;
#endif
}

static void ShowLicense()
{
    std::string showText;
    showText += PLUGIN_NAME;

    showText += " based on glTF-Maya-Exporter by Kashika, Inc.";

    showText += " ";
    showText += "ver";
    showText += PLUGIN_VERSION;

    PrintTextLn(showText);
}

#ifdef _WIN32
__declspec(dllexport)
#endif // _WIN32
    MStatus initializePlugin(MObject obj)
{
    MFnPlugin plugin(obj, VENDOR_NAME, PLUGIN_VERSION, "Any");

    std::shared_ptr<kml::Options> opts = kml::Options::GetGlobalOptions();
    opts->SetString("generator_name", PLUGIN_NAME);
    opts->SetString("generator_version", std::string("ver") + PLUGIN_VERSION);

    ShowLicense();

#if defined(GLTF_EXPORTER_ENABLE_HOT_RELOAD)
    // NOTE: (yliangsiew) Get the OS-specific path to the plugin
    MString pluginPath = plugin.loadPath();
    const char* pluginPathC = pluginPath.asChar();
    const sizet lenPluginPath = strlen(pluginPathC);
    char OSPluginPath[kMaxPathLen];
    strncpy(OSPluginPath, pluginPathC, lenPluginPath + 1);
    int replaced = convertPathSeparatorsToOSNative(OSPluginPath);
    if (replaced < 0)
    {
        MGlobal::displayError("Failed to format path of plugin to OS native version!");
        return MStatus::kFailure;
    }
    if (strlen(OSPluginPath) <= 0)
    {
        MGlobal::displayError("Could not find a path to the plugin!");
        return MStatus::kFailure;
    }

    fprintf(stderr, "OSPluginPath [ %s ]\n", OSPluginPath);
    kPluginLogicLibraryPath = getExporterLogicLibraryPath(OSPluginPath);
    MGlobal::displayInfo("PluginLogicLibrayPath [" + kPluginLogicLibraryPath + "]");

#endif

    // Register the translator with the system
    MStatus status = MS::kSuccess;
    if (status == MS::kSuccess)
    {
        status = plugin.registerFileTranslator(EXPOTER_NAME_GLTF, "none",
                                               glTFTranslator::creatorGLTF,
                                               (char*)gltfOptionScript,
                                               (char*)gltfDefaultOptions);
    }
    if (status == MS::kSuccess)
    {
        status = plugin.registerFileTranslator(EXPOTER_NAME_GLB, "none",
                                               glTFTranslator::creatorGLB,
                                               (char*)gltfOptionScript,
                                               (char*)gltfDefaultOptions);
    }

    return status;
}
//////////////////////////////////////////////////////////////

#ifdef _WIN32
__declspec(dllexport)
#endif // _WIN32
    MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

#if defined(GLTF_EXPORTER_ENABLE_HOT_RELOAD)
    if (kLogicLibrary.isValid && kLogicLibrary.handle)
    {
        unloadExporterLogicDLL(kLogicLibrary);
    }

#endif

    MStatus status = MS::kSuccess;
    if (status == MS::kSuccess)
    {
        status = plugin.deregisterFileTranslator(EXPOTER_NAME_GLB);
    }
    if (status == MS::kSuccess)
    {
        status = plugin.deregisterFileTranslator(EXPOTER_NAME_GLTF);
    }
    return status;
}

//////////////////////////////////////////////////////////////
