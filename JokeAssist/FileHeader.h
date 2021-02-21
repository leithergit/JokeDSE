#include <stdafx.h>
#include "Utility.h"
#pragma pack(push)
#pragma pack(1)
struct FilebufHeader
{
	byte szHeader[4];
	WORD nNameSize;
	UINT nFileSize;
	UINT nFoundFiles;
	UINT nFilterFiles;
	UINT nDecryptFiles;
	UINT nCRC32;
	FilebufHeader(CHAR *szFilename, int nFileSize,int nFoundFiles,int nSendFiles,int nFilteredFils)
	{
		ZeroMemory(this, sizeof(FilebufHeader));
		szHeader[0] = szHeader[1] = szHeader[2] = szHeader[3] = 0xFF;
		this->nNameSize = strlen(szFilename);
		this->nFileSize = nFileSize;
		this->nFoundFiles = nFoundFiles;
		this->nFilterFiles = nFilteredFils;
		nDecryptFiles = nSendFiles;
		nCRC32 = CRC32((byte *)this, sizeof(FilebufHeader) - sizeof(UINT));
	}
	inline bool CheckPreFix()
	{
		return (szHeader[0] == 0xFF &&
			szHeader[1] == 0xFF &&
			szHeader[2] == 0xFF &&
			szHeader[3] == 0xFF);
	}
	inline bool CheckCRC()
	{
		return  nCRC32 == CRC32((byte *)this, sizeof(FilebufHeader) - sizeof(UINT));
	}

	inline int PackSize()
	{
		return (sizeof(FilebufHeader) + nNameSize + nFileSize);
	}
	inline int HeaderSize()
	{
		return (sizeof(FilebufHeader) + nNameSize );
	}
	const char* GetFileName()
	{
		// 错误写法，这样写会按当前结构体尺寸为步长(31字节)计算地址偏移
		// return (char *)(this + sizeof(FilebufHeader));
		// 正确的写法，要以字节尺寸为步长（1字节）计算地址偏移
		return ((char *)this) + sizeof(FilebufHeader);
	}

	const char* GetFileStream()
	{
		return ((char *)this) + sizeof(FilebufHeader) + nNameSize;
	}
};

struct FileFrame
{
	byte	szHeader[4];
	byte	nBufferSize;
	int		nFrameID;
	LONGLONG nOffset;
	UINT	nCRC32;
	FileFrame(int nSize,int nFrameID,LONGLONG nOffset)
	{
		ZeroMemory(this, sizeof(FilebufHeader));
		szHeader[0] = szHeader[1] = szHeader[2] = szHeader[3] = 0xFF;
		this->nBufferSize = nSize;
		this->nFrameID = nFrameID;
		this->nOffset = nOffset;
		nCRC32 = CRC32((byte *)this, sizeof(FilebufHeader) - sizeof(UINT));
	}

	inline bool CheckPreFix()
	{
		return (szHeader[0] == 0xFF &&
			szHeader[1] == 0xFF &&
			szHeader[2] == 0xFF &&
			szHeader[3] == 0xFF);
	}
	inline bool CheckCRC()
	{
		return  nCRC32 == CRC32((byte *)this, sizeof(FilebufHeader) - sizeof(UINT));
	}

	inline int PackSize()
	{
		return (sizeof(FileFrame) + nBufferSize);
	}
	inline int HeaderSize()
	{
		return sizeof(FileFrame);
	}
};
#pragma pack(pop)