import spaudiopy as spa
import numpy as np
import json
import matplotlib.pyplot as plt

t_design_order = 10  # Results in 60 positions
export_path = "_configurationFiles/t_design_"

orders = [4, 6, 8, 9, 10]
# Load t-design

for order in orders:

    positions = spa.grids.load_t_design(order)
    nChannels = positions.shape[0]
    spa.plot.hull(spa.decoder.get_hull(*positions.T))

    # Convert to spherical coordinates
    azi, zen, r = spa.utils.cart2sph(positions[:, 0], positions[:, 1], positions[:, 2])

    azi = spa.utils.rad2deg(azi)
    zen = spa.utils.rad2deg(zen)

    # Convert zenith to elevation
    ele = 90 - zen

    # Parse as JSON
    ls = []
    for i in range(len(azi)):
        tmp_dict = {
            "Azimuth": azi[i],
            "Elevation": ele[i],
            "Radius": r[i],
            "IsImaginary": False,
            "Channel": i + 1,
            "Gain": 1.0,
        }

        ls.append(tmp_dict)

    out = {
        "Name": f"t-design ({nChannels} positions)",
        "Description": f"t-design of degree {order} ({nChannels} positions), which can be used to decode Ambisonic signals, process them with non-Ambisonic effects, and encode them again.",
        "LoudspeakerLayout": {
            "Name": f"t-design ({nChannels} positions)",
            "Description": f"t-design of degree {order} ({nChannels} positions), which can be used to decode Ambisonic signals, process them with non-Ambisonic effects, and encode them again.",
            "Loudspeakers": ls,
        },
    }

    fn = f"{export_path}{nChannels}ch.json"
    # Writing to sample.json
    with open(fn, "w") as outfile:
        outfile.write(json.dumps(out, indent=2))
