// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "testpb.pb.h"

/**
 * Protobuf 读写解析最小封装
 * - SerializeToBytes:  将 protobuf 消息序列化为二进制字节数组
 * - ParseFromBytes:    从二进制字节数组反序列化为 protobuf 消息
 * - SerializeToString: 将 protobuf 消息序列化为 std::string
 * - ParseFromString:   从 std::string 反序列化为 protobuf 消息
 */
class FProtobufHelper
{
public:
	// ── 序列化（写入） ──

	/** 将消息序列化为 TArray<uint8> */
	template<typename TMsg>
	static bool SerializeToBytes(const TMsg& Message, TArray<uint8>& OutBytes)
	{
		const int32 Size = Message.ByteSizeLong();
		if (Size == 0)
		{
			OutBytes.Empty();
			return true;
		}
		OutBytes.SetNumUninitialized(Size);
		return Message.SerializeToArray(OutBytes.GetData(), Size);
	}

	/** 将消息序列化为 FString（二进制内容存入 FString） */
	template<typename TMsg>
	static bool SerializeToString(const TMsg& Message, std::string& OutStr)
	{
		return Message.SerializeToString(&OutStr);
	}

	// ── 反序列化（读取/解析） ──

	/** 从 TArray<uint8> 反序列化 */
	template<typename TMsg>
	static bool ParseFromBytes(const TArray<uint8>& Bytes, TMsg& OutMessage)
	{
		if (Bytes.Num() == 0)
		{
			OutMessage.Clear();
			return true;
		}
		return OutMessage.ParseFromArray(Bytes.GetData(), Bytes.Num());
	}

	/** 从 std::string 反序列化 */
	template<typename TMsg>
	static bool ParseFromString(const std::string& Str, TMsg& OutMessage)
	{
		return OutMessage.ParseFromString(Str);
	}

	// ── 便捷方法：读写 pbt::User ──

	/** 创建一个示例 User 消息 */
	static pbt::User MakeSampleUser();

	/** 将 User 序列化为 TArray<uint8> */
	static bool SerializeUser(const pbt::User& User, TArray<uint8>& OutBytes);

	/** 从 TArray<uint8> 解析 User */
	static bool ParseUser(const TArray<uint8>& Bytes, pbt::User& OutUser);
};
