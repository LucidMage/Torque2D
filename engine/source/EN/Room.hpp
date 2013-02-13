#pragma once

#ifndef _SIMBASE_H_
#include "sim/ScriptObject.h"
#endif

#include <RakNetTypes.h>
#include <NetworkIDObject.h>
#include <NetworkIDManager.h>
#include <RPC3/RPC3.h>

#include <string>

namespace EN {

#define JOIN_ROOM_TEMPLATE "[%1%] <spush><color:00adeb>%2%<spop><spush><color:ff003c> has joined<spop>"
#define LEAVE_ROOM_TEMPLATE "[%1%] <spush><color:00adeb>%2%<spop><spush><color:ff003c> has left<spop>"
#define CHAT_ROOM_TEMPLATE "[%1%] <spush><color:00adeb>%2%<spop>: %3%"

class RoomManager;
class ChatServer;
class Room : public ScriptObject, public RakNet::NetworkIDObject
{
	typedef SimObject Parent;

public:
	Room(){}
	Room(RakNet::RPC3* rpc, RoomManager* parent, ChatServer* server);
	~Room(void);
	DECLARE_CONOBJECT(Room);

	void Deserialize(RakNet::BitStream& stream);
	void Init();
	void NotifyJoinResponse(RakNet::RakString who, RakNet::NetworkID nID);
	void NotifyLeaveResponse(RakNet::RakString who, RakNet::NetworkID nID);
	void NotifyChatResponse(RakNet::RakString who, RakNet::NetworkID nID, RakNet::RakString data, RakNet::RPC3 *rpcFromNetwork);
	void DataResponse(Room* same, RakNet::RPC3 *rpcFromNetwork);

	void AddChatToWindow(const char* data);
	void SendChat(const char* data);
	void LeaveRoom();

	std::string Name;
	std::string Topic;

	std::vector<std::string> Users;
	std::vector<unsigned short> Levels;

private:
	RakNet::RPC3* _Rpc;
	RoomManager* _Parent;
	ChatServer* _chatServer;

	void ExecCommand(const char* data);


};

}
