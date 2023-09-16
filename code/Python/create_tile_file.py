from customtkinter.windows.widgets.core_rendering.ctk_canvas import CTkCanvas
from tkinter import messagebox
import customtkinter as ctk
import json
import struct

# Initialize Global Veriables
tiles_array = []

"""
    tile = 
    {
        "index": 0,
        "color": (0, 0, 0),
        "z_coord": 0,
        "name": ''
    }

"""

def rgb_to_hex_with_alpha(rgb_tuple, alpha):
    result = 0xff000000

    red = rgb_tuple[0] << 16
    green = rgb_tuple[1] << 8
    blue = rgb_tuple[2]

    result = result | red | green | blue
    
    return result

def get_tiles():
    global tiles_array
    with open("tiles_file_1.json") as file:
        tiles_array = json.load(file)


def update_file_content():
    with open("tiles_file_1.json", "w") as file:
        json.dump(tiles_array, file)


def check_value(value, type):
    if type == "index":
        if value >= 65536:
            messagebox.showwarning("Warning", "Tile Index Can't be bigger then 65536!")
            return False
        else:
            return True
    elif type == "color":
        if value >= 256:
            messagebox.showwarning("Warning", "Tile ColorRange is from 0 to 255!")
            return False
        else:
            return True
    else:
        if value > 10:
            messagebox.showwarning("Warning", "Z coord can't be more then 10!")
            return False
        else:
            return True

def convert_str_to_int(value):
    result = 0
    
    if value != '':
        try:
            result = int(value)
        except ValueError:
            messagebox.showwarning("Warning", "The nambers have to be integers")
    else:
        pass

    return result


def add_tile():
    global tiles_array

    tile_index = convert_str_to_int(tile_index_entry.get())
    tile_color_R = convert_str_to_int(tile_red_color_entry.get())
    tile_color_G = convert_str_to_int(tile_green_color_entry.get())
    tile_color_B = convert_str_to_int(tile_blue_color_entry.get())
    tile_z_coord = convert_str_to_int(tile_z_coord_entry.get())
    tile_name = tile_name_entry.get()

    if check_value(tile_index, "index") and check_value(tile_color_R, "color") and \
        check_value(tile_color_G, "color") and check_value(tile_color_B, "color") and \
            check_value(tile_z_coord, "z") and tile_name != '':
        allow_to_add = True
        for tile in tiles_array:
            if tile["index"] == tile_index:
                messagebox.showerror("Error", "The tile with the same index is in the file!")
                allow_to_add = False
                break
            if tile["color"] == (tile_color_R, tile_color_G, tile_color_B):
                messagebox.showerror("Error", "The tile with the same color is in the file!")
                allow_to_add = False
                break
            if tile["name"] == tile_name:
                messagebox.showerror("Error", "The tile with the same name is in the file")
                allow_to_add = False
                break

        if allow_to_add:
            tile = {"index": tile_index, 
                    "color": (tile_color_R, tile_color_G, tile_color_B),
                    "z_coord": tile_z_coord,
                    "name": tile_name}
            
            tiles_array.append(tile)
            tiles_array = sorted(tiles_array, key=lambda x: x["index"])
            
            update_file_content()
    
    show_tiles_info()


def remove_tile():
    global tiles_array
    
    tile_index = convert_str_to_int(tile_index_entry_remove.get())
    if check_value(tile_index, "index"):
        for i in range(0, len(tiles_array)):
            if tiles_array[i]["index"] == tile_index:
                del tiles_array[i]
                break
        update_file_content()
    else:
        pass

    show_tiles_info()

def write_array_for_cpp():
    # Open the binary file in write mode
    with open("C:\\Paul\\Projects\\Tile_Map_Editor\\data\\output_0.bin", "wb") as file:
        for tile in tiles_array:
            numbers = []
            color = rgb_to_hex_with_alpha((tile["color"][0], tile["color"][1], tile["color"][2]), 255)
 
            numbers.append(tile["index"])
            numbers.append(color)

            for number in numbers:
                # Pack the number as a 4-byte signed integer (change the format if needed)
                binary_data = struct.pack("I", number)
                # Write the binary data to the file
                file.write(binary_data)
        


def clear_frame(frame):
    for widget in frame.winfo_children():
        widget.destroy()

