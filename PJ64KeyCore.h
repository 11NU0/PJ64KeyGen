// PJ64KeyCore.h - shared Project64 supporter key logic for the PJ64KeyGen
// console and GUI frontends.
//
// Key layout (HKCU\SOFTWARE\Project64, value "user", REG_BINARY):
//   SupportInfo struct  (raw, little endian, sizeof == 1232 on MSVC x64)
//   32 char uppercase hex MD5 of the SupportInfo struct
// The whole thing is deflated (Z_BEST_COMPRESSION) and then XORed with 0xAA.

#pragma once

#include <windows.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include <zlib.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

namespace pj64
{

// ---------------------------------------------------------------------------
// MD5 (public domain implementation, identical to Source/Common/md5.cpp)
// ---------------------------------------------------------------------------

namespace md5
{
class MD5
{
public:
    MD5()
    {
        Init();
    }

    void Update(const unsigned char * Input, unsigned int InputLength)
    {
        unsigned int InputIndex, BufferIndex;
        unsigned int BufferSpace;

        BufferIndex = (unsigned int)((m_count[0] >> 3) & 0x3F);

        if ((m_count[0] += ((uint4)InputLength << 3)) < ((uint4)InputLength << 3))
        {
            m_count[1]++;
        }
        m_count[1] += ((uint4)InputLength >> 29);

        BufferSpace = 64 - BufferIndex;

        if (InputLength >= BufferSpace)
        {
            memcpy(m_buffer + BufferIndex, Input, BufferSpace);
            Transform(m_buffer);

            for (InputIndex = BufferSpace; InputIndex + 63 < InputLength; InputIndex += 64)
            {
                Transform((unsigned char *)(Input + InputIndex));
            }

            BufferIndex = 0;
        }
        else
        {
            InputIndex = 0;
        }

        if (InputLength > InputIndex)
        {
            memcpy(m_buffer + BufferIndex, Input + InputIndex, InputLength - InputIndex);
        }
    }

    void Finalize()
    {
        unsigned char Bits[8];
        unsigned int Index, PadLen;
        static unsigned char PADDING[64] = {0x80};

        if (m_finalized)
        {
            return;
        }

        Encode(Bits, m_count, 8);

        Index = (uint4)((m_count[0] >> 3) & 0x3f);
        PadLen = (Index < 56) ? (56 - Index) : (120 - Index);
        Update(PADDING, PadLen);

        Update(Bits, 8);

        Encode(m_digest, m_state, 16);

        memset(m_buffer, 0, sizeof(m_buffer));
        m_finalized = true;
    }

    void Digest(unsigned char Out[16])
    {
        if (!m_finalized)
        {
            Finalize();
        }
        memcpy(Out, m_digest, 16);
    }

private:
    typedef unsigned int uint4;
    typedef unsigned char uint1;

    uint4 m_state[4];
    uint4 m_count[2];
    uint1 m_buffer[64];
    uint1 m_digest[16];
    bool m_finalized;

    void Init()
    {
        m_state[0] = 0x67452301;
        m_state[1] = 0xefcdab89;
        m_state[2] = 0x98badcfe;
        m_state[3] = 0x10325476;
        m_count[0] = 0;
        m_count[1] = 0;
        memset(m_buffer, 0, sizeof(m_buffer));
        memset(m_digest, 0, sizeof(m_digest));
        m_finalized = false;
    }

    static inline uint4 RotateLeft(uint4 x, uint4 n)
    {
        return (x << n) | (x >> (32 - n));
    }

    static inline uint4 F(uint4 x, uint4 y, uint4 z)
    {
        return (x & y) | (~x & z);
    }
    static inline uint4 G(uint4 x, uint4 y, uint4 z)
    {
        return (x & z) | (y & ~z);
    }
    static inline uint4 H(uint4 x, uint4 y, uint4 z)
    {
        return x ^ y ^ z;
    }
    static inline uint4 I(uint4 x, uint4 y, uint4 z)
    {
        return y ^ (x | ~z);
    }

