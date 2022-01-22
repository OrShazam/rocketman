# rocketman
reversed 'retro' launcher \
anatomy is quite simple:\
the program first acquires debug privilege to call a function to disable WFP in winlogon (hence the clever name 'retro' launcher)\
so it can move a executable from system32 to a temporary path and replace it with a minimal downloader which is stored in the\
program's resource section for stealth, the downloader is also responsible for calling the original executable for stealth
