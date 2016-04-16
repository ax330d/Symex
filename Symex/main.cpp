// Symbol resolver
//
// v 0.1 - 29/01/2015
// v 0.2 - 26/02/2016
// v 0.3 - 16/04/2016

#include <iostream>
#include <string>
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <TlHelp32.h>

#include "Psapi.h"
// Link with the dbghelp import library
#pragma comment(lib, "dbghelp.lib")

#ifdef UNICODE
#define DBGHELP_TRANSLATE_TCHAR
#endif
#include <dbghelp.h>


struct CSymbolInfoPackage : public SYMBOL_INFO_PACKAGE
{
    CSymbolInfoPackage()
    {
        si.SizeOfStruct = sizeof(SYMBOL_INFO);
        si.MaxNameLen = sizeof(name);
    }
};


BOOL
CALLBACK
SymRegisterCallbackProc64(
__in HANDLE hProcess,
__in ULONG ActionCode,
__in_opt ULONG64 CallbackData,
__in_opt ULONG64 UserContext
)
{
    UNREFERENCED_PARAMETER(hProcess);
    UNREFERENCED_PARAMETER(UserContext);

    PIMAGEHLP_CBA_EVENT evt;

    // If SYMOPT_DEBUG is set, then the symbol handler will pass
    // verbose information on its attempt to load symbols.
    // This information be delivered as text strings.

    switch (ActionCode)
    {
    case CBA_EVENT:
        evt = (PIMAGEHLP_CBA_EVENT)CallbackData;
        _tprintf(_T("%s"), (PTSTR)evt->desc);
        break;

        // CBA_DEBUG_INFO is the old ActionCode for symbol spew.
        // It still works, but we use CBA_EVENT in this example.
#if 0
    case CBA_DEBUG_INFO:
        _tprintf(_T("%s"), (PTSTR)CallbackData);
        break;
#endif

    default:
        // Return false to any ActionCode we don't handle
        // or we could generate some undesirable behavior.
        return FALSE;
    }

    return TRUE;
}


//You don't have to use this function if you don't want to..
int strcompare(const char* One, const char* Two, bool CaseSensitive)
{
#if defined _WIN32 || defined _WIN64
    return CaseSensitive ? strcmp(One, Two) : _stricmp(One, Two);
#else
    return CaseSensitive ? strcmp(One, Two) : strcasecmp(One, Two);
#endif
}


//You read module information like this..
MODULEENTRY32 GetModuleInfo(int ProcessID, char* ModuleName)
{
    void* hSnap = nullptr;
    MODULEENTRY32 Mod32 = { 0 };

    if ((hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessID)) == 
       INVALID_HANDLE_VALUE)
        return Mod32;

    Mod32.dwSize = sizeof(MODULEENTRY32);
    while (Module32Next(hSnap, &Mod32))
    {
        if (!strcompare(ModuleName, Mod32.szModule, false))
        {
            CloseHandle(hSnap);
            return Mod32;
        }
    }

    CloseHandle(hSnap);
    return{ 0 };
}


VOID usage()
{   
    printf(
"Symex is a tool to resolve symbols by address or offset.\n"
"Made by Arthur Gerkis.\n\n"
"Options:\n"
    "    -p | --pid             process PID (required)\n"
    "    -m | --module          module name (required)\n"
    "    -o | --offset          offset (required, either this or --address)\n"
    "    -a | --address         address to resolve (required, either this or --offset)\n"
    "    -s | --symbol-path     path to your symbols (defaults to C:\\Symbols)\n"
    "    -v | --v               verbosity (defaults to FALSE)\n\n"
"Example: symex.exe -p 1234 -m mshtml.dll -o 0x1234\n"
"Example: symex.exe -p 1234 -m mshtml.dll -a 0x41414141\n\n"
    );
}


VOID cleanup(HANDLE hProcess)
{
    if (SymCleanup(hProcess))
    {
        CloseHandle(hProcess);
    }
    else
    {
        printf("[?/?] SymCleanup returned error : %ul\n", GetLastError());
    }
}


