#include "oxygen/pch.h"

#include "oxygen/network/archipelago/ArchipelagoClient.h"
#include "oxygen/application/Application.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/application/GameProfile.h"
#include <oxygen/helper/JsonHelper.h>
#include "oxygen/extensions/jsonreader/JsonReaderWrapper.h"

#include <lemon/program/ModuleBindingsBuilder.h>

#if defined(PLATFORM_WINDOWS)
	#pragma comment(lib, "ixwebsocket.lib")
	#pragma comment(lib, "mbedcrypto.lib")
	#pragma comment(lib, "mbedx509.lib")
	#pragma comment(lib, "mbedtls.lib")
	#pragma comment(lib, "bcrypt.lib")
#endif

namespace functions
{
	JsonReaderWrapper getSlotDataReader()
	{
		return JsonReaderWrapper(3);
	}

	JsonReaderWrapper getLastPacketReader()
	{
		return JsonReaderWrapper(4);
	}
}

static char serverAddress[512] = "localhost:38281";
static char slotName[512] = "";
static char password[512] = "";
static std::string errorMessage = "";
static std::string socketError = "";

void ArchipelagoClient::start()
{
	mIniting = true;
}

void ArchipelagoClient::init(std::string address, std::string player_name, std::string password)
{
	mPlayerName = player_name;
	AP_Init(address.c_str(), GameProfile::instance().mShortName.c_str(), mPlayerName.c_str(), password.c_str());
	AP_SetItemRecvCallback([](AP_NetworkItem item, bool notify) { ArchipelagoClient::instance().onItemRecv(item, notify); });
	AP_SetItemClearCallback([]() { ArchipelagoClient::instance().mItems.clear(); });
	AP_SetLocationCheckedCallback([](int64_t location) { ArchipelagoClient::instance().onCheckLocation(location); });
	AP_Start();
}

// TODO: main AP Client code runs async, does lemonscript accept that?
void ArchipelagoClient::update(float timeElapsed)
{
	Simulation& sim = Application::instance().getSimulation();
	if (mIniting)
	{
		sim.setRunning(false);
		ImGui::Begin("Connection Input");
		ImGui::InputText("Server address", serverAddress, sizeof(serverAddress), ImGuiInputTextFlags_CharsNoBlank);
		ImGui::InputText("Slot name", slotName, sizeof(slotName));
		ImGui::InputText("Password", password, sizeof(password));
		bool connectClicked = ImGui::Button(mConnecting ? "Connecting..." : "Connect");
		if (connectClicked && !mConnecting)
		{
			if (std::strlen(serverAddress) <= 0)
			{
				errorMessage = "Please enter a server address";
				ImGui::OpenPopup("Error");
			}
			else if (std::strlen(slotName) <= 0)
			{
				errorMessage = "Please enter a slot name";
				ImGui::OpenPopup("Error");
			}
			else
			{
				mConnecting = true;
				mLastConnect = now();
				shutdown();
				init(serverAddress, slotName, password);
			}
		}
		else if (mConnecting)
		{
			bool timeOut = static_cast<unsigned long>(now() - mLastConnect) > 11000;
			if (timeOut)
			{
				socketError = "Connection timed out";
			}

			if (socketError.length() > 0)
			{
				mConnecting = false;
				shutdown();
				errorMessage = "Connection failed: " + socketError;
				socketError = "";
				ImGui::OpenPopup("Error");
				mIniting = false;
			}
		}

		if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("%s", errorMessage.c_str());
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
		return;
	}
	AP_ConnectionStatus state = AP_GetConnectionStatus();
	mConnecting = (state != AP_ConnectionStatus::Disconnected && state != AP_ConnectionStatus::Authenticated);
	if (isConnected() && mIniting)
	{
		mIniting = false;
		mSetupDone = true;
		sim.setRunning(true);
		mSlotDataReader.loadFromString(mWriter.write(ap_slot_data));
		callScriptFunction("Archipelago.OnConnected");
	}
	if (!isConnected() && !mConnecting && mSetupDone)
	{
		callScriptFunction("Archipelago.OnDisconnected");
	}
}

void ArchipelagoClient::shutdown()
{
	//AP_Shutdown();
	mIniting = false;
	mConnecting = false;
	mSetupDone = false;
	mPlayerName.clear();
	mItems.clear();
	mCheckedLocations.clear();
	mProcessedItems = 0;
}


