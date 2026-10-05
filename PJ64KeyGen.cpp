// PJ64KeyGen - console frontend for the Project64 supporter key generator.

#include "PJ64KeyCore.h"

#include <stdio.h>
#include <string>
#include <vector>

using namespace pj64;

static void PrintUsage(void)
{
    printf("PJ64KeyGen - Project64 supporter key generator\n\n");
    printf("Usage: PJ64KeyGen [options]\n\n");
    printf("  (no options)   validate the existing key, create one if missing/invalid\n");
    printf("  -i             show machine id and current key status\n");
    printf("  -t             self test (builds and verifies a key, does not touch the registry)\n");
    printf("  -f             force generate a new key, overwriting any existing one\n");
    printf("  -d             delete the stored key\n");
    printf("  -b <file>      back up the stored key blob to <file>\n");
    printf("  -r <file>      restore the key blob from <file>\n");
    printf("  -n <name>      supporter name (default: windows user name)\n");
    printf("  -e <email>     supporter email\n");
    printf("  -c <code>      supporter code\n");
    printf("  -h             this help\n");
}

int main(int argc, char *argv[])
{
    bool Force = false;
    bool ShowInfo = false;
    bool SelfTest = false;
    bool DoDelete = false;
    bool HaveName = false, HaveEmail = false, HaveCode = false;
    const char *BackupPath = nullptr;
    const char *RestorePath = nullptr;
    std::string Name, Email, Code;

    for (int i = 1; i < argc; i++)
    {
        std::string Arg = argv[i];
        if (Arg == "-f" || Arg == "--force")
        {
            Force = true;
        }
        else if (Arg == "-i" || Arg == "--info")
        {
            ShowInfo = true;
        }
        else if (Arg == "-t" || Arg == "--test")
        {
            SelfTest = true;
        }
        else if (Arg == "-d" || Arg == "--delete")
        {
            DoDelete = true;
        }
        else if ((Arg == "-b" || Arg == "--backup") && i + 1 < argc)
        {
            BackupPath = argv[++i];
        }
        else if ((Arg == "-r" || Arg == "--restore") && i + 1 < argc)
        {
            RestorePath = argv[++i];
        }
        else if ((Arg == "-n" || Arg == "--name") && i + 1 < argc)
        {
            Name = argv[++i];
            HaveName = true;
        }
        else if ((Arg == "-e" || Arg == "--email") && i + 1 < argc)
        {
            Email = argv[++i];
            HaveEmail = true;
        }
        else if ((Arg == "-c" || Arg == "--code") && i + 1 < argc)
        {
            Code = argv[++i];
            HaveCode = true;
        }
        else if (Arg == "-h" || Arg == "--help")
        {
            PrintUsage();
            return 0;
        }
        else
        {
            fprintf(stderr, "unknown or incomplete option: %s\n", Arg.c_str());
            PrintUsage();
            return 1;
        }
    }

    std::string MachineID = GenerateMachineID();

    if (SelfTest)
    {
        SupportInfo Info;
        BuildSupportInfo(Info, MachineID, HaveName ? Name : CurrentUserName(), Email, Code);

        std::vector<uint8_t> Blob;
        if (!EncodeSupportInfo(Info, Blob))
        {
            fprintf(stderr, "self test failed: could not encode key\n");
            return 1;
        }

        SupportInfo Decoded;
        if (!DecodeSupportInfo(Blob, MachineID, Decoded) || memcmp(&Info, &Decoded, sizeof(SupportInfo)) != 0 || !Decoded.Validated)
        {
            fprintf(stderr, "self test failed: could not decode key\n");
            return 1;
        }

        if (!RunSelfTest())
        {
            fprintf(stderr, "self test failed\n");
            return 1;
        }

        printf("self test passed\n");
        printf("sizeof(SupportInfo) = %u, payload = %u, blob = %u bytes\n",
               (unsigned int)sizeof(SupportInfo),
               (unsigned int)(sizeof(SupportInfo) + MD5_HEX_LENGTH),
               (unsigned int)Blob.size());
        printf("machine id = %s\n", MachineID.c_str());
        return 0;
    }

    if (DoDelete)
    {
        if (DeleteKeyBlob())
        {
            printf("deleted stored key\n");
        }
        else
        {
            printf("no stored key to delete\n");
        }
    }

    if (RestorePath != nullptr)
    {
        std::wstring Path = Utf8ToWide(RestorePath);
        std::vector<uint8_t> Blob;
        if (!ReadFileBlob(Path.c_str(), Blob) || Blob.empty())
        {
            fprintf(stderr, "could not read backup file: %s\n", RestorePath);
            return 1;
        }
        SupportInfo Info;
        if (!DecodeSupportInfo(Blob, MachineID, Info))
        {
            fprintf(stderr, "backup file is not a valid key for this machine\n");
            return 1;
        }
        if (!WriteKeyBlob(Blob))
        {
            fprintf(stderr, "could not write the registry key\n");
            return 1;
        }
        printf("restored key from %s (name: %s)\n", RestorePath, Utf8Field(Info.Name, sizeof(Info.Name)).c_str());
    }

    if (BackupPath != nullptr)
    {
        std::vector<uint8_t> Blob;
        if (!ReadKeyBlob(Blob) || Blob.empty())
        {
            fprintf(stderr, "no stored key to back up\n");
            return 1;
        }
        std::wstring Path = Utf8ToWide(BackupPath);
        if (!WriteFileBlob(Path.c_str(), Blob))
        {
            fprintf(stderr, "could not write backup file: %s\n", BackupPath);
            return 1;
        }
        printf("backed up stored key to %s (%u bytes)\n", BackupPath, (unsigned int)Blob.size());
    }

    if ((RestorePath != nullptr || DoDelete || BackupPath != nullptr) && !Force && !ShowInfo)
    {
        return 0;
    }

    SupportInfo Current;
    KeyState State = ReadStoredKey(MachineID, Current);

    if (ShowInfo)
    {
        printf("machine id = %s\n", MachineID.c_str());
        switch (State)
        {
        case KEY_VALID:
            printf("stored key  = valid\n");
            printf("  name      = %s\n", Utf8Field(Current.Name, sizeof(Current.Name)).c_str());
            printf("  email     = %s\n", Utf8Field(Current.Email, sizeof(Current.Email)).c_str());
            printf("  code      = %s\n", Utf8Field(Current.Code, sizeof(Current.Code)).c_str());
            printf("  run count = %u\n", (unsigned int)Current.RunCount);
            break;
        case KEY_NONE:
            printf("stored key  = none\n");
            break;
        default:
            printf("stored key  = present but invalid for this machine\n");
            break;
        }
        if (!Force)
        {
            return 0;
        }
        printf("regenerating forced...\n");
    }

    if (State == KEY_VALID && !Force)
    {
        printf("validated existing key (name: %s)\n", Utf8Field(Current.Name, sizeof(Current.Name)).c_str());
        return 0;
    }

    SupportInfo Written;
    if (!GenerateStoredKey(MachineID, HaveName ? Name : CurrentUserName(), Email, Code, Written))
    {
        fprintf(stderr, "could not generate key\n");
        return 1;
    }

    printf("generated new key (name: %s, machine: %s)\n", Utf8Field(Written.Name, sizeof(Written.Name)).c_str(), MachineID.c_str());
    return 0;
}