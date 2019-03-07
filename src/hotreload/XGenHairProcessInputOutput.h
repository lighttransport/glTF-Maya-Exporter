#pragma once

#include <vector>

#include <maya/MObject.h>
#include <maya/MDagPath.h>

struct XGenHairProcessInput
{
    MDagPath dagPath; // DagPath of xgen IG node(collection)

    const int num_strands = -1;         // # of strands to export. -1 = export all strands.
    const bool phantom_points = false;  // Add phantom points?
    const bool cv_repeat = true;        // Repeat the first and the last CV point?

    // NOTE:
    // Export CV points as in Maya XGen when
    // phantom_points == false and cv_repeat == false

};

struct XGenHairProcessOutput
{
    std::vector<float> texcoords;
    std::vector<float> points;
    std::vector<float> radiuss;
    std::vector<uint32_t> num_points;

    MObject shader; // surface shader(material) assigend to XGen IG node.
};
