// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "driving_data.pb.h"

/**
 * Driving Data 可视化颜色集中定义
 * 所有颜色在此一处维护，按数据类别分组
 */
namespace DrivingDataColors
{
	// ═══════════════════════════════════════════════
	//  Ego 相关
	// ═══════════════════════════════════════════════

	inline FColor EgoVehicle()          { return FColor::Green; }

	// ═══════════════════════════════════════════════
	//  障碍物
	// ═══════════════════════════════════════════════

	inline FColor Obstacle(pbt::ObstacleType Type)
	{
		switch (Type)
		{
		case pbt::OBSTACLE_VEHICLE:    return FColor::Red;
		case pbt::OBSTACLE_PEDESTRIAN: return FColor::Yellow;
		case pbt::OBSTACLE_CYCLIST:    return FColor::Orange;
		case pbt::OBSTACLE_TRUCK:      return FColor(200, 0, 0);     // DarkRed
		case pbt::OBSTACLE_BUS:        return FColor(30, 100, 255);  // Blue
		case pbt::OBSTACLE_MOTORCYCLE: return FColor::Cyan;
		case pbt::OBSTACLE_ANIMAL:     return FColor(139, 69, 19);   // Brown
		case pbt::OBSTACLE_CONE:       return FColor::Magenta;
		case pbt::OBSTACLE_GUARDRAIL:  return FColor(180, 180, 180); // Gray
		default:                       return FColor::White;
		}
	}

	// ═══════════════════════════════════════════════
	//  车道线
	// ═══════════════════════════════════════════════

	inline FColor LaneLine(pbt::LaneLineType Type, pbt::LaneLineColor Color)
	{
		// 优先使用颜色枚举（映射到感知颜色）
		switch (Color)
		{
		case pbt::LANE_COLOR_WHITE:  return FColor(240, 240, 240);
		case pbt::LANE_COLOR_YELLOW: return FColor::Yellow;
		case pbt::LANE_COLOR_BLUE:   return FColor(50, 100, 255);
		case pbt::LANE_COLOR_GREEN:  return FColor(50, 200, 50);
		default: break;
		}
		// 颜色未知时按线型回退
		switch (Type)
		{
		case pbt::LANE_CURB:      return FColor(160, 160, 160);
		case pbt::LANE_ROAD_EDGE: return FColor(100, 100, 100);
		default:                  return FColor::White;
		}
	}

	// ═══════════════════════════════════════════════
	//  地面标识
	// ═══════════════════════════════════════════════

	inline FColor GroundSign(pbt::GroundSignType Type)
	{
		switch (Type)
		{
		case pbt::SIGN_CROSSWALK:      return FColor(220, 220, 220); // 浅灰
		case pbt::SIGN_ZEBRA_CROSSING: return FColor(220, 220, 220);
		case pbt::SIGN_STOP_LINE:      return FColor::Red;
		case pbt::SIGN_SPEED_BUMP:     return FColor(255, 200, 50);  // 金色
		case pbt::SIGN_YIELD_LINE:     return FColor::Orange;
		case pbt::SIGN_PARKING_SPACE:  return FColor(50, 150, 255);  // 蓝色
		// 各类方向箭头
		default:                       return FColor(200, 100, 255); // 紫色
		}
	}

	// ═══════════════════════════════════════════════
	//  可行驶区域
	// ═══════════════════════════════════════════════

	inline FColor DrivableArea(int32 Index)
	{
		constexpr FColor Palette[] = {
			FColor(0, 200, 100),    // Green
			FColor(50, 150, 200),   // LightBlue
			FColor(150, 200, 50),   // YellowGreen
		};
		return Palette[Index % UE_ARRAY_COUNT(Palette)];
	}

} // namespace DrivingDataColors
