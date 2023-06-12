import customtkinter as ctk

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
add_button = ctk.CTkButton(entry_frame, text="Add Tile", width=300, height=64, font=("Arial", 20))
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