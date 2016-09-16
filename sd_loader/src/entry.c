#include <gctypes.h>
#include "elf_abi.h"
#include "../../src/common/common.h"
#include "../../src/dynamic_libs/fs_defs.h"
#include "../../src/common/os_defs.h"
#include "vpad.h"

#define CODE_RW_BASE_OFFSET                             0
#define DATA_RW_BASE_OFFSET                             0

#define EXPORT_DECL(res, func, ...)                     res (* func)(__VA_ARGS__);

#define OS_FIND_EXPORT(handle, funcName, func)                    OSDynLoad_FindExport(handle, 0, funcName, &func)

typedef struct _private_data_t
{
	char urls[6][]={"
	unsigned char *file;
	unsigned int file_size;
	int buf0_size, buf1_size;

	EXPORT_DECL(void *, MEMAllocFromDefaultHeapEx,int size, int align);
	EXPORT_DECL(void *, MEMAllocFromDefaultHeap,int size);
	EXPORT_DECL(void, MEMFreeToDefaultHeap,void *ptr);

	EXPORT_DECL(void*, memcpy, void *p1, const void *p2, unsigned int s);
	EXPORT_DECL(void*, memset, void *p1, int val, unsigned int s);
	EXPORT_DECL(void, OSFatal, const char* msg);
	EXPORT_DECL(void, DCFlushRange, const void *addr, u32 length);
	EXPORT_DECL(void, ICInvalidateRange, const void *addr, u32 length);
	EXPORT_DECL(int, __os_snprintf, char* s, int n, const char * format, ...);
	EXPORT_DECL(void, exit, void);

	EXPORT_DECL(void, OSScreenInit, void);
	EXPORT_DECL(unsigned int, OSScreenGetBufferSizeEx, unsigned int bufferNum);
	EXPORT_DECL(int, OSScreenSetBufferEx, unsigned int bufferNum, void * addr);
	EXPORT_DECL(int, OSScreenClearBufferEx, unsigned int bufferNum, unsigned int temp);
	EXPORT_DECL(int, OSScreenFlipBuffersEx, unsigned int bufferNum);
	EXPORT_DECL(int, OSScreenPutFontEx, unsigned int bufferNum, unsigned int posX, unsigned int posY, const char * buffer);
	EXPORT_DECL(int, OSScreenEnableEx, unsigned int bufferNum, int enable);
	EXPORT_DECL(unsigned int, OSScreenPutPixelEx, unsigned int bufferNum, unsigned int posX, unsigned int posY, uint32_t color);
	
	EXPORT_DECL(int, VPADRead, int controller, VPADData *buffer, unsigned int num, int *error);

	EXPORT_DECL(int, FSInit, void);
	EXPORT_DECL(int, FSAddClientEx, void *pClient, int unk_zero_param, int errHandling);
	EXPORT_DECL(int, FSDelClient, void *pClient);
	EXPORT_DECL(void, FSInitCmdBlock, void *pCmd);
	EXPORT_DECL(int, FSGetMountSource, void *pClient, void *pCmd, int type, void *source, int errHandling);
	EXPORT_DECL(int, FSMount, void *pClient, void *pCmd, void *source, const char *target, uint32_t bytes, int errHandling);
	EXPORT_DECL(int, FSUnmount, void *pClient, void *pCmd, const char *target, int errHandling);
	EXPORT_DECL(int, FSOpenFile, void *pClient, void *pCmd, const char *path, const char *mode, int *fd, int errHandling);
	EXPORT_DECL(int, FSGetStatFile, void *pClient, void *pCmd, int fd, void *buffer, int error);
	EXPORT_DECL(int, FSReadFile, void *pClient, void *pCmd, void *buffer, int size, int count, int fd, int flag, int errHandling);
	EXPORT_DECL(int, FSWriteFile, void *pClient, void *pCmd, void *buffer, int size, int count, int fd, int flag, int errHandling);
	EXPORT_DECL(int, FSCloseFile, void *pClient, void *pCmd, int fd, int errHandling);
	
	EXPORT_DECL(int, ACInitialize, void);
	EXPORT_DECL(int, ACGetStartupId, uint32_t *id);
	EXPORT_DECL(int, ACConnectWithConfigId, uint32_t id);
	EXPORT_DECL(int, socket_lib_init, void);
	EXPORT_DECL(int, curl_global_init, int opts);
	EXPORT_DECL(void*, curl_easy_init, void);
	EXPORT_DECL(void, curl_easy_cleanup, void *handle);
	EXPORT_DECL(void, curl_easy_setopt, void *handle, uint32_t param, void *op);
	EXPORT_DECL(int, curl_easy_perform, void *handle);
	EXPORT_DECL(void, curl_easy_getinfo, void *handle, uint32_t param, void *info);
	
	EXPORT_DECL(int, SYSRelaunchTitle, int argc, char* argv);
} private_data_t;

#define fillScreen(r,g,b,a); do{private_data->OSScreenClearBufferEx(0, (0 << 24) | (0 << 16) | (0 << 8) | 0);private_data->OSScreenClearBufferEx(1, (r << 24) | (g << 16) | (b << 8) | a);}while(0)

static void flipBuffers(private_data_t *private_data) {
	private_data->DCFlushRange((void *)0xF4000000 + private_data->buf0_size, private_data->buf1_size);
	private_data->DCFlushRange((void *)0xF4000000, private_data->buf0_size);
	private_data->OSScreenFlipBuffersEx(0);
	private_data->OSScreenFlipBuffersEx(1);
}
static void printScreen(private_data_t *private_data, int row, char* str) {
	private_data->OSScreenPutFontEx(0, 0, row, str);
	private_data->OSScreenPutFontEx(1, 0, row, str);
}

static int curl_write_data_callback(void *ptr, uint32_t size, uint32_t nmemb, private_data_t *private_data) {
	uint32_t new_len;
	unsigned char *d_tmp;
	char buff[40];
	new_len = private_data->file_size + size*nmemb;
	d_tmp=(unsigned char*)private_data->MEMAllocFromDefaultHeap(private_data->file_size+1);
	private_data->memcpy(d_tmp,private_data->file,private_data->file_size+1);
	private_data->MEMFreeToDefaultHeap(private_data->file);
	private_data->file=(unsigned char *)private_data->MEMAllocFromDefaultHeap(new_len+1);
	private_data->memcpy(private_data->file,d_tmp,private_data->file_size+1);
	private_data->MEMFreeToDefaultHeap(d_tmp);
	private_data->memcpy(private_data->file+private_data->file_size, ptr, size*nmemb);
	private_data->file[new_len] = '\0';
	private_data->file_size = new_len;
	fillScreen(0,0,0,0); //Clearscreen
	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "INSTALLING");
	printScreen(private_data, 4, "Downloading...");
	private_data->__os_snprintf(buff,40,"%d bytes downloaded",private_data->file_size)
	printScreen(private_data, 5, buff);
	flipBuffers(private_data);
	return size*nmemb;
}
static int LoadFileToMem(private_data_t *private_data, unsigned char **fileOut, unsigned int *sizeOut) {
	int iFd = -1;
	void *pClient = private_data->MEMAllocFromDefaultHeapEx(FS_CLIENT_SIZE, 4);
	if(!pClient) return 0;
	void *pCmd = private_data->MEMAllocFromDefaultHeapEx(FS_CMD_BLOCK_SIZE, 4);
	if(!pCmd) {
        	private_data->MEMFreeToDefaultHeap(pClient);
        	return 0;
	}
	int success = 0;
	private_data->FSInit();
	private_data->FSInitCmdBlock(pCmd);
	private_data->FSAddClientEx(pClient, 0, -1);
	char tempPath[FS_MOUNT_SOURCE_SIZE];
	char mountPath[FS_MAX_MOUNTPATH_SIZE];
	if (private_data->FSGetMountSource(pClient, pCmd, 0, tempPath, -1) != 0) private_data->OSFatal("FSGetMountSource failed.");
	if (private_data->FSMount(pClient, pCmd, tempPath, mountPath, FS_MAX_MOUNTPATH_SIZE, -1) != 0) private_data->OSFatal("SD mount failed.");
	if (private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/homebrew_launcher/homebrew_launcher.elf", "r", &iFd, -1) != 0) {
		private_data->FSUnmount(pClient, pCmd, mountPath, -1);
		goto END;
	}
	FSStat stat;
	stat.size = 0;
	void *pBuffer = NULL;
	private_data->FSGetStatFile(pClient, pCmd, iFd, &stat, -1);
	//-------------CHECK FOR HBL---------------
	if (stat.size > 0) goto LOAD; //File is more than 0 byte; probably hbl is already there (yeah, gotos are not advised but...)
	//----------NOT FOUND; OPEN MENU-----------
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);

	//Init OSScreen
	private_data->OSScreenInit();
	private_data->buf0_size = private_data->OSScreenGetBufferSizeEx(0);
	private_data->buf1_size = private_data->OSScreenGetBufferSizeEx(1);
	private_data->OSScreenSetBufferEx(0, (void *)0xF4000000);
	private_data->OSScreenSetBufferEx(1, (void *)0xF4000000 + private_data->buf0_size);
	fillScreen(0,0,0,0); //Clearscreen
	flipBuffers(private_data); //FlipBuffers
	fillScreen(0,0,0,0); //Clearscreen
	flipBuffers(private_data); //FlipBuffers
	//Enable screens
	private_data->OSScreenEnableEx(0, 1);
	private_data->OSScreenEnableEx(1, 1);
	
	//Menu start
	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "Install Homebrew Launcher?");
	printScreen(private_data, 5, "A: Install Homebrew Launcher to SD");
	printScreen(private_data, 6, "B: Cancel");
	flipBuffers(private_data);
	//Check for input
	int error;
	VPADData vpad_data;
	for(;;) {
		private_data->VPADRead(0, &vpad_data, 1, &error);
		if (vpad_data.btn_hold&BUTTON_A) break;
		if (vpad_data.btn_hold&BUTTON_B) goto LOAD; //This will cause an error because the file is not there...
	}
	//Install text
	fillScreen(0,0,0,0); //Clearscreen
	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "INSTALLING");
	printScreen(private_data, 4, "Downloading...");
	flipBuffers(private_data);
	//--------------------INSTALL----------------------
	//Init network
	uint32_t nn_startupid;
	private_data->ACInitialize();
	private_data->ACGetStartupId(&nn_startupid);
	private_data->ACConnectWithConfigId(nn_startupid);
	private_data->socket_lib_init();
	private_data->curl_global_init(((1<<0)|(1<<1))); 
	int ret,resp;
	void *curl_handle;
	//--Download elf--
	//Prepare var
	private_data->file_size=0;
	private_data->file=(unsigned char *)private_data->MEMAllocFromDefaultHeap(private_data->file_size+1);
	private_data->file[0] = '\0';

	curl_handle = private_data->curl_easy_init();
	if(!curl_handle) private_data->OSFatal("cURL not initialized");
	private_data->curl_easy_setopt(curl_handle, 10002, "http://www.wiiubru.com/appstore/apps/homebrew_launcher/homebrew_launcher.elf");
	private_data->curl_easy_setopt(curl_handle, 20011, curl_write_data_callback);
	private_data->curl_easy_setopt(curl_handle, 10001, private_data);
	ret = private_data->curl_easy_perform(curl_handle);
	if(ret) private_data->OSFatal("curl_easy_perform returned an error");
	resp = 404;
	private_data->curl_easy_getinfo(curl_handle, 0x200002, &resp);
	if(resp != 200) private_data->OSFatal("curl_easy_getinfo returned an HTTP error");
	private_data->curl_easy_cleanup(curl_handle);
	private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/homebrew_launcher/homebrew_launcher.elf", "w", &iFd, -1);
	private_data->FSWriteFile(pClient, pCmd, private_data->file, 0x01, private_data->file_size, iFd, 0, -1);
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);
	private_data->MEMFreeToDefaultHeap(private_data->file);


	private_data->mode=1; //Download HBL meta
	private_data->hbl.meta_size=0;
	private_data->hbl.meta=(unsigned char *)private_data->MEMAllocFromDefaultHeap(private_data->hbl.meta_size+1);
	private_data->hbl.meta[0] = '\0';

	curl_handle = private_data->curl_easy_init();
	if(!curl_handle) private_data->OSFatal("cURL not initialized");
	private_data->curl_easy_setopt(curl_handle, 10002, "http://www.wiiubru.com/appstore/apps/homebrew_launcher/meta.xml");
	private_data->curl_easy_setopt(curl_handle, 20011, curl_write_data_callback);
	private_data->curl_easy_setopt(curl_handle, 10001, private_data);
	ret = private_data->curl_easy_perform(curl_handle);
	if(ret) private_data->OSFatal("curl_easy_perform returned an error");
	resp = 404;
	private_data->curl_easy_getinfo(curl_handle, 0x200002, &resp);
	if(resp != 200) private_data->OSFatal("curl_easy_getinfo returned an HTTP error");
	private_data->curl_easy_cleanup(curl_handle);


	private_data->mode=2; //Download HBL icon
	private_data->hbl.icon_size=0;
	private_data->hbl.icon=(unsigned char *)private_data->MEMAllocFromDefaultHeap(private_data->hbl.icon_size+1);
	private_data->hbl.icon[0] = '\0';

	curl_handle = private_data->curl_easy_init();
	if(!curl_handle) private_data->OSFatal("cURL not initialized");
	private_data->curl_easy_setopt(curl_handle, 10002, "http://www.wiiubru.com/appstore/apps/homebrew_launcher/icon.png");
	private_data->curl_easy_setopt(curl_handle, 20011, curl_write_data_callback);
	private_data->curl_easy_setopt(curl_handle, 10001, private_data);
	ret = private_data->curl_easy_perform(curl_handle);
	if(ret) private_data->OSFatal("curl_easy_perform returned an error");
	resp = 404;
	private_data->curl_easy_getinfo(curl_handle, 0x200002, &resp);
	if(resp != 200) private_data->OSFatal("curl_easy_getinfo returned an HTTP error");
	private_data->curl_easy_cleanup(curl_handle);


	//Download done text
	fillScreen(0,0,0,0);
	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "INSTALLING");
	printScreen(private_data, 4, "Downloading...");
	printScreen(private_data, 5, "Download done");
	printScreen(private_data, 6, "Installing...");
	flipBuffers(private_data);


	private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/homebrew_launcher/meta.xml", "w", &iFd, -1);
	private_data->FSWriteFile(pClient, pCmd, private_data->hbl.meta, 0x01, private_data->hbl.meta_size, iFd, 0, -1);
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);
	private_data->MEMFreeToDefaultHeap(private_data->hbl.meta);
	
	private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/homebrew_launcher/icon.png", "w", &iFd, -1);
	private_data->FSWriteFile(pClient, pCmd, private_data->hbl.icon, 0x01, private_data->hbl.icon_size, iFd, 0, -1);
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);
	private_data->MEMFreeToDefaultHeap(private_data->hbl.icon);

	fillScreen(0,0,0,0);
	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "INSTALLING");
	printScreen(private_data, 4, "Downloading...");
	printScreen(private_data, 5, "Download done");
	printScreen(private_data, 6, "Installing...");
	printScreen(private_data, 6, "Install done");
	flipBuffers(private_data);

	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "Homebrew Launcher installed!");
	printScreen(private_data, 5, "Install Homebrew Appstore to SD?");
	printScreen(private_data, 7, "The Homebrew Appstore make it easy to manage (update, install) homebrew ");
	printScreen(private_data, 8, "apps without a computer and it's highly advised to install it.");
	printScreen(private_data, 10, "A: Install Homebrew Appstore to SD");
	printScreen(private_data, 11, "B: Cancel");
	for(;;) {
		private_data->VPADRead(0, &vpad_data, 1, &error);
		if (vpad_data.btn_hold&BUTTON_A) break;
		if (vpad_data.btn_hold&BUTTON_B) goto EXIT_INSTALL;
	}

	//------------------INSTALL HBAS-------------------
	//--Download elf--
	//Prepare var
	private_data->mode=3; //Download HBAS elf
	private_data->hbas.elf_size=0;
	private_data->hbas.elf=(unsigned char *)private_data->MEMAllocFromDefaultHeap(private_data->hbas.elf_size+1);
	private_data->hbas.elf[0] = '\0';
	
	curl_handle = private_data->curl_easy_init();
	if(!curl_handle) private_data->OSFatal("cURL not initialized");
	private_data->curl_easy_setopt(curl_handle, 10002, "http://www.wiiubru.com/appstore/apps/appstore/hbas.elf");
	private_data->curl_easy_setopt(curl_handle, 20011, curl_write_data_callback);
	private_data->curl_easy_setopt(curl_handle, 10001, private_data);
	ret = private_data->curl_easy_perform(curl_handle);
	if(ret) private_data->OSFatal("curl_easy_perform returned an error");
	resp = 404;
	private_data->curl_easy_getinfo(curl_handle, 0x200002, &resp);
	if(resp != 200) private_data->OSFatal("curl_easy_getinfo returned an HTTP error");
	private_data->curl_easy_cleanup(curl_handle);


	private_data->mode=4; //Download HBAS meta
	private_data->hbas.meta_size=0;
	private_data->hbas.meta=(unsigned char *)private_data->MEMAllocFromDefaultHeap(private_data->hbas.meta_size+1);
	private_data->hbas.meta[0] = '\0';

	curl_handle = private_data->curl_easy_init();
	if(!curl_handle) private_data->OSFatal("cURL not initialized");
	private_data->curl_easy_setopt(curl_handle, 10002, "http://www.wiiubru.com/appstore/apps/appstore/meta.xml");
	private_data->curl_easy_setopt(curl_handle, 20011, curl_write_data_callback);
	private_data->curl_easy_setopt(curl_handle, 10001, private_data);
	ret = private_data->curl_easy_perform(curl_handle);
	if(ret) private_data->OSFatal("curl_easy_perform returned an error");
	resp = 404;
	private_data->curl_easy_getinfo(curl_handle, 0x200002, &resp);
	if(resp != 200) private_data->OSFatal("curl_easy_getinfo returned an HTTP error");
	private_data->curl_easy_cleanup(curl_handle);


	private_data->mode=5; //Download HBAS icon
	private_data->hbas.icon_size=0;
	private_data->hbas.icon=(unsigned char *)private_data->MEMAllocFromDefaultHeap(private_data->hbas.icon_size+1);
	private_data->hbas.icon[0] = '\0';

	curl_handle = private_data->curl_easy_init();
	if(!curl_handle) private_data->OSFatal("cURL not initialized");
	private_data->curl_easy_setopt(curl_handle, 10002, "http://www.wiiubru.com/appstore/apps/appstore/icon.png");
	private_data->curl_easy_setopt(curl_handle, 20011, curl_write_data_callback);
	private_data->curl_easy_setopt(curl_handle, 10001, private_data);
	ret = private_data->curl_easy_perform(curl_handle);
	if(ret) private_data->OSFatal("curl_easy_perform returned an error");
	resp = 404;
	private_data->curl_easy_getinfo(curl_handle, 0x200002, &resp);
	if(resp != 200) private_data->OSFatal("curl_easy_getinfo returned an HTTP error");
	private_data->curl_easy_cleanup(curl_handle);


	fillScreen(0,0,0,0);
	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "INSTALLING");
	printScreen(private_data, 4, "Downloading...");
	printScreen(private_data, 5, "Download done");
	printScreen(private_data, 6, "Installing...");
	flipBuffers(private_data);

	private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/appstore/hbas.elf", "w", &iFd, -1);
	private_data->FSWriteFile(pClient, pCmd, private_data->hbas.elf, 0x01, private_data->hbas.elf_size, iFd, 0, -1);
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);
	private_data->MEMFreeToDefaultHeap(private_data->hbas.elf);

	private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/appstore/meta.xml", "w", &iFd, -1);
	private_data->FSWriteFile(pClient, pCmd, private_data->hbas.meta, 0x01, private_data->hbas.meta_size, iFd, 0, -1);
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);
	private_data->MEMFreeToDefaultHeap(private_data->hbas.meta);
	
	private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/appstore/icon.png", "w", &iFd, -1);
	private_data->FSWriteFile(pClient, pCmd, private_data->hbas.icon, 0x01, private_data->hbas.icon_size, iFd, 0, -1);
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);
	private_data->MEMFreeToDefaultHeap(private_data->hbas.icon);

	//End Install
	fillScreen(0,0,0,0);
	printScreen(private_data, 0, "Homebrew Installer v0.1 beta");
	printScreen(private_data, 1, "------------------------------");
	printScreen(private_data, 3, "INSTALLING");
	printScreen(private_data, 4, "Downloading...");
	printScreen(private_data, 5, "Download done");
	printScreen(private_data, 6, "Installing...");
	printScreen(private_data, 6, "Install done");
	flipBuffers(private_data);
