#pragma once

#ifndef _SIMBASE_H_
#include "sim/simBase.h"
#endif

#include <audio/recordStreamSource.h>


#include <RakPeerInterface.h>
#include <NetworkIDObject.h>
#include <NetworkIDManager.h>
#include <RPC3/RPC3.h>
#include <RakVoice.h>

#include <boost/function.hpp>
#include <vector>

#include "User.hpp"
#include "RoomManager.hpp"
#include "Room.hpp"
#include "VOIP.hpp"




namespace EN
{
	class AuthClient;
	class ChatServer;
	class ClientSystem: public SimObject
	{
		typedef SimObject Parent;

	public:
		ClientSystem(void);
		~ClientSystem(void);
		DECLARE_CONOBJECT(ClientSystem);
		bool onAdd();


		S32 Connect(const char* server, S32 port);
		void Tick();
		void Disconnected();
		void Connected();
		void TearDown(bool bReallyDestory=true);

		void Auth(const char* user, const char* pass);

		void AddUser(User* u);
		void RemoveUser(User* u);

		RoomManager* getRoomManager(){return _RoomManager;}

		void SetAudioCapHandle(U32 h)
		{
			_AudioCapHandle = h;
		}

		void SetVOIPRecordMute(bool muted)
		{
			if (_CaptureStreamSource)
				_CaptureStreamSource->mMuted = muted;
		}

		void ResetCaptureDevice()
		{
			auto streamSource = alxFindAudioStreamSource(_AudioCapHandle);
			_CaptureStreamSource = dynamic_cast<RecordStreamSource*>(streamSource);
			alxPlay(_AudioCapHandle);
		}


	private:
		
		bool _init;
		bool _connected;
		
		RakNet::NetworkIDManager networkIDManager;
		RakNet::RakPeerInterface *_NetworkInterface;
		RakNet::RPC3*	_RpcInst;
		VOIP* _VoicePlugin;

		RoomManager* _RoomManager;
		User* _MyUser;
		std::vector<User*> _Users;

		AuthClient* _AuthClient;
		ChatServer* _ChatServer;

		U32 _AudioCapHandle;
		RecordStreamSource* _CaptureStreamSource;
		bool _CaptureMuted;

		void Init();
		void AuthenticateResponse(int retCode,RakNet::NetworkID netID,  RakNet::RPC3 *rpcFromNetwork);

	};


	class AuthClient : public RakNet::NetworkIDObject
	{
	public:
		typedef boost::function<void(int,RakNet::NetworkID,RakNet::RPC3 *)> auth_response_func;

		auth_response_func _functor;
		AuthClient( auth_response_func func )
		{
			_functor = func;
		}

		void AuthenticateResponse(int retCode,RakNet::NetworkID netID,  RakNet::RPC3 *rpcFromNetwork)
		{
			_functor(retCode, netID, rpcFromNetwork);
		}
	};

	class ChatServer : public RakNet::NetworkIDObject
	{
	public:

		User* mMyUser;

		ChatServer(RoomManager* rm)
		{
			mMyUser = NULL;
			_StatusRoom = NULL;
			_RoomManager = rm;
		}
		~ChatServer()
		{
			std::for_each(_Rooms.begin(), _Rooms.end(),
				[&](Room* room)
				{
					room->unregisterObject();
					delete room;
				});

			_StatusRoom = NULL;
		}

		void NewRoomResponse(RakNet::NetworkID nID, RakNet::RakString cName, RakNet::RPC3 *rpcFromNetwork)
		{
			auto room = new Room(rpcFromNetwork, _RoomManager, this);
			room->SetNetworkIDManager(GetNetworkIDManager());
			room->SetNetworkID(nID);
			room->Name = cName.C_String();
			room->registerObject();
			room->Init();

			_Rooms.push_back(room);

			if (room->Name == "Status" && _StatusRoom == NULL)
				_StatusRoom = room;


		}

		void StatusMessageResponse(RakNet::RakString data, RakNet::RPC3* rpcFromNetwork, RakNet::NetworkID fromRoom)
		{
			if (_StatusRoom)
			{
				_StatusRoom->AddChatToWindow( data.C_String());
			}

			if (fromRoom != -1)
			{
				auto room = GetNetworkIDManager()->GET_OBJECT_FROM_ID<Room*>(fromRoom);
				room->AddChatToWindow( data.C_String());
			}

		}

		void GlobalMessageResponse(RakNet::RakString data, RakNet::RPC3* rpcFromNetwork)
		{
			std::for_each(_Rooms.begin(), _Rooms.end(), 
				[&](Room* room)
				{
					room->AddChatToWindow(data.C_String());
				});
		}

		void LeaveRoom(Room * room)
		{
			auto itr = std::find(_Rooms.begin(), _Rooms.end(), room);
			if (itr != _Rooms.end())
				_Rooms.erase(itr);
		}


		Room* GetRoom(std::string name)
		{
			return NULL;
		}

	private:
		std::vector<Room*> _Rooms;
		Room* _StatusRoom;
		RoomManager* _RoomManager;

	};


};

