#include "ClientSystem.hpp"
#include "Room.hpp"

#include <strsafe.h>
#include <console/consoleTypes.h>
#include <console/consoleInternal.h>
#include <audio/audio.h>
#include <MessageIdentifiers.h>

#include <algorithm>

namespace RakNet
{
	RakNet::BitStream& operator<<(RakNet::BitStream& out, EN::User& in)
	{
		return out;
	}
	RakNet::BitStream& operator>>(RakNet::BitStream& in, EN::User& out)
	{
		out.Deserialize(in);
		return in;
	}

	RakNet::BitStream& operator<<(RakNet::BitStream& out, EN::Room& in)
	{
		return out;
	}
	RakNet::BitStream& operator>>(RakNet::BitStream& in, EN::Room& out)
	{
		out.Deserialize(in);
		return in;
	}
}

namespace EN {

IMPLEMENT_CONOBJECT(ClientSystem);

unsigned char GetPacketIdentifier(RakNet::Packet *p)
{
	if (p==0)
		return 255;

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		RakAssert(p->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char) p->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char) p->data[0];
}

ClientSystem::ClientSystem(void) : SimObject(LinkSuperClassName | LinkClassName)
{
	_connected = false;
	_init = false;
	_VoicePlugin = NULL;
	_NetworkInterface = NULL;
	_AuthClient = NULL;
	_RpcInst = NULL;
	_MyUser = NULL;
	_RoomManager = NULL;
	_ChatServer = NULL;
	_CaptureStreamSource = NULL;
}


ClientSystem::~ClientSystem(void)
{
	TearDown();
}


bool ClientSystem::onAdd()
{
	if(!Parent::onAdd())
		return false;

	const char *name = getName();

	if(name && name[0] && getClassRep())
	{
		Namespace *parent = getClassRep()->getNameSpace();
		Con::linkNamespaces(parent->mName, name);
		mNameSpace = Con::lookupNamespace(name);

	}
	return true;
}


void ClientSystem::Init()
{
	if (_init) 
		return;

	TearDown();

	_init = true;

	_RpcInst = new RakNet::RPC3();
	_RpcInst->SetNetworkIDManager(&networkIDManager);

	_AuthClient = new AuthClient( boost::bind(&ClientSystem::AuthenticateResponse, this, _1, _2, _3) );
	_AuthClient->SetNetworkIDManager(&networkIDManager);
	_AuthClient->SetNetworkID(1);

	_RoomManager = new RoomManager(_RpcInst);

	_ChatServer = new ChatServer(_RoomManager);
	_ChatServer->SetNetworkIDManager(&networkIDManager);
	_ChatServer->SetNetworkID(2);

	RPC3_REGISTER_FUNCTION(_RpcInst,&AuthClient::AuthenticateResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst,&User::UserDataResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst, &ChatServer::NewRoomResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst, &ChatServer::StatusMessageResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst, &ChatServer::GlobalMessageResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst, &Room::NotifyJoinResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst, &Room::NotifyLeaveResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst, &Room::NotifyChatResponse);
	RPC3_REGISTER_FUNCTION(_RpcInst, &Room::DataResponse);


	_NetworkInterface = RakNet::RakPeerInterface::GetInstance();

	RakNet::SocketDescriptor socketDescriptor(0,0);
	socketDescriptor.socketFamily=AF_INET;
	_NetworkInterface->Startup(1,&socketDescriptor, 1);
	_NetworkInterface->SetOccasionalPing(true);
	_NetworkInterface->AttachPlugin(_RpcInst);

	_VoicePlugin = new VOIP(SAMPLE_RATE);

	auto streamSource = alxFindAudioStreamSource(_AudioCapHandle);
	_CaptureStreamSource = dynamic_cast<RecordStreamSource*>(streamSource);
}

void ClientSystem::TearDown(bool bReallyDestroy)
{
	_connected = false;
	_init = false;
	_CaptureMuted = true;

	if (_AuthClient)
	{
		delete _AuthClient;
		_AuthClient = NULL;
	}

	if (_MyUser)
	{
		delete _MyUser;
		_MyUser = NULL;
	}

	_Users.clear();

	if (_RoomManager)
	{
		delete _RoomManager;
		_RoomManager = NULL;
	}

	if (_ChatServer)
	{
		delete _ChatServer;
		_ChatServer = NULL;
	}

	if (_VoicePlugin)
	{
		delete _VoicePlugin;
		_VoicePlugin = NULL;
	}

	_CaptureStreamSource = NULL;

	if (_NetworkInterface) 
		_NetworkInterface->Shutdown(300, 0, HIGH_PRIORITY);


	if (bReallyDestroy)
	{

		if (_NetworkInterface) 
		{
			RakNet::AddressOrGUID addr(RakNet::UNASSIGNED_RAKNET_GUID);
			auto conState = _NetworkInterface->GetConnectionState(addr);
			if ( conState == RakNet::IS_CONNECTED ||
				 conState == RakNet::IS_CONNECTING|| 
				 conState == RakNet::IS_PENDING)
				_NetworkInterface->Shutdown(300, 0, HIGH_PRIORITY);
			RakNet::RakPeerInterface::DestroyInstance(_NetworkInterface);
			_NetworkInterface = NULL;
		}

		if (_RpcInst)
		{
			delete _RpcInst;
			_RpcInst = NULL;
		}


	}
}

