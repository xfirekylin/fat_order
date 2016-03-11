#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>
#include <winioctl.h>

// 以CreateFile方式读取扇区开关
#define SECTOR 

#ifdef SECTOR
//drv 是盘符，一个字符，替换字符串“\\.\A:”中的字符A
//startSector 是开始扇区号
//sectors 扇区数量
//lpSectBuff 缓冲区
BOOL ReadSectors(char drv, DWORD startSector, WORD sectors, LPBYTE lpSectBuff)
{
	char devName[] = "\\\\.\\A:";
	devName[4] =drv;
	
	//打开指定分区
	HANDLE hDev=CreateFile(devName, GENERIC_READ, FILE_SHARE_WRITE,NULL, OPEN_EXISTING, 0, NULL);
	if(hDev==INVALID_HANDLE_VALUE) 
		return 0;
		
	//将文件指针指向起始扇区
	SetFilePointer(hDev,512*startSector,0,FILE_BEGIN);
	DWORD dwCB;
	
	//读取指定数量的扇区
	BOOL bRet=ReadFile(hDev,lpSectBuff,512*sectors,&dwCB,NULL);
	CloseHandle(hDev);
	return bRet;
} 

BOOL WriteSectors( char drv, DWORD startSector, WORD sectors, LPBYTE lpSectBuff )
{
	char devName[] = "\\\\.\\A:";
	devName[4] =drv;
	HANDLE hDev=CreateFile(devName, GENERIC_WRITE, FILE_SHARE_WRITE,NULL, OPEN_EXISTING, 0, NULL);
	if(hDev==INVALID_HANDLE_VALUE) 
		return 0;
	SetFilePointer(hDev,512*startSector,0,FILE_BEGIN);
	DWORD dwCB;
	BOOL bRet=WriteFile(hDev,lpSectBuff,512*sectors,&dwCB,NULL);
	CloseHandle(hDev);
	return bRet;

}

