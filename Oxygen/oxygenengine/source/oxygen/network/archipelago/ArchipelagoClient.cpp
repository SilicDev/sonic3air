#include "oxygen/pch.h"
#include "oxygen/network/archipelago/ArchipelagoClient.h"
#include "oxygen/application/Application.h"
#include "oxygen/network/archipelago/APCpp/Archipelago.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/application/GameProfile.h"
#include <oxygen/helper/JsonHelper.h>

Json::Reader client_reader;

enum VariantType : uint8
{
	NONE,
	INT,
	FLOAT,
	STRING,
	ARRAY,
	INT64_ARRAY,
	STRING_ARRAY, // basically the same as INT64_ARRAY
	DICTIONARY,
};

#pragma pack(1)
struct Variant
{
	VariantType type;
	union {
		uint8 u8;
		int8 s8;
		uint16 u16;
		int16 s16;
		uint32 u32;
		int32 s32;
		uint64 u64;
		int64 s64;
		uint64 hash;
		uint64 arr_elems;
		uint64 dict_elems;
	};
};
#pragma pack()

void ArchipelagoClient::init_lemon(lemon::StringRef address, lemon::StringRef player_name, lemon::StringRef password)
{
	init(std::string(address.getString()), std::string(player_name.getString()), std::string(password.getString()));
}

void ArchipelagoClient::init(std::string address, std::string player_name, std::string password)
{
	mPlayerName = player_name;
	AP_Init(address.c_str(), GameProfile::instance().mShortName.c_str(), mPlayerName.c_str(), password.c_str());
	AP_SetLocationCheckedCallback([](int64_t location) { ArchipelagoClient::instance().onCheckLocation(location); });
	for (auto& key : mSlotDataIntKeys)
	{
		AP_RegisterSlotDataIntCallback(key, [key](uint64 value) { ArchipelagoClient::instance().OnIntSlotData(key, value); });
	}
	for (auto& key : mSlotDataStringKeys)
	{
		AP_RegisterSlotDataRawCallback(key, [key](std::string value) { ArchipelagoClient::instance().OnStringSlotData(key, value); });
	}
	AP_Start();
	mSetupDone = true;
}

void ArchipelagoClient::update(float timeElapsed)
{
}

void ArchipelagoClient::shutdown()
{
	AP_Shutdown();
	mSetupDone = false;
}

bool ArchipelagoClient::isConnected()
{
	return AP_GetConnectionStatus() == AP_ConnectionStatus::Authenticated;
}

void ArchipelagoClient::checkLocation(uint64 location_id)
{
	AP_SendItem(location_id);
}

void ArchipelagoClient::onCheckLocation(uint64 location_id)
{
	// Prepare and execute script call
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();

	// Call signature: "void Archipelago.onLocationChecked(u64 location)"
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::UINT_64, location_id);
	codeExec.executeScriptFunction("Archipelago.onLocationChecked", false, &execData);
}

void ArchipelagoClient::onItemRecv(uint64 item_id, bool notify)
{
	// Prepare and execute script call
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();

	// Call signature: "void Archipelago.onItemRecv(u64 item)"
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::UINT_64, item_id);
	codeExec.executeScriptFunction("Archipelago.onItemRecv", false, &execData);
}

void ArchipelagoClient::OnIntSlotData(std::string key, uint64 value)
{
	// Prepare and execute script call
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();

	// Call signature: "void Archipelago.onSlotData(string key, u64 value)"
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::STRING, runtime.getInternalLemonRuntime().addString(key));
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::UINT_64, value);
	codeExec.executeScriptFunction("Archipelago.onSlotData", false, &execData);
}

void ArchipelagoClient::OnStringSlotData(std::string key, std::string value)
{
	// Prepare and execute script call
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();

	Json::Value json;
	client_reader.parse(value, json);

	// Call signature: "void Archipelago.onSlotData(string key, string value)"
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::STRING, runtime.getInternalLemonRuntime().addString(key));
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::STRING, runtime.getInternalLemonRuntime().addString(json.asString()));
	codeExec.executeScriptFunction("Archipelago.onSlotData", false, &execData);
}

bool ArchipelagoClient::hasTag(lemon::StringRef tag)
{
	std::string tag_str = std::string{ tag.getString() };
	AP_RoomInfo room_info;
	AP_GetRoomInfo(&room_info);
	return std::find(room_info.tags.begin(), room_info.tags.end(), tag_str) != room_info.tags.end();
}

void ArchipelagoClient::removeTag(lemon::StringRef tag)
{
	std::string tag_str = std::string{ tag.getString() };
	AP_RoomInfo room_info;
	AP_GetRoomInfo(&room_info);
	std::vector<std::string> tags = room_info.tags;
	auto& itr = std::find(tags.begin(), tags.end(), tag_str);
	if (itr != tags.end())
	{
		tags.erase(itr);
		AP_SetTags(tags);
	}
}

void ArchipelagoClient::addTag(lemon::StringRef tag)
{
	std::string tag_str = std::string{ tag.getString() };
	AP_RoomInfo room_info;
	AP_GetRoomInfo(&room_info);
	std::vector<std::string> tags = room_info.tags;
	auto& itr = std::find(tags.begin(), tags.end(), tag_str);
	if (itr == tags.end())
	{
		tags.emplace_back(tag_str);
		AP_SetTags(tags);
	}
}

void ArchipelagoClient::sendGoal()
{
	AP_StoryComplete();
}

uint64 ArchipelagoClient::getPlayerID()
{
	return AP_GetPlayerID();
}

void ArchipelagoClient::addIntSlotDataKey(lemon::StringRef key)
{
	mSlotDataIntKeys.insert(std::string{ key.getString() });
}

void ArchipelagoClient::addStringSlotDataKey(lemon::StringRef key)
{
	mSlotDataStringKeys.insert(std::string{ key.getString() });
}

void ArchipelagoClient::clearSlotDataKeys()
{
	mSlotDataIntKeys.clear();
	mSlotDataStringKeys.clear();
}
