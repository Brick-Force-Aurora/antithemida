#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <TlHelp32.h>
#include <Psapi.h>
#include <sstream>
#include <algorithm>
#include <regex>

#define HookFast(original, hook) DetourAttach((LPVOID*)&original, (LPVOID)hook)
#define UnhookFast(original, hook) DetourDetach((LPVOID*)&original, (LPVOID)hook)

namespace Utils
{
	inline void Print(const std::string& str)
	{
		OutputDebugStringA(str.c_str());
	}

	inline bool IsValidIPWithPort(const std::string& input) {
		// Regex breakdown:
		// ^                          : Start of string
		// (?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3} : Matches 3 octets followed by a dot
		// (?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)          : Matches the 4th octet
		// :                          : Matches the colon separator
		// ([0-9]{1,5})               : Captures 1 to 5 digits for the port
		// $                          : End of string
		std::regex pattern(R"(^((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)):([0-9]{1,5})$)");

		std::smatch match;
		if (std::regex_match(input, match, pattern)) {
			// match[1] is the IP part, match[2] is the Port part
			int port = std::stoi(match[2].str());

			// Port must be between 0 and 65535
			return port >= 0 && port <= 65535;
		}

		return false;
	}

	inline void PatchMemory(std::uintptr_t address, const std::vector<unsigned char>& buffer)
	{
		DWORD oldProtect = 0, newProtect = 0;
		if (VirtualProtect((LPVOID)address, buffer.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			for (size_t i = 0; i < buffer.size(); i++)
				((unsigned char*)address)[i] = buffer[i];

			VirtualProtect((LPVOID)address, buffer.size(), oldProtect, &newProtect);
		}
	}

	template<typename Return, typename... Parameters>
	inline Return CallFunction(std::uintptr_t address, Parameters... params)
	{
		Return(*Function)(Parameters...) = (Return(*)(Parameters...))address;
		return Function(params...);
	}

	inline std::string ToHexString(std::uintptr_t num)
	{
		std::stringstream s;
		s << "0x" << std::hex << num;
		return s.str();
	}

	template <typename T>
	inline T clampEnum(T value, T max, T min = (T)0)
	{
		return std::clamp(value, min, (T)(max - 1));
	}

	inline HMODULE GetModuleContainingAddress(std::uintptr_t address)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		HMODULE hModule = NULL;
		if (hSnapshot != INVALID_HANDLE_VALUE)
		{
			MODULEENTRY32 moduleEntry = { 0 };
			moduleEntry.dwSize = sizeof(MODULEENTRY32);
			if (Module32First(hSnapshot, &moduleEntry))
			{
				while (Module32Next(hSnapshot, &moduleEntry))
				{
					if (address >= (std::uintptr_t)moduleEntry.modBaseAddr && address <= (std::uintptr_t)(moduleEntry.modBaseAddr + moduleEntry.modBaseSize))
					{
						hModule = moduleEntry.hModule;
						break;
					}
				}
			}
			CloseHandle(hSnapshot);
		}
		return hModule;
	}

	inline std::string GetModuleNameFromHandle(HMODULE hModule)
	{
		char buffer[MAX_PATH + 1];
		K32GetModuleBaseNameA(GetCurrentProcess(), hModule, buffer, sizeof(buffer));
		return std::string(buffer);
	}

	inline std::string GetModuleNameFromAddress(std::uintptr_t address)
	{
		return GetModuleNameFromHandle(GetModuleContainingAddress(address));
	}

	inline std::string GetModuleNameWithOffset(std::uintptr_t address)
	{
		HMODULE hModule = GetModuleContainingAddress(address);
		return GetModuleNameFromHandle(hModule) + " + " + ToHexString(address - (std::uintptr_t)hModule);
	}

	inline std::uintptr_t GetReturn(int index)
	{
		const int maxCallers = 5;
		void* callers[maxCallers];
		int count = RtlCaptureStackBackTrace(1, maxCallers, callers, NULL);
		return (std::uintptr_t)callers[index];
	}

	inline std::string GetStacktrace()
	{
		const int maxCallers = 5;
		std::uintptr_t callers[maxCallers]{};
		int count = RtlCaptureStackBackTrace(2, maxCallers, (PVOID*)callers, NULL);
		std::string result;
		count = std::min(maxCallers, count);
		for (int i = 0; i < count; i++)
		{
			result += GetModuleNameWithOffset(callers[i]);
			if (i + 1 < count)
				result += ", ";
		}

		return result;
	}
}