/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once
#include <ctime>
#include <lemon/program/StringRef.h>
#include "oxygen/extensions/jsonreader/JsonReader.h"
#include "oxygen/network/archipelago/APCpp/Archipelago.h"
#include "oxygen/menu/imgui/ImGuiDefinitions.h"
#if defined SUPPORT_IMGUI
	class ImGuiArchipelago;
#endif

namespace lemon
{
	class ModuleBindingsBuilder;
}

class ArchipelagoClient : public SingleInstance<ArchipelagoClient>
{
public:
	void start();
	void init(std::string address, std::string player_name, std::string password);
	void update(float timeElapsed);
	void shutdown();

	void registerScriptBindings(lemon::ModuleBindingsBuilder& builder);

	bool isConnected();

	void checkLocation(uint64 location_id);
	void onCheckLocation(uint64 location_id);
	void onItemRecv(AP_NetworkItem& item_id, bool notify);
	void onBounced(AP_Bounce& bounce);

	bool isLocationChecked(uint64 location_id);
	bool doesLocationExist(uint64 location_id);

	void sendGoal();

	bool hasTag(lemon::StringRef tag);
	void removeTag(lemon::StringRef tag);
	void addTag(lemon::StringRef tag);

	lemon::StringRef getSeedName();
	lemon::StringRef getPlayerName();
	uint64 getPlayerID();
	uint64 getItemCount(uint64 id);

	JsonReader& getSlotDataReader() { return mSlotDataReader; }
	JsonReader& getLastPacketReader() { return mLastPacketReader; }

	void sendBounce(lemon::StringRef bounce);
private:
	bool mIniting = false;
	bool mSetupDone = false;
	bool mConnecting = false;
	unsigned long mLastConnect = 0;
	static uint64 now()
	{
		return static_cast<uint64>(std::time(nullptr));
	}

	std::string mPlayerName;
	uint64 mProcessedItems = 0;
	bool mNewProg = false;
	std::unordered_map<uint64, uint64> mItems;
	std::set<uint64> mCheckedLocations;
	std::vector<uint64> mPendingLocations;
	std::vector<AP_NetworkItem> mItemData;
	std::vector<Json::Value> mBouncedData;

	Json::Reader mReader;
	Json::FastWriter mWriter;

	JsonReader mSlotDataReader;
	JsonReader mLastPacketReader;

	void callScriptFunction(std::string functionName);
	
	#ifdef SUPPORT_IMGUI
		friend class ImGuiArchipelago;
	#endif
};
