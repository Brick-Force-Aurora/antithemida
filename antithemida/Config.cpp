#include "Include.h"

bool Config::Parse()
{
    Utils::Print("antithemida.dll: Attempting to parse config file.");

    auto configPath = std::filesystem::path("Config\\Config.json");

    try
    {
        if (std::filesystem::exists(configPath) && std::filesystem::is_regular_file(configPath))
        {
            std::ifstream stream(configPath.string());
            std::ostringstream oss;
            oss << stream.rdbuf();

            if (stream.is_open())
            {
                std::string str = oss.str();
                Utils::Print("antithemida.dll: Read config file.");

                Utils::Print(str);
                if (nlohmann::json::accept(str))
                {
                    auto json = nlohmann::json::parse(str);

                    if (json.contains(killThemidaKey))
                        killThemida = json[killThemidaKey];
                    else
                        Utils::Print(std::string("antithemida.dll: Config does not contain key '") + killThemidaKey + "'.");

                    if (json.contains(unityDebugOptionKey))
                        unityDebugOption = Utils::clampEnum((UnityDebugOption)json[unityDebugOptionKey], UnityDebugOption::unityDebugMax);
                    else
                        Utils::Print(std::string("antithemida.dll: Config does not contain key '") + unityDebugOptionKey + "'.");

                    if (json.contains(unityDebugIPKey))
                    {
                        std::string ip = json[unityDebugIPKey];

                        if (Utils::IsValidIPWithPort(ip))
                            unityDebugIP = ip;
                        else
                            Utils::Print(std::string("antithemida.dll: Invalid IP:Port format in key '") + unityDebugIPKey + "' (" + ip + ").");
                    }
                    else
                        Utils::Print(std::string("antithemida.dll: Config does not contain key '") + unityDebugIPKey + "'.");

                    return true;
                }

                else
                    Utils::Print("antithemida.dll: Config file isn't valid json.");
            }
            else
                Utils::Print("antithemida.dll: Can't read config file.");
        }

        else
            Utils::Print("antithemida.dll: Config file not found.");
    }
    catch (std::exception& e)
    {
        Utils::Print((std::string("antithemida.dll: ParseConfig exception - ") + e.what()));
    }

    return false;
}
