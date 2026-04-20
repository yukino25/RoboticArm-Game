#!/usr/bin/env python3
"""Generate HUD / GUI sprites for the Robotic Arm Game."""
from PIL import Image, ImageDraw, ImageFont
import os

os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Palette
BG       = (28,  28,  46,  255)
BTN_BG   = (42,  42,  62,  255)
BTN_HI   = (60,  80, 110,  255)   # highlighted / active button fill
BORDER   = (69,  90, 100,  255)
ARROW    = (144, 202, 249, 255)
ACTIVE   = (79,  195, 247, 255)
INACTIVE = (55,   71,  79,  255)
TRANSP   = (0,    0,   0,   0)

def get_font(size=11):
    for path in [
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/SFNSMono.ttf",
    ]:
        try:
            return ImageFont.truetype(path, size)
        except Exception:
            pass
    return ImageFont.load_default()

def draw_text_centered(d, text, font, w, h, color=None):
    color = color or ARROW
    try:
        bbox = d.textbbox((0, 0), text, font=font)
        tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
        tx = (w - tw) // 2 - bbox[0]
        ty = (h - th) // 2 - bbox[1]
    except AttributeError:
        tw, th = d.textsize(text, font=font)
        tx, ty = (w - tw) // 2, (h - th) // 2
    d.text((tx, ty), text, fill=color, font=font)

# ── HUD icons ──────────────────────────────────────────────────────────────

def make_hud_bg(w=768, h=32):
    img = Image.new("RGBA", (w, h), BG)
    d = ImageDraw.Draw(img)
    d.line([(0, h-1), (w-1, h-1)], fill=BORDER, width=1)
    img.save("hud_bg.png")

def make_arrow(name, left=True):
    img = Image.new("RGBA", (24, 24), TRANSP)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([2, 2, 21, 21], radius=3, fill=BTN_BG, outline=BORDER)
    pts = [(16, 6), (16, 17), (8, 11)] if left else [(8, 6), (8, 17), (16, 11)]
    d.polygon(pts, fill=ARROW)
    img.save(f"{name}.png")

def make_pause():
    img = Image.new("RGBA", (24, 24), TRANSP)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([2, 2, 21, 21], radius=3, fill=BTN_BG, outline=BORDER)
    d.rectangle([ 8, 7, 10, 16], fill=ARROW)
    d.rectangle([13, 7, 15, 16], fill=ARROW)
    img.save("btn_pause.png")

def make_play():
    img = Image.new("RGBA", (24, 24), TRANSP)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([2, 2, 21, 21], radius=3, fill=BTN_BG, outline=BORDER)
    d.polygon([(8, 7), (8, 16), (16, 11)], fill=ARROW)
    img.save("btn_play.png")

def make_menu_btn():
    img = Image.new("RGBA", (24, 24), TRANSP)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([2, 2, 21, 21], radius=3, fill=BTN_BG, outline=BORDER)
    for y in [8, 12, 16]:
        d.rectangle([7, y, 17, y + 1], fill=ARROW)
    img.save("btn_menu.png")

def make_dot(name, active=False):
    img = Image.new("RGBA", (16, 16), TRANSP)
    d = ImageDraw.Draw(img)
    d.ellipse([2, 2, 13, 13], fill=ACTIVE if active else INACTIVE, outline=BORDER)
    img.save(f"{name}.png")

# ── Overlay text buttons ────────────────────────────────────────────────────

def make_text_btn(filename, text, w=160, h=28, highlighted=False):
    img = Image.new("RGBA", (w, h), TRANSP)
    d = ImageDraw.Draw(img)
    fill = BTN_HI if highlighted else BTN_BG
    d.rounded_rectangle([2, 2, w - 3, h - 3], radius=4, fill=fill, outline=BORDER)
    draw_text_centered(d, text, get_font(11), w, h)
    img.save(filename)

# ── Generate all ────────────────────────────────────────────────────────────

make_hud_bg()
make_arrow("btn_left",  left=True)
make_arrow("btn_right", left=False)
make_pause()
make_play()
make_menu_btn()
make_dot("dot",        active=False)
make_dot("dot_active", active=True)

# Menu overlay buttons
make_text_btn("menu_resume.png",  "RESUME")
make_text_btn("menu_restart.png", "RESTART")
make_text_btn("menu_choose.png",  "CHOOSE LEVEL")
make_text_btn("menu_back.png",    "< BACK", w=120)

# Level select buttons (inactive + active for each level)
for i in range(1, 6):
    make_text_btn(f"lbtn_{i}.png",        f"LEVEL  {i}", highlighted=False)
    make_text_btn(f"lbtn_{i}_active.png", f"LEVEL  {i}", highlighted=True)

print("GUI sprites generated.")
