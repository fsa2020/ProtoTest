// Copyright Epic Games, Inc. All Rights Reserved.

#include "PbTestActor.h"
#include "ProtobufHelper.h"
#include "DrivingDataVisualizer.h"
#include "driving_data.pb.h"
#include "driving_data.pb.cc"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// ===================================================================
// 枚举值 → 名称 手动映射（protobuf-lite 无 _Name() 反射）
// ===================================================================

static const TMap<pbt::ObstacleType, const TCHAR*> GObstacleTypeNames = {
	{ pbt::OBSTACLE_UNKNOWN,    TEXT("OBSTACLE_UNKNOWN") },
	{ pbt::OBSTACLE_VEHICLE,    TEXT("OBSTACLE_VEHICLE") },
	{ pbt::OBSTACLE_PEDESTRIAN, TEXT("OBSTACLE_PEDESTRIAN") },
	{ pbt::OBSTACLE_CYCLIST,    TEXT("OBSTACLE_CYCLIST") },
	{ pbt::OBSTACLE_TRUCK,      TEXT("OBSTACLE_TRUCK") },
	{ pbt::OBSTACLE_BUS,        TEXT("OBSTACLE_BUS") },
	{ pbt::OBSTACLE_MOTORCYCLE, TEXT("OBSTACLE_MOTORCYCLE") },
	{ pbt::OBSTACLE_ANIMAL,     TEXT("OBSTACLE_ANIMAL") },
	{ pbt::OBSTACLE_CONE,       TEXT("OBSTACLE_CONE") },
	{ pbt::OBSTACLE_GUARDRAIL,  TEXT("OBSTACLE_GUARDRAIL") },
};

static const TMap<pbt::LaneLineType, const TCHAR*> GLaneLineTypeNames = {
	{ pbt::LANE_UNKNOWN,      TEXT("LANE_UNKNOWN") },
	{ pbt::LANE_SOLID,        TEXT("LANE_SOLID") },
	{ pbt::LANE_DASHED,       TEXT("LANE_DASHED") },
	{ pbt::LANE_DOUBLE_SOLID, TEXT("LANE_DOUBLE_SOLID") },
	{ pbt::LANE_DOUBLE_DASHED,TEXT("LANE_DOUBLE_DASHED") },
	{ pbt::LANE_SOLID_DASHED, TEXT("LANE_SOLID_DASHED") },
	{ pbt::LANE_CURB,         TEXT("LANE_CURB") },
	{ pbt::LANE_ROAD_EDGE,    TEXT("LANE_ROAD_EDGE") },
};

static const TMap<pbt::LaneLineColor, const TCHAR*> GLaneLineColorNames = {
	{ pbt::LANE_COLOR_UNKNOWN, TEXT("LANE_COLOR_UNKNOWN") },
	{ pbt::LANE_COLOR_WHITE,   TEXT("LANE_COLOR_WHITE") },
	{ pbt::LANE_COLOR_YELLOW,  TEXT("LANE_COLOR_YELLOW") },
	{ pbt::LANE_COLOR_BLUE,    TEXT("LANE_COLOR_BLUE") },
	{ pbt::LANE_COLOR_GREEN,   TEXT("LANE_COLOR_GREEN") },
};

