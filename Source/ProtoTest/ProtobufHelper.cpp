// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProtobufHelper.h"
#include "testpb.pb.cc"

pbt::User FProtobufHelper::MakeSampleUser()
{
	pbt::User User;
	User.set_id(1);
	User.set_name("TestUser");
	User.set_email("test@example.com");
	User.set_age(25);
	User.add_tags("developer");
	User.add_tags("unreal");
	User.set_score(98.5);
	return User;
}

bool FProtobufHelper::SerializeUser(const pbt::User& User, TArray<uint8>& OutBytes)
{
	return SerializeToBytes(User, OutBytes);
}

bool FProtobufHelper::ParseUser(const TArray<uint8>& Bytes, pbt::User& OutUser)
{
	return ParseFromBytes(Bytes, OutUser);
}
