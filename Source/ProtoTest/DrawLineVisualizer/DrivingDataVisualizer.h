// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "driving_data.pb.h"
#include "DrivingDataVisualizer.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * 智能驾驶数据可视化 Actor
 * - 自车：自定义大小的包围盒（绿色），带朝向旋转
 * - 障碍物：长宽高包围盒，不同类型不同颜色
 * - 车道线：采样点连成的线，按类型/颜色区分
 * - 地面标识：点集围成的多边形
 * - 可行驶区域：边界多边形
 * - 自带俯视跟随相机（SpringArm + Camera）
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

	/** 当前自车世界坐标（每帧更新） */
	UPROPERTY(BlueprintReadOnly, Category = "DrivingData")
	FVector EgoWorldPosition = FVector::ZeroVector;

	/** 当前自车朝向角（度） */
	UPROPERTY(BlueprintReadOnly, Category = "DrivingData")
	float EgoHeading = 0.0f;

	// ── 可视化参数 ──

	/** 自车包围盒半尺寸 (长, 宽, 高) cm，默认 (225, 90, 75) ≈ 4.5x1.8x1.5m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	FVector EgoBoxExtent = FVector(225.0f, 90.0f, 75.0f);

	/** 自车包围盒线条粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization|Thickness")
	float EgoLineThickness = 3.0f;

	/** 障碍物包围盒线条粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization|Thickness")
	float ObstacleLineThickness = 2.0f;

	/** 车道线粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization|Thickness")
	float LaneLineThickness = 2.0f;

	/** 地面标识线条粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization|Thickness")
	float GroundSignThickness = 2.0f;

	/** 可行驶区域线条粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization|Thickness")
	float DrivableAreaThickness = 3.0f;

	/** 速度箭头线条粗细 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization|Thickness")
	float ArrowThickness = 2.0f;

	/** 是否显示障碍物速度方向箭头 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bShowVelocityArrows = true;

	/** 速度箭头的缩放倍数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	float VelocityArrowScale = 1.0f;

	/** 是否启用可视化 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bVisualizationEnabled = true;

	/** 相机是否跟随自车（勾选后 Actor 位置会随自车移动） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
	bool bFollowEgoCamera = true;

	/** 相机跟随 + 鼠标旋转时旋转速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraRotationSpeed = 1.0f;

	// ── 相机组件 ──

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	void DrawEgo();
	void DrawCross();

	// ── 绘制辅助（Thickness 参数从对应属性传入）──
	void DrawBox(const FVector& Center, const FVector& Extent, double Heading, const FColor& Color, float Thickness) const;
	void DrawArrow(const FVector& From, const FVector& To, const FColor& Color, float Thickness) const;
	void DrawPolygon(const TArray<FVector>& Points, const FColor& Color, float Thickness) const;
	void DrawPolyline(const TArray<FVector>& Points, const FColor& Color, float Thickness) const;

	// ── 数据存储 ──
	TOptional<pbt::Ego> CachedEgo;
	TOptional<pbt::Cross> CachedCross;
	bool bEgoDirty = false;
	bool bCrossDirty = false;

	// ── 坐标系锚点（World 原点，Cross 数据以此为基准，不随自车移动） ──
	FVector WorldAnchor = FVector::ZeroVector;
};
