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
            
            # Exclude ghost cage region: x in [290, 430], y in [120, 190]
            if 290 <= x_base <= 430 and 120 <= y_base <= 190:
                continue
                
            if check_collision(px, py, walls, size):
                continue
            
            if (px, py) in all_reachable:
                # Symmetry power pellets at 100, 280 and 600, 280
                is_power = (x_base == 100 and y_base == 280) or (x_base == 600 and y_base == 280)
                valid_pellets.append((x_base, y_base, is_power))
                
    print(f"Generated {len(valid_pellets)} valid pellets (excluding ghost cage).")
    
    cpp_lines = ["const Pellet pellets[] PROGMEM = {"]
    for idx, (x, y, is_power) in enumerate(valid_pellets):
        power_str = "true" if is_power else "false"
        comma = "," if idx < len(valid_pellets) - 1 else ""
        cpp_lines.append(f"  {{{x}, {y}, {power_str}}}{comma}")
    cpp_lines.append("};")
    
    with open('new_pellets.txt', 'w') as out:
        out.write("\n".join(cpp_lines))
    print("C++ definition written to new_pellets.txt")

    # Generate new ASCII map
    with open('new_map_ascii.txt', 'w', encoding='utf-8') as out:
        for y in range(0, 320, 10):
            row = ""
            for x in range(0, 720, 10):
                is_wall = check_collision(x + 5, y + 5, walls, 0)
                
                has_pellet = False
                is_power = False
                for px_base, py_base, ip in valid_pellets:
                    px = px_base + 10
                    py = py_base + 10
                    if x <= px < x + 10 and y <= py < y + 10:
                        has_pellet = True
                        is_power = ip
                        break
                
                if is_wall:
                    if has_pellet:
                        row += "X"
                    else:
                        row += "#"
                elif has_pellet:
                    row += "P" if is_power else "."
                else:
                    row += " "
            out.write(f"{y:03d}: {row}\n")
    print("New ASCII Map written to new_map_ascii.txt")

if __name__ == '__main__':
    main()
