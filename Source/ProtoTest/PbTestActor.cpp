// Copyright Epic Games, Inc. All Rights Reserved.

#include "PbTestActor.h"
#include "ProtobufHelper.h"
#include "driving_data.pb.h"
#include "driving_data.pb.cc"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

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

static const TMap<FString, pbt::ObstacleType> GNameToObstacleType = {
	{ TEXT("OBSTACLE_UNKNOWN"),    pbt::OBSTACLE_UNKNOWN },
	{ TEXT("OBSTACLE_VEHICLE"),    pbt::OBSTACLE_VEHICLE },
	{ TEXT("OBSTACLE_PEDESTRIAN"), pbt::OBSTACLE_PEDESTRIAN },
	{ TEXT("OBSTACLE_CYCLIST"),    pbt::OBSTACLE_CYCLIST },
	{ TEXT("OBSTACLE_TRUCK"),      pbt::OBSTACLE_TRUCK },
	{ TEXT("OBSTACLE_BUS"),        pbt::OBSTACLE_BUS },
	{ TEXT("OBSTACLE_MOTORCYCLE"), pbt::OBSTACLE_MOTORCYCLE },
	{ TEXT("OBSTACLE_ANIMAL"),     pbt::OBSTACLE_ANIMAL },
	{ TEXT("OBSTACLE_CONE"),       pbt::OBSTACLE_CONE },
	{ TEXT("OBSTACLE_GUARDRAIL"),  pbt::OBSTACLE_GUARDRAIL },
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

static const TMap<FString, pbt::LaneLineType> GNameToLaneLineType = {
	{ TEXT("LANE_UNKNOWN"),       pbt::LANE_UNKNOWN },
	{ TEXT("LANE_SOLID"),         pbt::LANE_SOLID },
	{ TEXT("LANE_DASHED"),        pbt::LANE_DASHED },
	{ TEXT("LANE_DOUBLE_SOLID"),  pbt::LANE_DOUBLE_SOLID },
	{ TEXT("LANE_DOUBLE_DASHED"), pbt::LANE_DOUBLE_DASHED },
	{ TEXT("LANE_SOLID_DASHED"),  pbt::LANE_SOLID_DASHED },
	{ TEXT("LANE_CURB"),          pbt::LANE_CURB },
	{ TEXT("LANE_ROAD_EDGE"),     pbt::LANE_ROAD_EDGE },
};

static const TMap<pbt::LaneLineColor, const TCHAR*> GLaneLineColorNames = {
	{ pbt::LANE_COLOR_UNKNOWN, TEXT("LANE_COLOR_UNKNOWN") },
	{ pbt::LANE_COLOR_WHITE,   TEXT("LANE_COLOR_WHITE") },
	{ pbt::LANE_COLOR_YELLOW,  TEXT("LANE_COLOR_YELLOW") },
	{ pbt::LANE_COLOR_BLUE,    TEXT("LANE_COLOR_BLUE") },
	{ pbt::LANE_COLOR_GREEN,   TEXT("LANE_COLOR_GREEN") },
};

static const TMap<FString, pbt::LaneLineColor> GNameToLaneLineColor = {
	{ TEXT("LANE_COLOR_UNKNOWN"), pbt::LANE_COLOR_UNKNOWN },
	{ TEXT("LANE_COLOR_WHITE"),   pbt::LANE_COLOR_WHITE },
	{ TEXT("LANE_COLOR_YELLOW"),  pbt::LANE_COLOR_YELLOW },
	{ TEXT("LANE_COLOR_BLUE"),    pbt::LANE_COLOR_BLUE },
	{ TEXT("LANE_COLOR_GREEN"),   pbt::LANE_COLOR_GREEN },
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

static const TMap<FString, pbt::GroundSignType> GNameToGroundSignType = {
	{ TEXT("SIGN_UNKNOWN"),              pbt::SIGN_UNKNOWN },
	{ TEXT("SIGN_ARROW_STRAIGHT"),       pbt::SIGN_ARROW_STRAIGHT },
	{ TEXT("SIGN_ARROW_LEFT"),           pbt::SIGN_ARROW_LEFT },
	{ TEXT("SIGN_ARROW_RIGHT"),          pbt::SIGN_ARROW_RIGHT },
	{ TEXT("SIGN_ARROW_UTURN"),          pbt::SIGN_ARROW_UTURN },
	{ TEXT("SIGN_ARROW_STRAIGHT_LEFT"),  pbt::SIGN_ARROW_STRAIGHT_LEFT },
	{ TEXT("SIGN_ARROW_STRAIGHT_RIGHT"), pbt::SIGN_ARROW_STRAIGHT_RIGHT },
	{ TEXT("SIGN_ARROW_LEFT_RIGHT"),     pbt::SIGN_ARROW_LEFT_RIGHT },
	{ TEXT("SIGN_CROSSWALK"),            pbt::SIGN_CROSSWALK },
	{ TEXT("SIGN_STOP_LINE"),            pbt::SIGN_STOP_LINE },
	{ TEXT("SIGN_SPEED_BUMP"),           pbt::SIGN_SPEED_BUMP },
	{ TEXT("SIGN_YIELD_LINE"),           pbt::SIGN_YIELD_LINE },
	{ TEXT("SIGN_PARKING_SPACE"),        pbt::SIGN_PARKING_SPACE },
	{ TEXT("SIGN_ZEBRA_CROSSING"),       pbt::SIGN_ZEBRA_CROSSING },
};

// ── JSON → Protobuf 转换辅助函数 ──

static pbt::Vec3 JsonToVec3(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::Vec3 V;
	V.set_x(Obj->GetNumberField(TEXT("x")));
	V.set_y(Obj->GetNumberField(TEXT("y")));
	V.set_z(Obj->GetNumberField(TEXT("z")));
	return V;
}

static pbt::EgoVehicle JsonToEgoVehicle(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::EgoVehicle V;
	V.set_id(Obj->GetIntegerField(TEXT("id")));
	*V.mutable_position() = JsonToVec3(Obj->GetObjectField(TEXT("position")));
	*V.mutable_velocity() = JsonToVec3(Obj->GetObjectField(TEXT("velocity")));
	*V.mutable_rotation() = JsonToVec3(Obj->GetObjectField(TEXT("rotation")));
	V.set_timestamp(static_cast<int64>(Obj->GetNumberField(TEXT("timestamp"))));
	return V;
}

static pbt::Obstacle JsonToObstacle(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::Obstacle O;
	O.set_id(Obj->GetIntegerField(TEXT("id")));
	O.set_type(GNameToObstacleType.FindRef(Obj->GetStringField(TEXT("type"))));
	*O.mutable_position() = JsonToVec3(Obj->GetObjectField(TEXT("position")));
	*O.mutable_velocity() = JsonToVec3(Obj->GetObjectField(TEXT("velocity")));
	*O.mutable_size() = JsonToVec3(Obj->GetObjectField(TEXT("size")));
	O.set_heading(Obj->GetNumberField(TEXT("heading")));
	return O;
}

static pbt::Ego JsonToEgo(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::Ego E;
	*E.mutable_vehicle() = JsonToEgoVehicle(Obj->GetObjectField(TEXT("vehicle")));
	for (const TSharedPtr<FJsonValue>& Val : Obj->GetArrayField(TEXT("obstacles")))
	{
		*E.add_obstacles() = JsonToObstacle(Val->AsObject());
	}
	return E;
}

static pbt::LaneLine JsonToLaneLine(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::LaneLine L;
	L.set_id(Obj->GetIntegerField(TEXT("id")));
	L.set_type(GNameToLaneLineType.FindRef(Obj->GetStringField(TEXT("type"))));
	L.set_color(GNameToLaneLineColor.FindRef(Obj->GetStringField(TEXT("color"))));
	for (const TSharedPtr<FJsonValue>& Val : Obj->GetArrayField(TEXT("points")))
	{
		*L.add_points() = JsonToVec3(Val->AsObject());
	}
	L.set_confidence(Obj->GetNumberField(TEXT("confidence")));
	L.set_width(Obj->GetNumberField(TEXT("width")));
	return L;
}

static pbt::GroundSign JsonToGroundSign(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::GroundSign G;
	G.set_id(Obj->GetIntegerField(TEXT("id")));
	G.set_type(GNameToGroundSignType.FindRef(Obj->GetStringField(TEXT("type"))));
	for (const TSharedPtr<FJsonValue>& Val : Obj->GetArrayField(TEXT("polygon")))
	{
		*G.add_polygon() = JsonToVec3(Val->AsObject());
	}
	return G;
}

static pbt::DrivableArea JsonToDrivableArea(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::DrivableArea D;
	D.set_id(Obj->GetIntegerField(TEXT("id")));
	for (const TSharedPtr<FJsonValue>& Val : Obj->GetArrayField(TEXT("boundary")))
	{
		*D.add_boundary() = JsonToVec3(Val->AsObject());
	}
	D.set_confidence(Obj->GetNumberField(TEXT("confidence")));
	D.set_speed_limit(Obj->GetNumberField(TEXT("speed_limit")));
	return D;
}

static pbt::Cross JsonToCross(const TSharedPtr<FJsonObject>& Obj)
{
	pbt::Cross C;
	for (const TSharedPtr<FJsonValue>& Val : Obj->GetArrayField(TEXT("lane_lines")))
	{
		*C.add_lane_lines() = JsonToLaneLine(Val->AsObject());
	}
	for (const TSharedPtr<FJsonValue>& Val : Obj->GetArrayField(TEXT("ground_signs")))
	{
		*C.add_ground_signs() = JsonToGroundSign(Val->AsObject());
	}
	for (const TSharedPtr<FJsonValue>& Val : Obj->GetArrayField(TEXT("drivable_areas")))
	{
		*C.add_drivable_areas() = JsonToDrivableArea(Val->AsObject());
	}
	return C;
}

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

// ── Actor 生命周期 ──

APbTestActor::APbTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APbTestActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("======================================================"));
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 智能驾驶数据 Protobuf 测试开始"));
	UE_LOG(LogTemp, Log, TEXT("======================================================"));

	// ── 1. 读取 JSON 文件 ──
	const FString JsonPath = FPaths::ProjectDir() / TEXT("Source/ProtoFiles/driving_data_sample.json");
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *JsonPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[PbTestActor] 无法读取 JSON 文件: %s"), *JsonPath);
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] JSON 文件读取成功，大小: %d 字符"), JsonString.Len());

	// ── 2. 解析 JSON ──
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[PbTestActor] JSON 解析失败！"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] JSON 解析成功"));

	// ── 3. 构建 DrivingDataFrame ──
	pbt::DrivingDataFrame DataFrame;
	DataFrame.set_frame_id(static_cast<int64>(RootObject->GetNumberField(TEXT("frame_id"))));
	DataFrame.set_timestamp(static_cast<int64>(RootObject->GetNumberField(TEXT("timestamp"))));
	*DataFrame.mutable_ego() = JsonToEgo(RootObject->GetObjectField(TEXT("ego")));
	*DataFrame.mutable_cross() = JsonToCross(RootObject->GetObjectField(TEXT("cross")));

	// ── 4. 序列化测试 ──
	TArray<uint8> SerializedBytes;
	if (FProtobufHelper::SerializeToBytes(DataFrame, SerializedBytes))
	{
		UE_LOG(LogTemp, Log, TEXT("[PbTestActor] DrivingDataFrame 序列化成功，大小: %d 字节"), SerializedBytes.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PbTestActor] DrivingDataFrame 序列化失败！"));
	}

	// ── 5. 打印所有数据 ──
	UE_LOG(LogTemp, Log, TEXT(""));
	UE_LOG(LogTemp, Log, TEXT("========== DrivingDataFrame 数据打印 =========="));
	UE_LOG(LogTemp, Log, TEXT("frame_id = %lld"), DataFrame.frame_id());
	UE_LOG(LogTemp, Log, TEXT("timestamp = %lld"), DataFrame.timestamp());

	const pbt::Ego& EgoData = DataFrame.ego();
	LogEgoVehicle(EgoData.vehicle());
	UE_LOG(LogTemp, Log, TEXT("--- Obstacles (%d) ---"), EgoData.obstacles_size());
	for (int32 i = 0; i < EgoData.obstacles_size(); ++i)
	{
		LogObstacle(EgoData.obstacles(i));
	}

	const pbt::Cross& CrossData = DataFrame.cross();
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

	UE_LOG(LogTemp, Log, TEXT("======================================================"));
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 智能驾驶数据 Protobuf 测试结束"));
	UE_LOG(LogTemp, Log, TEXT("======================================================"));
}

void APbTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor::EndPlay] Actor 销毁，EndPlayReason=%d"), static_cast<int32>(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}