static const TMap<pbt::GroundSignType, const TCHAR*> GGroundSignTypeNames = {
	{ pbt::SIGN_UNKNOWN,             TEXT("SIGN_UNKNOWN") },
	{ pbt::SIGN_ARROW_STRAIGHT,      TEXT("SIGN_ARROW_STRAIGHT") },
	{ pbt::SIGN_ARROW_LEFT,          TEXT("SIGN_ARROW_LEFT") },
	{ pbt::SIGN_ARROW_RIGHT,         TEXT("SIGN_ARROW_RIGHT") },
	{ pbt::SIGN_ARROW_UTURN,         TEXT("SIGN_ARROW_UTURN") },
	{ pbt::SIGN_ARROW_STRAIGHT_LEFT, TEXT("SIGN_ARROW_STRAIGHT_LEFT") },
	{ pbt::SIGN_ARROW_STRAIGHT_RIGHT,TEXT("SIGN_ARROW_STRAIGHT_RIGHT") },
	{ pbt::SIGN_ARROW_LEFT_RIGHT,    TEXT("SIGN_ARROW_LEFT_RIGHT") },
	{ pbt::SIGN_CROSSWALK,           TEXT("SIGN_CROSSWALK") },
	{ pbt::SIGN_STOP_LINE,           TEXT("SIGN_STOP_LINE") },
	{ pbt::SIGN_SPEED_BUMP,          TEXT("SIGN_SPEED_BUMP") },
	{ pbt::SIGN_YIELD_LINE,          TEXT("SIGN_YIELD_LINE") },
	{ pbt::SIGN_PARKING_SPACE,       TEXT("SIGN_PARKING_SPACE") },
	{ pbt::SIGN_ZEBRA_CROSSING,      TEXT("SIGN_ZEBRA_CROSSING") },
};

// ── 打印辅助函数 ──

static void LogVec3(const TCHAR* Prefix, const pbt::Vec3& V)
{
	UE_LOG(LogTemp, Log, TEXT("%s(%.2f, %.2f, %.2f)"), Prefix, V.x(), V.y(), V.z());
}

static void LogEgoVehicle(const pbt::EgoVehicle& V)
{
	UE_LOG(LogTemp, Log, TEXT("--- EgoVehicle id=%d ---"), V.id());
	LogVec3(TEXT("  pos    = "), V.position());
	LogVec3(TEXT("  vel    = "), V.velocity());
	LogVec3(TEXT("  rot    = "), V.rotation());
	UE_LOG(LogTemp, Log, TEXT("  ts     = %lld"), V.timestamp());
}

static void LogObstacle(const pbt::Obstacle& O)
{
	UE_LOG(LogTemp, Log, TEXT("  [%d] type=%s(%d), heading=%.1f"),
		O.id(), GObstacleTypeNames.FindRef(O.type()), static_cast<int32>(O.type()), O.heading());
	LogVec3(TEXT("    pos="), O.position());
	LogVec3(TEXT("    vel="), O.velocity());
	LogVec3(TEXT("    size="), O.size());
}

static void LogEgo(const pbt::Ego& EgoData)
{
	LogEgoVehicle(EgoData.vehicle());
	UE_LOG(LogTemp, Log, TEXT("--- Obstacles (%d) ---"), EgoData.obstacles_size());
	for (int32 i = 0; i < EgoData.obstacles_size(); ++i)
	{
		LogObstacle(EgoData.obstacles(i));
	}
}

static void LogLaneLine(const pbt::LaneLine& L)
{
	UE_LOG(LogTemp, Log, TEXT("  [%d] type=%s color=%s conf=%.2f width=%.2f pts=%d"),
		L.id(),
		GLaneLineTypeNames.FindRef(L.type()),
		GLaneLineColorNames.FindRef(L.color()),
		L.confidence(), L.width(), L.points_size());
}

static void LogGroundSign(const pbt::GroundSign& G)
{
	UE_LOG(LogTemp, Log, TEXT("  [%d] type=%s poly_pts=%d"),
		G.id(), GGroundSignTypeNames.FindRef(G.type()), G.polygon_size());
}

static void LogDrivableArea(const pbt::DrivableArea& D)
{
	UE_LOG(LogTemp, Log, TEXT("  [%d] boundary_pts=%d conf=%.2f speed_limit=%.1f"),
		D.id(), D.boundary_size(), D.confidence(), D.speed_limit());
}

