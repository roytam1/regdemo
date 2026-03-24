#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "resource.h"

#define APP_CLASS_NAME      "RegDemoMainWindow"
#define EDIT_CLASS_NAME     "RegDemoEditWindow"
#define APP_TITLE           "Win32 RegDemo"
#define EDIT_TITLE_STRING   "Edit String Value"
#define EDIT_TITLE_DWORD    "Edit DWORD Value"

#define MAX_DISPLAY_DATA    20000UL
#define KEY_INDENT_SPACES   2
#define NAME_DEFAULT        "(Default)"
#define NAME_DEFAULT_LEN    9

#define KEYNAMESIZE 300

#define IDC_KEY_LIST        1001
#define IDC_VALUE_LIST      1002
#define IDC_EXIT_BUTTON     1003
#define IDC_LABEL_KEY       1004
#define IDC_LABEL_VALUE     1005
#define IDC_LABEL_KEYS_DBL  1006
#define IDC_LABEL_VAL_DBL   1007
#define IDC_LABEL_PLUS      1008
#define IDC_LABEL_STAR      1009

#define IDC_EDIT_NAME       2001
#define IDC_EDIT_VALUE      2002
#define IDC_BUTTON_OK       2003
#define IDC_BUTTON_CANCEL   2004
#define IDC_GROUP_BASE      2005
#define IDC_RADIO_HEX       2006
#define IDC_RADIO_DEC       2007
#define IDC_STATIC_NAME     2008
#define IDC_STATIC_VALUE    2009

#define WM_APP_SET_EDIT_FOCUS  (WM_APP + 1)

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef LONG_PTR
#define LONG_PTR LPVOID
#endif

#ifndef ICON_BIG
#define ICON_BIG 1
#endif

#ifndef ICON_SMALL
#define ICON_SMALL 0
#endif

typedef struct tagKEYITEM {
    HKEY hRoot;
    char *pszSubPath;
    int nLevel;
} KEYITEM;

typedef struct tagVALUEITEM {
    char *pszValueName;
    DWORD dwType;
} VALUEITEM;

typedef struct tagENUMVALUETEMP {
    char *pszRawName;
    char *pszDisplayName;
    char *pszDisplayData;
    DWORD dwType;
} ENUMVALUETEMP;

typedef struct tagEDITCTX {
    HKEY hRoot;
    char *pszSubPath;
    char *pszValueName;
    DWORD dwType;
    char *pszInitialText;
    DWORD dwInitialDword;
    HWND hwnd;
    HWND hwndValueName;
    HWND hwndValueData;
    HWND hwndOk;
    HWND hwndCancel;
    HWND hwndGroup;
    HWND hwndHex;
    HWND hwndDec;
    int nCurrentBase; /* 0 = hex, 1 = decimal */
    int nResult;
} EDITCTX;

typedef struct tagAPPSTATE {
    HINSTANCE hInstance;
    HWND hwndMain;
    HWND hwndKeyList;
    HWND hwndValueList;
    HWND hwndExit;
    HWND hwndLabelKey;
    HWND hwndLabelValue;
    HWND hwndLabelKeysDbl;
    HWND hwndLabelValDbl;
    HWND hwndLabelPlus;
    HWND hwndLabelStar;
    HFONT hUiFont;
    HFONT hListFont;
    HBRUSH hbrFace;
    COLORREF crFace;
    COLORREF crWindow;
    int dpiX;
    int dpiY;
    int minWindowWidth;
    int minWindowHeight;
    int currentValueNameWidth;
    int isWin9x;
    HICON hIcon;
    WNDPROC groupBoxDefProc;
} APPSTATE;

static APPSTATE g_app;

static int TwipsX(int twips);
static int TwipsY(int twips);
static void CenterWindowRelative(HWND hwnd, HWND hwndOwner);
static HFONT CreatePointFontACompat(const char *pszFace, int pointX100, int nWeight);
static void FatalOutOfMemory(void);
static void *xmalloc(size_t cb);
static void *xrealloc(void *pv, size_t cb);
static char *xstrdup(const char *psz);
static char *xstrdup_len(const char *psz, size_t cch);
static void FreeKeyItem(KEYITEM *pItem);
static void FreeValueItem(VALUEITEM *pItem);
static void FreeAllListItemData(HWND hwndList, int isValueList);
static LONG AddRootKey(HKEY hRoot, const char *pszTitle);
static KEYITEM *GetSelectedKeyItem(void);
static VALUEITEM *GetSelectedValueItem(void);
static const char *RegistryTypeName(DWORD dwType);
static const char *RegistryErrorText(LONG lError, int id);
static void ShowRegistryError(HWND hwndOwner, LONG lError, int id);
static char *JoinSubPath(const char *pszParent, const char *pszChild);
static char *BuildKeyDisplayText(const char *pszKeyName, int nLevel, char chMarker);
static void DetermineSubkeyMarker(HKEY hRoot, const char *pszSubPath, int *pnHasSubkeys, int *pnAccessDenied);
static LONG InsertChildKeys(const KEYITEM *pParent, int nInsertIndex);
static void CollapseChildKeys(int nIndex);
static void RecomputeHorizontalExtent(HWND hwndList, HFONT hFont);
static char *FormatQuotedAnsiString(const BYTE *pbData, DWORD cbData);
static char *FormatMultiString(const BYTE *pbData, DWORD cbData);
static char *FormatBinaryString(const BYTE *pbData, DWORD cbData);
static char *FormatDwordString(const BYTE *pbData, DWORD cbData);
static char *FormatValueDisplay(DWORD dwType, const BYTE *pbData, DWORD cbData);
static LONG AppendTempValue(ENUMVALUETEMP **ppItems, int *pnCount, int *pnCapacity, const ENUMVALUETEMP *pValue);
static LONG PopulateValuesForKey(HKEY hRoot, const char *pszSubPath);
static LONG QueryValueData(HKEY hRoot, const char *pszSubPath, const char *pszValueName, DWORD *pdwType, BYTE **ppbData, DWORD *pcbData);
static LONG WriteValueData(HKEY hRoot, const char *pszSubPath, const char *pszValueName, DWORD dwType, const BYTE *pbData, DWORD cbData);
static int FindValueListIndexByName(const char *pszValueName);
static int ParseUnsignedDword(const char *pszText, int nBase, DWORD *pdwValue);
static char *DwordToHexText(DWORD dwValue);
static void SetButtonCheck(HWND hwndButton, int checked);
static int GetButtonCheck(HWND hwndButton);
static void UpdateDwordBaseButtons(EDITCTX *pCtx);
static void ConvertEditValueBase(EDITCTX *pCtx, int nNewBase);
static void ApplyUiFontToChildren(HWND hwndParent);
static void LayoutMainWindow(int cx, int cy);
static int DoEditValueModal(HWND hwndOwner, HKEY hRoot, const char *pszSubPath, const VALUEITEM *pValueItem);
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK GroupBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static int TwipsX(int twips)
{
    return MulDiv(twips, g_app.dpiX, 1440);
}

static int TwipsY(int twips)
{
    return MulDiv(twips, g_app.dpiY, 1440);
}

