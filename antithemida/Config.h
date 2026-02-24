#pragma once

enum UnityDebugOption : int
{
    unityDebugOff = 0,
    unityDebugOn,
    unityDebugWait,
    unityDebugMax
};

class Config
{
public:
	bool Parse();

    bool killThemida = false;
    UnityDebugOption unityDebugOption = unityDebugOff;
    std::string unityDebugIP = "127.0.0.1:56000";

private:
    static constexpr auto killThemidaKey = "kill_themida";
    static constexpr auto unityDebugOptionKey = "unity_debug_option";
    static constexpr auto unityDebugIPKey = "unity_debug_ip";
};