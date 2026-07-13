// Copyright Epic Games, Inc. All Rights Reserved.

#include "DrivingDataVisualizer.h"
#include "DrawDebugHelpers.h"

ADrivingDataVisualizer::ADrivingDataVisualizer()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADrivingDataVisualizer::SetEgoData(const pbt::Ego& InEgo)
{
	CachedEgo = InEgo;
	bEgoDirty = true;
}

void ADrivingDataVisualizer::SetCrossData(const pbt::Cross& InCross)
{
	CachedCross = InCross;
	bCrossDirty = true;
}

// ═══════════════════════════════════════════════════════
//  颜色映射
// ═══════════════════════════════════════════════════════

FColor ADrivingDataVisualizer::GetObstacleColor(pbt::ObstacleType Type)
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

FColor ADrivingDataVisualizer::GetLaneLineColor(pbt::LaneLineType Type, pbt::LaneLineColor Color)
{
	// 优先使用车道线颜色（白/黄/蓝/绿）
	switch (Color)
	{
	case pbt::LANE_COLOR_WHITE:  return FColor(240, 240, 240);
	case pbt::LANE_COLOR_YELLOW: return FColor::Yellow;
	case pbt::LANE_COLOR_BLUE:   return FColor(50, 100, 255);
	case pbt::LANE_COLOR_GREEN:  return FColor(50, 200, 50);
	default:
		// 根据线型回退
		switch (Type)
		{
		case pbt::LANE_CURB:      return FColor(160, 160, 160);
		case pbt::LANE_ROAD_EDGE: return FColor(100, 100, 100);
		default:                  return FColor::White;
		}
	}
}

FColor ADrivingDataVisualizer::GetGroundSignColor(pbt::GroundSignType Type)
{
	switch (Type)
	{
	case pbt::SIGN_CROSSWALK:
	case pbt::SIGN_ZEBRA_CROSSING:
		return FColor(220, 220, 220);
	case pbt::SIGN_STOP_LINE:
		return FColor::Red;
	case pbt::SIGN_SPEED_BUMP:
		return FColor(255, 200, 50);
	case pbt::SIGN_YIELD_LINE:
		return FColor::Orange;
	case pbt::SIGN_PARKING_SPACE:
		return FColor(50, 150, 255);
	// 各类箭头
	default:
		return FColor(200, 100, 255); // Purple
	}
}

FColor ADrivingDataVisualizer::GetDrivableAreaColor(int32 Index)
{
	static const FColor Colors[] = {
		FColor(0, 200, 100),   // Green
		FColor(50, 150, 200),  // LightBlue
		FColor(150, 200, 50),  // YellowGreen
	};
	return Colors[Index % 3];
}

// ═══════════════════════════════════════════════════════
//  绘制辅助
// ═══════════════════════════════════════════════════════

void ADrivingDataVisualizer::DrawBox(const FVector& Center, const FVector& Extent, double Heading, const FColor& Color) const
{
	const FQuat Rot = FRotator(0.0, Heading, 0.0).Quaternion();
	DrawDebugBox(GetWorld(), Center, Extent, Rot, Color,
		/*bPersistentLines=*/false, /*LifeTime=*/-1.0f, /*DepthPriority=*/0, LineThickness);
}

void ADrivingDataVisualizer::DrawArrow(const FVector& From, const FVector& To, const FColor& Color) const
{
	DrawDebugLine(GetWorld(), From, To, Color, false, -1.0f, 0, LineThickness);

	// 简单箭头尖端
	const float ArrowLen = 30.0f;
	const FVector Dir = (To - From).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		const FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
		DrawDebugLine(GetWorld(), To, To - Dir * ArrowLen + Right * ArrowLen * 0.5f, Color, false, -1.0f, 0, LineThickness);
		DrawDebugLine(GetWorld(), To, To - Dir * ArrowLen - Right * ArrowLen * 0.5f, Color, false, -1.0f, 0, LineThickness);
	}
}

void ADrivingDataVisualizer::DrawPolygon(const TArray<FVector>& Points, const FColor& Color) const
{
	if (Points.Num() < 2) return;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const int32 Next = (i + 1) % Points.Num();
		DrawDebugLine(GetWorld(), Points[i], Points[Next], Color, false, -1.0f, 0, LineThickness);
	}
}

void ADrivingDataVisualizer::DrawPolyline(const TArray<FVector>& Points, const FColor& Color) const
{
	if (Points.Num() < 2) return;
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		DrawDebugLine(GetWorld(), Points[i], Points[i + 1], Color, false, -1.0f, 0, LineThickness);
	}
}

// ═══════════════════════════════════════════════════════
//  可视化绘制
// ═══════════════════════════════════════════════════════

