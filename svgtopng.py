import os
import cairosvg

input_folder = "assets"
output_folder = "assets/png"

os.makedirs(output_folder, exist_ok=True)

for file in os.listdir(input_folder):
    if file.endswith(".svg"):
        input_path = os.path.join(input_folder, file)
        output_path = os.path.join(output_folder, file.replace(".svg", ".png"))

        print(f"Converting {input_path} -> {output_path}")
        cairosvg.svg2png(url=input_path, write_to=output_path)

print("✅ Conversion done!")