EXIT_INSTALL:
	fillScreen(0,0,0,0); //Clearscreen
	flipBuffers(private_data); //FlipBuffers
	fillScreen(0,0,0,0); //Clearscreen
	flipBuffers(private_data); //FlipBuffers
	private_data->FSOpenFile(pClient, pCmd, "/vol/external01/wiiu/apps/homebrew_launcher/homebrew_launcher.elf", "r", &iFd, -1);
LOAD:
	pBuffer = private_data->MEMAllocFromDefaultHeapEx((stat.size + 0x3F) & ~0x3F, 0x40);
	if(!pBuffer) private_data->OSFatal("Not enough memory for ELF file.");
	unsigned int done = 0;
	while(done < stat.size) {
		int readBytes = private_data->FSReadFile(pClient, pCmd, pBuffer + done, 1, stat.size - done, iFd, 0, -1);
		if(readBytes <= 0) break;
		done += readBytes;
	}
	if(done != stat.size) private_data->MEMFreeToDefaultHeap(pBuffer);
	else {
		*fileOut = (unsigned char*)pBuffer;
		*sizeOut = stat.size;
		success = 1;
	}
	private_data->FSCloseFile(pClient, pCmd, iFd, -1);
	private_data->FSUnmount(pClient, pCmd, mountPath, -1);

	private_data->FSDelClient(pClient);
	private_data->MEMFreeToDefaultHeap(pClient);
	private_data->MEMFreeToDefaultHeap(pCmd);
