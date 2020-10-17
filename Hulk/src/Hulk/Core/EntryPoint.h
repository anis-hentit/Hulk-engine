#pragma once
#include "../../../Core.h"

#ifdef HK_PLATFORM_WINDOWS

extern void Hulk::CreateApplication();

int main(int argc,char** argv)
{
	
	
	//entry point
	Hulk::CreateApplication();
	

	return 0;
}

#endif