def show_tiles_info():
    global tiles_array

    clear_frame(information_frame)

    info_label = ctk.CTkLabel(information_frame, width=620, text="TILES IN THE FILE", font=("Arial", 20))
    info_index_label = ctk.CTkLabel(information_frame, width=50, text="INDEX")
    info_color_label_r = ctk.CTkLabel(information_frame, width=50, text="RED")
    info_color_lable_g = ctk.CTkLabel(information_frame, width=50, text="GREEN")
    info_color_label_b = ctk.CTkLabel(information_frame, width=50, text="BLUE")
    info_z_coord_label = ctk.CTkLabel(information_frame, width=50, text="ZCOORD")
    info_name_label = ctk.CTkLabel(information_frame, width=50, text="NAME")
    info_label.grid(row=0, column=0, columnspan=7)
    info_index_label.grid(row=1, column=0, sticky="W")
    info_color_label_r.grid(row=1, column=1, sticky="W")
    info_color_lable_g.grid(row=1, column=2, sticky="W")
    info_color_label_b.grid(row=1, column=3, sticky="W")
    info_z_coord_label.grid(row=1, column=4, sticky="W")
    info_name_label.grid(row=1, column=5, sticky="W")


    i = 0
    for tile in tiles_array:
        canvas = CTkCanvas(information_frame, width=30, height=30)
        
        index_label = ctk.CTkLabel(information_frame, width=50, text=str(tile["index"]))
        color_label_r = ctk.CTkLabel(information_frame, width=50, text=str(tile["color"][0]))
        color_lable_g = ctk.CTkLabel(information_frame, width=50, text=str(tile["color"][1]))
        color_label_b = ctk.CTkLabel(information_frame, width=50, text=str(tile["color"][2]))
        z_coord_label = ctk.CTkLabel(information_frame, width=50, text=str(tile["z_coord"]))
        name_label = ctk.CTkLabel(information_frame, width=50, text=tile["name"])
        canvas.create_rectangle(0, 0, 30, 30, fill=('#%02x%02x%02x' % (tile["color"][0], tile["color"][1], tile["color"][2])))

        index_label.grid(row=2+i, column=0, sticky="W")
        color_label_r.grid(row=2+i, column=1, sticky="W")
        color_lable_g.grid(row=2+i, column=2, sticky="W")
        color_label_b.grid(row=2+i, column=3, sticky="W")
        z_coord_label.grid(row=2+i, column=4, sticky="W")
        name_label.grid(row=2+i, column=5, sticky="W")
        canvas.grid(row=2+i, column=6, stick="W")
        
        i += 1
    
    write_array_for_cpp()

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
tile_name_entry = ctk.CTkEntry(entry_frame, width=300, height=30)

# Create buttons
add_button = ctk.CTkButton(entry_frame, text="Add Tile", command=lambda: add_tile(), width=300, height=64, font=("Arial", 20))
remove_button = ctk.CTkButton(entry_frame, text="Remove Tile", command=lambda: remove_tile(), width=300, height=64, font=("Arial", 20))

# Create Labels
add_tile_label = ctk.CTkLabel(entry_frame, width=300, text="ADD TILE", font=("Arial", 20))
remove_tile_label = ctk.CTkLabel(entry_frame, width=300, text="REMOVE TILE", font=("Arial", 20))
index_label0 = ctk.CTkLabel(entry_frame, width=300, text="TileIndex")
index_label1 = ctk.CTkLabel(entry_frame, width=300, text="TileIndex")
red_label = ctk.CTkLabel(entry_frame, width=100, text="RED")
green_label = ctk.CTkLabel(entry_frame, width=100, text="GREEN")
blue_label = ctk.CTkLabel(entry_frame, width=100, text="BLUE")
zcoord_label = ctk.CTkLabel(entry_frame, width=300, text="Z coordinate")
name_label = ctk.CTkLabel(entry_frame, width=300, text="Name")
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
#space_label0.grid(row=7, column=0, columnspan=3)
name_label.grid(row=7, column=0, columnspan=3)
tile_name_entry.grid(row=8, column=0, columnspan=3)
space_label1.grid(row=9, column=0, columnspan=3)
add_button.grid(row=10, column=0, columnspan=3)

remove_tile_label.grid(row=11, column=0, columnspan=3)
index_label1.grid(row=12, column=0, columnspan=3)
tile_index_entry_remove.grid(row=13, column=0, columnspan=3)
space_label2.grid(row=14, column=0, columnspan=3)
remove_button.grid(row=15, column=0, columnspan=3)

show_tiles_info()

# Start the event loop
root.mainloop()