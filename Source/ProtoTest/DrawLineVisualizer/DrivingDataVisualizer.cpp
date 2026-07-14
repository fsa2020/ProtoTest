// Copyright Epic Games, Inc. All Rights Reserved.

#include "DrivingDataVisualizer.h"
#include "DrivingDataColors.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"

ADrivingDataVisualizer::ADrivingDataVisualizer()
{
	PrimaryActorTick.bCanEverTick = true;

	// ── 俯视跟随相机 ──
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 3000.0f;          // 30m 高度
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm);

	// ── 在构造函数中绑定委托，确保早于任何 BeginPlay 中的 Broadcast ──
	OnEgoDataReceived.AddUObject(this, &ADrivingDataVisualizer::HandleEgoData);
	OnCrossDataReceived.AddUObject(this, &ADrivingDataVisualizer::HandleCrossData);
}

void ADrivingDataVisualizer::BeginPlay()
{
	Super::BeginPlay();

	// ── 在 BeginPlay 设置俯视角度（构造函数会被关卡序列化覆盖）──
	SpringArm->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->bShowMouseCursor = true;
	}
}

void ADrivingDataVisualizer::HandleEgoData(const pbt::Ego& InEgo)
{
	CachedEgo = InEgo;
	bEgoDirty = true;
}

void ADrivingDataVisualizer::HandleCrossData(const pbt::Cross& InCross)
{
	CachedCross = InCross;
	bCrossDirty = true;
}

// ═══════════════════════════════════════════════════════
//  绘制辅助（颜色定义见 DrivingDataColors.h）
// ═══════════════════════════════════════════════════════

void ADrivingDataVisualizer::DrawBox(const FVector& Center, const FVector& Extent, double Heading, const FColor& Color, float Thickness) const
{
	const FQuat Rot = FRotator(0.0, Heading, 0.0).Quaternion();
	DrawDebugBox(GetWorld(), Center, Extent, Rot, Color,
		false, -1.0f, 0, Thickness);
}

void ADrivingDataVisualizer::DrawArrow(const FVector& From, const FVector& To, const FColor& Color, float Thickness) const
{
	DrawDebugLine(GetWorld(), From, To, Color, false, -1.0f, 0, Thickness);

	const float ArrowLen = 30.0f;
	const FVector Dir = (To - From).GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		const FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
		DrawDebugLine(GetWorld(), To, To - Dir * ArrowLen + Right * ArrowLen * 0.5f, Color, false, -1.0f, 0, Thickness);
		DrawDebugLine(GetWorld(), To, To - Dir * ArrowLen - Right * ArrowLen * 0.5f, Color, false, -1.0f, 0, Thickness);
	}
}

void ADrivingDataVisualizer::DrawPolygon(const TArray<FVector>& Points, const FColor& Color, float Thickness) const
{
	if (Points.Num() < 2) return;
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const int32 Next = (i + 1) % Points.Num();
		DrawDebugLine(GetWorld(), Points[i], Points[Next], Color, false, -1.0f, 0, Thickness);
	}
}

void ADrivingDataVisualizer::DrawPolyline(const TArray<FVector>& Points, const FColor& Color, float Thickness) const
{
	if (Points.Num() < 2) return;
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		DrawDebugLine(GetWorld(), Points[i], Points[i + 1], Color, false, -1.0f, 0, Thickness);
	}
}

// ═══════════════════════════════════════════════════════
//  可视化绘制（所有绘制基于 WorldAnchor，不依赖 ActorLocation）
// ═══════════════════════════════════════════════════════

void ADrivingDataVisualizer::DrawEgo()
{
	if (!CachedEgo.IsSet()) return;
	const pbt::Ego& Ego = CachedEgo.GetValue();

	static constexpr float M_TO_CM = 100.0f;

	// ── 自车：计算世界坐标 → 更新 Public 属性 + 可选相机跟随 ──
	if (Ego.has_vehicle())
	{
		const auto& V = Ego.vehicle();
		const FVector EgoPosCM(
			static_cast<float>(V.position().x()) * M_TO_CM,
			static_cast<float>(V.position().y()) * M_TO_CM,
			static_cast<float>(V.position().z()) * M_TO_CM);
		const float Heading = static_cast<float>(V.rotation().z());

		// 记录供外部读取 / Blueprint 使用
		EgoWorldPosition = WorldAnchor + EgoPosCM;
		EgoHeading = Heading;

		// 绘制自车包围盒
		DrawBox(EgoWorldPosition, EgoBoxExtent, Heading, FColor::Green, EgoLineThickness);

		if (bShowVelocityArrows)
		{
			const FVector Vel(
				static_cast<float>(V.velocity().x()) * M_TO_CM,
				static_cast<float>(V.velocity().y()) * M_TO_CM,
				static_cast<float>(V.velocity().z()) * M_TO_CM);
			if (!Vel.IsNearlyZero())
			{
				DrawArrow(EgoWorldPosition, EgoWorldPosition + Vel * VelocityArrowScale, FColor::Green, ArrowThickness);
			}
		}
	}

	// ── 障碍物 ──
	for (int32 i = 0; i < Ego.obstacles_size(); ++i)
	{
		const auto& O = Ego.obstacles(i);
		const FColor C = DrivingDataColors::Obstacle(O.type());
		const FVector Pos(
			static_cast<float>(O.position().x()) * M_TO_CM,
			static_cast<float>(O.position().y()) * M_TO_CM,
			static_cast<float>(O.position().z()) * M_TO_CM);
		const FVector Ext(
			static_cast<float>(O.size().x()) * 50.0f,
			static_cast<float>(O.size().y()) * 50.0f,
			static_cast<float>(O.size().z()) * 50.0f);
		const float Heading = static_cast<float>(O.heading());

		const FVector WorldPos = WorldAnchor + Pos;
		DrawBox(WorldPos, Ext, Heading, C, ObstacleLineThickness);

		if (bShowVelocityArrows)
		{
			const FVector Vel(
				static_cast<float>(O.velocity().x()) * M_TO_CM,
				static_cast<float>(O.velocity().y()) * M_TO_CM,
				static_cast<float>(O.velocity().z()) * M_TO_CM);
			if (!Vel.IsNearlyZero())
			{
				DrawArrow(WorldPos, WorldPos + Vel * VelocityArrowScale, C, ArrowThickness);
			}
		}
	}
}

