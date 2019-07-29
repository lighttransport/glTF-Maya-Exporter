cmake_minimum_required(VERSION 3.2.3)

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project("glTF-Maya-Exporter-LTE")

# -- options ----------------------------------------------------
option(GLTF_MAYA_EXPORTER_BUILD_WITH_DRACO "Build with Draco" ON)
option(GLTF_MAYA_EXPORTER_BUILD_MAYA_GLTF_EXPORTER "Build Maya glTF Exporter"
       ON)
option(GLTF_MAYA_EXPORTER_BUILD_MAYA_VRM_EXPORTER "Build Maya VRM Exporter" ON)
option(GLTF_MAYA_EXPORTER_BUILD_WITH_HOT_RELOAD
       "Build with hot reload feature(for developer)" OFF)
option(GLTF_MAYA_EXPORTER_BUILD_SERIALIZER
       "Build messegae serializer(for developer)" OFF)
option(GLTF_MAYA_EXPORTER_BUILD_CPP_INTERFACE "Build cpp interface and test"
       OFF)
option(GLTF_MAYA_EXPORTER_WITH_CCACHE
       "Compile with ccache(if available. for developer)" ON)

if(WIN32)
  set(GLTF_MAYA_EXPORTER_MAYA_PATH "C:/Program Files/Autodesk/Maya2018"
      CACHE PATH "Installed Maya Directory")
elseif(APPLE)
  set(GLTF_MAYA_EXPORTER_MAYA_PATH "/Applications/Autodesk/maya2018"
      CACHE PATH "Installed Maya Directory")
endif()
# ===============================================================

set(CMAKE_POSITION_INDEPENDENT_CODE On)

# [ccache]
if(GLTF_MAYA_EXPORTER_WITH_CCACHE)
  find_program(CCACHE_EXE ccache)
  if(CCACHE_EXE)

    message(STATUS "Use ccache : " ${CCACHE_EXE})
    # CMAKE_C_COMPILER_LAUNCHER = available from cmake 3.4+)
    if(CMAKE_C_COMPILER_LAUNCHER)
      set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}"
          "${CCACHE_EXE}")
    else()
      set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXE}")
    endif()

    if(CMAKE_CXX_COMPILER_LAUNCHER)
      set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_CXX_COMPILER_LAUNCHER}"
          "${CCACHE_EXE}")
    else()
      set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXE}")
    endif()

  endif(CCACHE_EXE)

endif()

# -- draco setting ----------------------------------------------
if(GLTF_MAYA_EXPORTER_BUILD_WITH_DRACO)
  # DRACO setting
  add_subdirectory(externals/draco)

  # beautify ME!
  set_target_properties(draco PROPERTIES FOLDER Draco)
  set_target_properties(draco_attributes PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_attributes_dec
                        PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_attributes_enc
                        PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_attributes_pred_schemes_dec
                        PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_attributes_pred_schemes_enc
                        PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_decode PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_encode PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_mesh_dec PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_mesh_enc PROPERTIES FOLDER Draco)
  set_target_properties(draco_core PROPERTIES FOLDER Draco)
  set_target_properties(draco_core_bit_coders PROPERTIES FOLDER Draco)
  set_target_properties(draco_dec_config PROPERTIES FOLDER Draco)
  set_target_properties(draco_decoder PROPERTIES FOLDER Draco)
  set_target_properties(draco_enc_config PROPERTIES FOLDER Draco)
  set_target_properties(draco_encoder PROPERTIES FOLDER Draco)
  set_target_properties(draco_io PROPERTIES FOLDER Draco)
  set_target_properties(draco_mesh PROPERTIES FOLDER Draco)
  set_target_properties(draco_metadata PROPERTIES FOLDER Draco)
  set_target_properties(draco_metadata_dec PROPERTIES FOLDER Draco)
  set_target_properties(draco_metadata_enc PROPERTIES FOLDER Draco)
  set_target_properties(draco_point_cloud PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_point_cloud_dec
                        PROPERTIES FOLDER Draco)
  set_target_properties(draco_compression_point_cloud_enc
                        PROPERTIES FOLDER Draco)
  set_target_properties(draco_points_dec PROPERTIES FOLDER Draco)
  set_target_properties(draco_points_enc PROPERTIES FOLDER Draco)
  set_target_properties(dracodec PROPERTIES FOLDER Draco)
  set_target_properties(dracoenc PROPERTIES FOLDER Draco)

  set(DRACO_LIB draco)
  add_definitions(-DENABLE_BUILD_WITH_DRACO)

  set(DRACO_SRC_PATH externals/draco/src/)
endif()