static void LogCross(const pbt::Cross& CrossData)
{
	UE_LOG(LogTemp, Log, TEXT("--- LaneLines (%d) ---"), CrossData.lane_lines_size());
	for (int32 i = 0; i < CrossData.lane_lines_size(); ++i)
	{
		LogLaneLine(CrossData.lane_lines(i));
	}
	UE_LOG(LogTemp, Log, TEXT("--- GroundSigns (%d) ---"), CrossData.ground_signs_size());
	for (int32 i = 0; i < CrossData.ground_signs_size(); ++i)
	{
		LogGroundSign(CrossData.ground_signs(i));
	}
	UE_LOG(LogTemp, Log, TEXT("--- DrivableAreas (%d) ---"), CrossData.drivable_areas_size());
	for (int32 i = 0; i < CrossData.drivable_areas_size(); ++i)
	{
		LogDrivableArea(CrossData.drivable_areas(i));
	}
}

// ── Actor 生命周期 ──

APbTestActor::APbTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APbTestActor::BeginPlay()
{
	Super::BeginPlay();

	const FString BinDir = FPaths::ProjectDir() / TEXT("Source/ProtoFiles/PythonScripts/testBins");

	// ── 动态生成可视化 Actor ──
	ADrivingDataVisualizer* Viz = GetWorld()->SpawnActor<ADrivingDataVisualizer>(
		ADrivingDataVisualizer::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (!Viz)
	{
		UE_LOG(LogTemp, Error, TEXT("[PbTestActor] 无法创建 DrivingDataVisualizer！"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] DrivingDataVisualizer 已创建"));

	// ── 切换视口跟随 Visualizer 的相机 ──
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetViewTarget(Viz);
		UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 视角已切换到 DrivingDataVisualizer"));
	}

	// ════════════════════════════════════════════════
	//  读取 & 打印 ego.bin
	// ════════════════════════════════════════════════

	UE_LOG(LogTemp, Log, TEXT("======================================================"));
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 读取 ego.bin"));
	UE_LOG(LogTemp, Log, TEXT("======================================================"));

	{
		const FString EgoPath = BinDir / TEXT("ego.bin");
		TArray<uint8> EgoBytes;
		if (!FFileHelper::LoadFileToArray(EgoBytes, *EgoPath))
		{
			UE_LOG(LogTemp, Error, TEXT("[PbTestActor] 无法读取 ego.bin: %s"), *EgoPath);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[PbTestActor] ego.bin 读取成功，大小: %d 字节"), EgoBytes.Num());

			pbt::Ego EgoData;
			if (FProtobufHelper::ParseFromBytes(EgoBytes, EgoData))
			{
				UE_LOG(LogTemp, Log, TEXT("[PbTestActor] Ego 反序列化成功"));
				LogEgo(EgoData);
				Viz->SetEgoData(EgoData);   // ── 传给可视化 ──
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[PbTestActor] Ego 反序列化失败！"));
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT(""));

	// ════════════════════════════════════════════════
	//  读取 & 打印 cross.bin
	// ════════════════════════════════════════════════

	UE_LOG(LogTemp, Log, TEXT("======================================================"));
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 读取 cross.bin"));
	UE_LOG(LogTemp, Log, TEXT("======================================================"));

	{
		const FString CrossPath = BinDir / TEXT("cross.bin");
		TArray<uint8> CrossBytes;
		if (!FFileHelper::LoadFileToArray(CrossBytes, *CrossPath))
		{
			UE_LOG(LogTemp, Error, TEXT("[PbTestActor] 无法读取 cross.bin: %s"), *CrossPath);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[PbTestActor] cross.bin 读取成功，大小: %d 字节"), CrossBytes.Num());

			pbt::Cross CrossData;
			if (FProtobufHelper::ParseFromBytes(CrossBytes, CrossData))
			{
				UE_LOG(LogTemp, Log, TEXT("[PbTestActor] Cross 反序列化成功"));
				LogCross(CrossData);
				Viz->SetCrossData(CrossData);  // ── 传给可视化 ──
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[PbTestActor] Cross 反序列化失败！"));
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("======================================================"));
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 测试结束"));
	UE_LOG(LogTemp, Log, TEXT("======================================================"));
}

void APbTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor::EndPlay] Actor 销毁，EndPlayReason=%d"), static_cast<int32>(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}
