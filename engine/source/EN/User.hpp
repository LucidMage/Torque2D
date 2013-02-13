#pragma once

#ifndef _SIMBASE_H_
#include "sim/simBase.h"
#endif
#ifndef _SCRIPT_OBJECT_H_
#include "sim/scriptObject.h"
#endif

#include <RakNetTypes.h>
#include <NetworkIDObject.h>
#include <NetworkIDManager.h>
#include <RPC3/RPC3.h>


#include <string>
#include <vector>


namespace EN {

class ClientSystem;
class User: public SimObject, public RakNet::NetworkIDObject
{
	typedef SimObject Parent;

public:
	User(){_Parent= NULL;}
	User(RakNet::RPC3* inst, ClientSystem* parent);
	~User(void);
	DECLARE_CONOBJECT(User);

	void Init();
	void Deserialize(RakNet::BitStream& stream);

	void UserDataResponse(User* u,  RakNet::RPC3 *rpcFromNetwork);
	void JoinSavedChanList();

private:

	ClientSystem* _Parent;
	RakNet::RPC3* _rpcInst;
	std::string _username;
	bool _bSetFullSend;

	std::vector<std::string > _AutoJoinRooms;
	std::vector< std::pair<int,std::string> > _FriendsList;
};

}