if(GLTF_MAYA_EXPORTER_BUILD_MAYA_GLTF_EXPORTER
   OR GLTF_MAYA_EXPORTER_BUILD_MAYA_VRM_EXPORTER)
  if(WIN32)
    set(MAYA_INCLUDE_PATH ${GLTF_MAYA_EXPORTER_MAYA_PATH}/include/)
    set(MAYA_LIBRARY_PATH ${GLTF_MAYA_EXPORTER_MAYA_PATH}/lib/)
  elseif(APPLE)
    set(MAYA_INCLUDE_PATH ${GLTF_MAYA_EXPORTER_MAYA_PATH}/include/)
    set(MAYA_LIBRARY_PATH
        ${GLTF_MAYA_EXPORTER_MAYA_PATH}/Maya.app/Contents/MacOS)
  elseif(UNIX)
    set(MAYA_INCLUDE_PATH ${GLTF_MAYA_EXPORTER_MAYA_PATH}/include/)
    set(MAYA_LIBRARY_PATH ${GLTF_MAYA_EXPORTER_MAYA_PATH}/lib/)
  endif()

  set(BUILD_DIR ${CMAKE_BINARY_DIR})
  message(BUILD_DIR: "${BUILD_DIR}")
  message(MAYA_INCLUDE_PATH: "${MAYA_INCLUDE_PATH}")
  message(MAYA_LIBRARY_PATH: "${MAYA_LIBRARY_PATH}")

  # Xgen
  set(XGEN_INCLUDE_PATH ${GLTF_MAYA_EXPORTER_MAYA_PATH}/plug-ins/xgen/include)
  set(XGEN_LIBRARY_PATH ${GLTF_MAYA_EXPORTER_MAYA_PATH}/plug-ins/xgen/lib/)
  if(WIN32)
	  set(XGEN_LIBS libAdskSeExpr libAdskXGen libAdskXpd)
  else()
    set(XGEN_LIBS AdskXGen)
  endif(WIN32)

endif()

# -- compile options, include and libraries -----------------

set(CMAKE_CXX_STANDARD 11)

include_directories(externals/glm/
                    externals/
                    src/
                    ${BUILD_DIR}/
                    ${DRACO_SRC_PATH}
                    ${MAYA_INCLUDE_PATH}
                    ${XGEN_INCLUDE_PATH})

link_directories(${MAYA_LIBRARY_PATH} ${XGEN_LIBRARY_PATH})

# -----------------------------------------------------------

# -- KIL lib ------------------------------------
add_library(kil
            STATIC
            ./src/kil/CopyTextureFile.cpp
            ./src/kil/CopyTextureFile_GdiPlus.cpp
            ./src/kil/CopyTextureFile_STB.cpp
            ./src/kil/ResizeTextureFile.cpp)

# -- KML lib ------------------------------------
if(WIN32)
  set(Compatibility ./src/kml/Compatibility.cpp)
endif()

add_library(kml
            STATIC
            ./src/kml/Bound.cpp
            ./src/kml/CalculateBound.cpp
            ./src/kml/CalculateNormalsMesh.cpp
            ${Compatibility}
            ./src/kml/FlatIndicesMesh.cpp
            ./src/kml/GLTF2GLB.cpp
            ./src/kml/glTFExporter.cpp
            ./src/kml/Material.cpp
            ./src/kml/Mesh.cpp
            ./src/kml/Node.cpp
            ./src/kml/NodeExporter.cpp
            ./src/kml/Options.cpp
            ./src/kml/SaveToDraco.cpp
            ./src/kml/SplitNodeByMaterialID.cpp
            ./src/kml/Transform.cpp
            ./src/kml/TriangulateMesh.cpp)

target_link_libraries(kml kil ${DRACO_LIB})

# force define for maya header ifdef condition (building for makefile project)
if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32 -DNT_PLUGIN")
elseif(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOSMac_")
elseif(UNIX)
  # Maya libraries are compiled with gcc 4.8(with old ABI)
  # Must disable new CXX ABI(which is default from gcc 5.1) otherwise some symbols failed to resolve when loading a plugin.
  # https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0 -Dlinux -m64 -g -pthread -D_BOOL -DLINUX -DLINUX_64 -DFUNCPROTO -DREQUIRE_IOSTREAM -fPIC -Wno-deprecated -fno-gnu-keywords"
    )
endif()

