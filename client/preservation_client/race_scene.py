"""
2D top-down interactive race scene using Pygame.
Clean-room implementation — arcade-style car physics.
Requires: pip install pygame
"""

from __future__ import annotations

import math
import time
from dataclasses import dataclass

try:
    import pygame
except ImportError:
    pygame = None


# Window / display
WIDTH = 800
HEIGHT = 600
FPS = 60

# Colors (RGB)
COLOR_BG = (60, 120, 60)       # grass green
COLOR_TRACK = (80, 80, 80)      # asphalt
COLOR_TRACK_EDGE = (200, 200, 200)  # curb/edge
COLOR_FINISH = (255, 255, 255)  # finish line
COLOR_CAR = (220, 30, 30)       # red car
COLOR_CAR2 = (30, 30, 220)     # blue accent (direction arrow)
COLOR_TEXT = (255, 255, 255)

# Track layout — simple oval
TRACK_CENTER_X = WIDTH // 2
TRACK_CENTER_Y = HEIGHT // 2
TRACK_OUTER_W = 700
TRACK_OUTER_H = 420
TRACK_INNER_W = 460
TRACK_INNER_H = 180
TRACK_WIDTH = (TRACK_OUTER_W - TRACK_INNER_W) // 2

# Car dimensions
CAR_W = 20
CAR_H = 32

# Physics
MAX_SPEED = 200.0          # px/s
ACCEL = 120.0              # px/s²
BRAKE = 180.0              # px/s²
FRICTION = 25.0             # px/s²
TURN_RATE = 2.8            # rad/s
GRIP = 0.92               # lateral velocity retention per frame

# Finish line
FINISH_LINE_Y = TRACK_CENTER_Y + TRACK_OUTER_H // 2 - 20
FINISH_LINE_X1 = TRACK_CENTER_X - TRACK_OUTER_W // 2 + 40
FINISH_LINE_X2 = TRACK_CENTER_X + TRACK_OUTER_H // 2 + 20


@dataclass
class Car:
    x: float
    y: float
    angle: float       # heading, 0 = pointing up
    speed: float       # forward speed (px/s)
    lateral: float     # lateral slip (px/s)
    lap: int
    lap_start_time: float
    best_lap: float
    last_checkpoint: int  # 0=top,1=right,2=bottom,3=left


def create_car() -> Car:
    """Spawn car at start position (bottom of oval, facing up)."""
    return Car(
        x=TRACK_CENTER_X,
        y=FINISH_LINE_Y + 30,
        angle=-math.pi / 2,  # pointing up
        speed=0.0,
        lateral=0.0,
        lap=0,
        lap_start_time=time.time(),
        best_lap=float("inf"),
        last_checkpoint=2,  # starting near bottom
    )


def is_on_track(car: Car) -> bool:
    """Check if car center is within the oval track."""
    dx = (car.x - TRACK_CENTER_X) / (TRACK_OUTER_W / 2)
    dy = (car.y - TRACK_CENTER_Y) / (TRACK_OUTER_H / 2)
    outer_check = dx * dx + dy * dy <= 1.0

    dx_i = (car.x - TRACK_CENTER_X) / (TRACK_INNER_W / 2)
    dy_i = (car.y - TRACK_CENTER_Y) / (TRACK_INNER_H / 2)
    inner_check = dx_i * dx_i + dy_i * dy_i >= 1.0

    return outer_check and inner_check


def get_checkpoint(car: Car) -> int:
    """Return checkpoint zone: 0=top, 1=right, 2=bottom, 3=left."""
    dx = car.x - TRACK_CENTER_X
    dy = car.y - TRACK_CENTER_Y
    angle = math.atan2(dy, dx)  # 0=right, pi/2=down, pi/-pi=left, -pi/2=up
    if -math.pi/4 <= angle < math.pi/4:
        return 1   # right
    elif math.pi/4 <= angle < 3*math.pi/4:
        return 2   # bottom
    elif -3*math.pi/4 <= angle < -math.pi/4:
        return 0   # top
    else:
        return 3   # left


def crossed_finish(car: Car, prev_y: float) -> bool:
    """Detect crossing the finish line going upward."""
    return (
        car.last_checkpoint == 2 and
        get_checkpoint(car) == 0 and
        prev_y > FINISH_LINE_Y and
        car.y <= FINISH_LINE_Y
    )


