from PIL import Image
import numpy as np
import json
import tiles
import generators
import sub_functions

import time

# ==========================================================================
# The first element is always 0
z_coords_tiles_ovbjects = \
[
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
    0, 0, 0, 0,  1, 1, 1, 1,  2, 2, 2, 1,  3, 3, 2, 0,
    1                            
] 

map_array = []

image_width = 11520
image_height = 10800
tile_side_pixels = 30
tiles_per_width = int(image_width / tile_side_pixels)

print(tiles_per_width)
# 384x360
# ==============================================================================

def ComputeTile(x, y, im):
    """ Compute each tile"""
    colors = []
    # Iterate each row in tile
    for i in range(y, y + tile_side_pixels):
        # Iterate each column in tile
        for j in range(x, x + tile_side_pixels):
            # Compute values
            color = im.getpixel((j, i))
            if color in tiles.ground_tiles:
                colors.append(sub_functions.ChooseTile(tiles.ground_tiles[color]))
            else:
                colors.append(0)

    arr = np.array(colors)
    counts = np.bincount(arr)
    most_frequent_element = np.argmax(counts)
    return most_frequent_element

# Open the BMP file
image_name = "data/forest_location" + ".bmp"
with Image.open(image_name) as im:
    # Iterate each row
    for y in reversed(range(0, image_height, tile_side_pixels)):
        # Iterate each column
        start_time = time.time()
        for x in range(0, image_width, tile_side_pixels):
            obj_tile_index = ComputeTile(x, y, im)
            map_array.append(int(obj_tile_index))
        end_time = time.time()
        #print(end_time - start_time)    
        print(y)

# Generators
new_map = generators.GenerateTrees(map_array, image_width)

# Write array into file for Python Renderer 
json_data = json.dumps(new_map)
with open("output/tile_map_array_py.json", "w") as json_file:
    json_file.write(json_data)

# Set values for Cpp Renderer
array_to_write = []
for element in new_map:
    number = (element << 16) | z_coords_tiles_ovbjects[element - 1]
    array_to_write.append(hex(number))

# Write array for Cpp Renderer
with open("output/tile_map_array_cpp.txt", "w") as f:
    # iterate over the list in chunks of 64 elements
    f.write("{\n")
    for i in range(0, len(array_to_write), tiles_per_width):
        # get a slice of 64 elements
        row = array_to_write[i:i+tiles_per_width]
        # format the row as a string with commas between the elements
        row_str = ", ".join(str(x) for x in row)
        # write the row to the file, followed by a newline character
        f.write("    {" + row_str + "}, \n")
    f.write("};")