import random
def roll(chance):
    """ Compute chances of objects"""
    random_number = random.randint(1, 100)
    if random_number <= chance:
        return True
    else:
        return False