def update_car(car: Car, dt: float, keys) -> bool:
    """
    Update car physics.
    Returns True if a lap was completed.
    """
    prev_y = car.y

    # Input
    if keys[pygame.K_UP] or keys[pygame.K_w]:
        car.speed += ACCEL * dt
    elif keys[pygame.K_DOWN] or keys[pygame.K_s]:
        car.speed -= BRAKE * dt
    else:
        # Coast friction
        car.speed -= math.copysign(FRICTION, car.speed) * dt
        if abs(car.speed) < 1.0:
            car.speed = 0.0

    # Clamp speed
    car.speed = max(-MAX_SPEED * 0.3, min(MAX_SPEED, car.speed))

    # Steering (only when moving)
    if abs(car.speed) > 5.0:
        turn = TURN_RATE * dt * (car.speed / MAX_SPEED)
        if keys[pygame.K_LEFT] or keys[pygame.K_a]:
            car.angle -= turn
        if keys[pygame.K_RIGHT] or keys[pygame.K_d]:
            car.angle += turn

    # Lateral grip (reduces drifting)
    car.lateral *= GRIP

    # Move
    fx = math.sin(car.angle)
    fy = -math.cos(car.angle)
    car.x += (car.speed * fx + car.lateral * math.cos(car.angle)) * dt
    car.y += (car.speed * fy + car.lateral * math.sin(car.angle)) * dt

    # Keep on track — push back if off
    if not is_on_track(car):
        # Soft penalty: slow down and gently push toward track center
        car.speed *= 0.85
        dx = TRACK_CENTER_X - car.x
        dy = TRACK_CENTER_Y - car.y
        norm = math.sqrt(dx*dx + dy*dy)
        if norm > 0:
            car.x += (dx / norm) * 2
            car.y += (dy / norm) * 2

    # Boundary clamp
    car.x = max(20, min(WIDTH - 20, car.x))
    car.y = max(20, min(HEIGHT - 20, car.y))

    # Checkpoint / lap detection
    cp = get_checkpoint(car)
    lap_complete = False
    if crossed_finish(car, prev_y):
        now = time.time()
        lap_time = now - car.lap_start_time
        if lap_time < car.best_lap and car.lap > 0:
            car.best_lap = lap_time
        car.lap += 1
        car.lap_start_time = now
        lap_complete = True
    car.last_checkpoint = cp

    return lap_complete


