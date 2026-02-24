// dllmain.cpp : Defines the entry point for the DLL application.
#include "Include.h"

Config config;

typedef BOOL(WINAPI* tIsDebuggerPresent)();
tIsDebuggerPresent oIsDebuggerPresent = nullptr;

typedef int(__cdecl* tMono_utf8_from_external)(char* Str);
tMono_utf8_from_external oMono_utf8_from_external = nullptr;

std::uintptr_t pMono_jit_parse_options = 0;
std::uintptr_t pMono_debug_init = 0;

bool mono_debug_init(int a1)
{
    if (pMono_debug_init != 0)
    {
        Utils::CallFunction<void>(pMono_debug_init, a1);
        return true;
    }
    else
        Utils::Print("antithemida.dll: pMono_debug_init was nullptr.");

    return false;
}

bool mono_jit_parse_options(int argc, const char** argv)
{
    if (pMono_jit_parse_options != 0)
    {
        Utils::CallFunction<void>(pMono_jit_parse_options, argc, argv);
        return true;
    }
    else
        Utils::Print("antithemida.dll: pMono_jit_parse_options was nullptr.");

    return false;
}

BOOL WINAPI hIsDebuggerPresent()
{
    auto pPeb = (PPEB)__readfsdword(0x30);
    pPeb->BeingDebugged = 0;
    pPeb->NtGlobalFlag = 0;

    Utils::Print(std::string("antithemida.dll: IsDebuggerPresent - " + Utils::GetStacktrace()));

    if ((std::uintptr_t)_ReturnAddress() == 0xFB0806)
    {
        Utils::Print("antithemida.dll: Terminating Themida thread.");
        TerminateThread(GetCurrentThread(), 0);
    }

    return 0;
}

std::string argvDebugOff = "";
std::string argvDebugDefer = "--debugger-agent=transport=dt_socket,server=y,address=%ip,defer=y";
std::string argvDebugSuspend = "--debugger-agent=transport=dt_socket,server=y,address=%ip,suspend=y";
std::string argvDebug = "";

int __cdecl hMono_utf8_from_external(char* Str)
{
    Utils::Print((std::string("antithemida.dll: mono_utf8_from_external - ") + Str + " - " + Utils::GetStacktrace()));

    switch (config.unityDebugOption)
    {
    case UnityDebugOption::unityDebugOff:
    default:
        argvDebug = argvDebugOff;
        Utils::Print("antithemida.dll: Skipping mono_debug_init.");
        return oMono_utf8_from_external(Str);
    case UnityDebugOption::unityDebugOn:
        argvDebug = std::regex_replace(argvDebugDefer, std::regex("%ip"), config.unityDebugIP);
        Utils::Print("antithemida.dll: Passing jit options for unityDebugOn. (" + argvDebug +  ")");
        break;
    case UnityDebugOption::unityDebugWait:
        argvDebug = std::regex_replace(argvDebugSuspend, std::regex("%ip"), config.unityDebugIP);
        Utils::Print("antithemida.dll: Passing jit options for unityDebugWait. (" + argvDebug + ")");
        break;
    }

    Utils::Print("antithemida.dll: Attempting mono_debug_init inside mono_utf8_from_external.");

    const char* jitArgv[] = { argvDebug.c_str() };

    if (mono_jit_parse_options(1, jitArgv) && mono_debug_init(1))
        Utils::Print("antithemida.dll: mono_debug_init success.");
    else
        Utils::Print("antithemida.dll: mono_debug_init failed.");

    auto result = oMono_utf8_from_external(Str);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    UnhookFast(oMono_utf8_from_external, &hMono_utf8_from_external);
    DetourTransactionCommit();

    Utils::Print("antithemida.dll: Unhooked mono.mono_utf8_from_external.");

    return result;
}

