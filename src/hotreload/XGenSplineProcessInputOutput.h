#pragma once

#include <vector>

#include <maya/MObject.h>
#include <maya/MDagPath.h>

struct XGenSplineProcessInput
{
    std::string hair_format = "cyhair";  // "cyhair" or "xpd"

    MDagPath dagPath; // DagPath of xgen IG node(collection)

    int num_strands = -1;         // # of strands to export. -1 = export all strands.
    bool phantom_points = false;  // Add phantom points?
    bool cv_repeat = false;        // Repeat the first and the last CV point?

    // NOTE:
    // Export CV points as-is(same data structure in Maya XGen) when
    // phantom_points == false and cv_repeat == false

};

struct XGenSplineProcessOutput
{
    std::vector<float> texcoords; // 1D texcoord array
    std::vector<float> points; // 1D points array
    std::vector<float> radiuss; // 1D radius(width) array
    std::vector<uint32_t> num_points; // array of # of CVs(control vertices) per strand.

    MObject shader; // SG node assigend to XGen IG node.

    std::string cyhair_data;    // XGen spline data serializd to CyHair binary format.

    std::string xpd_data;       // XGen spline data serialized to XPD format.
};
