# antithemida
Themida anti-debug bypass and Unity debug support helper for use in the Brick-Force Aurora project.
<br>
The dll is meant to be loaded with a custom patch at the start of mono_set_commandline_arguments inside mono.dll.
<br>
Verbose debug output is printed via *OutputDebugStringA* and can be viewed with [DebugView](https://learn.microsoft.com/de-de/sysinternals/downloads/debugview).
# Config
Reads config options from the Brick-Force Aurora Config.json file.
 - **kill_themida** - true, false - Restores Themida patches on *ntdll.DbgBreakPoint* and *ntdll.DbgUiRemoteBreakin*, hooks *kernelbase.IsDebuggerPresent* to set *PEB.BeingDebugged* to 0 and terminates Themida's anti-tamper thread once it calls *IsDebuggerPresent*.
 - **unity_debug_option** - Off, On, On (Wait For Debugger) - Hooks *mono.mono_utf8_from_external* inside *mono.mono_set_commandline_arguments* and enables mono debugging for the Unity debugger by injecting a custom debug command with *mono.mono_jit_parse_options*.
 - **unity_debug_ip** - IP:Port - Custom IP address for the Unity debugger to connect to, defaults to *127.0.0.1:56000*.

# Dependencies
- [detours](https://github.com/microsoft/Detours)
- [nlohmann json](https://github.com/nlohmann/json)
