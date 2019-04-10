
## XGen Iteractive Grooming

### Width

Width parameter can be obtained from Shape information(e.g. `description1_Shape.width`)

* `width`(Width Scale) float
* `widthTaper`(Taper) float
* `widthTaperStart`(Taper Start) float
* `widthRamp[]`(Width Ramp) array

Each width ramp parameter has the following parameters

* `widthRamp_Position` float
* `widthRamp_Interp` enum(int)
* `widthRamp_FloatValue` float

## legacy XGen

### Load from XPD file

Width scale and width taper cannot be changed after reading hair data from XPD file using `From XPD file` mode in legacy XGen.

Thus width setting must be embedded into XPD file propery. To do this, we first need to read width parameter from Shape node, then write width information to XPD. It looks there is no XGen IG python or C/C++ API to retrieve width parameter directly from XGen spline primitives.
