#include "RoomManager.hpp"

namespace EN {

RoomManager::RoomManager(RakNet::RPC3* rpc)
{
	_RpcInst = rpc;
}


RoomManager::~RoomManager(void)
{
}

void RoomManager::JoinRoom(const char* room_name)
{
	RakNet::RPC3::CallExplicitParameters cep(2, RakNet::UNASSIGNED_SYSTEM_ADDRESS,LOW_PRIORITY);
	_RpcInst->CallCPP("&ChatServer::JoinRoomRequest", &cep, RakNet::RakString(room_name) );
}

void RoomManager::LeaveRoom(Room* room)
{
	RakNet::RPC3::CallExplicitParameters cep(2, RakNet::UNASSIGNED_SYSTEM_ADDRESS,LOW_PRIORITY);
	_RpcInst->CallCPP("&ChatServer::LeaveRoomRequest", &cep, RakNet::RakString(room->Name.c_str()) );
}

}