static void CenterWindowRelative(HWND hwnd, HWND hwndOwner)
{
    RECT rcOwner;
    RECT rcWindow;
    int x;
    int y;
    int cx;
    int cy;

    GetWindowRect(hwnd, &rcWindow);
    cx = rcWindow.right - rcWindow.left;
    cy = rcWindow.bottom - rcWindow.top;

    if (hwndOwner != NULL && IsWindow(hwndOwner)) {
        GetWindowRect(hwndOwner, &rcOwner);
    } else {
        rcOwner.left = 0;
        rcOwner.top = 0;
        rcOwner.right = GetSystemMetrics(SM_CXSCREEN);
        rcOwner.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    x = rcOwner.left + ((rcOwner.right - rcOwner.left) - cx) / 2;
    y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - cy) / 2;

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

static HFONT CreatePointFontACompat(const char *pszFace, int pointX100, int nWeight)
{
    LOGFONTA lf;

    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = -MulDiv(pointX100, g_app.dpiY, 7200);
    lf.lfWeight = nWeight;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfPitchAndFamily = FF_DONTCARE | DEFAULT_PITCH;
    strncpy(lf.lfFaceName, pszFace, LF_FACESIZE);

    return CreateFontIndirectA(&lf);
}

static void FatalOutOfMemory(void)
{
    MessageBoxA(NULL, "Out of memory", APP_TITLE, MB_OK | MB_ICONSTOP);
    ExitProcess(ERROR_OUTOFMEMORY);
}

static void *xmalloc(size_t cb)
{
    void *pv;

    if (cb == 0) {
        cb = 1;
    }

    pv = malloc(cb);
    if (pv == NULL) {
        FatalOutOfMemory();
    }
    return pv;
}

static void *xrealloc(void *pv, size_t cb)
{
    void *pNew;

    if (cb == 0) {
        cb = 1;
    }

    pNew = realloc(pv, cb);
    if (pNew == NULL) {
        FatalOutOfMemory();
    }
    return pNew;
}

static char *xstrdup(const char *psz)
{
    size_t cch;
    char *pszCopy;

    if (psz == NULL) {
        psz = "";
    }

    cch = strlen(psz);
    pszCopy = (char *)xmalloc(cch + 1);
    memcpy(pszCopy, psz, cch + 1);
    return pszCopy;
}

static char *xstrdup_len(const char *psz, size_t cch)
{
    char *pszCopy;

    pszCopy = (char *)xmalloc(cch + 1);
    if (cch != 0) {
        memcpy(pszCopy, psz, cch);
    }
    pszCopy[cch] = '\0';
    return pszCopy;
}

static void FreeKeyItem(KEYITEM *pItem)
{
    if (pItem != NULL) {
        free(pItem->pszSubPath);
        free(pItem);
    }
}

static void FreeValueItem(VALUEITEM *pItem)
{
    if (pItem != NULL) {
        free(pItem->pszValueName);
        free(pItem);
    }
}

static void FreeAllListItemData(HWND hwndList, int isValueList)
{
    int i;
    int count;
    void *pData;

    count = (int)SendMessageA(hwndList, LB_GETCOUNT, 0, 0);
    if (count == LB_ERR) {
        count = 0;
    }

    for (i = 0; i < count; ++i) {
        pData = (void *)SendMessageA(hwndList, LB_GETITEMDATA, (WPARAM)i, 0);
        if ((LONG_PTR)pData != (LONG_PTR)LB_ERR && pData != NULL) {
            if (isValueList) {
                FreeValueItem((VALUEITEM *)pData);
            } else {
                FreeKeyItem((KEYITEM *)pData);
            }
        }
    }

    SendMessageA(hwndList, LB_RESETCONTENT, 0, 0);
    SendMessageA(hwndList, LB_SETHORIZONTALEXTENT, 0, 0);
}

static LONG AddRootKey(HKEY hRoot, const char *pszTitle)
{
    KEYITEM *pItem;
    LRESULT index;

    pItem = (KEYITEM *)xmalloc(sizeof(*pItem));
    pItem->hRoot = hRoot;
    pItem->pszSubPath = xstrdup("");
    pItem->nLevel = 0;

    index = SendMessageA(g_app.hwndKeyList, LB_ADDSTRING, 0, (LPARAM)pszTitle);
    if (index == LB_ERR || index == LB_ERRSPACE) {
        FreeKeyItem(pItem);
        return ERROR_OUTOFMEMORY;
    }

    SendMessageA(g_app.hwndKeyList, LB_SETITEMDATA, (WPARAM)index, (LPARAM)pItem);
    return ERROR_SUCCESS;
}

static KEYITEM *GetSelectedKeyItem(void)
{
    int index;
    LRESULT data;

    index = (int)SendMessageA(g_app.hwndKeyList, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR) {
        return NULL;
    }

    data = SendMessageA(g_app.hwndKeyList, LB_GETITEMDATA, (WPARAM)index, 0);
    if (data == LB_ERR) {
        return NULL;
    }

    return (KEYITEM *)data;
}

static VALUEITEM *GetSelectedValueItem(void)
{
    int index;
    LRESULT data;

    index = (int)SendMessageA(g_app.hwndValueList, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR) {
        return NULL;
    }

    data = SendMessageA(g_app.hwndValueList, LB_GETITEMDATA, (WPARAM)index, 0);
    if (data == LB_ERR) {
        return NULL;
    }

    return (VALUEITEM *)data;
}

static const char *RegistryTypeName(DWORD dwType)
{
    switch (dwType) {
        case REG_NONE:
            return "REG_NONE";
        case REG_SZ:
            return "REG_SZ";
        case REG_EXPAND_SZ:
            return "REG_EXPAND_SZ";
        case REG_BINARY:
            return "REG_BINARY";
        case REG_DWORD:
            return "REG_DWORD";
        case REG_DWORD_BIG_ENDIAN:
            return "REG_DWORD_BIG_ENDIAN";
        case REG_LINK:
            return "REG_LINK";
        case REG_MULTI_SZ:
            return "REG_MULTI_SZ";
        case REG_RESOURCE_LIST:
            return "REG_RESOURCE_LIST";
        case REG_FULL_RESOURCE_DESCRIPTOR:
            return "REG_FULL_RESOURCE_DESCRIPTOR";
        case REG_RESOURCE_REQUIREMENTS_LIST:
            return "REG_RESOURCE_REQUIREMENTS_LIST";
        default:
            return NULL;
    }
}

static const char *RegistryErrorText(LONG lError, int id)
{
    static char szBuffer[128];

    switch (lError) {
        case ERROR_BADDB:
        case 1015:
            wsprintfA(szBuffer, "The Registry Database is corrupt! (%ld) #%d", lError, id);
            break;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_BADKEY:
            wsprintfA(szBuffer, "Bad Key Name! (%ld) #%d", lError, id);
            break;
        case ERROR_CANTOPEN:
            wsprintfA(szBuffer, "Can't Open Key (%ld) #%d", lError, id);
            break;
        case ERROR_CANTREAD:
            wsprintfA(szBuffer, "Can't Read Key (%ld) #%d", lError, id);
            break;
        case ERROR_ACCESS_DENIED:
            wsprintfA(szBuffer, "Access to this key is denied. (%ld) #%d", lError, id);
            break;
        case ERROR_CANTWRITE:
            wsprintfA(szBuffer, "Can't Write Key (%ld) #%d", lError, id);
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            wsprintfA(szBuffer, "Out of memory (%ld) #%d", lError, id);
            break;
        case ERROR_INVALID_PARAMETER:
            wsprintfA(szBuffer, "Invalid Parameter (%ld) #%d", lError, id);
            break;
        case ERROR_MORE_DATA:
            wsprintfA(szBuffer, "Error - There is more data than the buffer can handle! (%ld) #%d", lError, id);
            break;
        default:
            wsprintfA(szBuffer, "Undefined Key Error Code %ld! #%d", lError, id);
    }
    return szBuffer;
}

static void ShowRegistryError(HWND hwndOwner, LONG lError, int id)
{
    MessageBoxA(hwndOwner, RegistryErrorText(lError, id), APP_TITLE, MB_OK | MB_ICONEXCLAMATION);
}

static char *JoinSubPath(const char *pszParent, const char *pszChild)
{
    size_t cchParent;
    size_t cchChild;
    char *pszJoined;

    if (pszParent == NULL || pszParent[0] == '\0') {
        return xstrdup(pszChild);
    }

    cchParent = strlen(pszParent);
    cchChild = strlen(pszChild);
    pszJoined = (char *)xmalloc(cchParent + 1 + cchChild + 1);

    memcpy(pszJoined, pszParent, cchParent);
    pszJoined[cchParent] = '\\';
    memcpy(pszJoined + cchParent + 1, pszChild, cchChild + 1);

    return pszJoined;
}

static char *BuildKeyDisplayText(const char *pszKeyName, int nLevel, char chMarker)
{
    size_t cchIndent;
    size_t cchName;
    size_t cchTotal;
    size_t offset;
    char *pszText;

    cchIndent = (size_t)(nLevel * KEY_INDENT_SPACES);
    cchName = strlen(pszKeyName);
    cchTotal = cchIndent + cchName + (chMarker ? 1 : 0);

    pszText = (char *)xmalloc(cchTotal + 1);
    memset(pszText, ' ', cchIndent);

    offset = cchIndent;
    if (chMarker != '\0') {
        pszText[offset++] = chMarker;
    }

    memcpy(pszText + offset, pszKeyName, cchName);
    pszText[cchTotal] = '\0';
    return pszText;
}

static void DetermineSubkeyMarker(HKEY hRoot, const char *pszSubPath, int *pnHasSubkeys, int *pnAccessDenied)
{
    HKEY hKey;
    LONG lRet;
    char szName[KEYNAMESIZE];

    *pnHasSubkeys = 0;
    *pnAccessDenied = 0;
    *szName = 0;

    lRet = RegOpenKeyA(hRoot, pszSubPath, &hKey);
    if (lRet == ERROR_ACCESS_DENIED) {
        *pnAccessDenied = 1;
        return;
    }
    if (lRet != ERROR_SUCCESS) {
        return;
    }

    lRet = RegEnumKey(hKey, 0, szName, KEYNAMESIZE);
    if (*szName) {
        *pnHasSubkeys = 1;
    }
    if (lRet == ERROR_ACCESS_DENIED) {
        *pnAccessDenied = 1;
    }

    RegCloseKey(hKey);
}

static LONG InsertChildKeys(const KEYITEM *pParent, int nInsertIndex)
{
    HKEY hKey;
    LONG lRet;
    DWORD index;
    DWORD cUnused;
    DWORD cSubKeys;
    DWORD cchMaxSubKey;
    DWORD cchMaxClass;
    char *pszName;
    char *pszClass;
    char szClassName[64];
    DWORD cchClassLen = 64;
    DWORD cchNameAlloc;
    DWORD cchClassAlloc;
    FILETIME ft;

    pszName = NULL;
    pszClass = NULL;

    lRet = RegOpenKeyA(pParent->hRoot, pParent->pszSubPath, &hKey);
    if (lRet != ERROR_SUCCESS) {
        ShowRegistryError(g_app.hwndMain, lRet, 1);
        return lRet;
    }

    cSubKeys = 0;
    cchMaxSubKey = 0;
    cchMaxClass = 0;
    lRet = RegQueryInfoKeyA(hKey, (LPSTR)szClassName, &cchClassLen, NULL, &cSubKeys, &cchMaxSubKey, &cchMaxClass,
                            &cUnused, &cUnused, &cUnused, &cUnused, &ft);
    if (lRet == ERROR_CALL_NOT_IMPLEMENTED) { /* win32s hack */
        lRet = ERROR_SUCCESS;
        cchMaxClass = cchMaxSubKey = KEYNAMESIZE/4;
    }
    if (lRet != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        ShowRegistryError(g_app.hwndMain, lRet, 2);
        return lRet;
    }

    cchNameAlloc = cchMaxSubKey + 2;
    if (cchNameAlloc < 2) {
        cchNameAlloc = 2;
    }
    cchClassAlloc = cchMaxClass + 2;
    if (cchClassAlloc < 2) {
        cchClassAlloc = 2;
    }

    pszName = (char *)xmalloc(cchNameAlloc);
    pszClass = (char *)xmalloc(cchClassAlloc);

    index = 0;
    for (;;) {
        DWORD cchName;
        DWORD cchClass;

        cchName = cchNameAlloc;
        cchClass = cchClassAlloc;

        lRet = RegEnumKeyA(hKey, index, pszName, cchName);
        if (lRet == ERROR_MORE_DATA) {
            if (cchName + 2 > cchNameAlloc) {
                cchNameAlloc = cchName + 2;
                pszName = (char *)xrealloc(pszName, cchNameAlloc);
            }
            if (cchClass + 2 > cchClassAlloc) {
                cchClassAlloc = cchClass + 2;
                pszClass = (char *)xrealloc(pszClass, cchClassAlloc);
            }
            continue;
        }

        if (lRet == ERROR_NO_MORE_ITEMS || lRet == ERROR_FILE_NOT_FOUND) {  /* win32s hack */
            lRet = ERROR_SUCCESS;
            break;
        }

        if (lRet != ERROR_SUCCESS) {
            ShowRegistryError(g_app.hwndMain, lRet, 3);
            break;
        }

        pszName[cchName] = '\0';

        {
            char *pszFullPath;
            char *pszDisplay;
            KEYITEM *pChild;
            int hasSubkeys;
            int accessDenied;
            char chMarker;
            LRESULT listIndex;

            pszFullPath = JoinSubPath(pParent->pszSubPath, pszName);
            DetermineSubkeyMarker(pParent->hRoot, pszFullPath, &hasSubkeys, &accessDenied);

            chMarker = '\0';
            if (accessDenied) {
                chMarker = '*';
            } else if (hasSubkeys) {
                chMarker = '+';
            }

            pszDisplay = BuildKeyDisplayText(pszName, pParent->nLevel + 1, chMarker);

            pChild = (KEYITEM *)xmalloc(sizeof(*pChild));
            pChild->hRoot = pParent->hRoot;
            pChild->pszSubPath = pszFullPath;
            pChild->nLevel = pParent->nLevel + 1;

            listIndex = SendMessageA(g_app.hwndKeyList, LB_INSERTSTRING, (WPARAM)nInsertIndex, (LPARAM)pszDisplay);
            if (listIndex == LB_ERR || listIndex == LB_ERRSPACE) {
                FreeKeyItem(pChild);
                free(pszDisplay);
                lRet = ERROR_OUTOFMEMORY;
                ShowRegistryError(g_app.hwndMain, lRet, 4);
                break;
            }

            SendMessageA(g_app.hwndKeyList, LB_SETITEMDATA, (WPARAM)listIndex, (LPARAM)pChild);
            free(pszDisplay);
            ++nInsertIndex;
        }

        ++index;
    }

    free(pszName);
    free(pszClass);
    RegCloseKey(hKey);
    return lRet;
}

static void CollapseChildKeys(int nIndex)
{
    KEYITEM *pItem;
    int count;
    int i;

    pItem = (KEYITEM *)SendMessageA(g_app.hwndKeyList, LB_GETITEMDATA, (WPARAM)nIndex, 0);
    if (pItem == NULL || (LONG_PTR)pItem == (LONG_PTR)LB_ERR) {
        return;
    }

    count = (int)SendMessageA(g_app.hwndKeyList, LB_GETCOUNT, 0, 0);
    i = nIndex + 1;
    while (i < count) {
        KEYITEM *pChild;

        pChild = (KEYITEM *)SendMessageA(g_app.hwndKeyList, LB_GETITEMDATA, (WPARAM)i, 0);
        if (pChild == NULL || (LONG_PTR)pChild == (LONG_PTR)LB_ERR) {
            break;
        }
        if (pChild->nLevel <= pItem->nLevel) {
            break;
        }

        FreeKeyItem(pChild);
        SendMessageA(g_app.hwndKeyList, LB_DELETESTRING, (WPARAM)i, 0);
        --count;
    }
}

static void RecomputeHorizontalExtent(HWND hwndList, HFONT hFont)
{
    HDC hdc;
    HFONT hOldFont;
    int count;
    int i;
    int maxWidth;
    SIZE size;

    maxWidth = 0;
    count = (int)SendMessageA(hwndList, LB_GETCOUNT, 0, 0);
    if (count == LB_ERR) {
        count = 0;
    }

    hdc = GetDC(hwndList);
    hOldFont = NULL;
    if (hdc != NULL && hFont != NULL) {
        hOldFont = (HFONT)SelectObject(hdc, hFont);
    }

    for (i = 0; i < count; ++i) {
        int cch;
        char *pszText;

        cch = (int)SendMessageA(hwndList, LB_GETTEXTLEN, (WPARAM)i, 0);
        if (cch == LB_ERR) {
            continue;
        }

        pszText = (char *)xmalloc((size_t)cch + 1);
        SendMessageA(hwndList, LB_GETTEXT, (WPARAM)i, (LPARAM)pszText);
        pszText[cch] = '\0';

        if (hdc != NULL) {
            GetTextExtentPointA(hdc, pszText, cch, &size);
            if ((int)size.cx > maxWidth) {
                maxWidth = size.cx;
            }
        }

        free(pszText);
    }

    if (hdc != NULL) {
        if (hOldFont != NULL) {
            SelectObject(hdc, hOldFont);
        }
        ReleaseDC(hwndList, hdc);
    }

    SendMessageA(hwndList, LB_SETHORIZONTALEXTENT, (WPARAM)maxWidth, 0);
}

static char *FormatQuotedAnsiString(const BYTE *pbData, DWORD cbData)
{
    DWORD usable;
    char *pszOut;

    usable = cbData;
    while (usable > 0 && pbData[usable - 1] == 0) {
        --usable;
    }

    pszOut = (char *)xmalloc((size_t)usable + 3);
    pszOut[0] = '"';
    if (usable != 0) {
        memcpy(pszOut + 1, pbData, usable);
    }
    pszOut[usable + 1] = '"';
    pszOut[usable + 2] = '\0';
    return pszOut;
}

static char *FormatMultiString(const BYTE *pbData, DWORD cbData)
{
    DWORD i;
    char *pszOut;
    size_t cch;

    pszOut = (char *)xmalloc((size_t)cbData + 1);
    for (i = 0; i < cbData; ++i) {
        pszOut[i] = (pbData[i] == 0) ? ' ' : (char)pbData[i];
    }
    pszOut[cbData] = '\0';

    cch = strlen(pszOut);
    while (cch > 0 && pszOut[cch - 1] == ' ') {
        pszOut[cch - 1] = '\0';
        --cch;
    }

    return pszOut;
}

static char *FormatBinaryString(const BYTE *pbData, DWORD cbData)
{
    char *pszOut;
    char *pszWrite;
    DWORD i;

    if (cbData == 0) {
        return xstrdup("");
    }

    pszOut = (char *)xmalloc((size_t)cbData * 3 + 1);
    pszWrite = pszOut;
    for (i = 0; i < cbData; ++i) {
        sprintf(pszWrite, "%02X ", (unsigned int)pbData[i]);
        pszWrite += 3;
    }
    *pszWrite = '\0';
    return pszOut;
}

static char *FormatDwordString(const BYTE *pbData, DWORD cbData)
{
    DWORD dwValue;
    char szBuffer[64];

    if (cbData < 4) {
        return xstrdup("(invalid DWORD)");
    }

    dwValue = ((DWORD)pbData[0]) |
              ((DWORD)pbData[1] << 8) |
              ((DWORD)pbData[2] << 16) |
              ((DWORD)pbData[3] << 24);

    sprintf(szBuffer, "&H%lX (%lu)", (unsigned long)dwValue, (unsigned long)dwValue);
    return xstrdup(szBuffer);
}

static char *FormatValueDisplay(DWORD dwType, const BYTE *pbData, DWORD cbData)
{
    char *pszOut;

    switch (dwType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            pszOut = FormatQuotedAnsiString(pbData, cbData);
            break;

        case REG_MULTI_SZ:
            pszOut = FormatMultiString(pbData, cbData);
            break;

        case REG_DWORD:
            pszOut = FormatDwordString(pbData, cbData);
            break;

        case REG_BINARY:
            pszOut = FormatBinaryString(pbData, cbData);
            break;

        case REG_RESOURCE_LIST:
            pszOut = xstrdup("REG_RESOURCE_LIST");
            break;

        case REG_FULL_RESOURCE_DESCRIPTOR:
            pszOut = xstrdup("REG_FULL_RESOURCE_DESCRIPTOR");
            break;

        case REG_RESOURCE_REQUIREMENTS_LIST:
            pszOut = xstrdup("REG_RESOURCE_REQUIREMENTS_LIST");
            break;

        case REG_LINK:
            pszOut = xstrdup("REG_LINK");
            break;

        case REG_NONE:
            if (cbData == 0) {
                pszOut = xstrdup("");
            } else {
                pszOut = FormatBinaryString(pbData, cbData);
            }
            break;

        default:
        {
            const char *pszTypeName;
            pszTypeName = RegistryTypeName(dwType);
            if (pszTypeName != NULL) {
                pszOut = xstrdup(pszTypeName);
            } else {
                pszOut = FormatBinaryString(pbData, cbData);
            }
            break;
        }
    }

    if (pszOut[0] == '\0') {
        free(pszOut);
        pszOut = xstrdup("(value not set)");
    }

    return pszOut;
}

static LONG AppendTempValue(ENUMVALUETEMP **ppItems, int *pnCount, int *pnCapacity, const ENUMVALUETEMP *pValue)
{
    if (*pnCount >= *pnCapacity) {
        int nNewCapacity;

        nNewCapacity = (*pnCapacity == 0) ? 16 : (*pnCapacity * 2);
        *ppItems = (ENUMVALUETEMP *)xrealloc(*ppItems, (size_t)nNewCapacity * sizeof(**ppItems));
        *pnCapacity = nNewCapacity;
    }

    (*ppItems)[*pnCount] = *pValue;
    ++(*pnCount);
    return ERROR_SUCCESS;
}

static LONG PopulateValuesForKey(HKEY hRoot, const char *pszSubPath)
{
    HKEY hKey;
    LONG lRet;
    DWORD cUnused;
    DWORD cValues;
    DWORD cchMaxValueName;
    DWORD cbMaxValueData;
    char szClassName[64];
    DWORD cchClassLen = 64;
    FILETIME ft;
    ENUMVALUETEMP *pItems;
    int nCount;
    int nCapacity;
    int nMaxNameWidth;
    DWORD index;
    int i;

    FreeAllListItemData(g_app.hwndValueList, 1);
    g_app.currentValueNameWidth = NAME_DEFAULT_LEN;

    lRet = RegOpenKeyA(hRoot, pszSubPath, &hKey);
    if (lRet != ERROR_SUCCESS) {
        ShowRegistryError(g_app.hwndMain, lRet, 5);
        return lRet;
    }

    cValues = 0;
    cchMaxValueName = 0;
    cbMaxValueData = 0;
    lRet = RegQueryInfoKeyA(hKey, (LPSTR)szClassName, &cchClassLen, NULL, &cUnused, &cUnused, &cUnused, &cValues,
                            &cchMaxValueName, &cbMaxValueData, &cUnused, &ft);
    if (lRet == ERROR_CALL_NOT_IMPLEMENTED) { /* win32s hack */
        lRet = ERROR_SUCCESS;
        cchMaxValueName = cbMaxValueData = KEYNAMESIZE/4;
    }
    if (lRet != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        ShowRegistryError(g_app.hwndMain, lRet, 6);
        return lRet;
    }

    pItems = NULL;
    nCount = 0;
    nCapacity = 0;
    nMaxNameWidth = NAME_DEFAULT_LEN;

    index = 0;
    for (;;) {
        char *pszNameBuf;
        BYTE *pbDataBuf;
        DWORD cchNameAlloc;
        DWORD cbDataAlloc;
        DWORD cchName;
        DWORD cbData;
        DWORD dwType;
        int fTooLarge;
        ENUMVALUETEMP temp;

        ZeroMemory(&temp, sizeof(temp));
        cchNameAlloc = cchMaxValueName + 2;
        if (cchNameAlloc < 2) {
            cchNameAlloc = 2;
        }
        cbDataAlloc = cbMaxValueData + 2;
        if (cbDataAlloc > MAX_DISPLAY_DATA + 1) {
            cbDataAlloc = MAX_DISPLAY_DATA + 1;
        }

        pszNameBuf = (char *)xmalloc(cchNameAlloc);
        pbDataBuf = NULL;
        if (cbDataAlloc != 0) {
            pbDataBuf = (BYTE *)xmalloc(cbDataAlloc);
        }

        cchName = cchNameAlloc;
        cbData = cbDataAlloc;
        dwType = REG_NONE;
        fTooLarge = 0;

RetryEnumValue:
        lRet = RegEnumValueA(hKey, index, pszNameBuf, &cchName, NULL, &dwType, pbDataBuf, &cbData);
        if (lRet == ERROR_MORE_DATA) {
            int fRetry;

            fRetry = 0;
            if (cchName + 2 > cchNameAlloc) {
                cchNameAlloc = cchName + 2;
                pszNameBuf = (char *)xrealloc(pszNameBuf, cchNameAlloc);
                fRetry = 1;
            }
            if (cbData > cbDataAlloc && cbData <= MAX_DISPLAY_DATA) {
                cbDataAlloc = cbData + 1;
                pbDataBuf = (BYTE *)xrealloc(pbDataBuf, cbDataAlloc);
                fRetry = 1;
            }
            if (cbData > MAX_DISPLAY_DATA) {
                fTooLarge = 1;
            }
            if (fRetry) {
                cchName = cchNameAlloc;
                if (!fTooLarge) {
                    cbData = cbDataAlloc;
                }
                goto RetryEnumValue;
            }
        }

        if (lRet == ERROR_NO_MORE_ITEMS) {
            lRet = ERROR_SUCCESS;
            free(pszNameBuf);
            free(pbDataBuf);
            break;
        }

        if (lRet != ERROR_SUCCESS && lRet != ERROR_MORE_DATA && lRet != ERROR_CALL_NOT_IMPLEMENTED) {
            ShowRegistryError(g_app.hwndMain, lRet, 7);
            free(pszNameBuf);
            free(pbDataBuf);
            break;
        }

        if (lRet == ERROR_CALL_NOT_IMPLEMENTED) { /* win32s hack */
            RegQueryValueA(hKey, NULL, pbDataBuf, &cbData);
            dwType = REG_SZ;
        }

        if (cchName >= cchNameAlloc) {
            cchName = cchNameAlloc - 1;
        }
        pszNameBuf[cchName] = '\0';

        temp.pszRawName = xstrdup(pszNameBuf);
        if (lRet == ERROR_CALL_NOT_IMPLEMENTED || temp.pszRawName[0] == '\0') {
            temp.pszRawName[0] = 0; /* win32s hack */
            temp.pszDisplayName = xstrdup(NAME_DEFAULT);
        } else {
            temp.pszDisplayName = xstrdup(temp.pszRawName);
        }
        if ((int)strlen(temp.pszDisplayName) > nMaxNameWidth) {
            nMaxNameWidth = (int)strlen(temp.pszDisplayName);
        }
        temp.dwType = dwType;

        if (fTooLarge || lRet == ERROR_MORE_DATA) {
            temp.pszDisplayData = xstrdup("(Value too large to display)");
        } else {
            temp.pszDisplayData = FormatValueDisplay(dwType, pbDataBuf, cbData);
        }

        AppendTempValue(&pItems, &nCount, &nCapacity, &temp);
        free(pszNameBuf);
        free(pbDataBuf);
        ++index;

        if (lRet == ERROR_CALL_NOT_IMPLEMENTED) {
            lRet = ERROR_SUCCESS;
            break;
        }

    }

    RegCloseKey(hKey);

    g_app.currentValueNameWidth = nMaxNameWidth;

    for (i = 0; i < nCount; ++i) {
        VALUEITEM *pValue;
        size_t cchDisplayName;
        size_t cchDisplayData;
        size_t cchRow;
        char *pszRow;
        LRESULT listIndex;

        cchDisplayName = strlen(pItems[i].pszDisplayName);
        cchDisplayData = strlen(pItems[i].pszDisplayData);
        cchRow = (size_t)g_app.currentValueNameWidth + 1 + cchDisplayData;

        pszRow = (char *)xmalloc(cchRow + 1);
        memcpy(pszRow, pItems[i].pszDisplayName, cchDisplayName);
        memset(pszRow + cchDisplayName, ' ', (size_t)g_app.currentValueNameWidth - cchDisplayName + 1);
        memcpy(pszRow + g_app.currentValueNameWidth + 1, pItems[i].pszDisplayData, cchDisplayData + 1);

        pValue = (VALUEITEM *)xmalloc(sizeof(*pValue));
        pValue->pszValueName = pItems[i].pszRawName;
        pValue->dwType = pItems[i].dwType;

        listIndex = SendMessageA(g_app.hwndValueList, LB_ADDSTRING, 0, (LPARAM)pszRow);
        if (listIndex == LB_ERR || listIndex == LB_ERRSPACE) {
            FreeValueItem(pValue);
            pItems[i].pszRawName = NULL;
            free(pszRow);
            lRet = ERROR_OUTOFMEMORY;
            ShowRegistryError(g_app.hwndMain, lRet, 8);
            break;
        }

        SendMessageA(g_app.hwndValueList, LB_SETITEMDATA, (WPARAM)listIndex, (LPARAM)pValue);

        free(pszRow);
        free(pItems[i].pszDisplayName);
        free(pItems[i].pszDisplayData);
    }

    for (; i < nCount; ++i) {
        free(pItems[i].pszRawName);
        free(pItems[i].pszDisplayName);
        free(pItems[i].pszDisplayData);
    }
    free(pItems);

    RecomputeHorizontalExtent(g_app.hwndValueList, g_app.hListFont);
    return lRet;
}

static LONG QueryValueData(HKEY hRoot, const char *pszSubPath, const char *pszValueName, DWORD *pdwType, BYTE **ppbData, DWORD *pcbData)
{
    HKEY hKey;
    LONG lRet;
    DWORD cbData;
    BYTE *pbData;
    const char *pszQueryName;

    *pdwType = REG_NONE;
    *ppbData = NULL;
    *pcbData = 0;

    pszQueryName = (pszValueName != NULL) ? pszValueName : "";

    lRet = RegOpenKeyA(hRoot, pszSubPath, &hKey);
    if (lRet != ERROR_SUCCESS) {
        return lRet;
    }

    cbData = 0;
    lRet = RegQueryValueExA(hKey, pszQueryName, NULL, pdwType, NULL, &cbData);
    if (lRet != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return lRet;
    }

    if (cbData == 0) {
        pbData = (BYTE *)xmalloc(1);
        pbData[0] = 0;
    } else {
        pbData = (BYTE *)xmalloc(cbData + 1);
        lRet = RegQueryValueExA(hKey, pszQueryName, NULL, pdwType, pbData, &cbData);
        if (lRet != ERROR_SUCCESS) {
            free(pbData);
            RegCloseKey(hKey);
            return lRet;
        }
        pbData[cbData] = 0;
    }

    RegCloseKey(hKey);
    *ppbData = pbData;
    *pcbData = cbData;
    return ERROR_SUCCESS;
}

static LONG WriteValueData(HKEY hRoot, const char *pszSubPath, const char *pszValueName, DWORD dwType, const BYTE *pbData, DWORD cbData)
{
    HKEY hKey;
    LONG lRet;
    const char *pszQueryName;

    pszQueryName = (pszValueName != NULL) ? pszValueName : "";

    lRet = RegOpenKeyA(hRoot, pszSubPath, &hKey);
    if (lRet != ERROR_SUCCESS) {
        return lRet;
    }

    lRet = RegSetValueExA(hKey, pszQueryName, 0, dwType, pbData, cbData);
    RegCloseKey(hKey);
    return lRet;
}

static int FindValueListIndexByName(const char *pszValueName)
{
    int count;
    int i;

    count = (int)SendMessageA(g_app.hwndValueList, LB_GETCOUNT, 0, 0);
    if (count == LB_ERR) {
        return LB_ERR;
    }

    for (i = 0; i < count; ++i) {
        VALUEITEM *pValue;

        pValue = (VALUEITEM *)SendMessageA(g_app.hwndValueList, LB_GETITEMDATA, (WPARAM)i, 0);
        if (pValue == NULL || (LONG_PTR)pValue == (LONG_PTR)LB_ERR) {
            continue;
        }
        if (lstrcmpA(pValue->pszValueName, pszValueName) == 0) {
            return i;
        }
    }

    return LB_ERR;
}

static int ParseUnsignedDword(const char *pszText, int nBase, DWORD *pdwValue)
{
    char *pszEnd;
    unsigned long ulValue;
    const char *pszParse;

    pszParse = pszText;
    while (*pszParse == ' ' || *pszParse == '\t') {
        ++pszParse;
    }
    if (*pszParse == '\0') {
        return 0;
    }

    if (nBase == 16) {
        if ((pszParse[0] == '&') && (pszParse[1] == 'H' || pszParse[1] == 'h')) {
            pszParse += 2;
        } else if (pszParse[0] == '0' && (pszParse[1] == 'x' || pszParse[1] == 'X')) {
            pszParse += 2;
        }
    }

    errno = 0;
    ulValue = strtoul(pszParse, &pszEnd, nBase);
    if (errno == ERANGE || ulValue > 0xFFFFFFFFUL) {
        return 0;
    }
    while (*pszEnd == ' ' || *pszEnd == '\t') {
        ++pszEnd;
    }
    if (*pszEnd != '\0') {
        return 0;
    }

    *pdwValue = (DWORD)ulValue;
    return 1;
}

static char *DwordToHexText(DWORD dwValue)
{
    char szBuffer[32];
    sprintf(szBuffer, "%lX", (unsigned long)dwValue);
    return xstrdup(szBuffer);
}

static void SetButtonCheck(HWND hwndButton, int checked)
{
    SendMessageA(hwndButton, BM_SETCHECK, (WPARAM)(checked ? BST_CHECKED : BST_UNCHECKED), 0);
}

static int GetButtonCheck(HWND hwndButton)
{
    return (SendMessageA(hwndButton, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

static void UpdateDwordBaseButtons(EDITCTX *pCtx)
{
    SetButtonCheck(pCtx->hwndHex, pCtx->nCurrentBase == 0);
    SetButtonCheck(pCtx->hwndDec, pCtx->nCurrentBase == 1);
}

static void ConvertEditValueBase(EDITCTX *pCtx, int nNewBase)
{
    char szText[128];
    DWORD dwValue;

    if (pCtx->dwType != REG_DWORD) {
        return;
    }
    if (pCtx->nCurrentBase == nNewBase) {
        UpdateDwordBaseButtons(pCtx);
        return;
    }

    GetWindowTextA(pCtx->hwndValueData, szText, ARRAYSIZE(szText));
    if (!ParseUnsignedDword(szText, (pCtx->nCurrentBase == 0) ? 16 : 10, &dwValue)) {
        MessageBeep(MB_ICONEXCLAMATION);
        UpdateDwordBaseButtons(pCtx);
        return;
    }

    if (nNewBase == 0) {
        char *pszHex;
        pszHex = DwordToHexText(dwValue);
        SetWindowTextA(pCtx->hwndValueData, pszHex);
        free(pszHex);
    } else {
        sprintf(szText, "%lu", (unsigned long)dwValue);
        SetWindowTextA(pCtx->hwndValueData, szText);
    }

    pCtx->nCurrentBase = nNewBase;
    UpdateDwordBaseButtons(pCtx);
}

static void ApplyUiFontToChildren(HWND hwndParent)
{
    HWND hwndChild;

    hwndChild = GetWindow(hwndParent, GW_CHILD);
    while (hwndChild != NULL) {
        SendMessageA(hwndChild, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);
        hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
    }
}

static void LayoutMainWindow(int cx, int cy)
{
    int listTop;
    int listHeight;
    int leftMargin;
    int leftWidth;
    int rightX;
    int rightWidth;
    int label3Y;
    int label4Y;
    int buttonW;
    int buttonH;
    int buttonX;
    int buttonY;

    listTop = TwipsY(300);
    listHeight = cy - TwipsY(1200);
    leftMargin = TwipsX(60);
    leftWidth = (cx / 2) - TwipsX(300);
    rightX = leftWidth + TwipsX(90);
    rightWidth = leftWidth + TwipsX(480);

    if (leftWidth < 32) {
        leftWidth = 32;
    }
    if (rightX < leftMargin + leftWidth + 2) {
        rightX = leftMargin + leftWidth + 2;
    }
    if (rightWidth < 32) {
        rightWidth = 32;
    }
    if (rightX + rightWidth > cx - 2) {
        rightWidth = cx - rightX - 2;
    }
    if (listHeight < 32) {
        listHeight = 32;
    }

    MoveWindow(g_app.hwndKeyList, leftMargin, listTop, leftWidth, listHeight, TRUE);
    MoveWindow(g_app.hwndValueList, rightX, listTop, rightWidth, listHeight, TRUE);

    MoveWindow(g_app.hwndLabelKey, leftMargin + TwipsX(60), TwipsY(60), TwipsX(1272), TwipsY(192), TRUE);
    MoveWindow(g_app.hwndLabelValue, rightX + TwipsX(60), TwipsY(60), TwipsX(1692), TwipsY(192), TRUE);

    label3Y = listTop + listHeight + TwipsY(150);
    label4Y = label3Y;
    MoveWindow(g_app.hwndLabelKeysDbl, TwipsX(90), label3Y, TwipsX(3612), TwipsY(252), TRUE);
    MoveWindow(g_app.hwndLabelValDbl, rightX + TwipsX(90), label4Y, TwipsX(3312), TwipsY(252), TRUE);
    MoveWindow(g_app.hwndLabelPlus, TwipsX(240), label3Y + TwipsY(240), TwipsX(2772), TwipsY(252), TRUE);
    MoveWindow(g_app.hwndLabelStar, TwipsX(240), label3Y + TwipsY(480), TwipsX(2772), TwipsY(252), TRUE);

    buttonW = TwipsX(972);
    buttonH = TwipsY(372);
    buttonX = cx - buttonW - TwipsX(120);
    buttonY = label4Y + TwipsY(330);
    MoveWindow(g_app.hwndExit, buttonX, buttonY, buttonW, buttonH, TRUE);
}

static int DoEditValueModal(HWND hwndOwner, HKEY hRoot, const char *pszSubPath, const VALUEITEM *pValueItem)
{
    BYTE *pbData;
    DWORD cbData;
    DWORD dwType;
    LONG lRet;
    EDITCTX ctx;
    RECT rc;
    DWORD dwStyle;
    DWORD dwExStyle;
    MSG msg;

    pbData = NULL;
    cbData = 0;
    dwType = REG_NONE;

    lRet = QueryValueData(hRoot, pszSubPath, pValueItem->pszValueName, &dwType, &pbData, &cbData);
    if (lRet != ERROR_SUCCESS) {
        ShowRegistryError(hwndOwner, lRet, 9);
        return IDCANCEL;
    }

    if (dwType != REG_SZ && dwType != REG_DWORD) {
        char szMessage[256];
        const char *pszTypeName;

        pszTypeName = RegistryTypeName(dwType);
        if (pszTypeName == NULL) {
            pszTypeName = "an unsupported type";
        }
        wsprintfA(szMessage,
                  "This Demo only supports editing of values with types of REG_SZ and REG_DWORD.  This value is of type %s.",
                  pszTypeName);
        MessageBoxA(hwndOwner, szMessage, APP_TITLE, MB_OK | MB_ICONINFORMATION);
        free(pbData);
        return IDCANCEL;
    }

    ZeroMemory(&ctx, sizeof(ctx));
    ctx.hRoot = hRoot;
    ctx.pszSubPath = xstrdup(pszSubPath);
    ctx.pszValueName = xstrdup(pValueItem->pszValueName);
    ctx.dwType = dwType;
    ctx.nCurrentBase = 0;
    ctx.nResult = IDCANCEL;

    if (dwType == REG_DWORD) {
        DWORD dwValue;
        dwValue = 0;
        if (cbData >= 4) {
            dwValue = ((DWORD)pbData[0]) |
                      ((DWORD)pbData[1] << 8) |
                      ((DWORD)pbData[2] << 16) |
                      ((DWORD)pbData[3] << 24);
        }
        ctx.dwInitialDword = dwValue;
        ctx.pszInitialText = DwordToHexText(dwValue);
    } else {
        ctx.pszInitialText = xstrdup_len((const char *)pbData, cbData ? (size_t)(cbData - (pbData[cbData - 1] == 0 ? 1 : 0)) : 0);
    }

    free(pbData);

    dwStyle = WS_CAPTION | WS_POPUP | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT;
    rc.left = 0;
    rc.top = 0;
    rc.right = TwipsX(5520);
    rc.bottom = TwipsY(2790);
    AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);

    ctx.hwnd = CreateWindowExA(dwExStyle,
                               EDIT_CLASS_NAME,
                               (dwType == REG_DWORD) ? EDIT_TITLE_DWORD : EDIT_TITLE_STRING,
                               dwStyle,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               rc.right - rc.left,
                               rc.bottom - rc.top,
                               hwndOwner,
                               NULL,
                               g_app.hInstance,
                               &ctx);

    if (ctx.hwnd == NULL) {
        free(ctx.pszSubPath);
        free(ctx.pszValueName);
        free(ctx.pszInitialText);
        return IDCANCEL;
    }

    EnableWindow(hwndOwner, FALSE);
    ShowWindow(ctx.hwnd, SW_SHOW);
    UpdateWindow(ctx.hwnd);

    while (IsWindow(ctx.hwnd) && GetMessage(&msg, NULL, 0, 0) > 0) {
        if ((msg.hwnd == ctx.hwnd || IsChild(ctx.hwnd, msg.hwnd)) && msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE) {
                SendMessageA(ctx.hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
                continue;
            }
            if (msg.wParam == VK_RETURN) {
                SendMessageA(ctx.hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
                continue;
            }
        }

        if (!IsDialogMessage(ctx.hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(hwndOwner, TRUE);
    SetActiveWindow(hwndOwner);
    SetForegroundWindow(hwndOwner);

    free(ctx.pszSubPath);
    free(ctx.pszValueName);
    free(ctx.pszInitialText);
    return ctx.nResult;
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
        {
            HCURSOR hOldCursor;

            g_app.hwndMain = hwnd;

            g_app.hwndKeyList = CreateWindowExA(0, "LISTBOX", "",
                                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                                                LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER,
                                                0, 0, 0, 0,
                                                hwnd, (HMENU)IDC_KEY_LIST, g_app.hInstance, NULL);
            g_app.hwndValueList = CreateWindowExA(0, "LISTBOX", "",
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
                                                  LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER,
                                                  0, 0, 0, 0,
                                                  hwnd, (HMENU)IDC_VALUE_LIST, g_app.hInstance, NULL);
            g_app.hwndExit = CreateWindowExA(0, "BUTTON", "Exit",
                                             WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                             0, 0, 0, 0,
                                             hwnd, (HMENU)IDC_EXIT_BUTTON, g_app.hInstance, NULL);

            g_app.hwndLabelKey = CreateWindowExA(0, "STATIC", "Key Name",
                                                 WS_CHILD | WS_VISIBLE,
                                                 0, 0, 0, 0,
                                                 hwnd, (HMENU)IDC_LABEL_KEY, g_app.hInstance, NULL);
            g_app.hwndLabelValue = CreateWindowExA(0, "STATIC", "Value Data",
                                                   WS_CHILD | WS_VISIBLE,
                                                   0, 0, 0, 0,
                                                   hwnd, (HMENU)IDC_LABEL_VALUE, g_app.hInstance, NULL);
            g_app.hwndLabelKeysDbl = CreateWindowExA(0, "STATIC", "Double-Click a Key to expand it",
                                                     WS_CHILD | WS_VISIBLE,
                                                     0, 0, 0, 0,
                                                     hwnd, (HMENU)IDC_LABEL_KEYS_DBL, g_app.hInstance, NULL);
            g_app.hwndLabelValDbl = CreateWindowExA(0, "STATIC", "Double-Click a Value to edit it",
                                                    WS_CHILD | WS_VISIBLE,
                                                    0, 0, 0, 0,
                                                    hwnd, (HMENU)IDC_LABEL_VAL_DBL, g_app.hInstance, NULL);
            g_app.hwndLabelPlus = CreateWindowExA(0, "STATIC", "+ means additional keys",
                                                  WS_CHILD | WS_VISIBLE,
                                                  0, 0, 0, 0,
                                                  hwnd, (HMENU)IDC_LABEL_PLUS, g_app.hInstance, NULL);
            g_app.hwndLabelStar = CreateWindowExA(0, "STATIC", "* means key cannot be opened",
                                                  WS_CHILD | WS_VISIBLE,
                                                  0, 0, 0, 0,
                                                  hwnd, (HMENU)IDC_LABEL_STAR, g_app.hInstance, NULL);

            SendMessageA(g_app.hwndKeyList, WM_SETFONT, (WPARAM)g_app.hListFont, TRUE);
            SendMessageA(g_app.hwndValueList, WM_SETFONT, (WPARAM)g_app.hListFont, TRUE);
            SendMessageA(g_app.hwndExit, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);
            SendMessageA(g_app.hwndLabelKey, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);
            SendMessageA(g_app.hwndLabelValue, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);
            SendMessageA(g_app.hwndLabelKeysDbl, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);
            SendMessageA(g_app.hwndLabelValDbl, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);
            SendMessageA(g_app.hwndLabelPlus, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);
            SendMessageA(g_app.hwndLabelStar, WM_SETFONT, (WPARAM)g_app.hUiFont, TRUE);

            if (g_app.hIcon != NULL) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_app.hIcon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_app.hIcon);
            }

            AddRootKey(HKEY_CLASSES_ROOT, "HKEY_CLASSES_ROOT");
            AddRootKey(HKEY_CURRENT_USER, "HKEY_CURRENT_USER");
            AddRootKey(HKEY_LOCAL_MACHINE, "HKEY_LOCAL_MACHINE");
            AddRootKey(HKEY_USERS, "HKEY_USERS");
#if 1
            AddRootKey(HKEY_CURRENT_CONFIG, "HKEY_CURRENT_CONFIG");
#else
            if (g_app.isWin9x) {
                AddRootKey(HKEY_CURRENT_CONFIG, "HKEY_CURRENT_CONFIG");
            }
#endif
            if (g_app.isWin9x) {
                AddRootKey(HKEY_DYN_DATA, "HKEY_DYN_DATA");
            }

            RecomputeHorizontalExtent(g_app.hwndKeyList, g_app.hListFont);
            CenterWindowRelative(hwnd, NULL);

            hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            SetCursor(hOldCursor);
            return 0;
        }

        case WM_SIZE:
            LayoutMainWindow(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *pInfo;
            pInfo = (MINMAXINFO *)lParam;
            pInfo->ptMinTrackSize.x = g_app.minWindowWidth;
            pInfo->ptMinTrackSize.y = g_app.minWindowHeight;
            return 0;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        {
            HDC hdc;
            hdc = (HDC)wParam;
            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
            SetBkColor(hdc, g_app.crFace);
            return (LRESULT)g_app.hbrFace;
        }

        case WM_COMMAND:
        {
            WORD id;
            WORD notify;

            id = LOWORD(wParam);
            notify = HIWORD(wParam);

            if (id == IDC_EXIT_BUTTON && notify == BN_CLICKED) {
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                return 0;
            }

            if (id == IDC_KEY_LIST && notify == LBN_SELCHANGE) {
                KEYITEM *pItem;
                HCURSOR hOldCursor;

                pItem = GetSelectedKeyItem();
                if (pItem != NULL) {
                    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    PopulateValuesForKey(pItem->hRoot, pItem->pszSubPath);
                    SetCursor(hOldCursor);
                }
                return 0;
            }

            if (id == IDC_KEY_LIST && notify == LBN_DBLCLK) {
                int index;
                int count;
                KEYITEM *pItem;
                KEYITEM *pNext;
                HCURSOR hOldCursor;

                index = (int)SendMessageA(g_app.hwndKeyList, LB_GETCURSEL, 0, 0);
                if (index == LB_ERR) {
                    return 0;
                }

                pItem = (KEYITEM *)SendMessageA(g_app.hwndKeyList, LB_GETITEMDATA, (WPARAM)index, 0);
                if (pItem == NULL || (LONG_PTR)pItem == (LONG_PTR)LB_ERR) {
                    return 0;
                }

                count = (int)SendMessageA(g_app.hwndKeyList, LB_GETCOUNT, 0, 0);
                if (index + 1 < count) {
                    pNext = (KEYITEM *)SendMessageA(g_app.hwndKeyList, LB_GETITEMDATA, (WPARAM)(index + 1), 0);
                } else {
                    pNext = NULL;
                }

                if (pNext != NULL && (LONG_PTR)pNext != (LONG_PTR)LB_ERR && pNext->nLevel > pItem->nLevel) {
                    CollapseChildKeys(index);
                } else {
                    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    InsertChildKeys(pItem, index + 1);
                    SetCursor(hOldCursor);
                }

                RecomputeHorizontalExtent(g_app.hwndKeyList, g_app.hListFont);
                SendMessageA(g_app.hwndKeyList, LB_SETCURSEL, (WPARAM)index, 0);
                return 0;
            }

            if (id == IDC_VALUE_LIST && notify == LBN_DBLCLK) {
                VALUEITEM *pValueItem;
                KEYITEM *pKeyItem;
                int nSelected;
                int nIndex;
                char *pszValueNameCopy;

                pValueItem = GetSelectedValueItem();
                pKeyItem = GetSelectedKeyItem();
                if (pValueItem == NULL || pKeyItem == NULL) {
                    return 0;
                }

                pszValueNameCopy = xstrdup(pValueItem->pszValueName);
                nSelected = DoEditValueModal(hwnd, pKeyItem->hRoot, pKeyItem->pszSubPath, pValueItem);
                if (nSelected == IDOK) {
                    int nReselect;
                    HCURSOR hOldCursor;

                    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    PopulateValuesForKey(pKeyItem->hRoot, pKeyItem->pszSubPath);
                    SetCursor(hOldCursor);
                    nReselect = FindValueListIndexByName(pszValueNameCopy);
                    if (nReselect != LB_ERR) {
                        SendMessageA(g_app.hwndValueList, LB_SETCURSEL, (WPARAM)nReselect, 0);
                    }
                } else {
                    nIndex = FindValueListIndexByName(pszValueNameCopy);
                    if (nIndex != LB_ERR) {
                        SendMessageA(g_app.hwndValueList, LB_SETCURSEL, (WPARAM)nIndex, 0);
                    }
                }
                free(pszValueNameCopy);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            FreeAllListItemData(g_app.hwndValueList, 1);
            FreeAllListItemData(g_app.hwndKeyList, 0);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK EditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    EDITCTX *pCtx;

    pCtx = (EDITCTX *)GetWindowLongA(hwnd, GWL_USERDATA);

    switch (uMsg) {
        case WM_CREATE:
        {
            CREATESTRUCTA *pcs;
            int xLabel;
            int yLabel;
            int xEdit;
            int yEditName;
            int yValueLabel;
            int yValueEdit;
            int xGroup;
            int yGroup;
            int wEdit;
            int hEdit;
            int wButton;
            int hButton;
            int xOk;
            int xCancel;
            int yButton;

            pcs = (CREATESTRUCTA *)lParam;
            pCtx = (EDITCTX *)pcs->lpCreateParams;
            SetWindowLongA(hwnd, GWL_USERDATA, (LONG)pCtx);
            pCtx->hwnd = hwnd;

            xLabel = TwipsX(360);
            yLabel = TwipsY(60);
            xEdit = TwipsX(300);
            yEditName = TwipsY(420);
            yValueLabel = TwipsY(840);
            yValueEdit = TwipsY(1260);
            xGroup = TwipsX(360);
            yGroup = TwipsY(1680);
            wEdit = TwipsX(4812);
            hEdit = TwipsY(312);
            wButton = TwipsX(912);
            hButton = TwipsY(312);
            xOk = TwipsX(2940);
            xCancel = TwipsX(4140);
            yButton = TwipsY(1920);

            CreateWindowExA(0, "STATIC", "Value Name:",
                            WS_CHILD | WS_VISIBLE,
                            xLabel, yLabel, TwipsX(2172), TwipsY(252),
                            hwnd, (HMENU)IDC_STATIC_NAME, g_app.hInstance, NULL);
            CreateWindowExA(0, "STATIC", "Value Data:",
                            WS_CHILD | WS_VISIBLE,
                            xLabel, yValueLabel, TwipsX(1632), TwipsY(312),
                            hwnd, (HMENU)IDC_STATIC_VALUE, g_app.hInstance, NULL);

            pCtx->hwndValueName = CreateWindowExA(0, "EDIT", "",
                                                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
                                                  xEdit, yEditName, wEdit, hEdit,
                                                  hwnd, (HMENU)IDC_EDIT_NAME, g_app.hInstance, NULL);
            pCtx->hwndValueData = CreateWindowExA(0, "EDIT", "",
                                                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
                                                  xEdit, yValueEdit, wEdit, hEdit,
                                                  hwnd, (HMENU)IDC_EDIT_VALUE, g_app.hInstance, NULL);
            pCtx->hwndOk = CreateWindowExA(0, "BUTTON", "OK",
                                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                           xOk, yButton, wButton, hButton,
                                           hwnd, (HMENU)IDOK, g_app.hInstance, NULL);
            pCtx->hwndCancel = CreateWindowExA(0, "BUTTON", "Cancel",
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                               xCancel, yButton, wButton, hButton,
                                               hwnd, (HMENU)IDCANCEL, g_app.hInstance, NULL);
            pCtx->hwndGroup = CreateWindowExA(0, "BUTTON", "Base",
                                              WS_CHILD | BS_GROUPBOX,
                                              xGroup, yGroup, TwipsX(2232), TwipsY(912),
                                              hwnd, (HMENU)IDC_GROUP_BASE, g_app.hInstance, NULL);
            pCtx->hwndHex = CreateWindowExA(0, "BUTTON", "Hexadecimal",
                                            WS_CHILD | WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON,
                                            xGroup + TwipsX(360), yGroup + TwipsY(240), TwipsX(1512), TwipsY(252),
                                            hwnd, (HMENU)IDC_RADIO_HEX, g_app.hInstance, NULL);
            pCtx->hwndDec = CreateWindowExA(0, "BUTTON", "Decimal",
                                            WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON,
                                            xGroup + TwipsX(360), yGroup + TwipsY(540), TwipsX(1512), TwipsY(252),
                                            hwnd, (HMENU)IDC_RADIO_DEC, g_app.hInstance, NULL);

            g_app.groupBoxDefProc = (WNDPROC)GetWindowLong(pCtx->hwndGroup, GWL_WNDPROC);
            SetWindowLong(pCtx->hwndGroup, GWL_WNDPROC, (LPARAM)GroupBoxWndProc);

            ApplyUiFontToChildren(hwnd);

            SetWindowTextA(pCtx->hwndValueName,
                           (pCtx->pszValueName[0] != '\0') ? pCtx->pszValueName : NAME_DEFAULT);
            SetWindowTextA(pCtx->hwndValueData, pCtx->pszInitialText);

            if (pCtx->dwType == REG_DWORD) {
                ShowWindow(pCtx->hwndGroup, SW_SHOW);
                ShowWindow(pCtx->hwndHex, SW_SHOW);
                ShowWindow(pCtx->hwndDec, SW_SHOW);
                pCtx->nCurrentBase = 0;
                UpdateDwordBaseButtons(pCtx);
            } else {
                ShowWindow(pCtx->hwndGroup, SW_HIDE);
                ShowWindow(pCtx->hwndHex, SW_HIDE);
                ShowWindow(pCtx->hwndDec, SW_HIDE);
            }

            if (g_app.hIcon != NULL) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_app.hIcon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_app.hIcon);
            }

            CenterWindowRelative(hwnd, GetParent(hwnd));
            PostMessageA(hwnd, WM_APP_SET_EDIT_FOCUS, 0, 0);
            return 0;
        }

        case WM_APP_SET_EDIT_FOCUS:
            SetFocus(pCtx->hwndValueData);
            SendMessageA(pCtx->hwndValueData, EM_SETSEL, 0, -1);
            return 0;

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        {
            HDC hdc;
            hdc = (HDC)wParam;
            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
            SetBkColor(hdc, g_app.crFace);
            return (LRESULT)g_app.hbrFace;
        }

        case WM_CTLCOLOREDIT:
        {
            HDC hdc;
            HWND hwndCtl;

            hdc = (HDC)wParam;
            hwndCtl = (HWND)lParam;
            if (pCtx != NULL && hwndCtl == pCtx->hwndValueName) {
                SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
                SetBkColor(hdc, g_app.crFace);
                return (LRESULT)g_app.hbrFace;
            }
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(hdc, g_app.crWindow);
            return (LRESULT)(COLOR_WINDOW + 1);
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_RADIO_HEX:
                    if (HIWORD(wParam) == BN_CLICKED && pCtx != NULL) {
                        ConvertEditValueBase(pCtx, 0);
                    }
                    return 0;

                case IDC_RADIO_DEC:
                    if (HIWORD(wParam) == BN_CLICKED && pCtx != NULL) {
                        ConvertEditValueBase(pCtx, 1);
                    }
                    return 0;

                case IDOK:
                    if (pCtx != NULL) {
                        char szText[512];
                        LONG lRet;

                        GetWindowTextA(pCtx->hwndValueData, szText, ARRAYSIZE(szText));
                        if (pCtx->dwType == REG_DWORD) {
                            DWORD dwValue;
                            BYTE rgbValue[4];

                            if (!ParseUnsignedDword(szText, (pCtx->nCurrentBase == 0) ? 16 : 10, &dwValue)) {
                                MessageBoxA(hwnd, "The DWORD value is not a valid number.", APP_TITLE,
                                            MB_OK | MB_ICONEXCLAMATION);
                                return 0;
                            }

                            rgbValue[0] = (BYTE)(dwValue & 0xFF);
                            rgbValue[1] = (BYTE)((dwValue >> 8) & 0xFF);
                            rgbValue[2] = (BYTE)((dwValue >> 16) & 0xFF);
                            rgbValue[3] = (BYTE)((dwValue >> 24) & 0xFF);

                            lRet = WriteValueData(pCtx->hRoot, pCtx->pszSubPath, pCtx->pszValueName,
                                                  REG_DWORD, rgbValue, 4);
                        } else {
                            lRet = WriteValueData(pCtx->hRoot, pCtx->pszSubPath, pCtx->pszValueName,
                                                  REG_SZ, (const BYTE *)szText, (DWORD)(strlen(szText) + 1));
                        }

                        if (lRet != ERROR_SUCCESS) {
                            ShowRegistryError(hwnd, lRet, 10);
                            return 0;
                        }

                        pCtx->nResult = IDOK;
                        DestroyWindow(hwnd);
                    }
                    return 0;

                case IDCANCEL:
                    if (pCtx != NULL) {
                        pCtx->nResult = IDCANCEL;
                    }
                    DestroyWindow(hwnd);
                    return 0;
            }
            break;

        case WM_CLOSE:
            if (pCtx != NULL) {
                pCtx->nResult = IDCANCEL;
            }
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK GroupBoxWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HBRUSH hBrush, hOldBrush;
    HPEN hPen, hOldPen;
    RECT rect;
    HDC hDC;
    HWND ghWnd;
    COLORREF gWindowColor;

    if (uMsg == WM_ERASEBKGND) {
        hDC = GetDC(hwnd);

        // Obtain a handle to the parent window's background brush.
        hBrush = g_app.hbrFace;
        hOldBrush = SelectObject(hDC, hBrush);

        // Erase the group box's background.
        GetClientRect(hwnd, &rect);
        Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

        FillRect(hDC,&rect, hBrush);

        // Restore the original objects before releasing the DC.
        SelectObject(hDC, hOldBrush);

        ReleaseDC(hwnd, hDC);

        // Instruct Windows to paint the group box text and frame.
        InvalidateRect(hwnd, NULL, FALSE);

        return TRUE; // Background has been erased.
    }
    return CallWindowProc(g_app.groupBoxDefProc, hwnd, uMsg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSA wc;
    RECT rc;
    HWND hwnd;
    MSG msg;
    DWORD dwVersion;
    HDC hdc;

    (void)hPrevInstance;
    (void)lpCmdLine;

    ZeroMemory(&g_app, sizeof(g_app));
    g_app.hInstance = hInstance;
    g_app.crFace = RGB(192, 192, 192);
    g_app.crWindow = GetSysColor(COLOR_WINDOW);
    g_app.hbrFace = CreateSolidBrush(g_app.crFace);

    hdc = GetDC(NULL);
    g_app.dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    g_app.dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);
    if (g_app.dpiX == 0) {
        g_app.dpiX = 96;
    }
    if (g_app.dpiY == 0) {
        g_app.dpiY = 96;
    }

    g_app.hUiFont = CreatePointFontACompat("MS Sans Serif", 825, FW_BOLD);
    g_app.hListFont = CreatePointFontACompat("Courier New", 800, FW_NORMAL);
    g_app.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_APPICON));

    dwVersion = GetVersion();
    g_app.isWin9x = ((dwVersion & 0x80000000UL) != 0);

    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_app.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_app.hbrFace;
    wc.lpszClassName = APP_CLASS_NAME;
    if (!RegisterClassA(&wc)) {
        return 0;
    }

    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = EditWndProc;
    wc.cbWndExtra = DLGWINDOWEXTRA;
    wc.hInstance = hInstance;
    wc.hIcon = g_app.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_app.hbrFace;
    wc.lpszClassName = EDIT_CLASS_NAME;
    if (!RegisterClassA(&wc)) {
        return 0;
    }

    rc.left = 0;
    rc.top = 0;
    rc.right = TwipsX(7590);
    rc.bottom = TwipsY(4980);
    AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);
    g_app.minWindowWidth = rc.right - rc.left;
    g_app.minWindowHeight = rc.bottom - rc.top;

    hwnd = CreateWindowExA(0,
                           APP_CLASS_NAME,
                           APP_TITLE,
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           g_app.minWindowWidth,
                           g_app.minWindowHeight,
                           NULL,
                           NULL,
                           hInstance,
                           NULL);
    if (hwnd == NULL) {
        if (g_app.hListFont != NULL) {
            DeleteObject(g_app.hListFont);
        }
        if (g_app.hUiFont != NULL) {
            DeleteObject(g_app.hUiFont);
        }
        if (g_app.hbrFace != NULL) {
            DeleteObject(g_app.hbrFace);
        }
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_app.hListFont != NULL) {
        DeleteObject(g_app.hListFont);
    }
    if (g_app.hUiFont != NULL) {
        DeleteObject(g_app.hUiFont);
    }
    if (g_app.hbrFace != NULL) {
        DeleteObject(g_app.hbrFace);
    }

    return (int)msg.wParam;
}
