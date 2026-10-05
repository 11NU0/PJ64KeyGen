# PJ64KeyGen

Project64（Nintendo 64 模擬器）支持者金鑰產生工具。

本專案依據 `project64-develop` 目前的原始碼，產生 Project64 存放於登錄檔中的「支持者金鑰」，
讓模擬器不再顯示支持提示畫面。提供 **主控台版本** 與 **Windows GUI 版本** 兩種介面，
兩者共用同一份核心邏輯。

---

## 目錄

- [對應的 Project64 原始碼](#對應的-project64-原始碼)
- [系統需求](#系統需求)
- [建置方式](#建置方式)
- [GUI 版本使用說明](#gui-版本使用說明)
- [主控台版本使用說明](#主控台版本使用說明)
- [檔案說明](#檔案說明)
- [金鑰格式（技術細節）](#金鑰格式技術細節)
- [注意事項](#注意事項)
- [支持 Project64](#支持-project64)
- [免責聲明](#免責聲明)

---

## 對應的 Project64 原始碼

| 功能 | 檔案 | 位置 |
| ---- | ---- | ---- |
| `SupportInfo` 結構定義 | `ProjectSupport.h` | `Source/Project64/UserInterface/` |
| `GenerateMachineID()` | `ProjectSupport.cpp` | `Source/Project64/UserInterface/` |
| `SaveSupportInfo()` | `ProjectSupport.cpp` | `Source/Project64/UserInterface/` |
| `LoadSupportInfo()` | `ProjectSupport.cpp` | `Source/Project64/UserInterface/` |
| 驗證金鑰是否有效 | `SupportWindow.cpp` | `Source/Project64/UserInterface/` |

本工具的實作與上述函式逐行對應，因此產生的金鑰可被 Project64 直接辨識。

---

## 系統需求

- Windows：與 Project64 相同，為 **64 位元 Windows 10／11**
  （本工具僅使用 Win32 API，32 位元環境理論上也可運作）
- 編譯工具（二選一）：
  - Visual Studio 2019 / 2022（僅需「使用 C++ 的桌面開發」工作負載）
  - MinGW-w64（需附帶 `windres`）
- zlib：本工具直接沿用 `project64-develop\Source\3rdParty\zlib` 內附的原始碼，**無須另外安裝**

---

## 建置方式

### 使用 Visual Studio（MSVC）

直接執行：

```bat
build_msvc.bat
```

腳本會自行呼叫 `vcvarsall.bat`（依序尋找 VS 2022、VS 2019），並產生：

```
build\PJ64KeyGen.exe
build\PJ64KeyGenGui.exe
```

> 備註：`rc.exe` 要求輸入檔案結尾必須有換行，修改 `PJ64KeyGenGui.rc` 或
> `PJ64KeyGenGui.h` 時請保留檔尾換行，否則資源編譯會出現 `RC1004` 錯誤。

### 使用 MinGW-w64

```sh
make
```

會在目前資料夾產生 `PJ64KeyGen.exe` 與 `PJ64KeyGenGui.exe`。
清除中間檔與執行檔請執行 `make clean`。

---

## GUI 版本使用說明

執行 `PJ64KeyGenGui.exe` 即可開啟視窗介面。

| 元件 | 說明 |
| ---- | ---- |
| Machine ID | 本機的機器識別碼，唯讀。金鑰與此值綁定，無法在其他電腦使用 |
| Name | 支持者名稱。留空時會自動填入 Windows 使用者名稱 |
| Email | 支持者電子郵件（選填） |
| Code | 支持者代碼（選填） |
| Stored key | 顯示目前金鑰狀態與支持者名稱 |

按鈕功能：

| 按鈕 | 功能 |
| ---- | ---- |
| Generate | 依上方欄位產生金鑰並寫入登錄檔 |
| Refresh | 重新讀取登錄檔並更新狀態列與欄位 |
| Backup... | 將目前的金鑰匯出成檔案（預設 `PJ64Key.bin`） |
| Restore... | 從備份檔還原金鑰（會先檢查是否為本機金鑰） |
| Delete | 刪除登錄檔中的金鑰（執行前會跳出的確認視窗） |
| Self test | 執行內建自我測試（MD5 測試向量 + 金鑰編解碼往返驗證） |
| Close | 關閉視窗 |

---

## 主控台版本使用說明

```bat
PJ64KeyGen.exe            REM 不帶參數：驗證現有金鑰，無效則自動產生
PJ64KeyGen.exe -i         REM 顯示機器識別碼與金鑰狀態
PJ64KeyGen.exe -t         REM 自我測試（不會碰觸登錄檔）
```

### 選項一覽

| 選項 | 說明 |
| ---- | ---- |
| （無） | 驗證現有金鑰，若無效或不存在則自動產生一把 |
| `-i`, `--info` | 顯示機器識別碼與目前金鑰狀態 |
| `-t`, `--test` | 自我測試，不會寫入登錄檔 |
| `-f`, `--force` | 強制產生新金鑰，覆蓋既有金鑰 |
| `-d`, `--delete` | 刪除登錄檔中的金鑰 |
| `-b <檔案>`, `--backup <檔案>` | 備份目前的金鑰 |
| `-r <檔案>`, `--restore <檔案>` | 從備份檔還原金鑰 |
| `-n <名稱>`, `--name <名稱>` | 指定支持者名稱（預設為 Windows 使用者名稱） |
| `-e <電子郵件>`, `--email <電子郵件>` | 指定支持者電子郵件 |
| `-c <代碼>`, `--code <代碼>` | 指定支持者代碼 |
| `-h`, `--help` | 顯示說明 |

### 常用範例

```bat
REM 產生一把自訂名稱的金鑰
PJ64KeyGen.exe -f -n "My Name" -e "me@example.com"

REM 先備份，再產生新金鑰
PJ64KeyGen.exe -b backup.bin
PJ64KeyGen.exe -f -n "My Name"

REM 回復到備份的金鑰
PJ64KeyGen.exe -r backup.bin

REM 刪除金鑰（Project64 會重新顯示支持提示）
PJ64KeyGen.exe -d
```

> `Backup`／`Restore` 僅處理登錄檔中未加密的位元組資料。若在重新產生金鑰之後才執行
> `-b`，備份到的就會是新金鑰；請務必在覆蓋**之前**備份。

---

## 檔案說明

| 檔案 | 說明 |
| ---- | ---- |
| `PJ64KeyCore.h` | 共用核心：MD5、機器識別碼、金鑰編解碼、登錄檔與檔案存取、自我測試 |
| `PJ64KeyGen.cpp` | 主控台介面（`main`） |
| `PJ64KeyGenGui.cpp` | GUI 介面（`WinMain` 與對話方塊程序） |
| `PJ64KeyGenGui.h` | GUI 使用的資源編號（同時供 `.rc` 使用） |
| `PJ64KeyGenGui.rc` | 對話方塊版面定義 |
| `PJ64KeyGenGui.manifest` | 應用程式資訊清單，啟用新版視覺樣式 |
| `build_msvc.bat` | MSVC 建置腳本 |
| `Makefile` | MinGW-w64 建置腳本 |

`PJ64KeyCore.h` 內含專案自帶的 MD5 實作，因此**不需要** OpenSSL 或其他外部函式庫。

---

## 金鑰格式（技術細節）

金鑰儲存在：

```
HKEY_CURRENT_USER\SOFTWARE\Project64
    user    REG_BINARY
```

產生流程（對應 `CProjectSupport::SaveSupportInfo`）：

```
SupportInfo（1232 bytes，未經封裝）
+ MD5 十六進位字串 32 碼（大寫）      →  1264 bytes
        │
        ├─ zlib 壓縮（Z_BEST_COMPRESSION）
        ├─ 每個位元組 XOR 0xAA
        └─ 寫入登錄檔
```

`SupportInfo` 欄位：

| 欄位 | 大小 | 說明 |
| ---- | ---- | ---- |
| `Code` | 300 | 支持者代碼 |
| `Email` | 300 | 支持者電子郵件 |
| `Name` | 300 | 支持者名稱 |
| `MachineID` | 300 | 機器識別碼 |
| `RunCount` | 4 | 執行次數 |
| `LastUpdated` | 8 | 上次更新時間 |
| `LastShown` | 8 | 上次顯示支持提示的時間 |
| `Validated` | 1 | 是否已驗證（`true` 即可停用支持提示） |

機器識別碼的計算方式（對應 `CProjectSupport::GenerateMachineID`）：

```
MD5( "<電腦名稱>.<系統磁碟序號>d.<MachineGuid>" )
```

其中 `MachineGuid` 取自
`HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Cryptography\MachineGuid`。
字串組成時 `%ud` 代表「序號數值後面接一個字元 `d`」。

載入時（對應 `CProjectSupport::LoadSupportInfo`）會檢查三件事，任一不符即視為無效金鑰：

1. 解壓縮後的長度必須剛好是 `sizeof(SupportInfo) + 32`
2. 內附的 MD5 必須與 `SupportInfo` 重新計算的結果相符
3. `MachineID` 必須與目前的機器識別碼相符

---

## 注意事項

- **金鑰與電腦綁定**：機器識別碼包含電腦名稱、系統磁碟序號與 `MachineGuid`，
  因此金鑰無法在另一台電腦使用，複製到其他機器只會得到「無效金鑰」。
- **重新產生會覆蓋**：使用 `-f` 或 GUI 的 `Generate` 會直接覆寫登錄檔中既有的金鑰，
  原本的 `Name`／`Email`／`RunCount` 等資料將一併被取代。重要資料請先備份。
- **只影響支援提示**：Project64 的核心功能不受影響；金鑰的作用僅是停用支持提示視窗。
- **登錄檔範圍**：僅寫入 `HKEY_CURRENT_USER`，不需要系統管理員權限。
- **刪除金鑰**：Project64 在啟動數次後仍會顯示支持提示；`Delete` 只能還原成「尚未驗證」的狀態。
- **版本相容性**：若 Project64 更改了 `SupportInfo` 結構或金鑰演算法，本工具需要同步修改。
  可利用 `-t`／`Self test` 快速確認產生與解析是否正常。

---

## 支持 Project64

> **本工具只關閉「支持提示畫面」，不會影響模擬器功能。**
> 但請注意：Project64 是免費的開源軟體，長久以來都由開發者與社群自掏腰包維護。
> 如果你使用了本工具，請考慮用下列方式實質支持各個 Project64 版本。

### 支持方式

| 方式 | 說明 | 連結 |
| ---- | ---- | ---- |
| 捐款支持 | 一次性或定期捐款，直接支持開發者 | <https://www.pj64-emu.com/support-project64.html> |
| 使用官方版本 | 使用官方發行的穩定版或 nightly 版，而非自行修改的版本 | <https://www.pj64-emu.com/windows-downloads>／<https://www.pj64-emu.com/nightly-builds> |
| 回報問題與討論 | 使用者回報與交流能替開發者省下大量時間 | <https://discord.gg/Cg3zquF> |
| 參與開發 | 提交修正、翻譯與外掛整合 | <https://github.com/project64/project64/blob/develop/Docs/BUILDING.md> |

### 本工具適用的版本

| Project64 版本 | 是否適用 | 說明 |
| ---- | ---- | ---- |
| `project64-develop`（Windows 版，3.0.x 開發分支） | ✅ 適用 | 本工具即是依此分支的 `ProjectSupport.cpp` 實作 |
| Windows 官方發行版（3.0.x） | ✅ 適用 | 金鑰機制未變 |
| Project64 Android 版 | ❌ 不適用 | 屬於獨立專案，使用不同的設定儲存方式，本工具無效 |

補充說明：

- Project64 的支持提示並非授權驗證，**不影響任何功能的可用性**；
  它存在的目的是讓願意支持的人可以捐款。
- 金鑰只記錄在本機登錄檔（`HKEY_CURRENT_USER`），不會傳送任何資料到 Project64 的伺服器。
- 若 Project64 官方提供免費的設定匯出／匯入功能，建議改用官方方式保存支援者資訊。

---

## 免責聲明

本專案為非官方工具，與 Project64 官方團隊沒有任何關聯。
原始碼位於 <https://github.com/project64/project64>，遵循其原有授權條款。
Project64 是免費的開源軟體，若你喜歡這個模擬器，請參考上方[支持 Project64](#支持-project64)一節。