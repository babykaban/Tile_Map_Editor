from PIL import Image
import numpy as np
import random
import json
import tiles
import objects

# ==========================================================================
z_coordinates = [0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 1, 1] # length: 1 + 11

map_array = []

image_width = 11520
image_height = 10800
tile_side_pixels = 30
tiles_per_width = int(image_width / tile_side_pixels)
print(tiles_per_width)
# 384x360
# ==============================================================================


def roll(chance):
    """ Compute chances of objects"""
    random_number = random.randint(1, 100)
    if random_number <= chance:
        return True
    else:
        return False
    
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
                colors.append(tiles.ground_tiles[color][0])
            else:
                colors.append(0)

    arr = np.array(colors)
    counts = np.bincount(arr)
    most_frequent_element = np.argmax(counts)
    return most_frequent_element


def CleanUpTrees():
    global map_array
    index = 0
    tree_count = 0
    for tile in map_array:
        if tile == 9 or tile == 10 or tile == 11:
            tree_count += 1
            if index + 1 < len(map_array) or index + 2 < len(map_array):
                if tile == map_array[index + 1] or tile == map_array[index + 2]:
                    map_array[index + 1] = 4
                    map_array[index + 2] = 4
            else:
                pass    
            
            if index + image_width < len(map_array) or index + image_width + 1 < len(map_array):
                if tile == map_array[index + image_width] or tile == map_array[index + image_width + 1]:
                    map_array[index + image_width]  = 4
                    map_array[index + image_width + 1]  = 4
            else:
                pass
        index += 1
    print(tree_count)


def GenerateTrees():
    global map_array
    tile_index = 0
    for tile in map_array:
        if tile == 4:
            rand = random.randint(0, 2) # Set value depending on amout of different trees
            if roll(objects.trees[rand]["chance"]) and roll(20):
                map_array[tile_index] = objects.trees[rand]["index"]
        else:
            pass
        tile_index += 1
    CleanUpTrees()


# Open the BMP file
image_name = "Map_bmp/forest_location" + ".bmp"
with Image.open(image_name) as im:
    # Iterate each row
    for y in reversed(range(0, image_height, tile_side_pixels)):
        # Iterate each column
        for x in range(0, image_width, tile_side_pixels):
            obj_tile_index = ComputeTile(x, y, im)
            map_array.append(int(obj_tile_index))
        print(y)

# Generators
GenerateTrees()

# Write array into file for Python Renderer 
json_data = json.dumps(map_array)
with open("output.json", "w") as json_file:
    json_file.write(json_data)

# Set values for Cpp Renderer
array_to_write = []
for element in map_array:
    number = (element << 16) | z_coordinates[element]
    array_to_write.append(hex(number))

# Write array for Cpp Renderer
with open("map_array.txt", "w") as f:
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