void ArchipelagoClient::registerScriptBindings(lemon::ModuleBindingsBuilder& builder)
{
	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);

	builder.addNativeFunction("Archipelago.start", lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::start), defaultFlags);

	builder.addNativeFunction("Archipelago.shutdown", lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::shutdown), defaultFlags);

	builder.addNativeFunction("Archipelago.isConnected", lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::isConnected), defaultFlags);

	builder.addNativeFunction("Archipelago.sendLocation", lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::checkLocation), defaultFlags)
		.setParameters("id");

	builder.addNativeFunction("Archipelago.getPlayerName",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::getPlayerName), defaultFlags);

	builder.addNativeFunction("Archipelago.getItemCount",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::getItemCount), defaultFlags)
		.setParameters("id");

	builder.addNativeFunction("Archipelago.setDataInt",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::setDataInt), defaultFlags)
		.setParameters("name", "value");

	builder.addNativeFunction("Archipelago.getDataInt",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::getDataInt), defaultFlags)
		.setParameters("name");

	builder.addNativeFunction("Archipelago.setDataFloat",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::setDataFloat), defaultFlags)
		.setParameters("name", "value");

	builder.addNativeFunction("Archipelago.getDataFloat",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::getDataFloat), defaultFlags)
		.setParameters("name");

	builder.addNativeFunction("Archipelago.getSeedName",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::getSeedName), defaultFlags);

	builder.addNativeFunction("Archipelago.isZoneAllowed",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::isZoneAllowed), defaultFlags)
		.setParameters("zone");

	builder.addNativeFunction("Archipelago.isLocationChecked",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::isLocationChecked), defaultFlags)
		.setParameters("id");

	builder.addNativeFunction("Archipelago.isLocationAllowedForChar",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::isLocationAllowedForChar), defaultFlags)
		.setParameters("id", "character");

	builder.addNativeFunction("Archipelago.triggerGoal",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::sendGoal), defaultFlags);

	builder.addNativeFunction("Archipelago.sendDeath",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::sendDeath), defaultFlags);

	builder.addNativeFunction("Archipelago.sendBounce",
		lemon::wrap(ArchipelagoClient::instance(), &ArchipelagoClient::sendBounce), defaultFlags);

	builder.addNativeFunction("Archipelago.getSlotData",
		lemon::wrap(functions::getSlotDataReader), defaultFlags);

	builder.addNativeFunction("Archipelago.getLastPacket",
		lemon::wrap(functions::getLastPacketReader), defaultFlags);
}


bool ArchipelagoClient::isConnected()
{
	return AP_GetConnectionStatus() == AP_ConnectionStatus::Authenticated;
}

void ArchipelagoClient::checkLocation(uint64 location_id)
{
	mCheckedLocations.emplace(location_id);
	AP_SendItem(location_id);
}

void ArchipelagoClient::onCheckLocation(uint64 location_id)
{
	mCheckedLocations.emplace(location_id);
	// Prepare and execute script call
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();

	// Call signature: "void Archipelago.onLocationChecked(u64 location)"
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::UINT_64, location_id);
	codeExec.executeScriptFunction("Archipelago.OnLocationChecked", false, &execData);
}

void ArchipelagoClient::onItemRecv(AP_NetworkItem& item, bool notify)
{
	// Prepare and execute script call
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();
	mProcessedItems++;
	if (notify) // TODO: notify does not work reliably due to being dependent on server state
	{
		// Call signature: "void Archipelago.OnNewItem(string itemName, u64 item)"
		CodeExec::FunctionExecData execData;
		execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
		execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::STRING, runtime.getInternalLemonRuntime().addString(item.itemName));
		execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::UINT_64, item.item);
		codeExec.executeScriptFunction("Archipelago.OnNewItem", false, &execData);
	}
	mItems[item.item] += 1;
	CodeExec::FunctionExecData execData;
	execData.mParams.mReturnType = &lemon::PredefinedDataTypes::VOID;
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::INT_32, mProcessedItems);
	execData.mParams.mParams.emplace_back(lemon::PredefinedDataTypes::BOOL, (item.flags & 0b001) != 0);
	// Call signature: "void Archipelago.OnReceivedItems(int index, bool is_prog)"
	codeExec.executeScriptFunction("Archipelago.OnReceivedItems", false, &execData);
}

void ArchipelagoClient::onBounced(AP_Bounce& bounce)
{
	Json::Value root;
	if (bounce.games != nullptr && !bounce.games->empty())
	{
		for (size_t i = 0; i < bounce.games->size(); i++)
		{
			root["games"].append(bounce.games->at(i));
		}
	}
	if (bounce.slots != nullptr && !bounce.slots->empty())
	{
		for (size_t i = 0; i < bounce.slots->size(); i++)
		{
			root["slots"].append(bounce.slots->at(i));
		}
	}
	if (bounce.tags != nullptr && !bounce.tags->empty())
	{
		for (size_t i = 0; i < bounce.tags->size(); i++)
		{
			root["tags"].append(bounce.tags->at(i));
		}
	}
	Json::Value data;
	mReader.parse(bounce.data, data);
	root["data"] = data;
	mLastPacketReader.loadFromString(mWriter.write(root));
	callScriptFunction("Archipelago.onBounced");
}

bool ArchipelagoClient::isLocationChecked(uint64 location_id)
{
	return mCheckedLocations.find(location_id) != mCheckedLocations.end();
}

void ArchipelagoClient::sendGoal()
{
	AP_StoryComplete();
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
		AP_UpdateTags(tags);
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
		AP_UpdateTags(tags);
	}
}

void ArchipelagoClient::setDataInt(lemon::StringRef name, int64 data)
{
	LogDisplay::instance().setLogDisplay(String("Archipelago.setDataInt is deprecated"), 6.0f);
	ap_slot_data[name.getString().data()] = data;
}

