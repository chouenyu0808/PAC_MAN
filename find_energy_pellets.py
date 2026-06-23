import re

def parse_walls(content):
    walls = []
    match = re.search(r'const Wall walls\[\] PROGMEM = \{(.*?)\};', content, re.DOTALL)
    if not match:
        return []
    wall_data = match.group(1)
    for item in re.findall(r'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', wall_data):
        walls.append({
            'x': int(item[0]),
            'y': int(item[1]),
            'w': int(item[2]),
            'h': int(item[3])
        })
    return walls

def check_collision(x, y, walls, size):
    for w in walls:
        wx, wy, ww, wh = w['x'], w['y'], w['w'], w['h']
        if x + size > wx and x - size < wx + ww and y + size > wy and y - size < wy + wh:
            return True
    return False

def get_reachable_cells(walls, size, start_x, start_y):
    queue = [(start_x, start_y)]
    visited = set([(start_x, start_y)])
    
    while queue:
        cx, cy = queue.pop(0)
        for dx, dy in [(-5, 0), (5, 0), (0, -5), (0, 5)]:
            nx, ny = cx + dx, cy + dy
            if 15 <= nx <= 705 and 15 <= ny <= 305:
                if (nx, ny) not in visited:
                    if not check_collision(nx, ny, walls, size):
                        visited.add((nx, ny))
                        queue.append((nx, ny))
    return visited

def main():
    with open('PAC_MAN.ino', 'r', encoding='utf-8') as f:
        content = f.read()
    
    walls = parse_walls(content)
    size = 8
    
    reachable_pixels = get_reachable_cells(walls, size, 280, 160)
    alt_reachable_p1 = get_reachable_cells(walls, size, 170, 30)
    alt_reachable_p2 = get_reachable_cells(walls, size, 420, 110)
    all_reachable = reachable_pixels.union(alt_reachable_p1).union(alt_reachable_p2)
    
    valid_pellets = []
    for r in range(16):
        y_base = r * 20
        py = y_base + 10
        for c in range(36):
            x_base = c * 20
            px = x_base + 10
            
            if 290 <= x_base <= 430 and 120 <= y_base <= 190:
                continue
                
            if check_collision(px, py, walls, size):
                continue
            
            if (px, py) in all_reachable:
                valid_pellets.append((x_base, y_base))
                
    # Now find symmetric pairs
    symmetric_pairs = []
    seen = set()
    for x, y in valid_pellets:
        sym_x = 700 - x
        if (sym_x, y) in valid_pellets:
            if (x, y) not in seen and (sym_x, y) not in seen:
                if x < sym_x:
                    symmetric_pairs.append(((x, y), (sym_x, y)))
                elif x == sym_x:
                    symmetric_pairs.append(((x, y),))
                seen.add((x, y))
                seen.add((sym_x, y))
                
    # Sort symmetric pairs by Y then X
    symmetric_pairs.sort(key=lambda item: (item[0][1], item[0][0]))
    
    print("Found symmetric pairs:")
    for pair in symmetric_pairs:
        if len(pair) == 2:
            print(f"y={pair[0][1]}: Left({pair[0][0]}), Right({pair[1][0]})")
        else:
            print(f"y={pair[0][1]}: Center({pair[0][0]})")

if __name__ == '__main__':
    main()
