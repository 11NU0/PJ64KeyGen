// PJ64KeyGenGui - simple Win32 GUI frontend for the Project64 supporter key generator.

#include "PJ64KeyCore.h"
#include "PJ64KeyGenGui.h"

#include <commctrl.h>
#include <commdlg.h>
#include <windows.h>

#include <string>
#include <vector>

using namespace pj64;

static std::string g_MachineID;

static std::basic_string<wchar_t> GetDlgText(HWND Dialog, int ControlId)
{
    HWND Control = GetDlgItem(Dialog, ControlId);
    int Length = GetWindowTextLengthW(Control);
    std::vector<wchar_t> Buffer((size_t)Length + 1, 0);
    GetWindowTextW(Control, Buffer.data(), Length + 1);
    return std::basic_string<wchar_t>(Buffer.data());
}

static void SetDlgText(HWND Dialog, int ControlId, const std::basic_string<wchar_t> &Text)
{
    SetWindowTextW(GetDlgItem(Dialog, ControlId), Text.c_str());
}

static std::string GetField(HWND Dialog, int ControlId)
{
    return WideToUtf8(GetDlgText(Dialog, ControlId).c_str());
}

static void SetField(HWND Dialog, int ControlId, const std::string &Text)
{
    SetDlgText(Dialog, ControlId, Utf8ToWide(Text));
}

static void SetStatus(HWND Dialog, const std::basic_string<wchar_t> &Text)
{
    SetDlgText(Dialog, IDC_STATUS, Text);
}

static std::basic_string<wchar_t> FormatKeyStatus(const SupportInfo &Info)
{
    wchar_t Buffer[512];
    SafeFormatW(Buffer, sizeof(Buffer) / sizeof(Buffer[0]), L"Valid key for this machine: %s  (runs: %u)",
               Utf8ToWide(Utf8Field(Info.Name, sizeof(Info.Name))).c_str(),
               (unsigned int)Info.RunCount);
    return Buffer;
}

static void ShowStoredKey(HWND Dialog, bool LoadFields)
{
    SupportInfo Info;
    switch (ReadStoredKey(g_MachineID, Info))
    {
    case KEY_VALID:
        if (LoadFields)
        {
            SetField(Dialog, IDC_NAME, Utf8Field(Info.Name, sizeof(Info.Name)));
            SetField(Dialog, IDC_EMAIL, Utf8Field(Info.Email, sizeof(Info.Email)));
            SetField(Dialog, IDC_CODE, Utf8Field(Info.Code, sizeof(Info.Code)));
        }
        SetStatus(Dialog, FormatKeyStatus(Info));
        break;
    case KEY_NONE:
        SetStatus(Dialog, L"No key stored yet.");
        break;
    default:
        SetStatus(Dialog, L"A key is stored but it is not valid for this machine.");
        break;
    }
}

static void OnGenerate(HWND Dialog)
{
    std::string Name = GetField(Dialog, IDC_NAME);
    if (Name.empty())
    {
        Name = CurrentUserName();
    }

    SupportInfo Written;
    if (!GenerateStoredKey(g_MachineID, Name, GetField(Dialog, IDC_EMAIL), GetField(Dialog, IDC_CODE), Written))
    {
        SetStatus(Dialog, L"Failed to generate the key.");
        MessageBoxW(Dialog, L"Could not generate or store the key.", L"PJ64 KeyGen", MB_OK | MB_ICONERROR);
        return;
    }

    ShowStoredKey(Dialog, true);
    SetStatus(Dialog, FormatKeyStatus(Written));
}

static void OnBackup(HWND Dialog)
{
    std::vector<uint8_t> Blob;
    if (!ReadKeyBlob(Blob) || Blob.empty())
    {
        SetStatus(Dialog, L"There is no key to back up.");
        return;
    }

    std::vector<wchar_t> FileName(MAX_PATH, 0);
    wcscpy_s(FileName.data(), MAX_PATH, L"PJ64Key.bin");

    wchar_t Filter[] = L"Key files (*.bin)\0*.bin\0All files (*.*)\0*.*\0\0";
    OPENFILENAMEW Ofn;
    memset(&Ofn, 0, sizeof(Ofn));
    Ofn.lStructSize = sizeof(Ofn);
    Ofn.hwndOwner = Dialog;
    Ofn.lpstrFilter = Filter;
    Ofn.lpstrFile = FileName.data();
    Ofn.nMaxFile = MAX_PATH;
    Ofn.lpstrTitle = L"Back up Project64 key";
    Ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameW(&Ofn))
    {
        return;
    }

    if (!WriteFileBlob(FileName.data(), Blob))
    {
        SetStatus(Dialog, L"Failed to write the backup file.");
        return;
    }

    wchar_t Status[512];
    SafeFormatW(Status, sizeof(Status) / sizeof(Status[0]), L"Key backed up to %s", FileName.data());
    SetStatus(Dialog, Status);
}

