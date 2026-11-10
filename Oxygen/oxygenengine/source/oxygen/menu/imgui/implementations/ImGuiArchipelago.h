#pragma once
#include "oxygen/menu/imgui/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)
#include "oxygen/menu/imgui/ImGuiContentProvider.h"
#include "oxygen/network/archipelago/ArchipelagoClient.h"

class ImGuiArchipelago : public ImGuiContentProvider
{
public:
	static const uint64 PROVIDER_KEY = rmx::constMurmur2_64("ImGuiArchipelago");
	ImGuiArchipelago();
	virtual void buildImGuiContent() override;
	bool mShowConnectInput = false;
private:
	char mServerAddress[512] = "localhost:38281";
	char mSlotName[512] = "";
	char mPassword[512] = "";
	char mTextInput[1024] = "";
	std::string mErrorMessage;
	std::string mSocketError;
	bool mShowChat = true;
};
#endif
