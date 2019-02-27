#include "HotReloadExporterPlatform.h"
#include <maya/MString.h>
#include <maya/MGlobal.h>

// -- global variables ----------

MString kPluginLogicLibraryPath;
ExporterLogicLibrary kLogicLibrary = {};

// ----------------


MString getExporterLogicLibraryPath(const char *pluginPath)
{
  
  MGlobal::displayInfo("getExporterLogicLibraryPath");

	if (strlen(pluginPath) <= 0) {
    MGlobal::displayError("pluginPath is empty or invalid");
		return MString();
	}
	char pathDelimiter[2] = {kPathDelimiter, '\0'};
	MString delimiter(pathDelimiter);
	MString pluginPathStr(pluginPath);
	MString libFilename = pluginPathStr + delimiter + kExporterLogicLibraryName;
  MGlobal::displayInfo("pluginPath [" + pluginPathStr + "], lbFilename = [" + libFilename + "]");

	return libFilename;
}


LibraryStatus loadExporterLogicDLL(ExporterLogicLibrary &library)
{
	const char *libFilenameC = kPluginLogicLibraryPath.asChar();
  MGlobal::displayInfo("loadExporterLogicDLL. library path [" + kPluginLogicLibraryPath + "]");

	FileTime lastModified = getLastWriteTime(libFilenameC);
	library.lastModified = lastModified;

  MGlobal::displayInfo("Loading shared library : [" + kPluginLogicLibraryPath + "]");
	DLLHandle handle = loadSharedLibrary(libFilenameC);
	if (!handle) {
		MGlobal::displayError("Unable to load logic library!");
		library.handle = NULL;
		library.lastModified = {};
		library.isValid = false;

		return LibraryStatus_InvalidLibrary;
	}

	library.handle = handle;

	FuncPtr getValueFuncAddr = loadSymbolFromLibrary(handle, "exportFunc");
	if (!getValueFuncAddr) {
		MGlobal::displayError("Could not find symbols in library!");
		return LibraryStatus_InvalidSymbol;
	}

	library.exportCB = (ExportFunc)getValueFuncAddr;
	library.isValid = true;

	MGlobal::displayInfo("Loaded library from: " + kPluginLogicLibraryPath);

	return LibraryStatus_Success;
}


LibraryStatus unloadExporterLogicDLL(ExporterLogicLibrary &library)
{
  fprintf(stderr, "unloadExporterLogicDLL\n");

	if (!kLogicLibrary.isValid) {
    fprintf(stderr, "invalid handle\n");
		return LibraryStatus_InvalidHandle;
	}
	int unload = unloadSharedLibrary(kLogicLibrary.handle);
	if (unload != 0) {
		MGlobal::displayError("Unable to unload shared library!");
		return LibraryStatus_UnloadFailure;
	}

  fprintf(stderr, "unloaded\n");

	library.exportCB = NULL;
	library.lastModified = {};
	library.isValid = false;

	return LibraryStatus_Success;
}
