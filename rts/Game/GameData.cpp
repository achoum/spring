#include "GameData.h"

#include <assert.h>

#include "BaseNetProtocol.h"
#include "Net/RawPacket.h"
#include "Net/PackPacket.h"

using namespace netcode;

GameData::GameData()
{
	mapChecksum = 0;
	modChecksum = 0;
}

GameData::GameData(UnpackPacket* packet)
{
	packet->Reset();
	unsigned char ID;
	unsigned short length;
	*packet >> ID;
	assert(ID == NETMSG_GAMEDATA);
	*packet >> length;
	*packet >> script;
	*packet >> map;
	*packet >> mapChecksum;
	*packet >> mod;
	*packet >> modChecksum;
}

const netcode::RawPacket* GameData::Pack() const
{
	unsigned short size = 3 + 2*sizeof(unsigned) + map.size() + mod.size() + script.size() + 3;
	PackPacket* buffer = new PackPacket(size);
	*buffer << (unsigned char)NETMSG_GAMEDATA;
	*buffer << size;
	*buffer << script;
	*buffer << map;
	*buffer << mapChecksum;
	*buffer << mod;
	*buffer << modChecksum;
	return buffer;
}

void GameData::SetScript(const std::string& newScript)
{
	script = newScript;
}

void GameData::SetMap(const std::string& newMap, const unsigned checksum)
{
	map = newMap;
	mapChecksum = checksum;
}

void GameData::SetMod(const std::string& newMod, const unsigned checksum)
{
	mod = newMod;
	modChecksum = checksum;
}