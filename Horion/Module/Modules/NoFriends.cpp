#include "NoFriends.h"

NoFriends::NoFriends() : IModule(0x0, Category::EXPLOITS, "Ignores friend list check")
{
}

NoFriends::~NoFriends()
{
}

const char* NoFriends::getModuleName()
{
	return ("NoFriends");
}