if(GLTF_MAYA_EXPORTER_BUILD_WITH_HOT_RELOAD)

  # Include generated *.pb.h files
  # include_directories("${CMAKE_CURRENT_BINARY_DIR}")

  # some protobuf headers are still required even when using flatbuffer serialization.
  #include_directories("${CMAKE_SOURCE_DIR}/externals/grpc/third_party/protobuf/src/")

  #
  # Experimental. Use flatbuffers for serialization
  #

  ## [flatbuffers]
  #set(FLATBUFFERS_BUILD_TESTS Off CACHE INTERNAL "" FORCE)
  #set(FLATBUFFERS_BUILD_FLATHASH Off CACHE INTERNAL "" FORCE)
  #set(FLATBUFFERS_INSTALL Off CACHE INTERNAL "" FORCE)

  #add_subdirectory(${CMAKE_SOURCE_DIR}/externals/flatbuffers)

  ## Flatbuffer header files.
  #include_directories("${CMAKE_SOURCE_DIR}/externals/flatbuffers/include")

  ## Assume `$BUILD/externals/flatbuffers/flatc`
  #set(_FLATBUFFER_FLATC $<TARGET_FILE:flatc>)

  ## Flatbuffer file
  #get_filename_component(hotreload_fbs
  #                       "${CMAKE_SOURCE_DIR}/src/hotreload/hotreload.fbs"
  #                       ABSOLUTE)
  #get_filename_component(hotreload_fbs_path "${hotreload_fbs}" PATH)

  ## Generated sources
  #set(hotreload_fb_hdrs "${CMAKE_CURRENT_BINARY_DIR}/hotreload_generated.h")
  ##set(hotreload_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/hotreload.grpc.fb.cc")
  ##set(hotreload_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/hotreload.grpc.fb.h")
  #add_custom_command(OUTPUT "${hotreload_fb_hdrs}" "${hotreload_grpc_srcs}"
  #                          "${hotreload_grpc_hdrs}"
  #                   COMMAND ${_FLATBUFFER_FLATC}
  #                           ARGS -o "${CMAKE_CURRENT_BINARY_DIR}"
  #                           --grpc -c "${hotreload_fbs}"
  #                   DEPENDS "${hotreload_fbs}")

  set(EXPORTER_LOGIC_PLUGIN_NAME "exporter_logic")

  add_library(${EXPORTER_LOGIC_PLUGIN_NAME}
              SHARED
              ./src/hotreload/HotReloadExporterLogic.cc
              ./src/hotreload/XGenSplineToCyHair.cc
              ./src/hotreload/XGenSplineToXPD.cc
              ./src/hotreload/cyhair-writer.cc)

  # Must link with OpenMaya otherwise undefined symbol error happens when
  # loading dll(Linux only?).
  target_link_libraries(${EXPORTER_LOGIC_PLUGIN_NAME}
                        ${XGEN_LIBS}
                        OpenMaya
                        OpenMayaAnim # For SkinCluster
                        OpenMayaUI # For progress window
                        Foundation)

  if(WIN32)
    set_target_properties(${EXPORTER_LOGIC_PLUGIN_NAME}
                          PROPERTIES PREFIX
                                     ""
                                     SUFFIX
                                     ".dll")
  elseif(APPLE OR UNIX)
    set_target_properties(${EXPORTER_LOGIC_PLUGIN_NAME}
                          PROPERTIES PREFIX
                                     ""
                                     SUFFIX
                                     ".so")
  endif()

  set_target_properties(
    ${EXPORTER_LOGIC_PLUGIN_NAME}
    PROPERTIES
      LINK_FLAGS
      "-shared -m64 -g -pthread -pipe -D_BOOL -DLINUX -DLINUX_64 -DFUNCPROTO -DREQUIRE_IOSTREAM -fPIC -Wno-deprecated -fno-gnu-keywords -Wl,-Bsymbolic"
    )

  ##
  ## Build dummy server
  ##
  #if (WIN32)
  #  # Don't build dummy server for a while.
  #  # If we need to build dummy server on windows, we must set _WIN32_WINNT >= 0x600
  #  # add_definitions("-D_WIN32_WINNT=0x600")
  #else()
  #  set(DUMMY_SERVER_SOURCES ./src/dummyserver/dummy-server.cc)

  #  add_executable(dummy_server
  #              ${DUMMY_SERVER_SOURCES}
  #              ${hotreload_fb_srcs})

  #  # Must link with OpenMaya otherwise undefined symbol error happens when
  #  # loading dll(Linux only?).
  #  target_link_libraries(dummy_server
  #                        ${_GRPC_GRPCPP_UNSECURE}
  #                        flatbuffers)
  #endif()

  add_definitions("-DGLTF_EXPORTER_ENABLE_HOT_RELOAD")
  list(APPEND EXTRA_GLTF_EXPORTER_SOURCES ./src/hotreload/HotReloadExporter.cc
                ./src/hotreload/HotReloadExporterPlatform.cc)

else()
  list(APPEND EXTRA_GLTF_EXPORTER_SOURCES
         ./src/hotreload/XGenSplineToCyHair.cc
	 ./src/hotreload/XGenSplineToXPD.cc
         ./src/hotreload/cyhair-writer.cc)