void ADrivingDataVisualizer::DrawCross()
{
	if (!CachedCross.IsSet()) return;
	const pbt::Cross& Cross = CachedCross.GetValue();

	static constexpr float M_TO_CM = 100.0f;
	auto ToWorld = [this](const pbt::Vec3& V) -> FVector
	{
		return WorldAnchor + FVector(
			static_cast<float>(V.x()) * M_TO_CM,
			static_cast<float>(V.y()) * M_TO_CM,
			static_cast<float>(V.z()) * M_TO_CM);
	};

	// ── 车道线 ──
	for (int32 i = 0; i < Cross.lane_lines_size(); ++i)
	{
		const auto& LL = Cross.lane_lines(i);
		const FColor C = DrivingDataColors::LaneLine(LL.type(), LL.color());
		TArray<FVector> Pts;
		for (int32 j = 0; j < LL.points_size(); ++j)
		{
			Pts.Add(ToWorld(LL.points(j)));
		}
		DrawPolyline(Pts, C, LaneLineThickness);
	}

	// ── 地面标识 ──
	for (int32 i = 0; i < Cross.ground_signs_size(); ++i)
	{
		const auto& GS = Cross.ground_signs(i);
		const FColor C = DrivingDataColors::GroundSign(GS.type());
		TArray<FVector> Pts;
		for (int32 j = 0; j < GS.polygon_size(); ++j)
		{
			Pts.Add(ToWorld(GS.polygon(j)));
		}
		DrawPolygon(Pts, C, GroundSignThickness);
	}

	// ── 可行驶区域 ──
	for (int32 i = 0; i < Cross.drivable_areas_size(); ++i)
	{
		const auto& DA = Cross.drivable_areas(i);
		const FColor C = DrivingDataColors::DrivableArea(i);
		TArray<FVector> Pts;
		for (int32 j = 0; j < DA.boundary_size(); ++j)
		{
			Pts.Add(ToWorld(DA.boundary(j)));
		}
		DrawPolygon(Pts, C, DrivableAreaThickness);
	}
}

// ═══════════════════════════════════════════════════════
//  Tick
// ═══════════════════════════════════════════════════════

void ADrivingDataVisualizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bVisualizationEnabled) return;

	// ── 按住左键拖拽旋转相机 ──
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		static FVector2D LastMouse;
		float MX, MY;
		if (PC->GetMousePosition(MX, MY))
		{
			const FVector2D CurrMouse(MX, MY);
			if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
			{
				const FVector2D Delta = CurrMouse - LastMouse;
				if (!Delta.IsNearlyZero())
				{
					FRotator Rot = SpringArm->GetRelativeRotation();
					Rot.Yaw += Delta.X * CameraRotationSpeed * 0.2f;
					Rot.Pitch = FMath::Clamp(Rot.Pitch - Delta.Y * CameraRotationSpeed * 0.2f, -89.0f, -10.0f);
					SpringArm->SetRelativeRotation(Rot);
				}
			}
			LastMouse = CurrMouse;
		}
	}

	// 锚定世界原点（首次以 Actor 出生位置为基准）
	if (WorldAnchor.IsNearlyZero() && !GetActorLocation().IsNearlyZero())
	{
		WorldAnchor = GetActorLocation();
	}
	else if (WorldAnchor.IsNearlyZero())
	{
		WorldAnchor = FVector::ZeroVector;
	}

	DrawEgo();
	DrawCross();

	// ── 相机跟随：Actor 移到自车位置（SpringArm+Camera 自动跟随）──
	if (bFollowEgoCamera && CachedEgo.IsSet() && CachedEgo->has_vehicle())
	{
		SetActorLocation(EgoWorldPosition);
	}

	bEgoDirty = false;
	bCrossDirty = false;
}
