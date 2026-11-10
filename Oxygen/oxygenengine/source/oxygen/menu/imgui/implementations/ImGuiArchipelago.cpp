#include "oxygen/pch.h"
#include "oxygen/menu/imgui/implementations/ImGuiArchipelago.h"

#if defined(SUPPORT_IMGUI)
#include "oxygen/network/archipelago/ArchipelagoClient.h"

ImGuiArchipelago::ImGuiArchipelago()
{
	mShowConnectInput = true;
}

// TODO: figure out a way to make enter presses not trigger in-game inputs
void ImGuiArchipelago::buildImGuiContent()
{
	ImGui::SetWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(300.0f, 200.0f), ImGuiCond_FirstUseEver);
	auto& client = ArchipelagoClient::instance();
	if (mShowConnectInput && client.mIniting)
	{
		mShowChat = true;
		ImGui::Begin("Archipelago Connection Input", &mShowConnectInput);
		ImGui::InputText("Server address", mServerAddress, sizeof(mServerAddress), ImGuiInputTextFlags_CharsNoBlank);
		ImGui::InputText("Slot name", mSlotName, sizeof(mSlotName));
		ImGui::InputText("Password", mPassword, sizeof(mPassword));
		bool connectClicked = ImGui::Button(client.mConnecting ? "Connecting..." : "Connect") || ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsWindowFocused();
		if (connectClicked && !client.mConnecting)
		{
			if (std::strlen(mServerAddress) <= 0)
			{
				mErrorMessage = "Please enter a server address";
				ImGui::OpenPopup("Error");
			}
			else if (std::strlen(mSlotName) <= 0)
			{
				mErrorMessage = "Please enter a slot name";
				ImGui::OpenPopup("Error");
			}
			else
			{
				client.mLastConnect = client.now();
				client.shutdown();
				client.mConnecting = true;
				client.init(mServerAddress, mSlotName, mPassword); // the client will handle a successful connection from here
			}
		}
		else if (client.mConnecting)
		{
			bool timeOut = static_cast<unsigned long>(client.now() - client.mLastConnect) > 11;
			if (timeOut)
			{
				mSocketError = "Connection timed out";
			}

			if (AP_GetConnectionStatus() == AP_ConnectionStatus::ConnectionRefused)
			{
				mSocketError = "Connection refused by the AP server. Most likely invalid slot name or password";
			}

			if (mSocketError.length() > 0)
			{
				client.mConnecting = false;
				client.shutdown();
				AP_Shutdown();
				mErrorMessage = "Connection failed: " + mSocketError;
				printf(mErrorMessage.c_str());
				mSocketError = "";
				ImGui::OpenPopup("Error");
			}
		}

		if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("%s", mErrorMessage.c_str());
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		
		ImGui::End();
	}
	else if (mShowChat && client.isConnected())
	{
		static std::vector<std::string> msgs;
		bool shouldScroll = false;
		while (AP_IsMessagePending())
		{
			AP_Message* msg = AP_GetLatestMessage();
			std::ostringstream stream;
			if (msg->type == AP_MessageType::ItemRecv)
			{
				AP_ItemRecvMessage* msg2 = static_cast<AP_ItemRecvMessage*>(msg);
				msg->text = "[NEW ITEM] " + std::string("Received ") + msg2->item.itemName;
				if (msg2->item.playerName != msg2->sendPlayer)
				{
					msg->text += std::string(" from ") + msg2->sendPlayer;
				}
				
				msg->text += std::string(" (") + msg2->item.locationName + std::string(")");
			}
			else if (msg->type == AP_MessageType::ItemSend)
			{
				AP_ItemSendMessage* msg2 = static_cast<AP_ItemSendMessage*>(msg);
				if (msg2->recvPlayer == msg2->item.playerName)
				{
					msg->text = msg2->recvPlayer + std::string(" found their ") + msg2->item.itemName;
				}
				else
				{
					msg->text = msg2->recvPlayer + std::string(" received ") + msg2->item.itemName;
					msg->text += std::string(" from ") + msg2->item.playerName;
				}
				
				msg->text += std::string(" (") + msg2->item.locationName + std::string(")");
			}
			else if (msg->type == AP_MessageType::Hint)
			{
				stream << "[HINT] ";
			}

			stream << msg->text;
			msgs.push_back(stream.str());
			shouldScroll = true;
			if (msgs.size() > 100)
			{
				msgs.erase(msgs.begin());
			}

			AP_ClearLatestMessage();
		}
		
		ImGui::Begin("Archipelago Text Chat", &mShowChat, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::BeginChild("TextField", ImVec2(0, -60));
		for (int i = 0; i < msgs.size(); i++)
		{
			ImGui::TextWrapped("%s", msgs[i].c_str());
		}
		
		if (shouldScroll && !ImGui::IsItemActive() && !ImGui::IsWindowFocused()) 
		{
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
		ImGui::Separator();
		ImGui::InputText("Input", mTextInput, sizeof(mTextInput));
		if ((ImGui::Button("Send") || ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsWindowFocused()) && std::strlen(mTextInput) > 0)
		{
			AP_Say(std::string(mTextInput));
			mTextInput[0] = '\0';
		}
		
		ImGui::End();
	}
}
#endif
