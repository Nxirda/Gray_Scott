#!/usr/bin/env python

import numpy as np
import matplotlib
from matplotlib import colormaps

#print(list(colormaps))

cmap = matplotlib.colormaps['inferno']

# Numbers of values to generate 
N = 255
values = np.linspace(0, 1, N)
colors = (cmap(values)[:,:3] * 255).astype(int)

header      = "#pragma once\n\n"
includes    = "#include \"types.h\"\n\n"
tab_init    = f"const u32 colormap[{N}] =\n"



def make_rgb(r,g,b):
    alpha = 255
    return (r << 24) | (g << 16) | (b << 8) | alpha

with open("colormap.h", "w") as f:
    f.write(header)
    f.write(includes)
    f.write(tab_init)
    f.write("{\n") 
    for i, (r,g,b) in enumerate(colors):
        comma = "," if i < N-1 else ""
        res = make_rgb(r,g,b)
        f.write(f"    {res}{comma}\n")
    f.write("};\n") 