    static inline void FF(uint4 & a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
    {
        a += F(b, c, d) + x + ac;
        a = RotateLeft(a, s) + b;
    }
    static inline void GG(uint4 & a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
    {
        a += G(b, c, d) + x + ac;
        a = RotateLeft(a, s) + b;
    }
    static inline void HH(uint4 & a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
    {
        a += H(b, c, d) + x + ac;
        a = RotateLeft(a, s) + b;
    }
    static inline void II(uint4 & a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac)
    {
        a += I(b, c, d) + x + ac;
        a = RotateLeft(a, s) + b;
    }

    void Transform(uint1 Block[64])
    {
        const uint4 S11 = 7, S12 = 12, S13 = 17, S14 = 22;
        const uint4 S21 = 5, S22 = 9, S23 = 14, S24 = 20;
        const uint4 S31 = 4, S32 = 11, S33 = 16, S34 = 23;
        const uint4 S41 = 6, S42 = 10, S43 = 15, S44 = 21;

        uint4 a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], x[16];

        for (unsigned int i = 0, j = 0; j < 64; i++, j += 4)
        {
            x[i] = ((uint4)Block[j]) | (((uint4)Block[j + 1]) << 8) | (((uint4)Block[j + 2]) << 16) | (((uint4)Block[j + 3]) << 24);
        }

        FF(a, b, c, d, x[0], S11, 0xd76aa478);
        FF(d, a, b, c, x[1], S12, 0xe8c7b756);
        FF(c, d, a, b, x[2], S13, 0x242070db);
        FF(b, c, d, a, x[3], S14, 0xc1bdceee);
        FF(a, b, c, d, x[4], S11, 0xf57c0faf);
        FF(d, a, b, c, x[5], S12, 0x4787c62a);
        FF(c, d, a, b, x[6], S13, 0xa8304613);
        FF(b, c, d, a, x[7], S14, 0xfd469501);
        FF(a, b, c, d, x[8], S11, 0x698098d8);
        FF(d, a, b, c, x[9], S12, 0x8b44f7af);
        FF(c, d, a, b, x[10], S13, 0xffff5bb1);
        FF(b, c, d, a, x[11], S14, 0x895cd7be);
        FF(a, b, c, d, x[12], S11, 0x6b901122);
        FF(d, a, b, c, x[13], S12, 0xfd987193);
        FF(c, d, a, b, x[14], S13, 0xa679438e);
        FF(b, c, d, a, x[15], S14, 0x49b40821);

        GG(a, b, c, d, x[1], S21, 0xf61e2562);
        GG(d, a, b, c, x[6], S22, 0xc040b340);
        GG(c, d, a, b, x[11], S23, 0x265e5a51);
        GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);
        GG(a, b, c, d, x[5], S21, 0xd62f105d);
        GG(d, a, b, c, x[10], S22, 0x2441453);
        GG(c, d, a, b, x[15], S23, 0xd8a1e681);
        GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);
        GG(a, b, c, d, x[9], S21, 0x21e1cde6);
        GG(d, a, b, c, x[14], S22, 0xc33707d6);
        GG(c, d, a, b, x[3], S23, 0xf4d50d87);
        GG(b, c, d, a, x[8], S24, 0x455a14ed);
        GG(a, b, c, d, x[13], S21, 0xa9e3e905);
        GG(d, a, b, c, x[2], S22, 0xfcefa3f8);
        GG(c, d, a, b, x[7], S23, 0x676f02d9);
        GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

        HH(a, b, c, d, x[5], S31, 0xfffa3942);
        HH(d, a, b, c, x[8], S32, 0x8771f681);
        HH(c, d, a, b, x[11], S33, 0x6d9d6122);
        HH(b, c, d, a, x[14], S34, 0xfde5380c);
        HH(a, b, c, d, x[1], S31, 0xa4beea44);
        HH(d, a, b, c, x[4], S32, 0x4bdecfa9);
        HH(c, d, a, b, x[7], S33, 0xf6bb4b60);
        HH(b, c, d, a, x[10], S34, 0xbebfbc70);
        HH(a, b, c, d, x[13], S31, 0x289b7ec6);
        HH(d, a, b, c, x[0], S32, 0xeaa127fa);
        HH(c, d, a, b, x[3], S33, 0xd4ef3085);
        HH(b, c, d, a, x[6], S34, 0x4881d05);
        HH(a, b, c, d, x[9], S31, 0xd9d4d039);
        HH(d, a, b, c, x[12], S32, 0xe6db99e5);
        HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
        HH(b, c, d, a, x[2], S34, 0xc4ac5665);

        II(a, b, c, d, x[0], S41, 0xf4292244);
        II(d, a, b, c, x[7], S42, 0x432aff97);
        II(c, d, a, b, x[14], S43, 0xab9423a7);
        II(b, c, d, a, x[5], S44, 0xfc93a039);
        II(a, b, c, d, x[12], S41, 0x655b59c3);
        II(d, a, b, c, x[3], S42, 0x8f0ccc92);
        II(c, d, a, b, x[10], S43, 0xffeff47d);
        II(b, c, d, a, x[1], S44, 0x85845dd1);
        II(a, b, c, d, x[8], S41, 0x6fa87e4f);
        II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
        II(c, d, a, b, x[6], S43, 0xa3014314);
        II(b, c, d, a, x[13], S44, 0x4e0811a1);
        II(a, b, c, d, x[4], S41, 0xf7537e82);
        II(d, a, b, c, x[11], S42, 0xbd3af235);
        II(c, d, a, b, x[2], S43, 0x2ad7d2bb);
        II(b, c, d, a, x[9], S44, 0xeb86d391);

        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;

        memset(x, 0, sizeof(x));
    }

