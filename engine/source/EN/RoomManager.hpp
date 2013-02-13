#pragma once

#include "Room.hpp"

#include <RakNetTypes.h>
#include <NetworkIDObject.h>
#include <NetworkIDManager.h>
#include <RPC3/RPC3.h>


namespace EN {

class RoomManager
{
public:
	RoomManager(RakNet::RPC3* rpc);
	~RoomManager(void);

	RakNet::RPC3* _RpcInst;

	void JoinRoom(const char* room_name);
	void LeaveRoom(Room* room);
};

}