void Initialize()
{
    Utils::Print("antithemida.dll: Init.");

    if (config.Parse())
        Utils::Print("antithemida.dll: Parsed config file.");
    else
        Utils::Print("antithemida.dll: Could not parse config file.");

    if (config.killThemida)
    {
        Utils::Print("antithemida.dll: Attempting to kill Themida.");
        Utils::Print("antithemida.dll: Attempting to restore ntdll.DbgBreakPoint and ntdll.DbgUiRemoteBreakin.");

        auto hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll != nullptr)
        {
            auto pDbgBreakPoint = GetProcAddress(hNtdll, "DbgBreakPoint");
            if (pDbgBreakPoint != nullptr)
            {
                Utils::PatchMemory((std::uintptr_t)pDbgBreakPoint, { 0xCC });
                Utils::Print("antithemida.dll: Restored ntdll.DbgBreakPoint.");
            }
            else
                Utils::Print("antithemida.dll: pDbgBreakPoint was nullptr.");

            auto pDbgUiRemoteBreakin = GetProcAddress(hNtdll, "DbgUiRemoteBreakin");
            if (pDbgUiRemoteBreakin != nullptr)
            {
                Utils::PatchMemory((std::uintptr_t)pDbgUiRemoteBreakin, { 0x6A, 0x08, 0x68, 0x38, 0xE8 });
                Utils::Print("antithemida.dll: Restored ntdll.DbgUiRemoteBreakin.");
            }
            else
                Utils::Print("antithemida.dll: pDbgUiRemoteBreakin was nullptr.");
        }
        else
            Utils::Print("antithemida.dll: hNtdll was nullptr.");

        Utils::Print("antithemida.dll: Attempting to hook kernelbase.IsDebuggerPresent.");
        auto hKernelbase = GetModuleHandleA("kernelbase.dll");
        if (hKernelbase != nullptr)
        {
            auto pIsDebuggerPresent = GetProcAddress(hKernelbase, "IsDebuggerPresent");
            if (pIsDebuggerPresent != nullptr)
                oIsDebuggerPresent = (tIsDebuggerPresent)pIsDebuggerPresent;
            else
                Utils::Print("antithemida.dll: pIsDebuggerPresent was nullptr.");
        }
        else
            Utils::Print("antithemida.dll: hKernelbase was nullptr.");
    }
    else
        Utils::Print("antithemida.dll: Skipping Themida patches.");

    if (config.unityDebugOption != UnityDebugOption::unityDebugOff)
    {
        Utils::Print("antithemida.dll: Attempting to enable Unity debugging.");
        Utils::Print("antithemida.dll: Attempting to hook mono.mono_utf8_from_external.");
        auto hMono = GetModuleHandleA("mono.dll");
        if (hMono != nullptr)
        {
            pMono_jit_parse_options = (std::uintptr_t)GetProcAddress(hMono, "mono_jit_parse_options");
            pMono_debug_init = (std::uintptr_t)GetProcAddress(hMono, "mono_debug_init");
            auto pMono_utf8_from_external = GetProcAddress(hMono, "mono_utf8_from_external");
            if (pMono_utf8_from_external != nullptr)
                oMono_utf8_from_external = (tMono_utf8_from_external)pMono_utf8_from_external;
            else
                Utils::Print("antithemida.dll: pMono_utf8_from_external was nullptr.");
        }
        else
            Utils::Print("antithemida.dll: hMono was nullptr.");
    }
    else
        Utils::Print("antithemida.dll: Skipping Unity debug.");

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (oIsDebuggerPresent != nullptr)
    {
        HookFast(oIsDebuggerPresent, &hIsDebuggerPresent);
        Utils::Print("antithemida.dll: Hooked kernelbase.IsDebuggerPresent.");
    }

    if (oMono_utf8_from_external != nullptr)
    {
        HookFast(oMono_utf8_from_external, &hMono_utf8_from_external);
        Utils::Print("antithemida.dll: Hooked mono.mono_utf8_from_external.");
    }

    DetourTransactionCommit();
}

void Shutdown()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (oIsDebuggerPresent != nullptr)
    {
        UnhookFast(oIsDebuggerPresent, &hIsDebuggerPresent);
        Utils::Print("antithemida.dll: Unhooked kernelbase.IsDebuggerPresent.");
    }

    DetourTransactionCommit();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        Initialize();
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
    {
        Shutdown();
        break;
    }
    }
    return TRUE;
}