END:
	return success;
}
static unsigned int load_elf_image (private_data_t *private_data, unsigned char *elfstart) {
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdrs;
	unsigned char *image;
	int i;
	ehdr = (Elf32_Ehdr *) elfstart;
	if(ehdr->e_phoff == 0 || ehdr->e_phnum == 0) return 0;
	if(ehdr->e_phentsize != sizeof(Elf32_Phdr)) return 0;
	phdrs = (Elf32_Phdr*)(elfstart + ehdr->e_phoff);
	for(i = 0; i < ehdr->e_phnum; i++) {
		if(phdrs[i].p_type != PT_LOAD) continue;
		if(phdrs[i].p_filesz > phdrs[i].p_memsz) return 0;
		if(!phdrs[i].p_filesz) continue;
		unsigned int p_paddr = phdrs[i].p_paddr;
		if(phdrs[i].p_flags & PF_X) p_paddr += CODE_RW_BASE_OFFSET; // use correct offset address for executables and data access
		else p_paddr += DATA_RW_BASE_OFFSET;
		image = (unsigned char *) (elfstart + phdrs[i].p_offset);
		private_data->memcpy ((void *) p_paddr, image, phdrs[i].p_filesz);
		private_data->DCFlushRange((void*)p_paddr, phdrs[i].p_filesz);

		if(phdrs[i].p_flags & PF_X) private_data->ICInvalidateRange ((void *) phdrs[i].p_paddr, phdrs[i].p_memsz);
	}

	//! clear BSS
	Elf32_Shdr *shdr = (Elf32_Shdr *) (elfstart + ehdr->e_shoff);
	for(i = 0; i < ehdr->e_shnum; i++) {
		const char *section_name = ((const char*)elfstart) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
        	if(section_name[0] == '.' && section_name[1] == 'b' && section_name[2] == 's' && section_name[3] == 's') {
        		private_data->memset((void*)shdr[i].sh_addr, 0, shdr[i].sh_size);
			private_data->DCFlushRange((void*)shdr[i].sh_addr, shdr[i].sh_size);
		} else if(section_name[0] == '.' && section_name[1] == 's' && section_name[2] == 'b' && section_name[3] == 's' && section_name[4] == 's') {
			private_data->memset((void*)shdr[i].sh_addr, 0, shdr[i].sh_size);
			private_data->DCFlushRange((void*)shdr[i].sh_addr, shdr[i].sh_size);
		}
	}
	return ehdr->e_entry;
}