    static void Encode(uint1 * Output, uint4 * Input, uint4 Len)
    {
        for (unsigned int i = 0, j = 0; j < Len; i++, j += 4)
        {
            Output[j] = (uint1)(Input[i] & 0xff);
            Output[j + 1] = (uint1)((Input[i] >> 8) & 0xff);
            Output[j + 2] = (uint1)((Input[i] >> 16) & 0xff);
            Output[j + 3] = (uint1)((Input[i] >> 24) & 0xff);
        }
    }
};

inline std::string StringifyMd5(const unsigned char * Digest)
{
    static const char *Hex = "0123456789ABCDEF";
    std::string Ret(32, '0');
    for (int i = 0; i < 16; i++)
    {
        Ret[i * 2] = Hex[(Digest[i] >> 4) & 0x0F];
        Ret[i * 2 + 1] = Hex[Digest[i] & 0x0F];
    }
    return Ret;
}

inline std::string Md5Hex(const unsigned char * Data, size_t Length)
{
    md5::MD5 Hasher;
    Hasher.Update(Data, (unsigned int)Length);
    unsigned char Digest[16];
    Hasher.Digest(Digest);
    return StringifyMd5(Digest);
}
} // namespace md5

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

inline const size_t MD5_HEX_LENGTH = 32;

inline int SafeFormat(char *Buffer, size_t Size, const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    int Result = _vsnprintf(Buffer, Size - 1, Format, Args);
    va_end(Args);
    Buffer[Size - 1] = '\0';
    return Result;
}

inline int SafeFormatW(wchar_t *Buffer, size_t Size, const wchar_t *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    int Result = _vsnwprintf(Buffer, Size - 1, Format, Args);
    va_end(Args);
    Buffer[Size - 1] = L'\0';
    return Result;
}

inline std::string WideToUtf8(const wchar_t *Text)
{
    if (Text == nullptr || *Text == L'\0')
    {
        return std::string();
    }
    int Needed = WideCharToMultiByte(CP_UTF8, 0, Text, -1, nullptr, 0, nullptr, nullptr);
    if (Needed <= 0)
    {
        return std::string();
    }
    std::vector<char> Buffer((size_t)Needed + 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, Text, -1, Buffer.data(), Needed, nullptr, nullptr);
    return std::string(Buffer.data());
}

inline std::basic_string<wchar_t> Utf8ToWide(const std::string &Text)
{
    if (Text.empty())
    {
        return std::basic_string<wchar_t>();
    }
    int Needed = MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), (int)Text.length(), nullptr, 0);
    if (Needed <= 0)
    {
        return std::basic_string<wchar_t>();
    }
    std::vector<wchar_t> Buffer((size_t)Needed + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, Text.c_str(), (int)Text.length(), Buffer.data(), Needed);
    return std::basic_string<wchar_t>(Buffer.data());
}

