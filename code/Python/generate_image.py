from PIL import Image
import json

# Define the image dimensions and color mode
width = 30
height = 30
color_mode = "RGB"  # or "RGBA" for an image with an alpha channel

tiles = []

with open("tiles_file.json") as file:
    tiles = json.load(file)

# Create a new image with the specified dimensions and color mode
image = Image.new(color_mode, (width, height))

name_array = ''
for tile in tiles:
    for x in range(width):
        for y in range(height):
            red = tile["color"][0]
            green = tile["color"][1]
            blue = tile["color"][2]
            image.putpixel((x, y), (red, green, blue))

    # Save the image as a BMP file
    #name_array += ', "{}.bmp"'.format(tile["name"])
    image.save("C:/Paul/Projects/SpellweaverSaga/data/tiles/" + tile["name"] + ".bmp", 
               compress_type=3)
    #print("Created " + tile["name"] + " tile")
print(name_array)
