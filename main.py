import pygame
import numpy as np
import math

from FluidSim import FluidSim
pygame.init()

WINDOW_SIZE = 640
GRID_SIZE = 40
CELL_SIZE = WINDOW_SIZE // GRID_SIZE
N = GRID_SIZE

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
RED = (255, 0, 0)
BLUE = (0, 0, 255)
GREEN = (0, 255, 0)
GRAY = (128, 128, 128)

screen = pygame.display.set_mode((WINDOW_SIZE, WINDOW_SIZE))
pygame.display.set_caption("Fluid Simulation")


def get_velocity_color(u, v, max_speed=10):
    speed = math.sqrt(u*u + v*v)
    speed = min(speed * 10, max_speed) / max_speed
    
    if speed < 0.1:
        return GRAY
    elif speed < 0.5:
        t = speed * 2
        return (int(255 * (1-t)), int(255 * t), 0)
    else:
        t = (speed - 0.5) * 2
        return (255, int(255 * (1-t)), 0)

def draw_arrow(surface, start_pos, vector, scale=1.0):
    x, y = start_pos
    u_comp, v_comp = vector
    length = math.sqrt(u_comp**2 + v_comp**2)
    
    if length < 0.01:
        pygame.draw.circle(surface, GRAY, (int(x), int(y)), 2)
        return
    
    arrow_length = min(CELL_SIZE * 0.4 * length * 10, CELL_SIZE * 0.8)
    end_x = x + (u_comp / length) * arrow_length
    end_y = y + (v_comp / length) * arrow_length
    
    color = get_velocity_color(u_comp, v_comp)
    
    pygame.draw.line(surface, color, (int(x), int(y)), (int(end_x), int(end_y)), 2)
    
    if arrow_length > 5:
        angle = math.atan2(v_comp, u_comp)
        arrowhead_length = min(8, arrow_length * 0.3)
        arrowhead_angle = math.pi / 6
        
        head1_x = end_x - arrowhead_length * math.cos(angle - arrowhead_angle)
        head1_y = end_y - arrowhead_length * math.sin(angle - arrowhead_angle)
        head2_x = end_x - arrowhead_length * math.cos(angle + arrowhead_angle)
        head2_y = end_y - arrowhead_length * math.sin(angle + arrowhead_angle)
        
        pygame.draw.line(surface, color, (int(end_x), int(end_y)), (int(head1_x), int(head1_y)), 2)
        pygame.draw.line(surface, color, (int(end_x), int(end_y)), (int(head2_x), int(head2_y)), 2)

clock = pygame.time.Clock()
running = True

fs = FluidSim(N, WINDOW_SIZE, WINDOW_SIZE)
ctr = 0
mouse_pressed = False

while running:
  for event in pygame.event.get():
    if event.type == pygame.QUIT:
      running = False
    elif event.type == pygame.MOUSEBUTTONDOWN:
      mouse_pressed = True
    elif event.type == pygame.MOUSEBUTTONUP:
      mouse_pressed = False
    elif event.type == pygame.KEYDOWN:
      if event.key == pygame.K_r:  # Reset
        fs = FluidSim(N, WINDOW_SIZE, WINDOW_SIZE)
  
    
    if mouse_pressed:
      mouse_x, mouse_y = pygame.mouse.get_pos()
      grid_x = int(mouse_x / CELL_SIZE)
      grid_y = int(mouse_y / CELL_SIZE)
      
      mouse_vel = pygame.mouse.get_rel()
      fx = mouse_vel[0] * 0.1
      fy = mouse_vel[1] * 0.1
      
      if 0 <= grid_x < N and 0 <= grid_y < N:
        fs.add_force(grid_y, grid_x, fx, fy)
      else:
        pygame.mouse.get_rel()
    

  fs.swap()
  fs.advect(fs.dt, fs.u, fs.u_prev)
  fs.advect(fs.dt, fs.v, fs.v_prev)

  screen.fill(WHITE)
  
  for i in range(N):
    for j in range(N):
      cell_x = j * CELL_SIZE + CELL_SIZE // 2
      cell_y = i * CELL_SIZE + CELL_SIZE // 2
      
      # pygame.draw.rect(screen, BLACK, (j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE, CELL_SIZE), 1)
      
      vector = (fs.u[i][j], fs.v[i][j])
      draw_arrow(screen, (cell_x, cell_y), vector)

      ctr += 1
      if ctr == 100:
        fs.dump_log()
  
  pygame.display.flip()
  clock.tick(60)

pygame.quit()