// ---------------------------------------------------------------------------
// Project64 supporter info
// ---------------------------------------------------------------------------

typedef struct
{
    char Code[300];
    char Email[300];
    char Name[300];
    char MachineID[300];
    uint32_t RunCount;
    time_t LastUpdated;
    time_t LastShown;
    bool Validated;
} SupportInfo;

inline const wchar_t *REG_KEY = L"SOFTWARE\\Project64";
inline const wchar_t *REG_VALUE = L"user";

// CProjectSupport::GenerateMachineID
inline std::string GenerateMachineID(void)
{
    wchar_t ComputerName[256] = {0};
    DWORD Length = sizeof(ComputerName) / sizeof(ComputerName[0]);
    GetComputerNameW(ComputerName, &Length);

    wchar_t SysPath[MAX_PATH] = {0}, VolumePath[MAX_PATH] = {0};
    GetSystemDirectoryW(SysPath, sizeof(SysPath) / sizeof(SysPath[0]));

    GetVolumePathNameW(SysPath, VolumePath, sizeof(VolumePath) / sizeof(VolumePath[0]));

    DWORD SerialNumber = 0;
    GetVolumeInformationW(VolumePath, nullptr, 0, &SerialNumber, nullptr, nullptr, nullptr, 0);

    wchar_t MachineGuid[200] = {0};
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS)
    {
        DWORD Type = 0, DataSize = sizeof(MachineGuid) / sizeof(MachineGuid[0]);
        RegQueryValueExW(hKey, L"MachineGuid", nullptr, &Type, (LPBYTE)MachineGuid, &DataSize);
        RegCloseKey(hKey);
    }

    std::string Computer = WideToUtf8(ComputerName);
    std::string Guid = WideToUtf8(MachineGuid);

    char Buffer[512];
    SafeFormat(Buffer, sizeof(Buffer), "%s.%ud.%s", Computer.c_str(), (unsigned int)SerialNumber, Guid.c_str());

    return md5::Md5Hex((const unsigned char *)Buffer, strlen(Buffer));
}

inline void CopyField(char *Dest, size_t DestSize, const std::string &Value)
{
    memset(Dest, 0, DestSize);
    strncpy(Dest, Value.c_str(), DestSize - 1);
    Dest[DestSize - 1] = '\0';
}

inline std::string CurrentUserName(void)
{
    wchar_t UserName[256] = {0};
    DWORD Size = sizeof(UserName) / sizeof(UserName[0]);
    if (GetUserNameW(UserName, &Size) == 0)
    {
        return std::string();
    }
    return WideToUtf8(UserName);
}

// CProjectSupport::SaveSupportInfo (blob form)
inline bool EncodeSupportInfo(const SupportInfo &Info, std::vector<uint8_t> &OutData)
{
    std::string Hash = md5::Md5Hex((const unsigned char *)&Info, sizeof(SupportInfo));

    std::vector<uint8_t> InData(sizeof(SupportInfo) + Hash.length());
    memcpy(InData.data(), &Info, sizeof(SupportInfo));
    memcpy(InData.data() + sizeof(SupportInfo), Hash.data(), Hash.length());

    uLong Bound = compressBound((uLong)InData.size());
    std::vector<uint8_t> Deflated((size_t)Bound);

    z_stream Stream;
    memset(&Stream, 0, sizeof(Stream));
    Stream.zalloc = Z_NULL;
    Stream.zfree = Z_NULL;
    Stream.opaque = Z_NULL;
    Stream.avail_in = (uInt)InData.size();
    Stream.next_in = (Bytef *)InData.data();
    Stream.avail_out = (uInt)Deflated.size();
    Stream.next_out = (Bytef *)Deflated.data();

    if (deflateInit(&Stream, Z_BEST_COMPRESSION) != Z_OK)
    {
        return false;
    }
    int Ret = deflate(&Stream, Z_FINISH);
    size_t TotalOut = (size_t)Stream.total_out;
    deflateEnd(&Stream);
    if (Ret != Z_STREAM_END)
    {
        return false;
    }

    Deflated.resize(TotalOut);
    for (size_t i = 0, n = Deflated.size(); i < n; i++)
    {
        Deflated[i] ^= 0xAA;
    }

    OutData.swap(Deflated);
    return true;
}