S32 ClientSystem::Connect(const char* server, S32 port)
{
	Init();

	RakNet::ConnectionAttemptResult car = _NetworkInterface->Connect(server, port, NULL, 0);
	RakAssert(car==RakNet::CONNECTION_ATTEMPT_STARTED);
	bool b = ( car == RakNet::CONNECTION_ATTEMPT_STARTED);
	_connected = b;

	Tick();

	return b;

}

void ClientSystem::AddUser(User* u)
{
	_Users.push_back(u);
}
void ClientSystem::RemoveUser(User* u)
{
	auto itr = std::find(_Users.begin(), _Users.end(), u);
	if (itr != _Users.end())
		_Users.erase(itr);
}

void ClientSystem::Auth(const char* user, const char* pass)
{
	if (!_connected) return;

	RakNet::RPC3::CallExplicitParameters cep(1, RakNet::UNASSIGNED_SYSTEM_ADDRESS,HIGH_PRIORITY);
	RakNet::RakString uName = user;
	RakNet::RakString uPass = pass;
	_RpcInst->CallCPP("&AuthServer::AuthenticateRequest", &cep, uName, uPass );
}

void ClientSystem::Connected()
{
	Con::executef(this, 2, "onConnected", Con::getIntArg(getId()));
}

void ClientSystem::Disconnected()
{

	{
		int argc = 2;
		char* argv[2];
		argv[1] = new char[10];
		argv[0] = new char[5];
		sprintf(argv[0],"TearDown");
		sprintf(argv[1], "%d", this->getId());

		SimConsoleEvent* sce = new SimConsoleEvent(argc, (const char**)&argv, true);
		Sim::postEvent( this,sce, Sim::getCurrentTime() + 50);
	}

	Con::executef(this, 1, "OnDisconnected");

}

void ClientSystem::AuthenticateResponse(int retCode,RakNet::NetworkID netID, RakNet::RPC3 *rpcFromNetwork)
{

	if (retCode == 101 & netID != 0)
	{
		if(_MyUser)
		{
			delete _MyUser;
			_ChatServer->mMyUser = NULL;
		}

		_MyUser = new User(_RpcInst, this);
		_MyUser->SetNetworkIDManager(&networkIDManager);
		_MyUser->SetNetworkID(netID);
		_MyUser->Init();
		_ChatServer->mMyUser = _MyUser;

		RakNet::BitStream out;
		out.Write((unsigned char)ID_RAKVOICE_OPEN_CHANNEL_REQUEST);
		out.Write((int32_t)SAMPLE_RATE);


		_NetworkInterface->Send(&out, HIGH_PRIORITY, RELIABLE_ORDERED,0, _NetworkInterface->GetGUIDFromIndex(0),false );

	}

	char buf1[4], buf2[50];
	dSprintf(buf1, sizeof(buf1), "%d", retCode);
	dSprintf(buf2, sizeof(buf2), "%d", netID);
	Con::executef(this, 4, "OnAuthenticationResponse", buf1, buf2);
}


