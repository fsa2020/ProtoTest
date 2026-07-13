"""
根据 driving_data.proto 生成测试二进制数据：
  - ego.bin     : 包含 Ego message（自车 + 所有障碍物类型）
  - cross.bin   : 包含 Cross message（车道线 / 地面标识 / 可行驶区域）

道路布局：双向各三车道，沿 X 轴 0～200m，单车道宽 3.5m
  自车在左向中间车道 (x=50, y=-1.75)，沿 +X 行驶 @ 60km/h
"""

import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROTO_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, ".."))
OUT_DIR = os.path.join(SCRIPT_DIR, "testBins")

sys.path.insert(0, PROTO_DIR)

import driving_data_pb2 as pbt


def v3(x, y, z=0.0):
    """快捷创建 Vec3"""
    v = pbt.Vec3()
    v.x = x
    v.y = y
    v.z = z
    return v


def build_ego():
    """构建 Ego message，包含自车 + 所有障碍物类型"""

    ego = pbt.Ego()

    # ── 自车：位于左向中间车道 (y=-1.75)，60km/h = 16.67m/s ──
    ev = ego.vehicle
    ev.id = 0
    ev.position.CopyFrom(v3(50.0, -1.75))
    ev.velocity.CopyFrom(v3(16.67, 0.0))
    ev.rotation.CopyFrom(v3(0.0, 0.0, 0.0))
    ev.timestamp = 1700000000000

    # ── 障碍物：覆盖全部 9 种 ObstacleType ──
    obs_list = []

    # 1. VEHICLE - 前方同车道，50km/h
    o = pbt.Obstacle()
    o.id = 1
    o.type = pbt.OBSTACLE_VEHICLE
    o.position.CopyFrom(v3(80.0, -1.75))
    o.velocity.CopyFrom(v3(13.89, 0.0))
    o.size.CopyFrom(v3(4.5, 1.8, 1.5))
    o.heading = 0.0
    obs_list.append(o)

    # 2. PEDESTRIAN - 右侧人行道，斜向行走
    o = pbt.Obstacle()
    o.id = 2
    o.type = pbt.OBSTACLE_PEDESTRIAN
    o.position.CopyFrom(v3(55.0, 12.0))
    o.velocity.CopyFrom(v3(-1.2, 0.5))
    o.size.CopyFrom(v3(0.5, 0.5, 1.7))
    o.heading = 135.0
    obs_list.append(o)

    # 3. CYCLIST - 左侧慢车道，约 21km/h
    o = pbt.Obstacle()
    o.id = 3
    o.type = pbt.OBSTACLE_CYCLIST
    o.position.CopyFrom(v3(40.0, -5.25))
    o.velocity.CopyFrom(v3(6.0, 0.0))
    o.size.CopyFrom(v3(1.8, 0.6, 1.6))
    o.heading = 0.0
    obs_list.append(o)

    # 4. TRUCK - 对向车道（右向），迎面驶来
    o = pbt.Obstacle()
    o.id = 4
    o.type = pbt.OBSTACLE_TRUCK
    o.position.CopyFrom(v3(70.0, 5.25))
    o.velocity.CopyFrom(v3(-10.0, 0.0))
    o.size.CopyFrom(v3(10.0, 2.5, 3.5))
    o.heading = 180.0
    obs_list.append(o)

    # 5. BUS - 对向快车道
    o = pbt.Obstacle()
    o.id = 5
    o.type = pbt.OBSTACLE_BUS
    o.position.CopyFrom(v3(130.0, 8.75))
    o.velocity.CopyFrom(v3(-8.0, 0.0))
    o.size.CopyFrom(v3(12.0, 2.5, 3.2))
    o.heading = 180.0
    obs_list.append(o)

    # 6. MOTORCYCLE - 左侧快车道，较快
    o = pbt.Obstacle()
    o.id = 6
    o.type = pbt.OBSTACLE_MOTORCYCLE
    o.position.CopyFrom(v3(60.0, -8.75))
    o.velocity.CopyFrom(v3(20.0, 0.0))
    o.size.CopyFrom(v3(2.0, 0.8, 1.2))
    o.heading = 0.0
    obs_list.append(o)

    # 7. ANIMAL - 右侧路边，向马路方向移动
    o = pbt.Obstacle()
    o.id = 7
    o.type = pbt.OBSTACLE_ANIMAL
    o.position.CopyFrom(v3(95.0, 13.5))
    o.velocity.CopyFrom(v3(1.0, -1.5))
    o.size.CopyFrom(v3(0.8, 0.4, 0.6))
    o.heading = -30.0
    obs_list.append(o)

    # 8. CONE (x3) - 前方施工路段锥桶阵列
    for i, (cx, cy) in enumerate([(150.0, -0.5), (153.0, 0.5), (156.0, -0.5)]):
        o = pbt.Obstacle()
        o.id = 8 + i
        o.type = pbt.OBSTACLE_CONE
        o.position.CopyFrom(v3(cx, cy))
        o.velocity.CopyFrom(v3(0.0, 0.0))
        o.size.CopyFrom(v3(0.3, 0.3, 0.7))
        o.heading = 0.0
        obs_list.append(o)

    # 9. GUARDRAIL (x2) - 左右两侧护栏
    o = pbt.Obstacle()
    o.id = 11
    o.type = pbt.OBSTACLE_GUARDRAIL
    o.position.CopyFrom(v3(25.0, 11.0))
    o.velocity.CopyFrom(v3(0.0, 0.0))
    o.size.CopyFrom(v3(50.0, 0.2, 0.8))
    o.heading = 0.0
    obs_list.append(o)

    o = pbt.Obstacle()
    o.id = 12
    o.type = pbt.OBSTACLE_GUARDRAIL
    o.position.CopyFrom(v3(100.0, -11.0))
    o.velocity.CopyFrom(v3(0.0, 0.0))
    o.size.CopyFrom(v3(40.0, 0.2, 0.8))
    o.heading = 0.0
    obs_list.append(o)

    ego.obstacles.extend(obs_list)
    return ego


