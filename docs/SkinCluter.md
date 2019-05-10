Pymel sample

```python
import pymel.core as pm

# Select skinCluter, then

skin = pm.ls(sl=True)

skin[0].getInfluence()
skin[0].getWeightedInfluence()
for i in skin[0].getWeights("pPlane1"):
    print i
skin[0].numInfluenceObjects()
skin[0].setGeometry("pCube2")
```


## Referencs

* https://github.com/OGRECave/ogre/blob/master/Tools/MayaExport/src/skeleton.cpp

