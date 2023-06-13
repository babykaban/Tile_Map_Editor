from tkinter import messagebox
from warnings import resetwarnings
import customtkinter as ctk
import json

# Initialize Global Veriables
tiles_array = 0

"""
    tile = 
    {
        "index": 0,
        "color": (0, 0, 0),
        "z_coord": 0
    }

"""

def get_tiles():
    global tiles_array
    with open("tiles_file.json") as file:
        tiles_array = json.load(file)

def push_tile_to_file():
    pass

def check_value(value, type):
    if type == "index":
        if value >= 65536:
            messagebox.showwarning("Warning", "Tile Index Can't be bigger then 65536!")
            return False
        else:
            pass
    elif type == "color":
        if value >= 256:
            messagebox.showwarning("Warning", "Tile ColorRange is from 0 to 255!")
            return False
        else:
            pass
    else:
        if value > 10:
            messagebox.showwarning("Warning", "Z coord can't be more then 10!")
            return False
        else:
            pass

def convert_str_to_int(value):
    result = 0
    
    if value != '':
        try:
            result = int(value)
            return result

        except ValueError:
            messagebox.showwarning("Warning", "The nambers have to be integers")
    else:
        pass

def add_tile():
    global tiles_array

    tile_index = convert_str_to_int(tile_index_entry.get())
    tile_color_R = convert_str_to_int(tile_red_color_entry.get())
    tile_color_G = convert_str_to_int(tile_green_color_entry.get())
    tile_color_B = convert_str_to_int(tile_blue_color_entry.get())
    tile_z_coord = convert_str_to_int(tile_z_coord_entry.get())

    check_value(tile_index)
    check_value(tile_color_R)
    check_value(tile_color_G)
    check_value(tile_color_B)
    check_value(tile_z_coord)

    tile = {"index": tile_index, 
            "color": (tile_color_R, tile_color_G, tile_color_B),
            "z_coord": tile_z_coord}

def remove_tile():
    global tiles_array
    
    pass

def write_array_for_cpp():
    pass

# Take Tiles Info
get_tiles()

# Create the main window
root = ctk.CTk()
root.title("Binary Calculator")
root.geometry("960x540")
root.resizable(False, False)

# Create frames for widgets
entry_frame = ctk.CTkFrame(root, width=300, height=540)
information_frame = ctk.CTkScrollableFrame(root, width=620, height=520)

# Create Entries 
tile_index_entry = ctk.CTkEntry(entry_frame, width=300, height=30)
tile_index_entry_remove = ctk.CTkEntry(entry_frame, width=300, height=30)
tile_red_color_entry = ctk.CTkEntry(entry_frame, width=100, height=30)
tile_green_color_entry = ctk.CTkEntry(entry_frame, width=100, height=30)
tile_blue_color_entry = ctk.CTkEntry(entry_frame, width=100, height=30)
tile_z_coord_entry = ctk.CTkEntry(entry_frame, width=300, height=30)

# Create buttons
add_button = ctk.CTkButton(entry_frame, text="Add Tile", command=add_tile(), width=300, height=64, font=("Arial", 20))
remove_button = ctk.CTkButton(entry_frame, text="Remove Tile", width=300, height=64, font=("Arial", 20))

# Create Labels
info_label = ctk.CTkLabel(information_frame, width=620, text="TILES IN THE FILE", font=("Arial", 20))
add_tile_label = ctk.CTkLabel(entry_frame, width=300, text="ADD TILE", font=("Arial", 20))
remove_tile_label = ctk.CTkLabel(entry_frame, width=300, text="REMOVE TILE", font=("Arial", 20))
index_label0 = ctk.CTkLabel(entry_frame, width=300, text="TileIndex")
index_label1 = ctk.CTkLabel(entry_frame, width=300, text="TileIndex")
red_label = ctk.CTkLabel(entry_frame, width=100, text="RED")
green_label = ctk.CTkLabel(entry_frame, width=100, text="GREEN")
blue_label = ctk.CTkLabel(entry_frame, width=100, text="BLUE")
zcoord_label = ctk.CTkLabel(entry_frame, width=300, text="Z coordinate")
space_label0 = ctk.CTkLabel(entry_frame, width=300, text="==========================================")
space_label1 = ctk.CTkLabel(entry_frame, width=300, text="==========================================")
space_label2 = ctk.CTkLabel(entry_frame, width=300, text="==========================================")

entry_frame.grid(row=0, column=0, padx=5, pady=5)
information_frame.grid(row=0, column=1, padx=5, pady=5)

add_tile_label.grid(row=0, column=0, columnspan=3)
index_label0.grid(row=1, column=0, columnspan=3)
tile_index_entry.grid(row=2, column=0, columnspan=3)
red_label.grid(row=3, column=0)
green_label.grid(row=3, column=1)
blue_label.grid(row=3, column=2)
tile_red_color_entry.grid(row=4, column=0)
tile_green_color_entry.grid(row=4, column=1)
tile_blue_color_entry.grid(row=4, column=2)
zcoord_label.grid(row=5, column=0, columnspan=3)
tile_z_coord_entry.grid(row=6, column=0, columnspan=3)
space_label0.grid(row=7, column=0, columnspan=3)
add_button.grid(row=8, column=0, columnspan=3)

space_label1.grid(row=9, column=0, columnspan=3)
remove_tile_label.grid(row=10, column=0, columnspan=3)
index_label1.grid(row=11, column=0, columnspan=3)
tile_index_entry_remove.grid(row=12, column=0, columnspan=3)
space_label2.grid(row=13, column=0, columnspan=3)
remove_button.grid(row=14, column=0, columnspan=3)

info_label.grid(row=0, column=0, columnspan=3)

# Start the event loop
root.mainloop()