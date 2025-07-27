import pygame
import numpy as np
import math

from FluidSim import FluidSim

pygame.init()

# —— Constants ——
WINDOW_SIZE = 640
GRID_SIZE = 40
CELL_SIZE = WINDOW_SIZE // GRID_SIZE
N = GRID_SIZE
MAX_SPEED = 10.0
ARROW_THRESHOLD = 0.1

WHITE = (255, 255, 255)
GRAY = (128, 128, 128)

# —— Pygame setup ——
screen = pygame.display.set_mode((WINDOW_SIZE, WINDOW_SIZE))
pygame.display.set_caption("Fluid Simulation")
clock = pygame.time.Clock()

# —— Pre‑render a proper arrow texture ——
arrow_surf = pygame.Surface((CELL_SIZE, CELL_SIZE), pygame.SRCALPHA)
cx, cy = CELL_SIZE // 2, CELL_SIZE // 2

# shaft and head dimensions
shaft_w = CELL_SIZE * 0.1
shaft_h = CELL_SIZE * 0.5
head_w = CELL_SIZE * 0.3
head_h = CELL_SIZE * 0.2

# draw shaft (centered vertically)
shaft_rect = pygame.Rect(
    cx - shaft_w / 2,
    cy - shaft_h / 2,
    shaft_w,
    shaft_h
)
pygame.draw.rect(arrow_surf, WHITE, shaft_rect)

# draw triangle head on top of shaft
head_pts = [
    (cx - head_w / 2, cy - shaft_h / 2),
    (cx + head_w / 2, cy - shaft_h / 2),
    (cx, cy - shaft_h / 2 - head_h),
]
pygame.draw.polygon(arrow_surf, WHITE, head_pts)

arrow_tex = arrow_surf.convert_alpha()

# —— FluidSim init ——
fs = FluidSim(N, WINDOW_SIZE / 4, WINDOW_SIZE / 4)
mouse_pressed = False


def get_velocity_color(u, v):
  speed = math.hypot(u, v)
  norm = min(speed * 10, MAX_SPEED) / MAX_SPEED
  if norm < 0.1:
    return GRAY
  elif norm < 0.5:
    t = norm * 2
    return (int(255 * (1 - t)), int(255 * t), 0)
  else:
    t = (norm - 0.5) * 2
    return (255, int(255 * (1 - t)), 0)


running = True
while running:
  for e in pygame.event.get():
    if e.type == pygame.QUIT:
      running = False
    elif e.type == pygame.MOUSEBUTTONDOWN:
      mouse_pressed = True
    elif e.type == pygame.MOUSEBUTTONUP:
      mouse_pressed = False
    elif e.type == pygame.KEYDOWN and e.key == pygame.K_r:
      fs = FluidSim(N, WINDOW_SIZE, WINDOW_SIZE)

  # apply mouse force
  if mouse_pressed:
    mx, my = pygame.mouse.get_pos()
    gx, gy = mx // CELL_SIZE, my // CELL_SIZE
    fx, fy = pygame.mouse.get_rel()
    fx *= 0.1
    fy *= 0.1
    if 0 <= gx < N and 0 <= gy < N:
      fs.add_force(gy, gx, fx, fy)
    else:
      pygame.mouse.get_rel()

  # simulation step
  fs.swap()
  fs.advect_u()
  fs.advect_v()
  fs.dampen(0.99)
  fs.boundary()

  # draw
  screen.fill(WHITE)
  for i in range(N):
    for j in range(N):
      u, v = fs.get_u(i, j), fs.get_v(i, j)
      mag = math.hypot(u, v)
      x = j * CELL_SIZE + CELL_SIZE // 2
      y = i * CELL_SIZE + CELL_SIZE // 2

      if mag < ARROW_THRESHOLD:
        # idle dot
        pygame.draw.circle(screen, GRAY, (x, y), 1)
        continue

      # compute corrected angle:
      #   atan2(v,u) gives angle CCW from +x, we need arrow texture (pointing up) to rotate CW by (θ+90)
      theta = math.degrees(math.atan2(v, u)) + 90
      angle = -theta

      # scale factor
      scale = min(mag * 3, 1.0)

      # tint arrow
      color = get_velocity_color(u, v)
      tinted = arrow_tex.copy()
      tinted.fill(color, special_flags=pygame.BLEND_RGB_MULT)

      # rotate & scale
      rot = pygame.transform.rotozoom(tinted, angle, scale)
      rect = rot.get_rect(center=(x, y))
      screen.blit(rot, rect)

  pygame.display.flip()
  clock.tick(60)

pygame.quit()
