import sys
from PIL import Image

if len(sys.argv) < 2:
    print("Использование: python wallgen.py <картинка.jpg>")
    sys.exit()

print("Конвертация в формат ОС (1024x768, 24-bit BGR)...")
img = Image.open(sys.argv[1]).convert("RGB").resize((1024, 768))

# Видеокарты x86 используют BGR порядок байтов!
r, g, b = img.split()
img_bgr = Image.merge("RGB", (b, g, r))

with open("WALL.BG", "wb") as f:
    f.write(img_bgr.tobytes())
print("Успех! Файл WALL.BG (2.3 МБ) создан! Положите его рядом с Makefile.")