// 3_18.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"
#include "windows.h"
#include "malloc.h"
#include "stdlib.h"

#define ShellcodeLen	0X12
#define MESSAGEBOXADDR	0X77D507EA
BYTE ShellCode[] = {
	0X6A,0X00,0X6A,0X00,0X6A,0X00,0X6A,0X00,
	0XE8,0X00,0X00,0X00,0X00,
	0XE9,0X00,0X00,0X00,0X00
};
BYTE NewSection[] = {
	0x2E,0x74,0x74,0x74,0x74,0x00,0x00,0x00,0x22,0x97,
	0x01,0x00,0x00,0x10,0x00,0x00,0x00,0xA0,0x01,0x00,
	0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x60,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
char File_path[] = "C:/fg/DllDemo.dll";
char New_File_path[] = "C:/fg/file.exe";

//读文件大小
int ReadFileBuffer(char* file_path,PVOID* FileBuffer){
	FILE* fp;
	unsigned int file_size = NULL;
	LPVOID TempFileBuffer = NULL;
	fp = fopen(file_path,"rb");
	if(!fp){
		printf("文件打开失败！");
		return 0;
	}
	fseek(fp,0,SEEK_END);
	file_size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	if(!file_size){
		printf("文件大小为空！");
		return 0;
	}
	TempFileBuffer = malloc(file_size);
	if(!TempFileBuffer){
		printf("初始化内存失败！");
		return 0;
	}
	size_t n = fread(TempFileBuffer,file_size,1,fp);
	if(!n){
		printf("读取内存失败！");
		return 0;
	}
	*FileBuffer = TempFileBuffer;
	TempFileBuffer = NULL;
	fclose(fp);
	return file_size;
}

//对FileBuffer进行拉伸
DWORD CopyImageBuffer(PVOID FileBuffer,PVOID* ImageBuffer){
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	LPVOID TempImageBuffer = NULL;
	int i;
	if(!FileBuffer){
		printf("FileBuffer内存为空！");
		return 0;
	}
	if(*((PWORD)FileBuffer) != IMAGE_DOS_SIGNATURE){
		printf("dos头不为MZ！");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*((PDWORD)((DWORD)FileBuffer + pDosHeader->e_lfanew)) != IMAGE_NT_SIGNATURE){
		printf("不是有效的PE文件！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)FileBuffer + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + IMAGE_SIZEOF_FILE_HEADER);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	//初始化大小
	TempImageBuffer = malloc(pOptionHeader->SizeOfImage);
	if(!TempImageBuffer){
		printf("初始化失败！");
		return 0;
	}
	//进行内存填充
	memset(TempImageBuffer,0,pOptionHeader->SizeOfImage);
	//进行头文件复制
	memcpy(TempImageBuffer,pDosHeader,pOptionHeader->SizeOfHeaders);
	PIMAGE_SECTION_HEADER pTempSectionHeader = pSectionHeader;
	for(i = 0;i < pFileHeader->NumberOfSections;i++,pTempSectionHeader++){
		//进行节文件拉伸复制
		memcpy((void*)((DWORD)TempImageBuffer + pTempSectionHeader->VirtualAddress),(void*)((DWORD)FileBuffer + pTempSectionHeader->PointerToRawData),pTempSectionHeader->SizeOfRawData);
	}
	*ImageBuffer = TempImageBuffer;
	TempImageBuffer = NULL;
	return pOptionHeader->SizeOfImage;
}

//对ImageBuffer大小进行压缩
DWORD CopyNewBuffer(PVOID ImageBuffer,PVOID* NewBuffer){
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	LPVOID TempNewBuffer = NULL;
	int i,j;
	if(!ImageBuffer){
		printf("ImageBuffer传入为空！");
		return 0;
	}
	if(*((PWORD)ImageBuffer) != IMAGE_DOS_SIGNATURE){
		printf("不是一个有效MZ！");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)ImageBuffer;
	if(*((PDWORD)((DWORD)ImageBuffer + pDosHeader->e_lfanew)) != IMAGE_NT_SIGNATURE){
		printf("不是一个有效PE！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + IMAGE_SIZEOF_FILE_HEADER);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	int New_size = pOptionHeader->SizeOfHeaders;
	for(i = 0;i < pFileHeader->NumberOfSections;i++){
		New_size += pSectionHeader[i].SizeOfRawData;
	}
	//初始化大小
	TempNewBuffer = malloc(New_size);
	//设置内存
	memset(TempNewBuffer,0,New_size);
	//拷贝头文件
	memcpy(TempNewBuffer,pDosHeader,pOptionHeader->SizeOfHeaders);
	PIMAGE_SECTION_HEADER pTempSectionHeader = pSectionHeader;
	for(j = 0;j < pFileHeader->NumberOfSections;j++,pTempSectionHeader++){
		memcpy((void*)((DWORD)TempNewBuffer + pTempSectionHeader->PointerToRawData),(void*)((DWORD)ImageBuffer + pTempSectionHeader->VirtualAddress),pTempSectionHeader->SizeOfRawData);
	}
	*NewBuffer = TempNewBuffer;
	TempNewBuffer = NULL;
	return New_size;
}

//进行写文件
DWORD WriteFilePath(PVOID NewBuffer,size_t New_Size,char* write_file_path){
	FILE *fp;
	fp = fopen(write_file_path,"wb+");
	if(!fp){
		printf("文件打开失败！");
		return 0;
	}
	fwrite(NewBuffer,New_Size,1,fp);
	fclose(fp);
	return 1;
}
/*
int Info(){
//	PVOID FileBuffer = NULL;
//	PVOID ImageBuffer = NULL;
//	PVOID NewBuffer = NULL;
//	int New_size;
//	char file_path[] = "C:/fg/fg.exe";
//	char write_file_path[] = "C:/fg/file.exe";
	//进行读文件大小.
	int ret1 = ReadFileBuffer(file_path,&FileBuffer);
	printf("ret1内存大小为%x\n",ret1);
	//进行filebuffer伸展，filebuffer->imagebuffer.
	int ret2 = CopyImageBuffer(FileBuffer,&ImageBuffer);
	printf("ret2内存大小为%x\n",ret2);
	//进行imagebuffer压缩，imagebuffer->newbuffer.
	New_size = CopyNewBuffer(ImageBuffer,&NewBuffer);
	printf("New_size的内存大小为%x\n",New_size);
	//进行文件保存，newbuffer-> xin.exe
	WriteFilePath(NewBuffer,New_size,write_file_path);
	return 0;
}
*/

DWORD ValueToAlignment(DWORD x,DWORD alignment){
	if(x < 0)
		return 0;
	if(x <= alignment){
		return alignment;
	}
	else{
		if(x % alignment != 0){
		return alignment * (x / alignment + 1);
		}
		else{
			return x;
		}
	}
	return 0;
}

//RVA---->FOA
DWORD RVAToFOA(PVOID FileBuffer,DWORD RVA){
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	int i;
	if(*(PWORD)FileBuffer != IMAGE_DOS_SIGNATURE){
		printf("不是有效MZ!");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*(PDWORD)((DWORD)pDosHeader + pDosHeader->e_lfanew) != IMAGE_NT_SIGNATURE){
		printf("不是有效PE!");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + IMAGE_SIZEOF_FILE_HEADER);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	if(RVA <= pOptionHeader->SizeOfHeaders){
		return RVA;
	}
	for(i = 0;i < pFileHeader->NumberOfSections;i++){
		if(RVA >= (pSectionHeader + i)->VirtualAddress && RVA < ((pSectionHeader + i)->VirtualAddress + (pSectionHeader + i)->Misc.VirtualSize)){
			return ((pSectionHeader + i)->PointerToRawData + (RVA - (pSectionHeader + i)->VirtualAddress));
		}
	}
	return 0;
}

int TestShellcode(){
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	PVOID FileBuffer = NULL;
	PVOID ImageBuffer = NULL;
	PVOID NewBuffer = NULL;
	size_t New_size = NULL;
	char file_path[] = "C:/fg/fg.exe";
	char write_file_path[] = "C:/fg/file.exe";
	//进行文件读取
	ReadFileBuffer(file_path,&FileBuffer);
	if(!FileBuffer){
		printf("文件读取失败！");
		return 0;
	}
	//进行filebuffer文件拉伸
	CopyImageBuffer(FileBuffer,&ImageBuffer);
	if(!ImageBuffer){
		printf("filebuffer内存拉伸失败！");
		free(FileBuffer);
		return 0;
	}
	if(*((PWORD)ImageBuffer) != IMAGE_DOS_SIGNATURE){
		printf("不为MZ");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)ImageBuffer;
	if(*((PDWORD)((DWORD)ImageBuffer + pDosHeader->e_lfanew)) != IMAGE_NT_SIGNATURE){
		printf("该文件不是有效pe文件！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + IMAGE_SIZEOF_FILE_HEADER);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	if((pSectionHeader->SizeOfRawData - pSectionHeader->Misc.VirtualSize) < ShellcodeLen){
		printf("代码空间区不够！");
		free(FileBuffer);
		free(ImageBuffer);
		return 0;
	}
	PBYTE CodeBegin = (PBYTE)((DWORD)ImageBuffer + pSectionHeader->VirtualAddress + pSectionHeader->Misc.VirtualSize);
	memcpy(CodeBegin,ShellCode,ShellcodeLen);
	printf("codebegin:%#x\n",CodeBegin);
	//E8修正
	DWORD CALLADDR = (MESSAGEBOXADDR - (pOptionHeader->ImageBase +((DWORD)(CodeBegin + 0xD) - (DWORD)ImageBuffer)));
	printf("CALLADDR:%#x\n",CALLADDR);
	*(PDWORD)(CodeBegin + 0x9) = CALLADDR;
	printf("CodeBegin+0x9:%#x\n",*(PDWORD)(CodeBegin+0x9));
	//E9修正
	DWORD JMPADDR = ((pOptionHeader->ImageBase + pOptionHeader->AddressOfEntryPoint) - (pOptionHeader->ImageBase + ((DWORD)(CodeBegin + ShellcodeLen) - (DWORD)ImageBuffer)));
	*(PDWORD)(CodeBegin + 0xE) = JMPADDR;
	//修改OEP
	pOptionHeader->AddressOfEntryPoint = (DWORD)CodeBegin - (DWORD)ImageBuffer;
	New_size = CopyNewBuffer(ImageBuffer,&NewBuffer);
	if(!New_size){
		printf("new_size无效！");
		return 0;
	}
	int y = WriteFilePath(NewBuffer,New_size,write_file_path);
	if(!y){
		printf("考盘失败！");
		return 0;
	}
	printf("考盘成功！");
	free(FileBuffer);
	free(ImageBuffer);
	free(NewBuffer);
	return 0;
}

//进行新增节区功能
int AddNewSection(PVOID FileBuffer,DWORD SizeofSection,DWORD file_size,PVOID* NewFileBuffer){
	PIMAGE_DOS_HEADER pDosHeader =NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	if(!FileBuffer){
		printf("文件读取失败！");
		return 0;
	}
	if(*((PWORD)FileBuffer) != IMAGE_DOS_SIGNATURE){
		printf("不为MZ!");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*((PDWORD)((DWORD)FileBuffer + pDosHeader->e_lfanew)) != IMAGE_NT_SIGNATURE){
		printf("不是有效的PE文件！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + IMAGE_SIZEOF_FILE_HEADER);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	//判断是否有80个空位可进行添加。
	if((pOptionHeader->SizeOfHeaders - (pDosHeader->e_lfanew + 4 + IMAGE_SIZEOF_FILE_HEADER + pFileHeader->SizeOfOptionalHeader + IMAGE_SIZEOF_SECTION_HEADER * pFileHeader->NumberOfSections)) < 80){
		printf("所剩空间不足80，无法添加节表！");
		return 0;
	}
	DWORD numSec = pFileHeader->NumberOfSections;
	//添加一个新的节
	memcpy((void*)(pSectionHeader + numSec),(void*)(pSectionHeader),sizeof(IMAGE_SECTION_HEADER));
	//在新增节后填充一个节大小的000
	memset((void*)(pSectionHeader + numSec + 1),0,sizeof(IMAGE_SECTION_HEADER));
	//修改PE头节的数量
	pFileHeader->NumberOfSections += 1;
	//修改sizeOfImage大小
	pOptionHeader->SizeOfImage += 0x1000;
	//修正新增节的属性
	//修改节表名字
	char* NewSecName = ".Song";
	memcpy((void*)(pSectionHeader + numSec),NewSecName,sizeof(NewSecName));
	//修改其他属性
	(pSectionHeader + numSec)->Misc.VirtualSize = 0x1000;
	//修改vadd
	(pSectionHeader + numSec)->VirtualAddress = (pSectionHeader + numSec - 1)->VirtualAddress + ValueToAlignment((pSectionHeader + numSec - 1)->Misc.VirtualSize,0x1000);
	//修改sizeofRa
	(pSectionHeader + numSec)->SizeOfRawData = 0x1000;
	//更新PointerToRa
	(pSectionHeader + numSec)->PointerToRawData = (pSectionHeader + numSec - 1)->PointerToRawData + (pSectionHeader + numSec - 1)->SizeOfRawData;
	//在原有数据的最后，新增一个节的数据
	//DWORD NewSize = file_size + SizeofSection;
	//PVOID pNewFileBuffer = malloc(NewSize);
	*NewFileBuffer = malloc(file_size + SizeofSection);
	//memset(pNewFileBuffer,0,NewSize);
	memcpy(*NewFileBuffer,FileBuffer,file_size);
	//*NewFileBuffer = pNewFileBuffer;
	return (pSectionHeader + numSec)->SizeOfRawData + (pSectionHeader + numSec)->PointerToRawData;
}

//进行新增节区注入shellcode
int AddNewSecShell(PVOID NewFileBuffer,DWORD file_size){
	PVOID NewBuffer = NULL;
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	if(*((PWORD)NewFileBuffer) != IMAGE_DOS_SIGNATURE){
		printf("不为MZ!");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)NewFileBuffer;
	if(*((PDWORD)((DWORD)pDosHeader + pDosHeader->e_lfanew)) != IMAGE_NT_SIGNATURE){
		printf("不是有效的PE!");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + 20);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	DWORD numSec = pFileHeader->NumberOfSections;
	if((pSectionHeader + numSec - 1)->Misc.VirtualSize < ShellcodeLen){
		printf("空间不足shellcode长度！");
		return 0;
	}
	PBYTE CodeBegin = (PBYTE)((DWORD)NewFileBuffer + (pSectionHeader + numSec - 1)->PointerToRawData);
	memcpy(CodeBegin,ShellCode,ShellcodeLen);
	DWORD rva_codeBegin = (pSectionHeader + numSec - 1)->VirtualAddress;
	char* Image_CodeBegin = (char*)((pSectionHeader + numSec - 1)->VirtualAddress + pOptionHeader->ImageBase);
	char* CALLADDR = (char*)Image_CodeBegin + 0x8;
	*(PDWORD)(CodeBegin + 0x9) = MESSAGEBOXADDR - (DWORD)CALLADDR - 5;
	char* JMPADDR = (char*)CALLADDR + 0X5;
	char* OEPADDR = (char*)(pOptionHeader->ImageBase + pOptionHeader->AddressOfEntryPoint);
	*(PDWORD)(CodeBegin + 0x9 + 0x5) = (DWORD)OEPADDR - (DWORD)JMPADDR - 5;
	pOptionHeader->AddressOfEntryPoint = rva_codeBegin;
	printf("SHELLCODE添加成功！\n");
	file_size += 0x1000;
	FILE *fp;
	fp = fopen(New_File_path,"wb");
	if(!fp){
		printf("new_file打开失败！");
		return 0;
	}
	fwrite(NewFileBuffer,file_size,1,fp);
	printf("考盘成功！");
	return 0;
}

//新添节中转站
int AddOneSec(){
	PVOID FileBuffer = NULL;
	PVOID NewFileBuffer = NULL;
	PVOID ImageBuffer = NULL;
	DWORD file_size = ReadFileBuffer(File_path,&FileBuffer);
	DWORD SizeofSection_Add = AddNewSection(FileBuffer,0x1000,file_size,&NewFileBuffer);
	printf("%#x\n",SizeofSection_Add);
	AddNewSecShell(NewFileBuffer,file_size);
	return 0;
}

//查询输出字典
int TestPrintDirectory(){
	PVOID FileBuffer = NULL;
	ReadFileBuffer(File_path,&FileBuffer);
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	if(*(PWORD)FileBuffer != IMAGE_DOS_SIGNATURE){
		printf("不是有效MZ！");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*(PDWORD)((DWORD)pDosHeader + pDosHeader->e_lfanew) != IMAGE_NT_SIGNATURE){
		printf("不是有效PE文件！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + 20);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	PIMAGE_DATA_DIRECTORY pDirectoryAddr = pOptionHeader->DataDirectory;
	printf("导出表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("导入表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("资源表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("异常信息表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("安全证书表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("重定位表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("调试信息表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("版权所以表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("全局指针表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("TLS表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("加载配置表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("绑定导入表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("IAT表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("延迟导入表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("COM信息表地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	printf("保留地址：%#x\t\t大小：%#x\n",pDirectoryAddr->VirtualAddress,pDirectoryAddr->Size);
	pDirectoryAddr++;
	return 0;
}

//打印导出表信息
int PrintExport(PVOID FileBuffer){
	if(!FileBuffer){
		printf("文件读取失败！");
		return 0;
	}
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	DWORD i;
	if(*(PWORD)FileBuffer != IMAGE_DOS_SIGNATURE){
		printf("不是有效MZ！");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*(PDWORD)((DWORD)pDosHeader + pDosHeader->e_lfanew) != IMAGE_NT_SIGNATURE){
		printf("不是有效PE文件！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + 20);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	PIMAGE_DATA_DIRECTORY pDirectory = (PIMAGE_DATA_DIRECTORY)(pOptionHeader->DataDirectory);
	if(!pDirectory->VirtualAddress){
		printf("该程序没有导出表！");
		return 0;
	}
	printf("导出表RVA：%#x\n",pDirectory->VirtualAddress);
	DWORD ExportFOA = RVAToFOA(FileBuffer,pOptionHeader->DataDirectory[0].VirtualAddress); 
	if(ExportFOA){
		printf("导出表FOA：%#x\n",ExportFOA);
	}
	PIMAGE_EXPORT_DIRECTORY ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(ExportFOA + (DWORD)FileBuffer);
	printf("\n\n---------------------打印导出表结构---------------------\n\n");
	//printf("文件名为：%s\n",ExportDirectory->Name);
	printf("Characteristics:%x\n",ExportDirectory->Characteristics);
	printf("TimeDateStamp:%x\n",ExportDirectory->TimeDateStamp);
	printf("Name:%x\n",ExportDirectory->Name);
	printf("Name:%s\n",((DWORD)FileBuffer + ExportDirectory->Name));
	printf("Base:%x\n",ExportDirectory->Base);
	printf("NumberOfFunctions:%x\n",ExportDirectory->NumberOfFunctions);
	printf("NumberOfNames:%x\n",ExportDirectory->NumberOfNames);
/*	printf("\n\n---------------------打印导出表函数地址------------------\n\n");
	printf("AddressOfFunctions:%x\n",RVAToFOA(FileBuffer,ExportDirectory->AddressOfFunctions));
	printf("AddressOfNames:%x\n",RVAToFOA(FileBuffer,ExportDirectory->AddressOfNames));
	printf("AddressOfNameOrdinals:%x\n",RVAToFOA(FileBuffer,ExportDirectory->AddressOfNameOrdinals));*/
	printf("\n\n---------------------打印函数地址表------------------\n\n");
	DWORD FOA_AddressOfFunctions = RVAToFOA(FileBuffer,ExportDirectory->AddressOfFunctions);
	printf("导出函数地址表RVA:%#x\t\t,FOA:%#x\n",ExportDirectory->AddressOfFunctions,FOA_AddressOfFunctions);
	DWORD* AddressFunctions = (DWORD*)(FOA_AddressOfFunctions + (DWORD)FileBuffer);
	for(i = 0;i < ExportDirectory->NumberOfFunctions;i++){
		printf("函数地址[%d]RVA:%#x\t\t函数地址[%d]FOA:%#x\n",i,AddressFunctions[i],i,RVAToFOA(FileBuffer,AddressFunctions[i]));
	}
	printf("\n\n---------------------打印函数名称表------------------\n\n");
	DWORD FOA_AddressOfNames = RVAToFOA(FileBuffer,ExportDirectory->AddressOfNames);
	printf("导出函数名称表RVA:%#x\t\t,FOA:%#x\n",ExportDirectory->AddressOfNames,FOA_AddressOfNames);
	DWORD* AddressNames = (DWORD*)(FOA_AddressOfNames + (DWORD)FileBuffer);
	for(i = 0;i < ExportDirectory->NumberOfNames;i++){
		DWORD FOA_AddressNames_Functions = RVAToFOA(FileBuffer,AddressNames[i]);
		char* AddressNames_Name = (char*)(FOA_AddressNames_Functions + (DWORD)FileBuffer);
		printf("名称表[%d]RVA:%#x\t\t,名称表[%d]FOA:%#x\t\t,函数名字:%s\n",i,AddressNames[i],i,FOA_AddressNames_Functions,AddressNames_Name);
	}
	printf("\n\n---------------------打印函数序号表------------------\n\n");
	DWORD FOA_AddressOfNameOrdinals = RVAToFOA(FileBuffer,ExportDirectory->AddressOfNameOrdinals);
	printf("函数序号表RVA:%#x\t\t,FOA:%#x\n",ExportDirectory->AddressOfNameOrdinals,FOA_AddressOfNameOrdinals);
	WORD* AddressOfNameOrdinals = (WORD*)(FOA_AddressOfNameOrdinals + (DWORD)FileBuffer);
	for(i = 0;i < ExportDirectory->NumberOfNames;i++){
		printf("序号表[%d]:%#x(%d)\n",i,AddressOfNameOrdinals[i],AddressOfNameOrdinals[i]);
	}
	return 0;
}

//利用函数名获取函数地址
int GetFunctionAddrByName(PVOID FileBuffer,const char* Name){
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	PIMAGE_DATA_DIRECTORY pDirectory = NULL;
	DWORD i;
	if(!FileBuffer){
		printf("文件读取失败！");
		return 0;
	}
	if(*(PWORD)FileBuffer != IMAGE_DOS_SIGNATURE){
		printf("不是有效的MZ!");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*(PDWORD)((DWORD)pDosHeader + pDosHeader->e_lfanew) != IMAGE_NT_SIGNATURE){
		printf("不是有效的PE文件！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + 20);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	pDirectory = (PIMAGE_DATA_DIRECTORY)((DWORD)pOptionHeader->DataDirectory);
	DWORD pExportDir = RVAToFOA(FileBuffer,pDirectory->VirtualAddress);
	PIMAGE_EXPORT_DIRECTORY pExportFOA = (PIMAGE_EXPORT_DIRECTORY)((DWORD)FileBuffer + pExportDir);
	//导出表名字地址
	DWORD FOA_AddressName = RVAToFOA(FileBuffer,pExportFOA->AddressOfNames);
	DWORD* AddressName = (DWORD*)((DWORD)FileBuffer + FOA_AddressName);
	DWORD ret = -1;
	for(i = 0;i < pExportFOA->NumberOfNames;i++){
		DWORD FOA_AddressName_Fun = RVAToFOA(FileBuffer,AddressName[i]);
		char* AddressN = (char*)(FOA_AddressName_Fun + (DWORD)FileBuffer);
		if(memcmp(Name,(char*)AddressN,strlen(Name)) == 0){
			printf("导出表RVA:%#x\t\tFOA:%#x\n",pDirectory->VirtualAddress,pExportFOA);
			printf("找到了！\n名字表[%d]RVA:%#x\t\t,名字表[%d]FOA:%#x\t\t函数名字：%s\n",i,AddressName[i],
				i,RVAToFOA(FileBuffer,AddressName[i]),(char*)AddressN);
			ret = i;
		}
	}
	if(ret == -1){
		printf("未找到此函数名！");
		return 0;
	}
	DWORD FOA_AddressNameOrdinals = RVAToFOA(FileBuffer,pExportFOA->AddressOfNameOrdinals);
	WORD ret2 = ((WORD*)(FOA_AddressNameOrdinals + (DWORD)FileBuffer))[ret];
	DWORD FOA_AddressFunctions = RVAToFOA(FileBuffer,pExportFOA->AddressOfFunctions);
	DWORD RVA_FunName = ((DWORD*)(FOA_AddressFunctions + (DWORD)FileBuffer))[ret2];
	DWORD FOA_FunName = RVAToFOA(FileBuffer,RVA_FunName);
	printf("此函数在AddressOrdinals表中的下标是：%d\n",ret);
	printf("此函数在AddressFunctions表中的下标是：%d\n",ret2);
	printf("此函数RVA:%#x\t\tFOA:%#x\n",RVA_FunName,FOA_FunName);
	return RVA_FunName + pOptionHeader->ImageBase;
}

//利用函数序号获取函数地址
int GetFunctionAddrByOrdinals(PVOID FileBuffer,int Ordinals){
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	PIMAGE_DATA_DIRECTORY pDirectory = NULL;
	if(!FileBuffer){
		printf("文件读取失败！");
		return 0;
	}
	if(*(PWORD)FileBuffer != IMAGE_DOS_SIGNATURE){
		printf("不是有效的MZ!");
		return 0;
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*(PDWORD)((DWORD)pDosHeader + pDosHeader->e_lfanew) != IMAGE_NT_SIGNATURE){
		printf("不是有效的PE文件！");
		return 0;
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + 20);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	pDirectory = (PIMAGE_DATA_DIRECTORY)((DWORD)pOptionHeader->DataDirectory);
	DWORD pExportDir = RVAToFOA(FileBuffer,pDirectory->VirtualAddress);
	PIMAGE_EXPORT_DIRECTORY pExportFOA = (PIMAGE_EXPORT_DIRECTORY)((DWORD)FileBuffer + pExportDir);
	int a = Ordinals - pExportFOA->Base;
	DWORD FOA_AddressFunctions = RVAToFOA(FileBuffer,pExportFOA->AddressOfFunctions);
	DWORD RVA_AddressName = ((DWORD*)((DWORD)FileBuffer + FOA_AddressFunctions))[a];
	DWORD FOA_AddressName = RVAToFOA(FileBuffer,RVA_AddressName);
	printf("对应RVA为:%#x\t\t,FOA:%#x\n",RVA_AddressName,FOA_AddressName);
	return RVA_AddressName + pOptionHeader->ImageBase;
}

//打印重定位表数据信息
void PrintBaseRelocation(PVOID FileBuffer){
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_OPTIONAL_HEADER pOptionHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	PIMAGE_DATA_DIRECTORY RVA_pDirectory = NULL;
	if(!FileBuffer){
		printf("文件读取失败！");
		exit(0);
	}
	if(*(PWORD)FileBuffer != IMAGE_DOS_SIGNATURE){
		printf("不是有效MZ！");
		exit(0);
	}
	pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
	if(*(PDWORD)((DWORD)pDosHeader + pDosHeader->e_lfanew) != IMAGE_NT_SIGNATURE){
		printf("不是有效PE文件！");
		exit(0);
	}
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)pDosHeader + pDosHeader->e_lfanew);
	pFileHeader = (PIMAGE_FILE_HEADER)((DWORD)pNtHeader + 4);
	pOptionHeader = (PIMAGE_OPTIONAL_HEADER)((DWORD)pFileHeader + 20);
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pOptionHeader + pFileHeader->SizeOfOptionalHeader);
	RVA_pDirectory = (PIMAGE_DATA_DIRECTORY)((DWORD)FileBuffer + pOptionHeader->DataDirectory);
	DWORD FOA_BaseRelocation = RVAToFOA(FileBuffer,pNtHeader->OptionalHeader.DataDirectory[5].VirtualAddress);
	PIMAGE_BASE_RELOCATION BaseRelocation = (PIMAGE_BASE_RELOCATION)((DWORD)FileBuffer + FOA_BaseRelocation);
	while(BaseRelocation->VirtualAddress != NULL || BaseRelocation->SizeOfBlock != NULL){
		for(DWORD i = 0;i < (BaseRelocation->SizeOfBlock - 8) / 2;i++){
			WORD high4 = *((PWORD)((DWORD)BaseRelocation + 8) + i) >> 12;
			WORD low12 = *((PWORD)((DWORD)BaseRelocation + 8) + i) & 0x0FFF;
			printf("high4:%d\t\tlow:%d\n",high4,low12);
			printf("%d:%d = %x = %x\n",i,high4,BaseRelocation->VirtualAddress + low12,RVAToFOA(FileBuffer,BaseRelocation->VirtualAddress + low12));
		}
		printf("=======================");
		BaseRelocation = (PIMAGE_BASE_RELOCATION)((DWORD)BaseRelocation + BaseRelocation->SizeOfBlock);
	}
}

//查询导出表
int TestPrintExport(){
	PVOID FileBuffer = NULL;
	ReadFileBuffer(File_path,&FileBuffer);
	PrintExport(FileBuffer);
	return 0;
}

//利用函数名、函数序号获取函数地址
int FunctionAddrNaOr(){
	int x;
	PVOID FileBuffer = NULL;
	ReadFileBuffer(File_path,&FileBuffer);
	printf("请选择您要进行的功能：\n");
	printf("1.利用函数名获取函数地址\n");
	printf("2.利用函数序号获取函数地址\n");
	scanf("%d",&x);
	switch(x){
		case 1:
			printf("以函数名字找Div函数地址：%#x\n",GetFunctionAddrByName(FileBuffer,"Div"));
			getchar();
			break;
		case 2:
			int Ordinals = 3;
			printf("以函数序号找3函数地址：%#x\n",GetFunctionAddrByOrdinals(FileBuffer,Ordinals));
			getchar();
			break;
	}
	return 0;
}

int TestPrintBaseRelocation(){
	PVOID FileBuffer = NULL;
	ReadFileBuffer(File_path,&FileBuffer);
	PrintBaseRelocation(FileBuffer);
	return 0;
}

int main(int argc, char* argv[])
{
	//Info();
	//进行shellcode填充
	//TestShellcode();
	//新增节区
	//AddOneSec();
	//查询目录
	//TestPrintDirectory();
	//查询导出表
	//TestPrintExport();
	//利用函数名、函数序号获取函数地址
	//FunctionAddrNaOr();
	//打印重定位表数据信息
	TestPrintBaseRelocation();
	getchar();
	return 0;
}

