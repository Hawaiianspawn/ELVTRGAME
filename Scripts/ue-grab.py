#!/usr/bin/env python
"""ue-grab.py — grab the PIE game viewport from the screen to a PNG.

Slate screenshots can't see the game view (the viewport is GPU-presented), so
this screen-grabs the virtual desktop and crops the editor window's viewport
panel. Finds the window by title each call, so it survives the window being
moved between monitors. Usage: python ue-grab.py <out.png>
"""
import ctypes
import ctypes.wintypes
import sys
from PIL import ImageGrab

user32 = ctypes.windll.user32

# Fixed chrome around the viewport panel, measured from the Slate widget tree:
# left edge 4px, tab bar + toolbars ~135px top, details column ~394px right,
# bottom bar ~36px.
INSETS = (4, 135, 394, 36)


def virtual_origin():
    xv = user32.GetSystemMetrics(76)  # SM_XVIRTUALSCREEN
    yv = user32.GetSystemMetrics(77)  # SM_YVIRTUALSCREEN
    w = user32.GetSystemMetrics(78)   # SM_CXVIRTUALSCREEN
    h = user32.GetSystemMetrics(79)   # SM_CYVIRTUALSCREEN
    return xv, yv, w, h


def find_editor_window():
    found = []

    def cb(hwnd, _):
        if user32.IsWindowVisible(hwnd):
            buf = ctypes.create_unicode_buffer(256)
            user32.GetWindowTextW(hwnd, buf, 256)
            if "Unreal Editor" in buf.value and "ELVTR" in buf.value:
                r = ctypes.wintypes.RECT()
                user32.GetWindowRect(hwnd, ctypes.byref(r))
                found.append((r.left, r.top, r.right, r.bottom))
        return True

    EnumProc = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    user32.EnumWindows(EnumProc(cb), 0)
    if not found:
        sys.exit("editor window not found")
    return max(found, key=lambda r: (r[2] - r[0]) * (r[3] - r[1]))


out = sys.argv[1] if len(sys.argv) > 1 else "ue-grab.png"
xv, yv, w, h = virtual_origin()
shot = ImageGrab.grab(bbox=(xv, yv, xv + w, yv + h))
l, t, r, b = find_editor_window()
rect = (l + INSETS[0] - xv, t + INSETS[1] - yv, r - INSETS[2] - xv, b - INSETS[3] - yv)
shot.crop(rect).save(out)
print(out)
