// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "driving_data.pb.h"
#include "DrivingDataVisualizer.generated.h"

/**
 * 智能驾驶数据可视化 Actor
 * - 自车：自定义大小的包围盒（绿色），带朝向旋转
 * - 障碍物：长宽高包围盒，不同类型不同颜色
 * - 车道线：采样点连成的线，按类型/颜色区分
 * - 地面标识：点集围成的多边形
 * - 可行驶区域：边界多边形
 */
UCLASS()
class PROTOTEST_API ADrivingDataVisualizer : public AActor
{
	GENERATED_BODY()

public:
	ADrivingDataVisualizer();

	/** 设置 Ego 数据并触发刷新 */
	void SetEgoData(const pbt::Ego& InEgo);

	/** 设置 Cross 数据并触发刷新 */
	void SetCrossData(const pbt::Cross& InCross);

	// ── 可视化参数 ──

	/** 自车包围盒半尺寸 (长, 宽, 高) cm，默认 (225, 90, 75) ≈ 4.5x1.8x1.5m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	FVector EgoBoxExtent = FVector(225.0f, 90.0f, 75.0f);

	/** Debug 线条粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	float LineThickness = 3.0f;

	/** 是否显示障碍物速度方向箭头 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bShowVelocityArrows = true;

	/** 速度箭头的缩放倍数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	float VelocityArrowScale = 1.0f;

	/** 是否启用可视化 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bVisualizationEnabled = true;

protected:
	virtual void Tick(float DeltaTime) override;

private:
	void DrawEgo();
	void DrawCross();

	// ── 颜色映射 ──
	static FColor GetObstacleColor(pbt::ObstacleType Type);
	static FColor GetLaneLineColor(pbt::LaneLineType Type, pbt::LaneLineColor Color);
	static FColor GetGroundSignColor(pbt::GroundSignType Type);
	static FColor GetDrivableAreaColor(int32 Index);

	// ── 绘制辅助 ──
	void DrawBox(const FVector& Center, const FVector& Extent, double Heading, const FColor& Color) const;
	void DrawArrow(const FVector& From, const FVector& To, const FColor& Color) const;
	void DrawPolygon(const TArray<FVector>& Points, const FColor& Color) const;
	void DrawPolyline(const TArray<FVector>& Points, const FColor& Color) const;

	// ── 数据存储 ──
	TOptional<pbt::Ego> CachedEgo;
	TOptional<pbt::Cross> CachedCross;
	bool bEgoDirty = false;
	bool bCrossDirty = false;
};
