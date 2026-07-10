// Copyright Epic Games, Inc. All Rights Reserved.

#include "PbTestActor.h"
#include "ProtobufHelper.h"

APbTestActor::APbTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APbTestActor::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[PbTestActor::BeginPlay] ===== Protobuf 生命周期测试开始 ====="));

	// 1. 创建示例 User 消息
	pbt::User OriginalUser = FProtobufHelper::MakeSampleUser();
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 原始数据: id=%lld, name=%s, email=%s, age=%d, score=%.2f"),
		OriginalUser.id(),
		UTF8_TO_TCHAR(OriginalUser.name().c_str()),
		UTF8_TO_TCHAR(OriginalUser.email().c_str()),
		OriginalUser.age(),
		OriginalUser.score());

	// 2. 序列化为 TArray<uint8>
	TArray<uint8> SerializedBytes;
	if (FProtobufHelper::SerializeUser(OriginalUser, SerializedBytes))
	{
		UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 序列化成功，大小: %d 字节"), SerializedBytes.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PbTestActor] 序列化失败！"));
		return;
	}

	// 3. 从 TArray<uint8> 反序列化
	pbt::User ParsedUser;
	if (FProtobufHelper::ParseUser(SerializedBytes, ParsedUser))
	{
		UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 反序列化成功: id=%lld, name=%s, email=%s, age=%d, score=%.2f"),
			ParsedUser.id(),
			UTF8_TO_TCHAR(ParsedUser.name().c_str()),
			UTF8_TO_TCHAR(ParsedUser.email().c_str()),
			ParsedUser.age(),
			ParsedUser.score());

		// 4. 打印 tags
		FString TagsStr;
		for (int32 i = 0; i < ParsedUser.tags_size(); ++i)
		{
			if (i > 0) TagsStr += TEXT(", ");
			TagsStr += UTF8_TO_TCHAR(ParsedUser.tags(i).c_str());
		}
		UE_LOG(LogTemp, Log, TEXT("[PbTestActor] Tags: [%s]"), *TagsStr);

		// 5. 比对原始和解析结果
		if (OriginalUser.id() == ParsedUser.id() &&
			OriginalUser.name() == ParsedUser.name() &&
			OriginalUser.email() == ParsedUser.email() &&
			OriginalUser.age() == ParsedUser.age() &&
			FMath::IsNearlyEqual(OriginalUser.score(), ParsedUser.score()))
		{
			UE_LOG(LogTemp, Log, TEXT("[PbTestActor] 数据一致性校验通过！"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PbTestActor] 数据一致性校验不匹配！"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PbTestActor] 反序列化失败！"));
	}

	UE_LOG(LogTemp, Log, TEXT("[PbTestActor::BeginPlay] ===== Protobuf 生命周期测试结束 ====="));
}

void APbTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Log, TEXT("[PbTestActor::EndPlay] Actor 销毁，EndPlayReason=%d"), static_cast<int32>(EndPlayReason));
	Super::EndPlay(EndPlayReason);
}
