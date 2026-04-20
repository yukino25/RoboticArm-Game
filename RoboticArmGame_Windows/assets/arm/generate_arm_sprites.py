"""
Generate pixel art arm sprites for RoboticArm-Game.
Industrial dark-blue palette to match existing tileset.

Segment length variants  (suffix: _sm / _md / _lg / _xl):
  arm_segment_{size}.png            32→24/32/48/64 × 16
  arm_segment_{size}_active.png
  arm_segment_{size}_inactive.png
  arm_inner_{size}.png              48→32/48/64/80 × 12
  arm_outer_{size}.png              48→32/48/64/80 × 20
  arm_outer_{size}_active.png
  arm_outer_{size}_inactive.png

Grabber end-pieces  (hang downward from arm tip):
  arm_grab_claw.png                 20 × 24
  arm_grab_gripper.png              20 × 22
  arm_grab_suction.png              16 × 22
"""

from PIL import Image, ImageDraw
import os

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

# ── Palette ────────────────────────────────────────────────────────────────
T           = (0,   0,   0,   0  )
OUTLINE     = (18,  24,  38,  255)
DARK        = (42,  56,  76,  255)
MID         = (68,  88,  112, 255)
LIGHT       = (96,  122, 150, 255)
SHINE       = (126, 156, 184, 255)
JOINT_RIM   = (30,  40,  58,  255)
JOINT_FILL  = (55,  72,  96,  255)
JOINT_HI    = (80,  104, 132, 255)
BOLT        = (30,  40,  55,  255)
INNER_BODY  = (55,  75,  98,  255)
INNER_STRIPE= (80,  104, 130, 255)
RUBBER      = (28,  32,  38,  255)   # suction pad rubber
RUBBER_HI   = (50,  56,  66,  255)   # suction pad sheen
CLAW_TIP    = (85,  110, 136, 255)   # lighter claw tip

LED_ON_RIM  = (20,  80,  30,  255)
LED_ON_FILL = (60,  210, 80,  255)
LED_ON_HI   = (160, 255, 170, 255)
LED_OFF_RIM = (40,  44,  50,  255)
LED_OFF_FILL= (80,  86,  94,  255)
LED_OFF_HI  = (110, 116, 124, 255)


def px(d, x, y, c):
    d.point((x, y), fill=c)


def grad_line(d, x0, x1, y, top_c, bot_c, H):
    frac = y / max(H - 1, 1)
    r = int(top_c[0] + (bot_c[0] - top_c[0]) * frac)
    g = int(top_c[1] + (bot_c[1] - top_c[1]) * frac)
    b = int(top_c[2] + (bot_c[2] - top_c[2]) * frac)
    d.line([(x0, y), (x1, y)], fill=(r, g, b, 255))


def draw_led(d, cx, cy, w=6, h=4, active=True):
    rim  = LED_ON_RIM   if active else LED_OFF_RIM
    fill = LED_ON_FILL  if active else LED_OFF_FILL
    hi   = LED_ON_HI    if active else LED_OFF_HI
    x0, y0 = cx - w // 2, cy - h // 2
    x1, y1 = x0 + w - 1,  y0 + h - 1
    d.rectangle([x0, y0, x1, y1], fill=rim)
    d.rectangle([x0+1, y0+1, x1-1, y1-1], fill=fill)
    px(d, x0+1, y0+1, hi)


# ══════════════════════════════════════════════════════════════════════════════
# ARM SEGMENTS
# ══════════════════════════════════════════════════════════════════════════════