// CProjectSupport::LoadSupportInfo (blob form)
inline bool DecodeSupportInfo(const std::vector<uint8_t> &Blob, const std::string &MachineID, SupportInfo &Info)
{
    if (Blob.empty())
    {
        return false;
    }

    std::vector<uint8_t> InData(Blob);
    for (size_t i = 0, n = InData.size(); i < n; i++)
    {
        InData[i] ^= 0xAA;
    }

    std::vector<uint8_t> OutData(sizeof(SupportInfo) + 100);
    uLongf DestLen = (uLongf)OutData.size();
    if (uncompress(OutData.data(), &DestLen, InData.data(), (uLong)InData.size()) != Z_OK)
    {
        return false;
    }
    OutData.resize((size_t)DestLen);

    if (OutData.size() != sizeof(SupportInfo) + MD5_HEX_LENGTH)
    {
        return false;
    }

    SupportInfo Temp;
    memcpy(&Temp, OutData.data(), sizeof(SupportInfo));
    const char *StoredHash = (const char *)(OutData.data() + sizeof(SupportInfo));
    std::string Hash = md5::Md5Hex((const unsigned char *)&Temp, sizeof(SupportInfo));

    if (strcmp(Hash.c_str(), StoredHash) != 0 || strcmp(Temp.MachineID, MachineID.c_str()) != 0)
    {
        return false;
    }

    memcpy(&Info, &Temp, sizeof(SupportInfo));
    return true;
}

inline void BuildSupportInfo(SupportInfo &Info, const std::string &MachineID, const std::string &Name, const std::string &Email, const std::string &Code)
{
    memset(&Info, 0, sizeof(Info));
    CopyField(Info.MachineID, sizeof(Info.MachineID), MachineID);
    CopyField(Info.Name, sizeof(Info.Name), Name);
    CopyField(Info.Email, sizeof(Info.Email), Email);
    CopyField(Info.Code, sizeof(Info.Code), Code);
    Info.RunCount = 0;
    Info.LastUpdated = 0;
    Info.LastShown = 0;
    Info.Validated = true;
}

// ---------------------------------------------------------------------------
// key store access
// ---------------------------------------------------------------------------

inline bool ReadKeyBlob(std::vector<uint8_t> &Blob)
{
    Blob.clear();

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD DataSize = 0;
    DWORD Type = 0;
    bool Success = false;
    if (RegQueryValueExW(hKey, REG_VALUE, nullptr, &Type, nullptr, &DataSize) == ERROR_SUCCESS && DataSize > 0)
    {
        Blob.resize((size_t)DataSize);
        if (RegQueryValueExW(hKey, REG_VALUE, nullptr, &Type, Blob.data(), &DataSize) == ERROR_SUCCESS)
        {
            Blob.resize((size_t)DataSize);
            Success = true;
        }
        else
        {
            Blob.clear();
        }
    }

    RegCloseKey(hKey);
    return Success;
}

inline bool WriteKeyBlob(const std::vector<uint8_t> &Blob)
{
    HKEY hKey = nullptr;
    DWORD Disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, &Disposition) != ERROR_SUCCESS)
    {
        return false;
    }

    LONG Result = RegSetValueExW(hKey, REG_VALUE, 0, REG_BINARY, (const BYTE *)Blob.data(), (DWORD)Blob.size());
    RegCloseKey(hKey);
    return Result == ERROR_SUCCESS;
}

inline bool DeleteKeyBlob(void)
{
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
    {
        return false;
    }
    LONG Result = RegDeleteValueW(hKey, REG_VALUE);
    RegCloseKey(hKey);
    return Result == ERROR_SUCCESS;
}