static void OnRestore(HWND Dialog)
{
    std::vector<wchar_t> FileName(MAX_PATH, 0);
    wcscpy_s(FileName.data(), MAX_PATH, L"PJ64Key.bin");

    wchar_t Filter[] = L"Key files (*.bin)\0*.bin\0All files (*.*)\0*.*\0\0";
    OPENFILENAMEW Ofn;
    memset(&Ofn, 0, sizeof(Ofn));
    Ofn.lStructSize = sizeof(Ofn);
    Ofn.hwndOwner = Dialog;
    Ofn.lpstrFilter = Filter;
    Ofn.lpstrFile = FileName.data();
    Ofn.nMaxFile = MAX_PATH;
    Ofn.lpstrTitle = L"Restore Project64 key";
    Ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&Ofn))
    {
        return;
    }

    std::vector<uint8_t> Blob;
    if (!ReadFileBlob(FileName.data(), Blob) || Blob.empty())
    {
        SetStatus(Dialog, L"Could not read that file.");
        return;
    }

    SupportInfo Info;
    if (!DecodeSupportInfo(Blob, g_MachineID, Info))
    {
        SetStatus(Dialog, L"That file is not a valid key for this machine.");
        MessageBoxW(Dialog, L"That file does not contain a key for this machine.", L"PJ64 KeyGen", MB_OK | MB_ICONERROR);
        return;
    }

    if (!WriteKeyBlob(Blob))
    {
        SetStatus(Dialog, L"Failed to write the registry key.");
        return;
    }

    ShowStoredKey(Dialog, true);
}

static void OnDelete(HWND Dialog)
{
    if (MessageBoxW(Dialog, L"Delete the stored key?", L"PJ64 KeyGen", MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        return;
    }

    if (!DeleteKeyBlob())
    {
        SetStatus(Dialog, L"There was no key to delete.");
        return;
    }

    SetField(Dialog, IDC_NAME, std::string());
    SetField(Dialog, IDC_EMAIL, std::string());
    SetField(Dialog, IDC_CODE, std::string());
    ShowStoredKey(Dialog, false);
}

static INT_PTR CALLBACK KeyGenDlgProc(HWND Dialog, UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        SetDlgText(Dialog, IDC_MACHINEID, Utf8ToWide(g_MachineID));
        ShowStoredKey(Dialog, true);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(WParam))
        {
        case IDC_GENERATE:
            OnGenerate(Dialog);
            return TRUE;
        case IDC_REFRESH:
            ShowStoredKey(Dialog, true);
            return TRUE;
        case IDC_BACKUP:
            OnBackup(Dialog);
            return TRUE;
        case IDC_RESTORE:
            OnRestore(Dialog);
            return TRUE;
        case IDC_DELETE:
            OnDelete(Dialog);
            return TRUE;
        case IDC_SELFTEST:
            SetStatus(Dialog, RunSelfTest() ? L"Self test passed." : L"Self test FAILED.");
            return TRUE;
        case IDC_CLOSE:
        case IDCANCEL:
            EndDialog(Dialog, LOWORD(WParam));
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(Dialog, IDC_CLOSE);
        return TRUE;
    }
    return FALSE;
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE, LPSTR, int)
{
    INITCOMMONCONTROLSEX Controls;
    memset(&Controls, 0, sizeof(Controls));
    Controls.dwSize = sizeof(Controls);
    Controls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&Controls);

    g_MachineID = GenerateMachineID();

    return (int)DialogBoxParamW(Instance, MAKEINTRESOURCEW(IDD_KEYGEN), nullptr, KeyGenDlgProc, 0);
}