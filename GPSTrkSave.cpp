// GPSTrkSave.cpp : WinMain ‚ÌŽÀ‘•


#include "stdafx.h"
#include "resource.h"


class CGPSTrkSaveModule : public CAtlExeModuleT< CGPSTrkSaveModule >
{
public :
};

CGPSTrkSaveModule _AtlModule;



//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
                                LPTSTR /*lpCmdLine*/, int nShowCmd)
{
    return _AtlModule.WinMain(nShowCmd);
}

