#include "windows_app_identity.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QWindow>
#include <QDebug>

#include <windows.h>
#include <propkey.h>
#include <propvarutil.h>
#include <propsys.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>

namespace {

// {9F4C2855-9F79-4B39-A8D0-E1E42606E2E2}, PID 2
constexpr PROPERTYKEY kRelaunchDisplayNameKey{
    {0x9F4C2855, 0x9F79, 0x4B39, {0xA8, 0xD0, 0xE1, 0x42, 0xCB, 0x06, 0x6E, 0xE2}},
    2,
};

bool setPropertyString(IPropertyStore *store, const PROPERTYKEY &key, PCWSTR value)
{
    if (!store || !value) {
        return false;
    }

    PROPVARIANT variant;
    if (FAILED(InitPropVariantFromString(value, &variant))) {
        return false;
    }

    const HRESULT hr = store->SetValue(key, variant);
    PropVariantClear(&variant);
    return SUCCEEDED(hr);
}

QString startMenuShortcutPath()
{
    PWSTR appDataPath = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath)) || !appDataPath) {
        return {};
    }

    const QString shortcutPath = QDir(QString::fromWCharArray(appDataPath)).filePath(
        QStringLiteral("Microsoft/Windows/Start Menu/Programs/Xiaoxiong Music Player.lnk"));
    CoTaskMemFree(appDataPath);
    return shortcutPath;
}

bool createStartMenuShortcut(const QString &exePath)
{
    const QString shortcutPath = startMenuShortcutPath();
    if (shortcutPath.isEmpty()) {
        return false;
    }

    IShellLinkW *shellLink = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_ShellLink,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&shellLink));
    if (FAILED(hr) || !shellLink) {
        return false;
    }

    shellLink->SetPath(reinterpret_cast<LPCWSTR>(exePath.utf16()));
    shellLink->SetDescription(WindowsAppIdentity::displayName());
    shellLink->SetShowCmd(SW_SHOWNORMAL);

    IPropertyStore *propertyStore = nullptr;
    hr = shellLink->QueryInterface(IID_PPV_ARGS(&propertyStore));
    if (FAILED(hr) || !propertyStore) {
        shellLink->Release();
        return false;
    }

    setPropertyString(propertyStore, PKEY_AppUserModel_ID, WindowsAppIdentity::appUserModelId());
    setPropertyString(propertyStore, kRelaunchDisplayNameKey, WindowsAppIdentity::displayName());
    propertyStore->Commit();
    propertyStore->Release();

    IPersistFile *persistFile = nullptr;
    hr = shellLink->QueryInterface(IID_PPV_ARGS(&persistFile));
    if (FAILED(hr) || !persistFile) {
        shellLink->Release();
        return false;
    }

    hr = persistFile->Save(reinterpret_cast<LPCWSTR>(shortcutPath.utf16()), TRUE);
    persistFile->Release();
    shellLink->Release();

    if (SUCCEEDED(hr)) {
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        qInfo() << "MediaSession: registered Start Menu shortcut at" << shortcutPath;
    }

    return SUCCEEDED(hr);
}

} // namespace

namespace WindowsAppIdentity {

void initializeProcessIdentity()
{
    ::SetCurrentProcessExplicitAppUserModelID(appUserModelId());
}

void ensureShellRegistration()
{
    const QString exePath = QCoreApplication::applicationFilePath();
    if (exePath.isEmpty()) {
        return;
    }

    const HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comHr) && comHr != RPC_E_CHANGED_MODE) {
        qWarning() << "MediaSession: COM unavailable for shell registration";
        return;
    }

    if (!createStartMenuShortcut(exePath)) {
        qWarning() << "MediaSession: failed to create Start Menu shortcut";
    }
}

void assignWindowIdentity(QWindow *window)
{
    if (!window) {
        return;
    }

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        return;
    }

    IPropertyStore *store = nullptr;
    if (FAILED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&store))) || !store) {
        return;
    }

    setPropertyString(store, PKEY_AppUserModel_ID, appUserModelId());
    setPropertyString(store, kRelaunchDisplayNameKey, displayName());
    store->Commit();
    store->Release();
}

} // namespace WindowsAppIdentity