int64 ArchipelagoClient::getDataInt(lemon::StringRef name)
{
	LogDisplay::instance().setLogDisplay(String("Archipelago.getDataInt is deprecated"), 6.0f);
	if (ap_slot_data.isNull() || !ap_slot_data.isMember(name.getString().data()) || ap_slot_data[name.getString().data()].isNull())
	{
		return 0;
	}

	return ap_slot_data[name.getString().data()].asInt64();
}

void ArchipelagoClient::setDataFloat(lemon::StringRef name, float data)
{
	LogDisplay::instance().setLogDisplay(String("Archipelago.setDataFloat is deprecated"), 6.0f);
	ap_slot_data[name.getString().data()] = data;
}

float ArchipelagoClient::getDataFloat(lemon::StringRef name)
{
	LogDisplay::instance().setLogDisplay(String("Archipelago.getDataFloat is deprecated"), 6.0f);
	if (ap_slot_data.isNull() || !ap_slot_data.isMember(name.getString().data()) || ap_slot_data[name.getString().data()].isNull())
	{
		return 0.0;
	}

	return ap_slot_data[name.getString().data()].asFloat();
}

lemon::StringRef ArchipelagoClient::getSeedName()
{
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();
	AP_RoomInfo room_info;
	AP_GetRoomInfo(&room_info);
	return lemon::StringRef(runtime.getInternalLemonRuntime().addString(room_info.seed_name));
}

lemon::StringRef ArchipelagoClient::getPlayerName()
{
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();
	return lemon::StringRef(runtime.getInternalLemonRuntime().addString(mPlayerName));
}

uint64 ArchipelagoClient::getPlayerID()
{
	return AP_GetPlayerID();
}

uint64 ArchipelagoClient::getItemCount(uint64 id)
{
	return mItems[id];
}

bool ArchipelagoClient::isZoneAllowed(lemon::StringRef zone)
{
	LogDisplay::instance().setLogDisplay(String("Archipelago.isZoneAllowed is deprecated, use Archipelago.GetSlotData instead"), 6.0f);
	CodeExec& codeExec = Application::instance().getSimulation().getCodeExec();
	LemonScriptRuntime& runtime = codeExec.getLemonScriptRuntime();
	std::string zone_str = std::string(zone.getString());
	for (size_t i = 0; i < ap_slot_data["ZonesAllowed"].size(); i++)
	{
		if (ap_slot_data["ZonesAllowed"][(int32)i].asString() == zone_str)
			return true;
	}
	return false;
}

bool ArchipelagoClient::isLocationAllowedForChar(uint64 id, uint8 character)
{
	LogDisplay::instance().setLogDisplay(String("Archipelago.isLocationAllowedForChar is deprecated, use Archipelago.GetSlotData instead"), 6.0f);
	auto chars = ap_slot_data["LocationCharWhitelists"][std::to_string(id)];
	for (size_t i = 0; i < chars.size(); i++)
	{
		if (chars.asInt() == character)
			return true;
	}
	return false;
}

void ArchipelagoClient::sendDeath()
{
	LogDisplay::instance().setLogDisplay(String("Archipelago.sendDeath is deprecated, use Archipelago.sendBounce instead"), 6.0f);
	AP_RoomInfo room_info;
	AP_GetRoomInfo(&room_info);
	bool deathlink = std::find(room_info.tags.begin(), room_info.tags.end(), "DeathLink") != room_info.tags.end();
	if (!isConnected() || !deathlink)
		return;
	AP_Bounce b;
	Json::Value v;
	v["time"] = (int64_t)now();
	v["source"] = mPlayerName; // Name and Shame >:D
	b.data = mWriter.write(v);
	b.games = nullptr;
	b.slots = nullptr;
	std::vector<std::string> tags = { std::string("DeathLink") };
	b.tags = &tags;
	AP_SendBounce(b);
}

void ArchipelagoClient::sendBounce(lemon::StringRef bounce)
{
	AP_Bounce b;
	Json::Value v;
	mReader.parse(std::string{ bounce.getString() }, v);
	b.data = mWriter.write(v["data"]);
	if (v.isMember("games"))
	{
		std::vector<std::string> games;
		for (size_t i = 0; i < v["games"].size(); i++)
		{
			games.push_back(v["games"][(int32)i].asString());
		}
		b.games = &games;
	}
	if (v.isMember("slots"))
	{
		std::vector<int64_t> slots;
		for (size_t i = 0; i < v["slots"].size(); i++)
		{
			slots.push_back(v["slots"][(int32)i].asInt());
		}
		b.slots = &slots;
	}
	if (v.isMember("tags"))
	{
		std::vector<std::string> tags;
		for (size_t i = 0; i < v["tags"].size(); i++)
		{
			tags.push_back(v["tags"][(int32)i].asString());
		}
		b.tags = &tags;
	}
	AP_SendBounce(b);
}

// helper function to call event hooks with no args
void ArchipelagoClient::callScriptFunction(std::string functionName)
{
	Application::instance().getSimulation().getCodeExec().getLemonScriptRuntime().callFunctionByName(functionName);
}