int main(int argc, char* argv[])
{
    int pid = -1;
    int offset = -1;
    int address = -1;
    char moduleName[MAX_MODULE_NAME32] = { 0 };
    char symbolPath[MAX_PATH] = { 0 };

    HANDLE hProcess;
    BOOL verbose = FALSE;

    struct {
        SYMBOL_INFO info;
        char buf[MAX_SYM_NAME];
    } info = { 0 };

    for (int i = 0; i < argc; i += 1)
    {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--pid") == 0)
        {
            sscanf_s(argv[i + 1], "%i", &pid);
        } else
        if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--module") == 0)
        {
            sscanf_s(argv[i + 1], "%s", moduleName,
                     (unsigned)_countof(moduleName));
        } else 
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--symbol-path") == 0)
        {
            sscanf_s(argv[i + 1], "%s", symbolPath,
                     (unsigned)_countof(symbolPath));
        } else 
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--offset") == 0)
        {
            sscanf_s(argv[i + 1], "%i", &offset);
        } else 
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--address") == 0)
        {
            sscanf_s(argv[i + 1], "%i", &address);
        } 
        else
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            verbose = TRUE;
        }
    }

    if (verbose) {
        printf("Symex - symbol resolver, v 0.3\n");
    }

    if (pid == -1 || (offset == -1 && address == -1) || !strlen(moduleName)) {
        usage();
        return FALSE;
    }

    //
    // Set Symbol options
    //
    if (verbose) {
        SymSetOptions(SYMOPT_DEBUG | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    }
    else {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    }

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);


    //
    // Initialise symbol handler
    //

    char programSymbolPath[MAX_PATH] = { 0 };
    if (strlen(symbolPath)) 
    {
        strcpy_s(programSymbolPath, sizeof(programSymbolPath), symbolPath);
        if (verbose) {
            printf("Using provided programSymbolPath = %s\n",
                   programSymbolPath);
        }

    }
    else {
        strcpy_s(programSymbolPath, sizeof(programSymbolPath), "C:\\Symbols");
        if (verbose) {
            printf("Using default programSymbolPath = %s\n",
                   programSymbolPath);
        }

    }

    if (!SymInitialize(hProcess, programSymbolPath, TRUE))
    {
        printf("[1/5] SymInitialize returned error : 0x%x\n",
               GetLastError());
        cleanup(hProcess);
        return FALSE;
    }


    //
    // Register callbacks
    //

    if (!SymRegisterCallback64(hProcess, SymRegisterCallbackProc64, NULL))
    {
        printf("[2/5] SymRegisterCallback64 returned error : 0x%x\n",
               GetLastError());
        cleanup(hProcess);
        return FALSE;
    }


    //
    // Loading symbol module
    //

    DWORD64 dwBaseAddr = (DWORD64)GetModuleInfo(pid, moduleName).modBaseAddr;
    if (verbose) {
        printf("dwBaseAddr = %I64u\n",
               dwBaseAddr);
    }

    if (!SymLoadModuleEx(hProcess,  // target process 
        NULL,                       // handle to image - not used
        (PCSTR)moduleName,          // name of image file
        NULL,                       // name of module - not required
        dwBaseAddr,                 // base address - not required
        0,                          // size of image - not required
        NULL,                       // MODLOAD_DATA used for special cases 
        0))                         // flags - not required
    {
        printf("[3/5] SymLoadModuleEx returned error : 0x%x\n",
               GetLastError());
        cleanup(hProcess);
        return FALSE;
    }


    //
    // Retrieve symbol information by address
    //

    DWORD64 dwAddress;
    if (address != -1) {
        dwAddress = address;
    }
    else if (offset != -1) {
        dwAddress = dwBaseAddr + offset;
    }
    else {
        usage();
        cleanup(hProcess);
        return FALSE;
    }

    info.info.SizeOfStruct = sizeof(info.info);
    info.info.ModBase = dwBaseAddr;
    info.info.MaxNameLen = MAX_SYM_NAME;
    DWORD64 displacement = 0;
    if (!SymFromAddr(hProcess, dwAddress, &displacement, &info.info))
    {
        printf("[4/5] SymFromAddr returned error : 0x%x\n", 
               GetLastError());
        cleanup(hProcess);
        return FALSE;
    }
    else
    {
        printf("%s+0x%I64u\n", info.info.Name, displacement);
    }

    //
    // Unloading module
    //

    if (SymUnloadModule64(hProcess, (DWORD64)dwBaseAddr))
    {
        cleanup(hProcess);
        return TRUE;
    }
    else
    {
        printf("[5/5] SymUnloadModule64 returned error : 0x%x\n",
               GetLastError());
        cleanup(hProcess);
        return FALSE;
    }

    return TRUE;
}