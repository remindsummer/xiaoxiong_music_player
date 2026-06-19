#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

namespace WindowsAppIdentity {

inline constexpr const wchar_t *appUserModelId()
{
    return L"Xiaoxiong.XiaoxiongMusicPlayer";
}

inline constexpr const wchar_t *displayName()
{
    return L"\u5c0f\u718a\u97f3\u4e50\u64ad\u653e\u5668";
}

#ifdef Q_OS_WIN
void initializeProcessIdentity();
void ensureShellRegistration();
void assignWindowIdentity(QWindow *window);
#endif

} // namespace WindowsAppIdentity
