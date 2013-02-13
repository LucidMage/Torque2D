#include "Room.hpp"

#include "RoomManager.hpp"
#include "ClientSystem.hpp"

#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time.hpp>
#include <locale>
#include <algorithm>

namespace EN {

IMPLEMENT_CONOBJECT(Room);

Room::Room(RakNet::RPC3* rpc, RoomManager* parent, ChatServer* server)
{
	_Parent = parent;
	_chatServer = server;
	_Rpc = rpc;
}


Room::~Room(void)
{
}

void Room::Deserialize(RakNet::BitStream& stream)
{
	RakNet::RakString tmp;

	stream.Read(tmp); Name = tmp.C_String();
	stream.Read(tmp); Topic = tmp.C_String();

	unsigned short count = 0;
	stream.Read(count);

	for(unsigned short i=0; i < count; i++)
	{
		unsigned short level = 0;
		stream.Read(tmp); Users.push_back( tmp.C_String());
		stream.Read(level); Levels.push_back(level);
	}
}

void Room::Init()
{
	RakNet::RPC3::CallExplicitParameters cep(1, RakNet::UNASSIGNED_SYSTEM_ADDRESS, MEDIUM_PRIORITY);
	_Rpc->CallCPP("&ChatServer::RoomDataRequest", &cep, this );
}

void Room::ExecCommand(const char* data)
{
	//get the first word after the slash.

	int len = dStrlen(data);
	int commandPosStart = 0;
	std::string commandStr;
	std::string strData = data;
	std::string args;
	for(int i = 0; i < len; i++)
	{
		char c = data[i];

		if (c == ' ')
		{
			commandPosStart = i+1;
			commandStr = strData.substr(1, i-1);
			break;
		}
	}
	if (commandStr == "")
	{
		commandStr = strData.substr(1);
		args = "";
	}
	else
	{
		args = strData.substr(commandPosStart);
	}

	std::transform(commandStr.begin(), commandStr.end(), commandStr.begin(), ::tolower);

	if (commandStr == "join")
	{
		_Parent->JoinRoom(args.c_str());
	}
	else if (commandStr == "leave")
	{
		LeaveRoom();
	}

}

void Room::AddChatToWindow(const char* data)
{
	auto dSize = dStrlen(data)+1;
	auto buf1 = Con::getArgBuffer(dSize);
	dStrncpy(buf1, data, dSize-1);
	buf1[dSize-1] = 0;

	Con::executef((SimObject*)this, 3, "OnAddToChatWindow", buf1);
}

void Room::SendChat(const char* data)
{
	if (data == NULL || dStrlen(data) == 0) return;

	//check if this is a command or general chat.

// 	if (data[0] == '/')
// 	{
// 		ExecCommand(data);
// 		return;
// 	}

	RakNet::RPC3::CallExplicitParameters cep(GetNetworkID(), RakNet::UNASSIGNED_SYSTEM_ADDRESS,LOW_PRIORITY);
	_Rpc->CallCPP("&Room::ChatMessageRequest", &cep, RakNet::RakString(data) );
}

void Room::LeaveRoom()
{
	_Parent->LeaveRoom(this);
	_chatServer->LeaveRoom(this);
	Con::executef(this,2, "OnLeftRoom");
}

void Room::DataResponse(Room* same, RakNet::RPC3 *rpcFromNetwork)
{
	//this room should now be filled in with a topic and user list. notify the UI object to update.

	Con::executef(this, 3, "OnUpdateTopic", Topic.c_str());

	int idx = 0;
	std::for_each(Users.begin(), Users.end(),
		[&](std::string& username)
		{
			auto buf = Con::getIntArg(Levels[idx++]);
			Con::executef(this,4, "OnAddUser", username.c_str(), buf);
		});

}

void Room::NotifyJoinResponse(RakNet::RakString who, RakNet::NetworkID nID)
{	
	boost::posix_time::time_facet* facet = new boost::posix_time::time_facet("%I:%M");	
	std::stringstream date_stream;
	date_stream.imbue(std::locale(date_stream.getloc(), facet));

	auto local_time = boost::posix_time::second_clock::local_time();
	date_stream << local_time;

	AddChatToWindow( (boost::format(JOIN_ROOM_TEMPLATE) % date_stream.str() % who).str().c_str() );

	auto buf = Con::getIntArg(0);
	Con::executef(this,4, "OnAddUser", who.C_String(), buf);

}

void Room::NotifyLeaveResponse(RakNet::RakString who, RakNet::NetworkID nID)
{	
	if (this->_chatServer->mMyUser->GetNetworkID() == nID)
	{
		//we are leaving this room.
		LeaveRoom();
		return;
	}

	boost::posix_time::time_facet* facet = new boost::posix_time::time_facet("%I:%M");	
	std::stringstream date_stream;
	date_stream.imbue(std::locale(date_stream.getloc(), facet));

	auto local_time = boost::posix_time::second_clock::local_time();
	date_stream << local_time;

	AddChatToWindow( (boost::format(LEAVE_ROOM_TEMPLATE) % date_stream.str() % who).str().c_str() );

	auto buf = Con::getIntArg(0);
	Con::executef(this,4, "OnRemoveUser", who.C_String(), buf);

}


void Room::NotifyChatResponse(RakNet::RakString who, RakNet::NetworkID nID, RakNet::RakString data, RakNet::RPC3 *rpcFromNetwork)
{
	boost::posix_time::time_facet* facet = new boost::posix_time::time_facet("%I:%M");	
	std::stringstream date_stream;
	date_stream.imbue(std::locale(date_stream.getloc(), facet));

	auto local_time = boost::posix_time::second_clock::local_time();
	date_stream << local_time;

	AddChatToWindow( (boost::format(CHAT_ROOM_TEMPLATE) % date_stream.str() % who.C_String() % data.C_String()).str().c_str() );
}

////CONSOLE//////
ConsoleMethod( Room, getName, const char*, 2,2, "getName() - Returns the name of the room")
{
	char *buf = Con::getReturnBuffer(1024);
	dSprintf(buf, 1024, "%s", object->Name.c_str());
	return buf;
}

ConsoleMethod( Room, sendChat, void, 3,3, "sendChat(<data>) - sends data to everyone in the room")
{
	object->SendChat(argv[2]);
}

ConsoleMethod( Room, Leave, void, 2,2, "Leave() - leaves the room")
{
	object->LeaveRoom();
}
}


