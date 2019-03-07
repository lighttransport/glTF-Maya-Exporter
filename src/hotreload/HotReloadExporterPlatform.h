#ifndef HOT_REALOAD_EXPORTER_PLATFORM_H_


// TODO(LTE): move `HotReadExporterPlatform` file to separate directory?

#include <climits> // INT_MIN

#include <ssmath/platform.h>
#include <ssmath/vector_math.h>
#include <ssmath/filesys.h>

#include <maya/MString.h>

/// This is the prototype for the function that will be dynamically hotloaded.
typedef void (*ExportFunc)(const void *in_arg, void *out_arg);


/// This is initialized to the path of the exporter's **business logic** DLL
/// whenever the plugin is initialized.
extern MString kPluginLogicLibraryPath;

#ifdef _WIN32
globalVar const char *kExporterLogicLibraryName = "exporter_logic.dll";

#elif __linux__ || __APPLE__
globalVar const char *kExporterLogicLibraryName = "exporter_logic.so";

#endif // Library filename

enum LibraryStatus
{
  LibraryStatus_Failure = INT_MIN,
  LibraryStatus_InvalidLibrary,
  LibraryStatus_InvalidSymbol,
  LibraryStatus_InvalidHandle,
  LibraryStatus_UnloadFailure,
  LibraryStatus_Success = 0
};

/// This is a data structure that contains information about the state of a DLL
/// that contains all the so-called *business logic* required for the deformer
/// to do its work.
struct ExporterLogicLibrary
{
  DLLHandle handle;
  FileTime lastModified;

  ExportFunc exportCB;
  bool isValid;
};


/// This is the global reference to the *business logic* DLL that is loaded.
extern ExporterLogicLibrary kLogicLibrary;
// = {};


/**
 * This function gets the full path to the *business logic* DLL. This file may/may
 * not exist on disk yet at the time this path is formatted.
 *
 * @param pluginPath  The path to the host Maya plugin DLL. Must use the OS-specific
 *          path separators.
 *
 * @return        The path to the *business logic* DLL.
 */
MString getExporterLogicLibraryPath(const char *pluginPath);


LibraryStatus loadExporterLogicDLL(ExporterLogicLibrary &library);


LibraryStatus unloadExporterLogicDLL(ExporterLogicLibrary &library);





#endif // HOT_REALOAD_EXPORTER_PLATFORM_H_