VOID Display_sector_data(LONGLONG start_addr, ULONG dwBytes, LPBYTE lpBuffer)
{
    for (ULONG nOffset = 0; nOffset < dwBytes; nOffset += 0x10) {
        
        ULONG nBytes, nIdx;
        
        //
        // 显示地址
        //
        _tprintf(_T("%011I64x "), (start_addr) + nOffset);
        
        //
        // 显示16进制数据
        //
        nBytes = min(0x10, dwBytes - nOffset);
        
        for (nIdx = 0; nIdx < nBytes; nIdx++) {
         _tprintf(_T("%02x %s"), ((PUCHAR)lpBuffer)[nOffset + nIdx], ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        for ( ; nIdx < 0x10; nIdx++) {
         _tprintf(_T(" %s"), ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        //
        // 显示ascii格式数据
        //
        for (nIdx = 0; nIdx < nBytes; nIdx++) {
         _tprintf(_T("%c"), isprint(((PUCHAR)lpBuffer)[nOffset + nIdx]) ? ((PUCHAR)lpBuffer)[nOffset + nIdx] : _T('.'));
        }
        
        _tprintf(_T("\n"));
   }
}

typedef struct {
}FAT_DBP;
VOID __cdecl _tmain (INT Argc, PTCHAR Argv[])
{
    char drv = 'G';
    DWORD startSector = _ttoi(Argv[1]);
    DWORD sectors = 1;
    DWORD dwSize = sectors * 512;
    
    PVOID lpBuffer = new BYTE [dwSize];
    
    if (ReadSectors(drv, startSector, sectors, (LPBYTE)lpBuffer))
    {
        Display_sector_data(startSector * 512, dwSize, (LPBYTE)lpBuffer);
    }

    delete [] lpBuffer;
}

VOID __cdecl _tmain2 (INT Argc, PTCHAR Argv[])
{
    TCHAR szName[MAX_PATH] = { 0 };
    HANDLE hDisk;
 
    //
    // 命令行参数可在Project Settings->Debug->Program arguments下指定 如：0 1
    //
    if (Argc != 3) {
        _tprintf(_T("Reads a sector on the disk/n/n"));
        _tprintf(_T("%s [disk number] [sector]/n"), Argv[0]);
        return;
    }
 
    _sntprintf(szName, sizeof(szName) / sizeof(szName[0]) - 1,
                _T("\\\\.\\Physicaldrive%d"), _ttoi(Argv[1]));
 
    //
    // 打开磁盘
    //
    hDisk = CreateFile(szName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            NULL,
            0);
 
    if (hDisk != INVALID_HANDLE_VALUE) 
    {
  
        // 磁盘的结构信息存在此结束中。
        /*
        typedef struct _DISK_GEOMETRY 
        {
        LARGE_INTEGER Cylinders;    // 柱面数
        MEDIA_TYPE MediaType;       // 磁盘类型，见MSDN
        DWORD TracksPerCylinder;    // 每道柱面数
        DWORD SectorsPerTrack;      // 每道扇区数
        DWORD BytesPerSector;       // 每扇区字节数
        } DISK_GEOMETRY;
        */
  
  DISK_GEOMETRY diskGeometry;
  
  
  DWORD dwBytes = 0;
  
  //
  // 获得磁盘结构信息
  //
  if (DeviceIoControl(hDisk,
       IOCTL_DISK_GET_DRIVE_GEOMETRY,    // 调用了CTL_CODE macro宏
       NULL,
       0,
       &diskGeometry,
       sizeof(DISK_GEOMETRY),
       &dwBytes,
       NULL))
  {
   
   DWORD dwSize = diskGeometry.BytesPerSector;    // 每sector字节数
   PVOID lpBuffer = new BYTE [dwSize];
   
   if (lpBuffer) 
   {
    /*
                    typedef struct _PARTITION_INFORMATION
                    {
                    LARGE_INTEGER StartingOffset;     // 启动分区偏移
                    LARGE_INTEGER PartitionLength;    // 分区长度(字节)
                    DWORD HiddenSectors;              // 分区中隐藏扇区数
                    DWORD PartitionNumber;            // 分区数
                    BYTE PartitionType;               // 分区类型
                    BOOLEAN BootIndicator;            // 是否为引导分区，TRUE是
                    BOOLEAN RecognizedPartition;      // 验证过的分区。TRUE是
                    BOOLEAN RewritePartition;         // 分区是否可改变。TRUE
                    } PARTITION_INFORMATION, *PPARTITION_INFORMATION;
            */
    // 提供关于磁盘分区的信息
    PARTITION_INFORMATION partitionInfo;
    
    //
    // 获得磁盘大小
    //
    if (DeviceIoControl(hDisk,
     IOCTL_DISK_GET_PARTITION_INFO,
     NULL,
     0,
     &partitionInfo,
     sizeof(PARTITION_INFORMATION),
     &dwBytes,
     NULL)) 
    {
     
     // 获取总扇区数
     LONGLONG sectorCount = partitionInfo.PartitionLength.QuadPart / diskGeometry.BytesPerSector;
     
     _tprintf ( _T("磁盘空间为 %.2fGB 每扇区 %ld字节 共%ld个扇区/r\n"), 
      partitionInfo.PartitionLength.QuadPart/1024./1024./1024.    //  磁盘空间
      , diskGeometry.BytesPerSector                               //  每扇区字节数
      , partitionInfo.PartitionLength.QuadPart / diskGeometry.BytesPerSector);  // 总扇区数
     
     LONGLONG nIndex = _ttoi64(Argv[2]);
     
     // 以16进制输出
     _tprintf(_T("Disk %d has 0x%I64x sectors with 0x%x bytes in every sector\n"), _ttoi(Argv[1]), sectorCount, diskGeometry.BytesPerSector);
     
     //
     // 读取被请求的sector
     //
     if (nIndex < sectorCount) {
      
      // 有符号的64位整型表示
      LARGE_INTEGER offset;
      
      // sector数所占字节
      offset.QuadPart = (nIndex) * diskGeometry.BytesPerSector;
      
      // 从打开的文件（磁盘）中移动文件指针。offset.LowPart低32位为移动字节数
      SetFilePointer(hDisk, offset.LowPart, &offset.HighPart, FILE_BEGIN);
      
      // 读取扇区的数据
      if (ReadFile(hDisk, lpBuffer, dwSize, &dwBytes, NULL)) 
      {
       
       //
       // The dwBytes field holds the number of bytes that were actually read [ <= dwSize ]
       //
       for (ULONG nOffset = 0; nOffset < dwBytes; nOffset += 0x10) {
        
        ULONG nBytes, nIdx;
        
        //
        // 显示地址
        //
        _tprintf(_T("%011I64x "), (offset.QuadPart) + nOffset);
        
        //
        // 显示16进制数据
        //
        nBytes = min(0x10, dwBytes - nOffset);
        
        for (nIdx = 0; nIdx < nBytes; nIdx++) {
         _tprintf(_T("%02x %s"), ((PUCHAR)lpBuffer)[nOffset + nIdx], ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        for ( ; nIdx < 0x10; nIdx++) {
         _tprintf(_T(" %s"), ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        //
        // 显示ascii格式数据
        //
        for (nIdx = 0; nIdx < nBytes; nIdx++) {
         _tprintf(_T("%c"), isprint(((PUCHAR)lpBuffer)[nOffset + nIdx]) ? ((PUCHAR)lpBuffer)[nOffset + nIdx] : _T('.'));
        }
        
        _tprintf(_T("\n"));
       }
       
      } // end ReadFile
      else
      {
       _tprintf(_T("ReadFile() on sector 0x%I64x failed with error code: %d\n"), nIndex, GetLastError());
      }
      
     } // end if (nIndex < sectorCount) 
     else
     {
      _tprintf(_T("The requested sector is out-of-bounds\n"));
     }
     
    } // end 1 if (DeviceIoControl   
    else
    {
     _tprintf(_T("IOCTL_DISK_GET_PARTITION_INFO failed with error code %d\n"), GetLastError());
    }
    
    delete [] lpBuffer;
    
   } // if (lpBuffer) 
   else
   {
    _tprintf(_T("Unable to allocate resources, exiting\n"));
   }
   
  } // end 2 if (DeviceIoControl
  else
  {
   _tprintf(_T("IOCTL_DISK_GET_DRIVE_GEOMETRY failed with error code %d\n"), GetLastError());
  }
  
  CloseHandle(hDisk);
  
 } // if (hDisk != INVALID_HANDLE_VALUE) 
 else
 {
  _tprintf(_T("CreateFile() on %s failed with error code %d\n"), szName, GetLastError());
 }

_tprintf(_T("\n"));

return;
}

#else

/* -------------------------------------------------------------------------- *
* 
* 
*    2    _getdiskfree显示磁盘扇区信息
* 
*
* -------------------------------------------------------------------------- */
// crt_getdiskfree.c
#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <tchar.h>

TCHAR   g_szBorder[] = _T("======================================================================\n");
TCHAR   g_szTitle1[] = _T("|DRIVE|TOTAL CLUSTERS|AVAIL CLUSTERS|SECTORS / CLUSTER|BYTES / SECTOR|\n");
TCHAR   g_szTitle2[] = _T("|=====|==============|==============|=================|==============|\n");
TCHAR   g_szLine[]   = _T("|  A: |              |              |                 |              |\n");

void utoiRightJustified(TCHAR* szLeft, TCHAR* szRight, unsigned uVal);

int main(int argc, char* argv[]) {
 TCHAR szMsg[4200];
 struct _diskfree_t df = {0};
 ULONG uDriveMask = _getdrives();   // 表示当前可用的磁盘
 unsigned uErr, uLen, uDrive;
 
 printf(g_szBorder);
 printf(g_szTitle1);
 printf(g_szTitle2);
 
 int ifor = 0;
 for (uDrive=1; uDrive<=26; ++uDrive) 
 {
  if (uDriveMask & 1) 
  {
   uErr = _getdiskfree(uDrive, &df);
   memcpy(szMsg, g_szLine, sizeof(g_szLine));
   szMsg[3] = uDrive + 'A' - 1;
   char lp[5] = "C://";
   lp [0] = uDrive + 'A' - 1;
  
   if (uErr == 0) {
    utoiRightJustified(szMsg+8,  szMsg+19, df.total_clusters);
    utoiRightJustified(szMsg+23, szMsg+34, df.avail_clusters);
    utoiRightJustified(szMsg+38, szMsg+52, df.sectors_per_cluster);
    utoiRightJustified(szMsg+56, szMsg+67, df.bytes_per_sector);

    ifor ++;
   }
   else {
    uLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
     uErr, 0, szMsg+8, 4100, NULL);
    szMsg[uLen+6] = ' ';
    szMsg[uLen+7] = ' ';
    szMsg[uLen+8] = ' ';
    ifor ++;
   }
   
   printf(szMsg);
  }
  
  uDriveMask >>= 1;
  if (!uDriveMask)
   break;
 }
 
 printf(g_szBorder);
 printf ("共%d个", ifor);
 return 0;
}

void utoiRightJustified(TCHAR* szLeft, TCHAR* szRight, unsigned uVal) {
 TCHAR* szCur = szRight;
 int nComma = 0;
 
 if (uVal) {
  while (uVal && (szCur >= szLeft)) {
   if   (nComma == 3) {
    *szCur = ',';
    nComma = 0;
   }
   else {
    *szCur = (uVal % 10) | 0x30;
    uVal /= 10;
    ++nComma;
   }
   
   --szCur;
  }
 }
 else {
  *szCur = '0';
  --szCur;
 }
 
 if (uVal) {
  szCur = szLeft;
  
  while   (szCur <= szRight) {
   *szCur = '*';
   ++szCur;
  }
 }
}
#endif