def draw_track(surface: pygame.Surface) -> None:
    """Draw the oval track."""
    # Background
    surface.fill(COLOR_BG)

    # Outer oval
    outer_rect = pygame.Rect(
        TRACK_CENTER_X - TRACK_OUTER_W // 2,
        TRACK_CENTER_Y - TRACK_OUTER_H // 2,
        TRACK_OUTER_W,
        TRACK_OUTER_H,
    )
    pygame.draw.ellipse(surface, COLOR_TRACK, outer_rect)

    # Inner oval (cut out = grass)
    inner_rect = pygame.Rect(
        TRACK_CENTER_X - TRACK_INNER_W // 2,
        TRACK_CENTER_Y - TRACK_INNER_H // 2,
        TRACK_INNER_W,
        TRACK_INNER_H,
    )
    pygame.draw.ellipse(surface, COLOR_BG, inner_rect)

    # Track edge lines
    pygame.draw.ellipse(surface, COLOR_TRACK_EDGE, outer_rect, 3)
    pygame.draw.ellipse(surface, COLOR_TRACK_EDGE, inner_rect, 3)

    # Finish line (white bar at bottom of oval)
    fl_x = TRACK_CENTER_X - 8
    fl_y = FINISH_LINE_Y - 4
    pygame.draw.rect(surface, COLOR_FINISH, (fl_x, fl_y, 16, 8))

    # Direction arrows on track
    for cx, cy, angle in [
        (TRACK_CENTER_X, TRACK_CENTER_Y - TRACK_OUTER_H // 2 + TRACK_INNER_H // 2 + 30, 0),       # top — arrow up
        (TRACK_CENTER_X + TRACK_OUTER_W // 2 - TRACK_INNER_W // 2 - 30, TRACK_CENTER_Y, -math.pi/2),   # right
        (TRACK_CENTER_X, TRACK_CENTER_Y + TRACK_OUTER_H // 2 - TRACK_INNER_H // 2 - 30, math.pi),        # bottom
        (TRACK_CENTER_X - TRACK_OUTER_W // 2 + TRACK_INNER_W // 2 + 30, TRACK_CENTER_Y, math.pi/2),     # left
    ]:
        _draw_arrow(surface, cx, cy, angle, (150, 150, 150))


def _draw_arrow(surface: pygame.Surface, x: float, y: float, angle: float, color) -> None:
    """Draw a small direction arrow at (x,y) rotated by angle."""
    size = 10
    tip_x = x + math.sin(angle) * size
    tip_y = y - math.cos(angle) * size
    base_x = x - math.sin(angle) * size * 0.5
    base_y = y + math.cos(angle) * size * 0.5
    pygame.draw.line(surface, color, (base_x, base_y), (tip_x, tip_y), 2)


def draw_car(surface: pygame.Surface, car: Car) -> None:
    """Draw the player's car."""
    # Rotate car surface
    rotated = pygame.Surface((CAR_W, CAR_H), pygame.SRCALPHA)
    pygame.draw.rect(rotated, COLOR_CAR, (0, 0, CAR_W, CAR_H))
    # Direction dot (front of car)
    pygame.draw.circle(rotated, COLOR_CAR2, (CAR_W // 2, 5), 4)
    rotated.set_alpha(255)

    angle_deg = math.degrees(car.angle)
    rotated = pygame.transform.rotate(rotated, angle_deg + 180)

    w, h = rotated.get_size()
    surface.blit(rotated, (car.x - w // 2, car.y - h // 2))


def draw_hud(surface: pygame.Surface, car: Car, race_time: float) -> None:
    """Draw heads-up display."""
    font = pygame.font.Font(None, 28)
    small = pygame.font.Font(None, 22)

    # Background panel
    panel = pygame.Surface((180, 90), pygame.SRCALPHA)
    panel.set_alpha(180)
    pygame.draw.rect(panel, (0, 0, 0), panel.get_rect())
    surface.blit(panel, (8, 8))

    texts = [
        font.render(f"Lap:   {car.lap}", True, COLOR_TEXT),
        font.render(f"Speed: {abs(int(car.speed))} px/s", True, COLOR_TEXT),
        small.render(f"Best:  {car.best_lap:.1f}s" if car.best_lap < float("inf") else "Best:  --", True, (200, 200, 100)),
        small.render(f"Time:  {race_time:.1f}s", True, COLOR_TEXT),
    ]
    for i, t in enumerate(texts):
        surface.blit(t, (14, 14 + i * 22))

    # Controls hint
    hint = small.render("W/S: throttle  A/D: steer  ESC: quit", True, (180, 180, 180))
    surface.blit(hint, (8, HEIGHT - 24))


def draw_start_screen(surface: pygame.Surface, total_laps: int) -> None:
    """Draw countdown overlay."""
    font = pygame.font.Font(None, 72)
    panel = pygame.Surface((WIDTH, 120), pygame.SRCALPHA)
    panel.set_alpha(200)
    pygame.draw.rect(panel, (0, 0, 0), panel.get_rect())
    surface.blit(panel, (0, HEIGHT // 2 - 60))
    t = font.render(f"GO! Complete {total_laps} lap(s)", True, (100, 255, 100))
    surface.blit(t, (WIDTH // 2 - t.get_width() // 2, HEIGHT // 2 - 20))


def run_race(total_laps: int = 1, time_limit: float = 120.0) -> dict:
    """
    Run an interactive pygame race.
    Returns dict: finish_time_ms, laps_completed, dnf (bool)
    """
    if pygame is None:
        return _run_text_race_fallback(total_laps, time_limit)

    pygame.init()
    surface = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("MCO Race — Arrow Keys to Drive")
    clock = pygame.time.Clock()
    font_large = pygame.font.Font(None, 72)

    car = create_car()
    race_start = time.time()
    race_finished = False
    finish_time = 0.0

    # Countdown
    for count in [3, 2, 1]:
        draw_track(surface)
        draw_car(surface, car)
        t = font_large.render(str(count), True, (255, 255, 255))
        surface.blit(t, (WIDTH // 2 - t.get_width() // 2, HEIGHT // 2 - t.get_height() // 2))
        pygame.display.flip()
        pygame.time.wait(900)
    draw_start_screen(surface, total_laps)
    pygame.display.flip()
    pygame.time.wait(800)

    running = True
    while running:
        dt = clock.tick(FPS) / 1000.0
        race_elapsed = time.time() - race_start

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                running = False

        keys = pygame.key.get_pressed()

        lap_complete = update_car(car, dt, keys)

        draw_track(surface)
        draw_car(surface, car)
        draw_hud(surface, car, race_elapsed)

        pygame.display.set_caption(f"MCO Race — {car.lap}/{total_laps} laps")
        pygame.display.flip()

        if lap_complete and car.lap >= total_laps and not race_finished:
            finish_time = race_elapsed
            race_finished = True
            running = False

        if race_elapsed >= time_limit and not race_finished:
            running = False

    pygame.quit()

    return {
        "finish_time_ms": int(finish_time * 1000),
        "laps_completed": min(car.lap, total_laps),
        "dnf": not race_finished,
        "best_lap_ms": int(car.best_lap * 1000) if car.best_lap < float("inf") else None,
        "total_time_ms": int(race_elapsed * 1000),
    }


def _run_text_race_fallback(total_laps: int, time_limit: float) -> dict:
    """Text-based fallback when pygame is not installed."""
    print("[race_scene] pygame not available — text mode")
    print(f"Complete {total_laps} lap(s) by typing 'lap' {total_laps} times")
    laps = 0
    start = time.time()
    while laps < total_laps:
        cmd = input(f"lap (laps: {laps}/{total_laps}) > ").strip().lower()
        if cmd in ("lap", "l"):
            laps += 1
        elif cmd == "q":
            break
    elapsed = time.time() - start
    dnf = laps < total_laps
    return {
        "finish_time_ms": int(elapsed * 1000) if not dnf else 0,
        "laps_completed": laps,
        "dnf": dnf,
        "best_lap_ms": int(elapsed * 1000) if not dnf else None,
        "total_time_ms": int(elapsed * 1000),
    }


if __name__ == "__main__":
    result = run_race(total_laps=1, time_limit=60.0)
    print("Race result:", result)
