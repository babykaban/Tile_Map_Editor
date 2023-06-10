import sub_functions
import objects
import tiles

def CheckTileObjects(tile, items):
    result = False
    for item in items:
        if item["index"] == tile:
            result = True
            break
    return result 

def CleanUpTrees(array, width):
    result = array
    index = 0
    tree_count = 0
    for tile in result:
        if CheckTileObjects(tile, objects.trees):
            tree_count += 1
            if index + 1 < len(result) or index + 2 < len(result):
                if tile == result[index + 1] or tile == result[index + 2]:
                    result[index + 1] = sub_functions.ChooseTile(
                        tiles.ground_tiles[(58, 162, 0, 255)])
                    result[index + 2] = sub_functions.ChooseTile(
                        tiles.ground_tiles[(58, 162, 0, 255)])
            else:
                pass    
            
            if index + width < len(result) or index + width + 1 < len(result):
                if tile == result[index + width] or tile == result[index + width + 1]:
                    result[index + width]  = sub_functions.ChooseTile(
                        tiles.ground_tiles[(58, 162, 0, 255)])
                    result[index + width + 1]  = sub_functions.ChooseTile(
                        tiles.ground_tiles[(58, 162, 0, 255)])
            else:
                pass
        index += 1
    print(tree_count)
    return result


def GenerateTrees(array, width):
    tile_index = 0
    for tile in array:
        if tile <= 8 and tile >= 5:
            rand = sub_functions.randint(0, 2) # Set value depending on amout of different trees
            if sub_functions.roll(objects.trees[rand]["chance"]) and sub_functions.roll(20):
                array[tile_index] = objects.trees[rand]["index"]
        else:
            pass
        tile_index += 1
    return CleanUpTrees(array, width)


# def GenerateBushes(array, width):
#     tile_index = 0
#     for tile in array:
#         if tile <= 4 and tile != 0:
#             #rand = sub_functions.randint(0, 2) # Set value depending on amout of different trees
#             if sub_functions.roll(objects.trees[rand]["chance"]) and sub_functions.roll(20):
#                 array[tile_index] = objects.trees[rand]["index"]
#         else:
#             pass
#         tile_index += 1
#     return CleanUpTrees(array, width)