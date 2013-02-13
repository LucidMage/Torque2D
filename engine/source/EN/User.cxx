#include "User.hpp"
#include "ClientSystem.hpp"

namespace EN {

IMPLEMENT_CONOBJECT(User);

User::User(RakNet::RPC3* inst, ClientSystem* parent)
{
	_Parent = parent;
	_rpcInst = inst;
}


User::~User(void)
{
	if (_Parent)
	{
		this->unregisterObject();
		_Parent->RemoveUser(this);
	}
}

void User::Init()
{
	this->registerObject();
	RakNet::RPC3::CallExplicitParameters cep(1, RakNet::UNASSIGNED_SYSTEM_ADDRESS, MEDIUM_PRIORITY);
	_rpcInst->CallCPP("&AuthServer::UserDataRequest", &cep, this );

	_Parent->AddUser(this);
}


void User::Deserialize(RakNet::BitStream& stream)
{
	RakNet::RakString tmpU;
	stream.Read(tmpU); _username = tmpU.C_String();
	stream.Read(_bSetFullSend);

	if (_bSetFullSend)
	{
		USHORT count = 0;
		stream.Read(count);
		for(int i=0; i < count; i++)
		{
			RakNet::RakString roomName;
			stream.Read(roomName);
			_AutoJoinRooms.push_back(roomName.C_String());
		}

		stream.Read(count);
		for(int i=0; i < count; i++)
		{
			stream.Read(tmpU);
			_FriendsList.push_back( std::pair<int,std::string>( 0, tmpU.C_String()));
		}
	}

}

void User::JoinSavedChanList()
{
	auto rm = _Parent->getRoomManager();

	std::for_each(_AutoJoinRooms.begin(), _AutoJoinRooms.end(), 
		[&](std::string& room_name)
		{
			rm->JoinRoom(room_name.c_str());
		});
}

//This is called to pass down the User object. We don't really do anything with this right now, since it just fills in the user data.
void User::UserDataResponse(User* u,  RakNet::RPC3 *rpcFromNetwork)
{
	Con::executef(this, 2, "OnData");
}


////CONSOLE//////
ConsoleMethod( User, JoinSavedChanList, void, 2,2, "JoinSavedChanList() - Joins all the channels in the saved list for this user.")
{
	object->JoinSavedChanList();
}

}