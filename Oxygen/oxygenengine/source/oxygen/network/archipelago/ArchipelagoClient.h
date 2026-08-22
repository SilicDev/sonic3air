#pragma once

#define IXWEBSOCKET_USE_ZLIB

#include <lemon/program/StringRef.h>

class ArchipelagoClient : public SingleInstance<ArchipelagoClient>
{
public:
	void init_lemon(lemon::StringRef address, lemon::StringRef player_name, lemon::StringRef password);
	void init(std::string address, std::string player_name, std::string password);
	void update(float timeElapsed);
	void shutdown();

	bool isConnected();

	void checkLocation(uint64 location_id);
	void onCheckLocation(uint64 location_id);
	void onItemRecv(uint64 item_id, bool notify);

	void OnIntSlotData(std::string key, uint64 value);
	void OnStringSlotData(std::string key, std::string value);

	bool hasTag(lemon::StringRef tag);
	void removeTag(lemon::StringRef tag);
	void addTag(lemon::StringRef tag);

	void sendGoal();

	uint64 getPlayerID();
	void addIntSlotDataKey(lemon::StringRef key);
	void addStringSlotDataKey(lemon::StringRef key);
	void clearSlotDataKeys();

private:
	bool mSetupDone = false;

	std::string mPlayerName;

	std::set<std::string> mSlotDataIntKeys;
	std::set<std::string> mSlotDataStringKeys;
};

