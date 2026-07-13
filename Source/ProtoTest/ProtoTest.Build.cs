// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ProtoTest : ModuleRules
{
	public ProtoTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// ── Protobuf 集成 ──
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "..", "ThirdParty", "Protobuf");
		string ProtoIncludePath = Path.Combine(ThirdPartyPath, "include");
		string ProtoLibPath = Path.Combine(ThirdPartyPath, "lib", "Win64", "x64");

		// 添加 protobuf 头文件搜索路径
		PublicIncludePaths.Add(ProtoIncludePath);

		// 添加生成的 .pb.h 文件搜索路径
		string ProtoFilesPath = Path.Combine(ModuleDirectory, "..", "ProtoFiles");
		PublicIncludePaths.Add(ProtoFilesPath);

		// 添加库搜索路径 & 链接 protobuf-lite
		PublicSystemLibraryPaths.Add(ProtoLibPath);
		PublicSystemLibraries.Add("libprotobuf-lite.lib");

		// 禁用 RTTI（protobuf 编译要求）
		PrivateDefinitions.Add("GOOGLE_PROTOBUF_NO_RTTI=1");

		// 修复头文件版本不一致导致的未定义宏
		PrivateDefinitions.Add("GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE=0");

	}
}