endif()

# -- glTF Exporter for maya --------------------
if(GLTF_MAYA_EXPORTER_BUILD_MAYA_GLTF_EXPORTER)

  add_library(glTFExporter
              SHARED
              ./src/glTFExporter/glTFExporterRegister.cpp
              ./src/glTFExporter/glTFExporter.cpp
              ./src/glTFExporter/glTFTranslator.cpp
              ./src/glTFExporter/murmur3.c
              ./src/glTFExporter/ProgressWindow.cpp
              ${EXTRA_GLTF_EXPORTER_SOURCES})

  target_link_libraries(glTFExporter
                        kml
                        ${XGEN_LIBS}
                        OpenMaya
                        OpenMayaAnim # For SkinCluster
                        OpenMayaUI # For progress window
                        Foundation)
  set_target_properties(glTFExporter PROPERTIES PREFIX "")
  if(WIN32)
    # It looks xgen lib requires some extra definitions for MSVC target otherwise crashes at runtime
    # FIXME(LTE): _HAS_ITERATOR_DEBUGGING=0 will fail when Debug build
    target_compile_definitions(glTFExporter PUBLIC NDEBUG OSWin_ NT_PLUGIN _USRDLL TBB_USE_DEBUG=0 __TBB_LIB_NAME=tbb.lib Bits64_ _WINDLL) 
    set_target_properties(glTFExporter PROPERTIES SUFFIX ".mll")
  elseif(APPLE)
    set_target_properties(glTFExporter PROPERTIES SUFFIX ".bundle")
  else()
    set_target_properties(glTFExporter PROPERTIES SUFFIX ".so")
    set_target_properties(
      glTFExporter
      PROPERTIES
        LINK_FLAGS
        "-shared -m64 -g -pthread -pipe -D_BOOL -DLINUX -DLINUX_64 -DFUNCPROTO -DREQUIRE_IOSTREAM -fPIC -Wno-deprecated -fno-gnu-keywords -Wl,-Bsymbolic"
      )
  endif()

  # [VisualStudio] default startup
  if(WIN32)
    if(CMAKE_VERSION VERSION_GREATER 3.6.0)
      set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT glTFExporter)
    endif()
  endif()
endif()

# -- VRM Exporter for maya --------------------
if(GLTF_MAYA_EXPORTER_BUILD_MAYA_VRM_EXPORTER)

  add_library(vrmExporter
              SHARED
              ./src/glTFExporter/vrmExporterRegister.cpp
              ./src/glTFExporter/glTFExporter.cpp
              ./src/glTFExporter/glTFTranslator.cpp
              ./src/glTFExporter/murmur3.c
              ./src/glTFExporter/ProgressWindow.cpp
              ${EXTRA_GLTF_EXPORTER_SOURCES})

  target_compile_definitions(vrmExporter PUBLIC ENABLE_VRM=1)

  target_link_libraries(vrmExporter
                        kml
                        ${XGEN_LIBS}
                        OpenMaya
                        OpenMayaAnim
                        OpenMayaUI
                        Foundation)
  set_target_properties(vrmExporter PROPERTIES PREFIX "")
  if(WIN32)
    set_target_properties(vrmExporter PROPERTIES SUFFIX ".mll")
  elseif(APPLE)
    set_target_properties(vrmExporter PROPERTIES SUFFIX ".bundle")
  else()
    set_target_properties(vrmExporter PROPERTIES SUFFIX ".so")
    set_target_properties(
      vrmExporter
      PROPERTIES
        LINK_FLAGS
        "-shared -m64 -g -pthread -pipe -D_BOOL -DLINUX -DLINUX_64 -DFUNCPROTO -DREQUIRE_IOSTREAM -fPIC -Wno-deprecated -fno-gnu-keywords -Wl,-Bsymbolic"
      )
  endif()

endif()

# -- cpp interface and test -----------------------------

if(GLTF_MAYA_EXPORTER_BUILD_CPP_INTERFACE)

  # -- dll interface --
  add_library(glTFCppExporterLib SHARED ./src/cppexporter/main.cpp)
  if(ANDROID)
    set(androidlibs log m)
  endif()
  target_link_libraries(glTFCppExporterLib kml ${androidlibs})

  # -- interface test ---
  add_executable(glTFCppExporterLibTest ./src/cppexporter/test/test.cpp)
  target_link_libraries(glTFCppExporterLibTest glTFCppExporterLib)

  # [VisualStudio] default startup
  if(WIN32)
    if(CMAKE_VERSION VERSION_GREATER 3.6.0)
      set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT glTFCppExporterLibTest)
    endif()
  endif()
endif()