def make_lane_points(y, xs):
    """生成一条车道线的采样点序列"""
    return [v3(float(x), float(y)) for x in xs]


def build_cross():
    """构建 Cross message，包含所有车道线类型/颜色、所有地面标识类型、可行驶区域"""

    cross = pbt.Cross()

    # ── 车道线：覆盖全部 7 种类型 + 4 种颜色 ──
    XS = [0, 50, 100, 150, 200]
    lane_configs = [
        # (id, type, color, y, confidence, width, points_xs)
        (1,  pbt.LANE_ROAD_EDGE,     pbt.LANE_COLOR_WHITE,  -10.5, 0.98, 0.15, XS),
        (2,  pbt.LANE_SOLID,         pbt.LANE_COLOR_WHITE,   -7.0, 0.99, 0.12, XS),
        (3,  pbt.LANE_DASHED,        pbt.LANE_COLOR_GREEN,   -3.5, 0.97, 0.12, XS),
        (4,  pbt.LANE_DOUBLE_SOLID,  pbt.LANE_COLOR_YELLOW,   0.0, 0.99, 0.30, XS),
        (5,  pbt.LANE_DASHED,        pbt.LANE_COLOR_WHITE,    3.5, 0.98, 0.12, XS),
        (6,  pbt.LANE_SOLID_DASHED,  pbt.LANE_COLOR_WHITE,    7.0, 0.96, 0.15, XS),
        (7,  pbt.LANE_CURB,          pbt.LANE_COLOR_BLUE,    10.5, 0.95, 0.20, XS),
        # 接近路口段：黄色双虚线
        (8,  pbt.LANE_DOUBLE_DASHED, pbt.LANE_COLOR_YELLOW,  -3.5, 0.94, 0.25, [175, 185, 195, 200]),
    ]

    for cfg in lane_configs:
        ll = cross.lane_lines.add()
        ll.id = cfg[0]
        ll.type = cfg[1]
        ll.color = cfg[2]
        for x in cfg[6]:
            ll.points.append(v3(x, cfg[3]))
        ll.confidence = cfg[4]
        ll.width = cfg[5]

    # ── 地面标识：覆盖全部 13 种类型 ──
    sign_configs = [
        # (id, type, polygon_rect: x0, y0, x1, y1)
        (1,  pbt.SIGN_ARROW_STRAIGHT,       15.0, -2.5,  18.0, -1.0),
        (2,  pbt.SIGN_ARROW_LEFT,           25.0, -6.0,  28.0, -4.5),
        (3,  pbt.SIGN_ARROW_RIGHT,          35.0,  2.5,  38.0,  1.0),
        (4,  pbt.SIGN_ARROW_UTURN,          45.0, -9.5,  48.0, -8.0),
        (5,  pbt.SIGN_ARROW_STRAIGHT_LEFT,  55.0,  4.5,  58.0,  2.5),
        (6,  pbt.SIGN_ARROW_STRAIGHT_RIGHT, 65.0,  6.0,  68.0,  8.5),
        (7,  pbt.SIGN_ARROW_LEFT_RIGHT,     75.0,  7.5,  78.0, 10.0),
        (8,  pbt.SIGN_CROSSWALK,           105.0, -10.5, 110.0, 10.5),
        (9,  pbt.SIGN_STOP_LINE,           180.0, -10.5, 180.5, 10.5),
        (10, pbt.SIGN_SPEED_BUMP,           85.0, -10.0,  86.5, 10.0),
        (11, pbt.SIGN_YIELD_LINE,           50.0,  10.5,  50.5, 14.0),
        (12, pbt.SIGN_PARKING_SPACE,       155.0,  11.5, 160.0, 14.0),
        (13, pbt.SIGN_ZEBRA_CROSSING,      112.0, -10.5, 117.0, 10.5),
    ]

    for cfg in sign_configs:
        gs = cross.ground_signs.add()
        gs.id = cfg[0]
        gs.type = cfg[1]
        x0, y0, x1, y1 = cfg[2], cfg[3], cfg[4], cfg[5]
        gs.polygon.append(v3(x0, y0))
        gs.polygon.append(v3(x1, y0))
        gs.polygon.append(v3(x1, y1))
        gs.polygon.append(v3(x0, y1))

    # ── 可行驶区域 ──
    # 主路区域
    da = cross.drivable_areas.add()
    da.id = 1
    da.boundary.append(v3(0.0, -10.5))
    da.boundary.append(v3(200.0, -10.5))
    da.boundary.append(v3(200.0, 10.5))
    da.boundary.append(v3(0.0, 10.5))
    da.confidence = 0.99
    da.speed_limit = 80.0

    # 右侧停车区
    da = cross.drivable_areas.add()
    da.id = 2
    da.boundary.append(v3(160.0, 10.5))
    da.boundary.append(v3(180.0, 10.5))
    da.boundary.append(v3(180.0, 15.0))
    da.boundary.append(v3(160.0, 15.0))
    da.confidence = 0.92
    da.speed_limit = 15.0

    return cross


def main():
    ego_path = os.path.join(OUT_DIR, "ego.bin")
    cross_path = os.path.join(OUT_DIR, "cross.bin")

    # ── 生成 & 写入 Ego ──
    ego = build_ego()
    with open(ego_path, "wb") as f:
        f.write(ego.SerializeToString())
    print(f"[OK] {ego_path}  ({ego.ByteSize()} bytes)")
    print(f"     vehicle + {len(ego.obstacles)} obstacles")

    # ── 生成 & 写入 Cross ──
    cross = build_cross()
    with open(cross_path, "wb") as f:
        f.write(cross.SerializeToString())
    print(f"[OK] {cross_path}  ({cross.ByteSize()} bytes)")
    print(f"     {len(cross.lane_lines)} lane_lines"
          f" + {len(cross.ground_signs)} ground_signs"
          f" + {len(cross.drivable_areas)} drivable_areas")


if __name__ == "__main__":
    main()