//SCHEDULED EVENT
void ClientSystem::Tick()
{
	if (_NetworkInterface == NULL || !_connected) return;

	RakNet::Packet* p = NULL;
	p=_NetworkInterface->Receive();
	for (; p && _NetworkInterface && _connected; )
	{
		unsigned char packetIdentifier = GetPacketIdentifier(p);
		// Check if this is a network message packet
		switch (packetIdentifier)
		{
		case ID_DISCONNECTION_NOTIFICATION:
			{
				// Connection lost normally
				Disconnected();
			}
			break;
		case ID_ALREADY_CONNECTED:
			// Connection lost normally
			Disconnected();
			break;
		case ID_INCOMPATIBLE_PROTOCOL_VERSION:
			Disconnected(); 
			break;
		case ID_CONNECTION_BANNED: // Banned from this server
			Disconnected();
			break;			
		case ID_CONNECTION_ATTEMPT_FAILED:
			Disconnected();
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			// Sorry, the server is full.  I don't do anything here but
			// A real app should tell the user
			Disconnected();
			break;

		case ID_INVALID_PASSWORD:
			Disconnected();
			break;

		case ID_CONNECTION_LOST:
			// Couldn't deliver a reliable packet - i.e. the other system was abnormally
			// terminated
			Disconnected();
			break;

		case ID_CONNECTION_REQUEST_ACCEPTED:
			// This tells the client they have connected
			Connected();
			break;
		case ID_RAKVOICE_OPEN_CHANNEL_REPLY:
		case ID_RAKVOICE_OPEN_CHANNEL_REQUEST:
			{
				alxPlay(_AudioCapHandle);
				break;
			}
		case ID_RAKVOICE_DATA:
			{
				int frameSize = _VoicePlugin->GetFrameSize();
				int byteFrameSize= ( frameSize ) * 2;
				byte* soundBuffer = new byte[ FRAMES_PER_BUFFER * 2 ];

				int frameCount = FRAMES_PER_BUFFER / frameSize;

				RakNet::BitStream in(p->data+1, p->length-1,false);

				for(int i =0; i < frameCount; i++)
				{
					unsigned short bSize;
					in.Read(bSize);

					byte* inBuffer = new byte[bSize];
					in.ReadBits(inBuffer, bSize * 8);
					byte* outBuffer = new byte[byteFrameSize];

					_VoicePlugin->Decode(inBuffer, outBuffer, frameSize);
					delete[] inBuffer;

					memcpy(soundBuffer+ (i * byteFrameSize), outBuffer, byteFrameSize);
					delete[] outBuffer;
				}

				if (_CaptureStreamSource != NULL)
					_CaptureStreamSource->mSoundBuffer.push_back(soundBuffer);

				break;
			}
		default:
			// It's a client, so just show the message
			break;
		}
		
		if(_NetworkInterface) 
		{
			_NetworkInterface->DeallocatePacket(p);
			p=_NetworkInterface->Receive();
		}
	}

	if (_CaptureStreamSource)
	{
		//update VOIP
		int frameSize = _VoicePlugin->GetFrameSize();
		int byteFrameSize= ( frameSize ) * 2;
		std::for_each(_CaptureStreamSource->mHoldingBuffer.begin(), _CaptureStreamSource->mHoldingBuffer.end(),
			[&](byte* data)
		{
			bool bStopSend = false;
			std::list< std::pair<byte*, int> > sendData;
			for(int x =0; x < (FRAMES_PER_BUFFER / frameSize) ; ++x )
			{
				byte* encodeBuffer = new byte[byteFrameSize];
				byte *outBuffer = new byte[byteFrameSize];
				memcpy(encodeBuffer, data + (byteFrameSize * x), byteFrameSize);
				int bWritten = _VoicePlugin->Encode(encodeBuffer, outBuffer, frameSize);

				sendData.push_back( std::pair<byte*, int>(outBuffer, bWritten) );
				delete[] encodeBuffer;

				if (bWritten == 0)
				{
					//don't send this packet.
					bStopSend = true;
				}
			}

			RakNet::BitStream out;
			out.Write((unsigned char)ID_RAKVOICE_DATA);
			std::for_each(sendData.begin(), sendData.end(), 
				[&]( std::pair<byte*, int> kvp)
			{
				out.Write((unsigned short)kvp.second);
				out.WriteBits(kvp.first,  kvp.second*8);
				delete[] kvp.first;
			});

			if (!bStopSend)
				_NetworkInterface->Send(&out, HIGH_PRIORITY, UNRELIABLE,0, _NetworkInterface->GetGUIDFromIndex(0),false);
			delete[] data;			
		});
		_CaptureStreamSource->mHoldingBuffer.clear();

	}

	if (!_connected)
	{
		return;
	}

	//re-post Tick.
	{
		int argc = 2;
		char* argv[2];
		argv[1] = new char[10];
		argv[0] = new char[5];
		sprintf(argv[0],"Tick");
		sprintf(argv[1], "%d", this->getId());

		SimConsoleEvent* sce = new SimConsoleEvent(argc, (const char**)&argv, true);
		Sim::postEvent( this,sce, Sim::getCurrentTime() + 32);
	}
}

////CONSOLE//////
ConsoleMethod( ClientSystem, Tick, void, 2,2, "Tick() - Runs a network step, though you don't have to worry about this.")
{
	object->Tick();
}

ConsoleMethod( ClientSystem, Connect, S32, 4,4, "Connect(<ip>,<port>)")
{
	return object->Connect( argv[2], dAtoi(argv[3]) );
}

ConsoleMethod( ClientSystem, Disconnect, void, 2,2, "Disconnect()")
{
	 object->Disconnected();
}

ConsoleMethod( ClientSystem, TearDown, void, 2,2, "Teardown()")
{
	object->TearDown(true);
}


ConsoleMethod( ClientSystem, Authenticate, void, 4,4, "Authenticate(<username>,<password>)")
{
	return object->Auth( argv[2], argv[3] );
}

ConsoleMethod( ClientSystem, SetCaptureHandle, void, 3,3, "SetCapturehandle(AUDIOHANDLE)")
{
	U32 handle = dAtoi(argv[2]);

	object->SetAudioCapHandle(handle);
}

ConsoleMethod( ClientSystem, ResetCaptureDevice, void, 2,2, "ResetCaptureDevice()")
{

	object->ResetCaptureDevice();
}



ConsoleMethod( ClientSystem, SetVOIPRecordMute, void, 3,3, "SetVOIPRecordMute(bool)")
{
	bool muted = dAtoi(argv[2]);

	object->SetVOIPRecordMute(muted);
}

}
