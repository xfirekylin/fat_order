#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>
#include <winioctl.h>

// ��CreateFile��ʽ��ȡ��������
#define SECTOR 

#ifdef SECTOR
//drv ���̷���һ���ַ����滻�ַ�����\\.\A:���е��ַ�A
//startSector �ǿ�ʼ������
//sectors ��������
//lpSectBuff ������
BOOL ReadSectors(char drv, DWORD startSector, WORD sectors, LPBYTE lpSectBuff)
{
	char devName[] = "\\\\.\\A:";
	devName[4] =drv;
	
	//��ָ������
	HANDLE hDev=CreateFile(devName, GENERIC_READ, FILE_SHARE_WRITE,NULL, OPEN_EXISTING, 0, NULL);
	if(hDev==INVALID_HANDLE_VALUE) 
		return 0;
		
	//���ļ�ָ��ָ����ʼ����
	SetFilePointer(hDev,512*startSector,0,FILE_BEGIN);
	DWORD dwCB;
	
	//��ȡָ������������
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
        // ��ʾ��ַ
        //
        _tprintf(_T("%011I64x "), (start_addr) + nOffset);
        
        //
        // ��ʾ16��������
        //
        nBytes = min(0x10, dwBytes - nOffset);
        
        for (nIdx = 0; nIdx < nBytes; nIdx++) {
         _tprintf(_T("%02x %s"), ((PUCHAR)lpBuffer)[nOffset + nIdx], ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        for ( ; nIdx < 0x10; nIdx++) {
         _tprintf(_T(" %s"), ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        //
        // ��ʾascii��ʽ����
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
    // �����в�������Project Settings->Debug->Program arguments��ָ�� �磺0 1
    //
    if (Argc != 3) {
        _tprintf(_T("Reads a sector on the disk/n/n"));
        _tprintf(_T("%s [disk number] [sector]/n"), Argv[0]);
        return;
    }
 
    _sntprintf(szName, sizeof(szName) / sizeof(szName[0]) - 1,
                _T("\\\\.\\Physicaldrive%d"), _ttoi(Argv[1]));
 
    //
    // �򿪴���
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
  
        // ���̵Ľṹ��Ϣ���ڴ˽����С�
        /*
        typedef struct _DISK_GEOMETRY 
        {
        LARGE_INTEGER Cylinders;    // ������
        MEDIA_TYPE MediaType;       // �������ͣ���MSDN
        DWORD TracksPerCylinder;    // ÿ��������
        DWORD SectorsPerTrack;      // ÿ��������
        DWORD BytesPerSector;       // ÿ�����ֽ���
        } DISK_GEOMETRY;
        */
  
  DISK_GEOMETRY diskGeometry;
  
  
  DWORD dwBytes = 0;
  
  //
  // ��ô��̽ṹ��Ϣ
  //
  if (DeviceIoControl(hDisk,
       IOCTL_DISK_GET_DRIVE_GEOMETRY,    // ������CTL_CODE macro��
       NULL,
       0,
       &diskGeometry,
       sizeof(DISK_GEOMETRY),
       &dwBytes,
       NULL))
  {
   
   DWORD dwSize = diskGeometry.BytesPerSector;    // ÿsector�ֽ���
   PVOID lpBuffer = new BYTE [dwSize];
   
   if (lpBuffer) 
   {
    /*
                    typedef struct _PARTITION_INFORMATION
                    {
                    LARGE_INTEGER StartingOffset;     // ��������ƫ��
                    LARGE_INTEGER PartitionLength;    // ��������(�ֽ�)
                    DWORD HiddenSectors;              // ����������������
                    DWORD PartitionNumber;            // ������
                    BYTE PartitionType;               // ��������
                    BOOLEAN BootIndicator;            // �Ƿ�Ϊ����������TRUE��
                    BOOLEAN RecognizedPartition;      // ��֤���ķ�����TRUE��
                    BOOLEAN RewritePartition;         // �����Ƿ�ɸı䡣TRUE
                    } PARTITION_INFORMATION, *PPARTITION_INFORMATION;
            */
    // �ṩ���ڴ��̷�������Ϣ
    PARTITION_INFORMATION partitionInfo;
    
    //
    // ��ô��̴�С
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
     
     // ��ȡ��������
     LONGLONG sectorCount = partitionInfo.PartitionLength.QuadPart / diskGeometry.BytesPerSector;
     
     _tprintf ( _T("���̿ռ�Ϊ %.2fGB ÿ���� %ld�ֽ� ��%ld������/r\n"), 
      partitionInfo.PartitionLength.QuadPart/1024./1024./1024.    //  ���̿ռ�
      , diskGeometry.BytesPerSector                               //  ÿ�����ֽ���
      , partitionInfo.PartitionLength.QuadPart / diskGeometry.BytesPerSector);  // ��������
     
     LONGLONG nIndex = _ttoi64(Argv[2]);
     
     // ��16�������
     _tprintf(_T("Disk %d has 0x%I64x sectors with 0x%x bytes in every sector\n"), _ttoi(Argv[1]), sectorCount, diskGeometry.BytesPerSector);
     
     //
     // ��ȡ�������sector
     //
     if (nIndex < sectorCount) {
      
      // �з��ŵ�64λ���ͱ�ʾ
      LARGE_INTEGER offset;
      
      // sector����ռ�ֽ�
      offset.QuadPart = (nIndex) * diskGeometry.BytesPerSector;
      
      // �Ӵ򿪵��ļ������̣����ƶ��ļ�ָ�롣offset.LowPart��32λΪ�ƶ��ֽ���
      SetFilePointer(hDisk, offset.LowPart, &offset.HighPart, FILE_BEGIN);
      
      // ��ȡ����������
      if (ReadFile(hDisk, lpBuffer, dwSize, &dwBytes, NULL)) 
      {
       
       //
       // The dwBytes field holds the number of bytes that were actually read [ <= dwSize ]
       //
       for (ULONG nOffset = 0; nOffset < dwBytes; nOffset += 0x10) {
        
        ULONG nBytes, nIdx;
        
        //
        // ��ʾ��ַ
        //
        _tprintf(_T("%011I64x "), (offset.QuadPart) + nOffset);
        
        //
        // ��ʾ16��������
        //
        nBytes = min(0x10, dwBytes - nOffset);
        
        for (nIdx = 0; nIdx < nBytes; nIdx++) {
         _tprintf(_T("%02x %s"), ((PUCHAR)lpBuffer)[nOffset + nIdx], ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        for ( ; nIdx < 0x10; nIdx++) {
         _tprintf(_T(" %s"), ((nIdx + 1) % 0x8) ? _T("") : _T(" "));
        }
        
        //
        // ��ʾascii��ʽ����
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
*    2    _getdiskfree��ʾ����������Ϣ
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
 ULONG uDriveMask = _getdrives();   // ��ʾ��ǰ���õĴ���
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
 printf ("��%d��", ifor);
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

