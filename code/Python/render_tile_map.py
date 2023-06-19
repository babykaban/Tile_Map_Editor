import json
import pygame
from pygame.locals import *

pygame.init()

# Diffine tile_map
tile_map = []
with open("tilemap_py.txt", "r") as file:
    data = file.read().split()
    for element in data:
        try:
            integer = int(element)
            tile_map.append(integer)
        except ValueError:
            pass

# Load tiles
tiles = []
with open("tiles_file.json") as json_file:
    tiles = json.load(json_file)

# Define the dimensions of the game world
world_width, world_height = 11520, 10800

# Define the dimensions of the screen or viewport
viewport_width, viewport_height = 1200, 640

# Create the screen or viewport
screen = pygame.display.set_mode((viewport_width, viewport_height))

# Create the game world
world = pygame.Surface((world_width, world_height))

# Set the initial position of the viewport
viewport_x, viewport_y = 0, 0

def find_tile_color(index, tiles):
    result = 0
    for tile in tiles:
        if tile["index"] == index:
            result = (tile["color"][0], tile["color"][0], tile["color"][0])
            break
    
    return result

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
    for tile in tile_map:
        tile_index = tile >> 16
        tile_color = find_tile_color(tile_index, tiles)

        draw_tile = pygame.surface.Surface((30, 30))
        draw_tile.fill(tile_color)
        world.blit(draw_tile, (tile_x, tile_y)) 
        
        tile_x += 30
        if tile_x == 11520:
            tile_y -= 30
            tile_x = 0

    # Render the portion of the game world within the viewport onto the screen
    screen.blit(world, (0, 0), (viewport_x, viewport_y, viewport_width, viewport_height))

    pygame.display.flip()

pygame.quit()