inline bool WriteFileBlob(const wchar_t *Path, const std::vector<uint8_t> &Blob)
{
    HANDLE File = CreateFileW(Path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    DWORD Written = 0;
    bool Success = Blob.empty() || WriteFile(File, Blob.data(), (DWORD)Blob.size(), &Written, nullptr) != 0 && Written == Blob.size();
    CloseHandle(File);
    return Success;
}

inline bool ReadFileBlob(const wchar_t *Path, std::vector<uint8_t> &Blob)
{
    HANDLE File = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (File == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    LARGE_INTEGER Size;
    if (!GetFileSizeEx(File, &Size) || Size.QuadPart < 0 || Size.QuadPart > 0x10000)
    {
        CloseHandle(File);
        return false;
    }
    Blob.resize((size_t)Size.QuadPart);
    DWORD Read = 0;
    bool Success = Blob.empty() || ReadFile(File, Blob.data(), (DWORD)Blob.size(), &Read, nullptr) != 0 && Read == Blob.size();
    CloseHandle(File);
    if (!Success)
    {
        Blob.clear();
    }
    return Success;
}

// High level helpers shared by both frontends

inline bool RunSelfTest(void)
{
    static const char *KnownVectors[][2] = {
        {"", "D41D8CD98F00B204E9800998ECF8427E"},
        {"a", "0CC175B9C0F1B6A831C399E269772661"},
        {"abc", "900150983CD24FB0D6963F7D28E17F72"},
        {"message digest", "F96B697D7CB7938D525A2F31AAF161D0"},
        {"abcdefghijklmnopqrstuvwxyz", "C3FCD3D76192E4007DFB496CCA67E13B"},
    };
    for (size_t i = 0; i < sizeof(KnownVectors) / sizeof(KnownVectors[0]); i++)
    {
        std::string Got = md5::Md5Hex((const unsigned char *)KnownVectors[i][0], strlen(KnownVectors[i][0]));
        if (Got != KnownVectors[i][1])
        {
            return false;
        }
    }

    std::string MachineID = GenerateMachineID();
    SupportInfo Info;
    BuildSupportInfo(Info, MachineID, CurrentUserName(), "test@example.com", "TESTCODE");

    std::vector<uint8_t> Blob;
    if (!EncodeSupportInfo(Info, Blob))
    {
        return false;
    }

    SupportInfo Decoded;
    if (!DecodeSupportInfo(Blob, MachineID, Decoded))
    {
        return false;
    }
    return memcmp(&Info, &Decoded, sizeof(SupportInfo)) == 0 && Decoded.Validated;
}

enum KeyState
{
    KEY_NONE,
    KEY_VALID,
    KEY_FOREIGN,
};

// Looks at the stored key for this machine. Info is only filled in when the
// result is KEY_VALID.
inline KeyState ReadStoredKey(const std::string &MachineID, SupportInfo &Info)
{
    std::vector<uint8_t> Blob;
    if (!ReadKeyBlob(Blob) || Blob.empty())
    {
        return KEY_NONE;
    }
    if (!DecodeSupportInfo(Blob, MachineID, Info))
    {
        return KEY_FOREIGN;
    }
    return KEY_VALID;
}

// Generates a key for this machine and stores it. Returns the info that was
// written on success.
inline bool GenerateStoredKey(const std::string &MachineID, const std::string &Name, const std::string &Email, const std::string &Code, SupportInfo &Written)
{
    SupportInfo Info;
    BuildSupportInfo(Info, MachineID, Name, Email, Code);

    std::vector<uint8_t> Blob;
    if (!EncodeSupportInfo(Info, Blob))
    {
        return false;
    }
    if (!WriteKeyBlob(Blob))
    {
        return false;
    }

    SupportInfo Verify;
    if (!DecodeSupportInfo(Blob, MachineID, Verify) || !Verify.Validated)
    {
        return false;
    }

    memcpy(&Written, &Verify, sizeof(SupportInfo));
    return true;
}

inline std::string Utf8Field(const char *Field, size_t FieldSize)
{
    size_t Length = 0;
    while (Length < FieldSize && Field[Length] != '\0')
    {
        Length++;
    }
    return std::string(Field, Length);
}

} // namespace pj64