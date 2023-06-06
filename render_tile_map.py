import json
import pygame
from pygame.locals import *

pygame.init()

# Difene tile_map
with open("output/tile_map_array_py.json", "r") as json_file:
    json_data = json_file.read()

tile_map = json.loads(json_data)

# Define the dimensions of the game world
world_width, world_height = 11520, 10800

# Define the dimensions of the screen or viewport
viewport_width, viewport_height = 1200, 640

# Create the screen or viewport
screen = pygame.display.set_mode((viewport_width, viewport_height))

# Create the game world
world = pygame.Surface((world_width, world_height))

# Load the images
tiles_objects = ["tiles/grass_5.bmp", "tiles/grass_6.bmp", "tiles/grass_7.bmp", "tiles/grass_8.bmp",
                "tiles/grass_1.bmp", "tiles/grass_2.bmp", "tiles/grass_3.bmp", "tiles/grass_4.bmp",
                "tiles/water_1.bmp", "tiles/water_2.bmp", "tiles/water_3.bmp", "tiles/water_4.bmp",
                "tiles/sand_1.bmp", "tiles/sand_2.bmp", "tiles/sand_3.bmp", "tiles/sand_4.bmp",
                "tiles/path_1.bmp", "tiles/path_2.bmp", "tiles/path_3.bmp", "tiles/path_4.bmp",
                "oak_tree.bmp", "spruce_tree.bmp", "birch_tree.bmp", "bush_1.bmp",
                "cliff_waterfall.bmp", "cliff_left_bottom.bmp", "cliff_right_bottom.bmp",
                "stairs.bmp", "cliff_vertical_bottom.bmp", "cliff_rightmost_bottom.bmp",
                "bridge.bmp", "tiles/grass_1.bmp", "tiles/grass_2.bmp"]

images = []
for file_path in tiles_objects:
    im = pygame.image.load(file_path)
    images.append(im)

# Set the initial position of the viewport
viewport_x, viewport_y = 0, 0

running = True
while running:
    for event in pygame.event.get():
        if event.type == QUIT:
            running = False

    # Move the viewport with arrow keys
    keys = pygame.key.get_pressed()
    if keys[K_LEFT]:
        viewport_x -= 60
    if keys[K_RIGHT]:
        viewport_x += 60    
    if keys[K_UP]:
        viewport_y -= 60
    if keys[K_DOWN]:
        viewport_y += 60

    # Limit the viewport within the game world
    viewport_x = max(0, min(viewport_x, world_width - viewport_width))
    viewport_y = max(0, min(viewport_y, world_height - viewport_height))

    # Fill the game world with a background color (optional)
    world.fill((0, 0, 0))

    # Blit the image onto the game world at its original position
    tile_x = 0
    tile_y = 10800
    for tile_index in tile_map:
        world.blit(images[tile_index - 1], (tile_x, tile_y)) 
        
        tile_x += 30
        if tile_x == 11520:
            tile_y -= 30
            tile_x = 0

    # Render the portion of the game world within the viewport onto the screen
    screen.blit(world, (0, 0), (viewport_x, viewport_y, viewport_width, viewport_height))

    pygame.display.flip()

pygame.quit()