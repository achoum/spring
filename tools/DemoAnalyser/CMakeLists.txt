ADD_DEFINITIONS(-DDEDICATED)

INCLUDE_DIRECTORIES(/usr/include/SDL/)
INCLUDE_DIRECTORIES(../../rts/ ../../rts/Game ../../rts/lib/7zip ../../rts/lib/lua/include ../../rts/System)


AUX_SOURCE_DIRECTORY(../../rts/System/FileSystem/ fsfiles)
SET(build_files ${fsfiles} ${platformfiles}
../../rts/Game/GameSetup
../../rts/Game/PlayerBase
../../rts/System/TdfParser
../../rts/System/ConfigHandler
../../rts/System/Net/RawPacket
../../rts/System/LogOutput
../../rts/System/DemoReader
../../rts/System/Demo
../../rts/System/UnsyncedRNG
../../rts/Game/GameVersion
../../rts/Lua/LuaParser
../../rts/Lua/LuaUtils
../../rts/Map/MapParser
../../rts/Rendering/Textures/TAPalette
../../rts/Lua/LuaIO)

ADD_EXECUTABLE(analyser EXCLUDE_FROM_ALL main ${build_files})
TARGET_LINK_LIBRARIES(analyser SDL hpiutil2 7zip lua minizip boost_regex boost_thread)