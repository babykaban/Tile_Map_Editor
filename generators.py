import sub_functions
import objects

def CleanUpTrees(array, width):
    result = array
    index = 0
    tree_count = 0
    for tile in result:
        if tile == 9 or tile == 10 or tile == 11:
            tree_count += 1
            if index + 1 < len(result) or index + 2 < len(result):
                if tile == result[index + 1] or tile == result[index + 2]:
                    result[index + 1] = 4
                    result[index + 2] = 4
            else:
                pass    
            
            if index + width < len(result) or index + width + 1 < len(result):
                if tile == result[index + width] or tile == result[index + width + 1]:
                    result[index + width]  = 4
                    result[index + width + 1]  = 4
            else:
                pass
        index += 1
    print(tree_count)
    return result


def GenerateTrees(array, width):
    tile_index = 0
    for tile in array:
        if tile == 4:
            rand = sub_functions.random.randint(0, 2) # Set value depending on amout of different trees
            if sub_functions.roll(objects.trees[rand]["chance"]) and sub_functions.roll(20):
                array[tile_index] = objects.trees[rand]["index"]
        else:
            pass
        tile_index += 1
    return CleanUpTrees(array, width)