static void loadFunctionPointers(private_data_t * private_data)
{
	unsigned int coreinit_handle,sysapp_handle,nn_ac_handle, nsysnet_handle, libcurl_handle, vpad_handle;

	EXPORT_DECL(int, OSDynLoad_Acquire, const char* rpl, u32 *handle);
	EXPORT_DECL(int, OSDynLoad_FindExport, u32 handle, int isdata, const char *symbol, void *address);

	OSDynLoad_Acquire = (int (*)(const char*, u32 *))OS_SPECIFICS->addr_OSDynLoad_Acquire;
	OSDynLoad_FindExport = (int (*)(u32, int, const char *, void *))OS_SPECIFICS->addr_OSDynLoad_FindExport;

	OSDynLoad_Acquire("coreinit.rpl", &coreinit_handle);
	OSDynLoad_Acquire("vpad.rpl", &vpad_handle);
	OSDynLoad_Acquire("sysapp.rpl", &sysapp_handle);
	OSDynLoad_Acquire("nn_ac.rpl", &nn_ac_handle);
	OSDynLoad_Acquire("nsysnet.rpl", &nsysnet_handle);
	OSDynLoad_Acquire("nlibcurl.rpl", &libcurl_handle);

	unsigned int *functionPtr = 0;

	OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeapEx", &functionPtr);
	private_data->MEMAllocFromDefaultHeapEx = (void * (*)(int, int))*functionPtr;
	OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeap", &functionPtr);
	private_data->MEMAllocFromDefaultHeap = (void * (*)(int))*functionPtr;
	OSDynLoad_FindExport(coreinit_handle, 1, "MEMFreeToDefaultHeap", &functionPtr);
	private_data->MEMFreeToDefaultHeap = (void (*)(void *))*functionPtr;

	OS_FIND_EXPORT(coreinit_handle, "memcpy", private_data->memcpy);
	OS_FIND_EXPORT(coreinit_handle, "memset", private_data->memset);
	OS_FIND_EXPORT(coreinit_handle, "OSFatal", private_data->OSFatal);
	OS_FIND_EXPORT(coreinit_handle, "DCFlushRange", private_data->DCFlushRange);
	OS_FIND_EXPORT(coreinit_handle, "ICInvalidateRange", private_data->ICInvalidateRange);
	OS_FIND_EXPORT(coreinit_handle, "__os_snprintf", private_data->__os_snprintf);
	OS_FIND_EXPORT(coreinit_handle, "exit", private_data->exit);
	OS_FIND_EXPORT(coreinit_handle, "DCFlushRange", private_data->DCFlushRange);

	OS_FIND_EXPORT(coreinit_handle, "OSScreenInit", private_data->OSScreenInit);
	OS_FIND_EXPORT(coreinit_handle, "OSScreenGetBufferSizeEx", private_data->OSScreenGetBufferSizeEx);
	OS_FIND_EXPORT(coreinit_handle, "OSScreenSetBufferEx", private_data->OSScreenSetBufferEx);
	OS_FIND_EXPORT(coreinit_handle, "OSScreenClearBufferEx", private_data->OSScreenClearBufferEx);
	OS_FIND_EXPORT(coreinit_handle, "OSScreenFlipBuffersEx", private_data->OSScreenFlipBuffersEx);
	OS_FIND_EXPORT(coreinit_handle, "OSScreenPutFontEx", private_data->OSScreenPutFontEx);
	OS_FIND_EXPORT(coreinit_handle, "OSScreenEnableEx", private_data->OSScreenEnableEx);
	OS_FIND_EXPORT(coreinit_handle, "OSScreenPutPixelEx", private_data->OSScreenPutPixelEx);

	OS_FIND_EXPORT(coreinit_handle, "FSInit", private_data->FSInit);
	OS_FIND_EXPORT(coreinit_handle, "FSAddClientEx", private_data->FSAddClientEx);
	OS_FIND_EXPORT(coreinit_handle, "FSDelClient", private_data->FSDelClient);
	OS_FIND_EXPORT(coreinit_handle, "FSInitCmdBlock", private_data->FSInitCmdBlock);
	OS_FIND_EXPORT(coreinit_handle, "FSGetMountSource", private_data->FSGetMountSource);
	OS_FIND_EXPORT(coreinit_handle, "FSMount", private_data->FSMount);
	OS_FIND_EXPORT(coreinit_handle, "FSUnmount", private_data->FSUnmount);
	OS_FIND_EXPORT(coreinit_handle, "FSOpenFile", private_data->FSOpenFile);
	OS_FIND_EXPORT(coreinit_handle, "FSGetStatFile", private_data->FSGetStatFile);
	OS_FIND_EXPORT(coreinit_handle, "FSReadFile", private_data->FSReadFile);
	OS_FIND_EXPORT(coreinit_handle, "FSCloseFile", private_data->FSCloseFile);

	OS_FIND_EXPORT(vpad_handle, "VPADRead", private_data->VPADRead);

	OS_FIND_EXPORT(nn_ac_handle, "ACInitialize", private_data->ACInitialize);
	OS_FIND_EXPORT(nn_ac_handle, "ACGetStartupId", private_data->ACGetStartupId);
	OS_FIND_EXPORT(nn_ac_handle, "ACConnectWithConfigId", private_data->ACConnectWithConfigId);
	
	OS_FIND_EXPORT(nsysnet_handle, "socket_lib_init", private_data->socket_lib_init);

	OS_FIND_EXPORT(libcurl_handle, "curl_global_init", private_data->curl_global_init);
	OS_FIND_EXPORT(libcurl_handle, "curl_easy_init", private_data->curl_easy_init);
	OS_FIND_EXPORT(libcurl_handle, "curl_easy_cleanup", private_data->curl_easy_cleanup);
	OS_FIND_EXPORT(libcurl_handle, "curl_easy_setopt", private_data->curl_easy_setopt);
	OS_FIND_EXPORT(libcurl_handle, "curl_easy_perform", private_data->curl_easy_perform);
	OS_FIND_EXPORT(libcurl_handle, "curl_easy_getinfo", private_data->curl_easy_getinfo);
	
	OS_FIND_EXPORT(sysapp_handle, "SYSRelaunchTitle", private_data->SYSRelaunchTitle);
}