void ADrivingDataVisualizer::DrawEgo()
{
	if (!CachedEgo.IsSet()) return;
	const pbt::Ego& Ego = CachedEgo.GetValue();
	const FVector Origin = GetActorLocation();

	// proto 坐标单位是米，UE 单位是厘米
	static constexpr float M_TO_CM = 100.0f;

	// ── 自车：绿色包围盒 + 速度箭头 ──
	if (Ego.has_vehicle())
	{
		const auto& V = Ego.vehicle();
		const FVector Pos(
			static_cast<float>(V.position().x()) * M_TO_CM,
			static_cast<float>(V.position().y()) * M_TO_CM,
			static_cast<float>(V.position().z()) * M_TO_CM);
		// EgoBoxExtent 已为 cm，直接使用
		const float Heading = static_cast<float>(V.rotation().z());
		DrawBox(Origin + Pos, EgoBoxExtent, Heading, FColor::Green);

		if (bShowVelocityArrows)
		{
			const FVector Vel(
				static_cast<float>(V.velocity().x()) * M_TO_CM,
				static_cast<float>(V.velocity().y()) * M_TO_CM,
				static_cast<float>(V.velocity().z()) * M_TO_CM);
			if (!Vel.IsNearlyZero())
			{
				DrawArrow(Origin + Pos, Origin + Pos + Vel * VelocityArrowScale, FColor::Green);
			}
		}
	}

	// ── 障碍物：按类型着色包围盒 + 速度箭头 ──
	for (int32 i = 0; i < Ego.obstacles_size(); ++i)
	{
		const auto& O = Ego.obstacles(i);
		const FColor C = GetObstacleColor(O.type());
		const FVector Pos(
			static_cast<float>(O.position().x()) * M_TO_CM,
			static_cast<float>(O.position().y()) * M_TO_CM,
			static_cast<float>(O.position().z()) * M_TO_CM);
		// size 单位 m，×100 转 cm，×0.5 得到半尺寸
		const FVector Ext(
			static_cast<float>(O.size().x()) * 50.0f,
			static_cast<float>(O.size().y()) * 50.0f,
			static_cast<float>(O.size().z()) * 50.0f);
		const float Heading = static_cast<float>(O.heading());

		DrawBox(Origin + Pos, Ext, Heading, C);

		if (bShowVelocityArrows)
		{
			const FVector Vel(
				static_cast<float>(O.velocity().x()) * M_TO_CM,
				static_cast<float>(O.velocity().y()) * M_TO_CM,
				static_cast<float>(O.velocity().z()) * M_TO_CM);
			if (!Vel.IsNearlyZero())
			{
				DrawArrow(Origin + Pos, Origin + Pos + Vel * VelocityArrowScale, C);
			}
		}
	}
}

void ADrivingDataVisualizer::DrawCross()
{
	if (!CachedCross.IsSet()) return;
	const pbt::Cross& Cross = CachedCross.GetValue();
	const FVector Origin = GetActorLocation();

	static constexpr float M_TO_CM = 100.0f;
	auto ToFVector = [](const pbt::Vec3& V) -> FVector
	{
		return FVector(
			static_cast<float>(V.x()) * M_TO_CM,
			static_cast<float>(V.y()) * M_TO_CM,
			static_cast<float>(V.z()) * M_TO_CM);
	};

	// ── 车道线：按类型+颜色着色，点集连成折线 ──
	for (int32 i = 0; i < Cross.lane_lines_size(); ++i)
	{
		const auto& LL = Cross.lane_lines(i);
		const FColor C = GetLaneLineColor(LL.type(), LL.color());
		TArray<FVector> Pts;
		for (int32 j = 0; j < LL.points_size(); ++j)
		{
			Pts.Add(Origin + ToFVector(LL.points(j)));
		}
		DrawPolyline(Pts, C);
	}

	// ── 地面标识：点集围成的多边形 ──
	for (int32 i = 0; i < Cross.ground_signs_size(); ++i)
	{
		const auto& GS = Cross.ground_signs(i);
		const FColor C = GetGroundSignColor(GS.type());
		TArray<FVector> Pts;
		for (int32 j = 0; j < GS.polygon_size(); ++j)
		{
			Pts.Add(Origin + ToFVector(GS.polygon(j)));
		}
		DrawPolygon(Pts, C);
	}

	// ── 可行驶区域：边界多边形 ──
	for (int32 i = 0; i < Cross.drivable_areas_size(); ++i)
	{
		const auto& DA = Cross.drivable_areas(i);
		const FColor C = GetDrivableAreaColor(i);
		TArray<FVector> Pts;
		for (int32 j = 0; j < DA.boundary_size(); ++j)
		{
			Pts.Add(Origin + ToFVector(DA.boundary(j)));
		}
		DrawPolygon(Pts, C);
	}
}

// ═══════════════════════════════════════════════════════
//  Tick
// ═══════════════════════════════════════════════════════

void ADrivingDataVisualizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bVisualizationEnabled) return;

	// 持续绘制（因为 DrawDebug 的 Lifetime = -1 只在单帧可见）
	DrawEgo();
	DrawCross();

	bEgoDirty = false;
	bCrossDirty = false;
}