def make_segment(W, H, name):
    img = Image.new('RGBA', (W, H), T)
    d   = ImageDraw.Draw(img)

    d.rectangle([3, 0, W-4, H-1], fill=OUTLINE)
    for y in range(1, H-1):
        grad_line(d, 4, W-5, y, LIGHT, DARK, H)
    d.line([(4, 1), (W-5, 1)], fill=SHINE)

    # Bolts — spaced based on width
    bolt_xs = [max(8, W//4), min(W-9, W*3//4)]
    for bx in bolt_xs:
        d.rectangle([bx, 4, bx+1, H-5], fill=BOLT)
        px(d, bx,   3, JOINT_HI)
        px(d, bx+1, 3, JOINT_HI)

    d.ellipse([0, 3, 7, H-4],     fill=JOINT_RIM)
    d.ellipse([1, 4, 6, H-5],     fill=JOINT_FILL)
    d.ellipse([2, 5, 4, H-7],     fill=JOINT_HI)

    d.ellipse([W-8, 3, W-1, H-4], fill=JOINT_RIM)
    d.ellipse([W-7, 4, W-2, H-5], fill=JOINT_FILL)
    d.ellipse([W-6, 5, W-4, H-7], fill=JOINT_HI)

    img.save(os.path.join(OUT_DIR, f'{name}.png'))
    print(f'  {name}.png  ({W}×{H})')

    for active, suffix in [(True, 'active'), (False, 'inactive')]:
        v = img.copy()
        draw_led(ImageDraw.Draw(v), W//2, H//2, active=active)
        v.save(os.path.join(OUT_DIR, f'{name}_{suffix}.png'))
        print(f'  {name}_{suffix}.png  ({W}×{H})')


# ══════════════════════════════════════════════════════════════════════════════
# INNER ROD
# ══════════════════════════════════════════════════════════════════════════════

def make_inner(W, H, name):
    img = Image.new('RGBA', (W, H), T)
    d   = ImageDraw.Draw(img)

    d.rectangle([5, 1, W-1, H-2], fill=OUTLINE)
    for y in range(2, H-2):
        grad_line(d, 6, W-2, y, SHINE, INNER_BODY, H)
    d.line([(6, H//2),     (W-2, H//2)],     fill=INNER_STRIPE)
    d.line([(6, H//2 - 1), (W-2, H//2 - 1)], fill=JOINT_HI)

    for tx in range(14, W-4, 10):
        d.line([(tx, 2), (tx, H-3)], fill=BOLT)

    d.ellipse([0, 1, 9, H-2],  fill=JOINT_RIM)
    d.ellipse([1, 2, 8, H-3],  fill=JOINT_FILL)
    d.ellipse([2, 3, 5, H-5],  fill=JOINT_HI)

    img.save(os.path.join(OUT_DIR, f'{name}.png'))
    print(f'  {name}.png  ({W}×{H})')


# ══════════════════════════════════════════════════════════════════════════════
# OUTER SLEEVE
# ══════════════════════════════════════════════════════════════════════════════

def make_outer(W, H, name):
    img = Image.new('RGBA', (W, H), T)
    d   = ImageDraw.Draw(img)

    d.rectangle([0, 0, W-5, H-1], fill=OUTLINE)
    for y in range(1, H-1):
        grad_line(d, 1, W-6, y, LIGHT, DARK, H)
    d.line([(1, 1), (W-6, 1)], fill=SHINE)

    for rx in [8, W//2, W-16]:
        for ry in [2, H-3]:
            px(d, rx,   ry, BOLT)
            px(d, rx+1, ry, BOLT)

    d.ellipse([W-10, 3,  W-1, H-4], fill=JOINT_RIM)
    d.ellipse([W-9,  4,  W-2, H-5], fill=JOINT_FILL)
    d.ellipse([W-8,  5,  W-5, H-7], fill=JOINT_HI)

    img.save(os.path.join(OUT_DIR, f'{name}.png'))
    print(f'  {name}.png  ({W}×{H})')

    for active, suffix in [(True, 'active'), (False, 'inactive')]:
        v = img.copy()
        draw_led(ImageDraw.Draw(v), (W-5)//2, H//2, active=active)
        v.save(os.path.join(OUT_DIR, f'{name}_{suffix}.png'))
        print(f'  {name}_{suffix}.png  ({W}×{H})')


# ══════════════════════════════════════════════════════════════════════════════
# GRABBER: CLAW  (20 × 24)
# Two curved prongs hanging down, sharp tips, pivot block at top.
# ══════════════════════════════════════════════════════════════════════════════

def make_grab_claw():
    W, H = 20, 24
    img = Image.new('RGBA', (W, H), T)
    d   = ImageDraw.Draw(img)

    # ── Connector block (top, full width, 5px tall) ──
    d.rectangle([2, 0, W-3, 4], fill=OUTLINE)
    d.rectangle([3, 1, W-4, 3], fill=MID)
    d.line([(3, 1), (W-4, 1)], fill=SHINE)

    # ── Left prong  (pixels 1-6, rows 5-21) ──
    # Shaft: straight down then curves inward at tip
    left_shaft = [
        (1, 5,  6, 5),
        (1, 6,  6, 6),
        (1, 7,  6, 7),
        (1, 8,  5, 8),
        (1, 9,  5, 9),
        (1, 10, 5, 10),
        (1, 11, 5, 11),
        (1, 12, 5, 12),
        (1, 13, 5, 13),
        (1, 14, 4, 14),
        (2, 15, 4, 15),
        (3, 16, 5, 16),
        (4, 17, 6, 17),
        (5, 18, 7, 18),
        (5, 19, 6, 19),
        (5, 20, 5, 20),
        (5, 21, 5, 21),
    ]
    for x0, y, x1, _ in left_shaft:
        d.rectangle([x0, y, x1, y], fill=DARK)
        px(d, x0, y, OUTLINE)
        px(d, x1, y, OUTLINE)
    # Shine on left edge
    for x0, y, x1, _ in left_shaft:
        px(d, x0+1, y, LIGHT)

    # Claw tip left
    px(d, 6, 22, CLAW_TIP)
    px(d, 7, 22, OUTLINE)

    # ── Right prong  (mirror of left) ──
    right_shaft = [(W-1-x1, y, W-1-x0, y) for x0, y, x1, _ in left_shaft]
    for x0, y, x1, _ in right_shaft:
        d.rectangle([x0, y, x1, y], fill=DARK)
        px(d, x0, y, OUTLINE)
        px(d, x1, y, OUTLINE)
    for x0, y, x1, _ in right_shaft:
        px(d, x1-1, y, LIGHT)

    px(d, W-7, 22, OUTLINE)
    px(d, W-7, 22, CLAW_TIP)

    # ── Centre gap detail (shadow between prongs) ──
    for y in range(5, 14):
        px(d, W//2, y, JOINT_RIM)

    img.save(os.path.join(OUT_DIR, 'arm_grab_claw.png'))
    print(f'  arm_grab_claw.png  ({W}×{H})')


# ══════════════════════════════════════════════════════════════════════════════
# GRABBER: GRIPPER  (20 × 22)
# Two parallel flat jaws — industrial parallel-jaw gripper.
# ══════════════════════════════════════════════════════════════════════════════

def make_grab_gripper():
    W, H = 20, 22
    img = Image.new('RGBA', (W, H), T)
    d   = ImageDraw.Draw(img)

    # Connector block
    d.rectangle([2, 0, W-3, 4], fill=OUTLINE)
    d.rectangle([3, 1, W-4, 3], fill=MID)
    d.line([(3, 1), (W-4, 1)], fill=SHINE)

    # Central body (neck)
    d.rectangle([7, 4, 12, 8], fill=OUTLINE)
    d.rectangle([8, 5, 11, 7], fill=JOINT_FILL)

    # Left jaw body (rows 8-20, cols 0-7)
    d.rectangle([0, 8, 7, H-2], fill=OUTLINE)
    for y in range(9, H-2):
        grad_line(d, 1, 6, y, LIGHT, DARK, H-8)
    d.line([(1, 9), (6, 9)], fill=SHINE)
    # Left jaw grip surface (right face, rows 9-19)
    for y in range(9, H-2, 2):
        px(d, 7, y, MID)

    # Right jaw body (rows 8-20, cols 12-19)
    d.rectangle([12, 8, W-1, H-2], fill=OUTLINE)
    for y in range(9, H-2):
        grad_line(d, 13, W-2, y, LIGHT, DARK, H-8)
    d.line([(13, 9), (W-2, 9)], fill=SHINE)
    for y in range(9, H-2, 2):
        px(d, 12, y, MID)

    # Jaw tips (flat bottom)
    d.rectangle([0,  H-2, 7,  H-1], fill=JOINT_RIM)
    d.rectangle([12, H-2, W-1, H-1], fill=JOINT_RIM)

    # Bolt on each jaw
    for bx, by in [(3, 13), (W-4, 13)]:
        px(d, bx,   by, BOLT)
        px(d, bx+1, by, BOLT)
        px(d, bx,   by+1, BOLT)
        px(d, bx+1, by+1, BOLT)

    img.save(os.path.join(OUT_DIR, 'arm_grab_gripper.png'))
    print(f'  arm_grab_gripper.png  ({W}×{H})')


# ══════════════════════════════════════════════════════════════════════════════
# GRABBER: SUCTION  (24 × 26)
# Pivot joint (circle) sits at the TIP/TOP of the triangle.
# Triangle flares DOWNWARD from that pivot point to a wide flat rubber pad.
#
#        O        ← round pivot joint (connects to arm)
#       /|\
#      / | \
#     /  |  \     ← metallic triangle body
#    /   |   \
#   /    |    \
#  [============] ← flat rubber suction pad (grabbing side)
# ══════════════════════════════════════════════════════════════════════════════

def make_grab_suction():
    W, H   = 24, 20
    CX     = W // 2          # horizontal centre = 12
    JR     = 4               # pivot joint radius
    APEX_Y = JR              # triangle apex = centre of joint circle
    PAD_H  = 4               # rubber pad height
    PAD_Y0 = H - PAD_H       # rubber pad top row  (= row 16, so triangle is only 12 rows tall)
    PAD_X0 = 1
    PAD_X1 = W - 2

    img = Image.new('RGBA', (W, H), T)
    d   = ImageDraw.Draw(img)

    # ── Triangle body (fans out from apex downward to pad) ────────────────
    # apex is at (CX, APEX_Y); at PAD_Y0 width reaches full pad width
    TRI_ROWS = PAD_Y0 - APEX_Y
    for i, row in enumerate(range(APEX_Y, PAD_Y0)):
        progress = i / max(TRI_ROWS - 1, 1)
        half_w   = int(progress * (PAD_X1 - CX))
        x0 = CX - half_w
        x1 = CX + half_w

        if x0 == x1:          # single tip pixel — joint will cover it
            px(d, CX, row, DARK)
            continue

        px(d, x0, row, OUTLINE)
        px(d, x1, row, OUTLINE)
        if x1 - 1 >= x0 + 1:
            grad_line(d, x0+1, x1-1, row, LIGHT, DARK, TRI_ROWS)
        if x0 + 1 < CX < x1 - 1:
            px(d, CX, row, MID)

    # ── Rubber suction pad ────────────────────────────────────────────────
    d.rectangle([PAD_X0, PAD_Y0, PAD_X1, H-1], fill=OUTLINE)
    d.rectangle([PAD_X0+1, PAD_Y0+1, PAD_X1-1, H-2], fill=RUBBER)
    d.line([(PAD_X0+1, PAD_Y0+1), (PAD_X1-1, PAD_Y0+1)], fill=RUBBER_HI)
    px(d, PAD_X0+1, PAD_Y0+2, RUBBER_HI)
    d.line([(PAD_X0, H-1), (PAD_X1, H-1)], fill=RUBBER)

    # ── Pivot joint — circle centred exactly on the triangle apex ─────────
    d.ellipse([CX-JR, APEX_Y-JR, CX+JR, APEX_Y+JR], fill=JOINT_RIM)
    d.ellipse([CX-JR+1, APEX_Y-JR+1, CX+JR-1, APEX_Y+JR-1], fill=JOINT_FILL)
    d.ellipse([CX-JR+2, APEX_Y-JR+2, CX-1, APEX_Y], fill=JOINT_HI)

    img.save(os.path.join(OUT_DIR, 'arm_grab_suction.png'))
    print(f'  arm_grab_suction.png  ({W}×{H})')


# ══════════════════════════════════════════════════════════════════════════════
# RUN — generate all sizes
# ══════════════════════════════════════════════════════════════════════════════

SIZES = [
    # (label,  seg_W, seg_H, rod_W, rod_H, sleeve_W, sleeve_H)
    ('sm',     32,    16,    48,    12,    48,       20),
    ('md',     48,    16,    64,    12,    64,       20),
    ('lg',     64,    16,    80,    12,    80,       20),
    ('xl',     80,    16,    96,    12,    96,       20),
]

print('Generating arm sprites...')
print()

print('── Segments ──')
for label, sw, sh, rw, rh, ow, oh in SIZES:
    make_segment(sw, sh, f'arm_segment_{label}')

print()
print('── Inner rods ──')
for label, sw, sh, rw, rh, ow, oh in SIZES:
    make_inner(rw, rh, f'arm_inner_{label}')

print()
print('── Outer sleeves ──')
for label, sw, sh, rw, rh, ow, oh in SIZES:
    make_outer(ow, oh, f'arm_outer_{label}')

print()
print('── Grabbers ──')
make_grab_claw()
make_grab_gripper()
make_grab_suction()

print()
print('Done.')