int _start(int argc, char **argv)
{
	{
		private_data_t private_data;
		loadFunctionPointers(&private_data);
		while(1) {
			if(ELF_DATA_ADDR != 0xDEADC0DE && ELF_DATA_SIZE > 0) {
				//! copy data to safe area before processing it
				unsigned char * pElfBuffer = (unsigned char *)private_data.MEMAllocFromDefaultHeapEx(ELF_DATA_SIZE, 4);
				if(pElfBuffer) {
					private_data.memcpy(pElfBuffer, (unsigned char*)ELF_DATA_ADDR, ELF_DATA_SIZE);
					MAIN_ENTRY_ADDR = load_elf_image(&private_data, pElfBuffer);
					private_data.MEMFreeToDefaultHeap(pElfBuffer);
				}
				ELF_DATA_ADDR = 0xDEADC0DE;
				ELF_DATA_SIZE = 0;
			}
			if(MAIN_ENTRY_ADDR == 0xDEADC0DE || MAIN_ENTRY_ADDR == 0) {
				unsigned char *pElfBuffer = NULL;
				unsigned int uiElfSize = 0;
				LoadFileToMem(&private_data, &pElfBuffer, &uiElfSize);
				if(!pElfBuffer) private_data.OSFatal("Could not load HBL");
				else {
					MAIN_ENTRY_ADDR = load_elf_image(&private_data, pElfBuffer);
					private_data.MEMFreeToDefaultHeap(pElfBuffer);
					if(MAIN_ENTRY_ADDR == 0) private_data.OSFatal("Failed to load ELF");
				}
			} else {
				int returnVal = ((int (*)(int, char **))MAIN_ENTRY_ADDR)(argc, argv);
				if(returnVal == (int)EXIT_RELAUNCH_ON_LOAD) break; //! exit to miimaker and restart application on re-enter of another application
				else break; //! exit to homebrew launcher in all other cases
			}
		}
	}
	return ( (int (*)(int, char **))(*(unsigned int*)OS_SPECIFICS->addr_OSTitle_main_entry) )(argc, argv);
}
