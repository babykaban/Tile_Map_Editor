from random import randint

def roll(chance):
    """ Compute chances of objects"""
    random_number = randint(1, 100)
    if random_number <= chance:
        return True
    else:
        return False

def ChooseTile(tiles):
    index = randint(0, len(tiles) - 1)
    return tiles[index]