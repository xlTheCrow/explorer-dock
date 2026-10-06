#include "explorer-browser-widget.hpp"
#include "plugin-main.hpp"

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <knownfolders.h>

#include <QBoxLayout>
#include <QEvent>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <utility>

namespace {
QString text(const char *key)
{
    return QString::fromUtf8(obs_module_text(key));
}

QString hresultText(HRESULT hr)
{
    return QStringLiteral("0x%1").arg(static_cast<qulonglong>(static_cast<unsigned long>(hr)), 8, 16, QLatin1Char('0')).toUpper();
}
}

class ExplorerBrowserWidget::EventSink final : public IExplorerBrowserEvents {
public:
    explicit EventSink(ExplorerBrowserWidget *owner) : owner_(owner) {}

    void detachOwner() { owner_ = nullptr; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;

        *ppvObject = nullptr;
        if (riid == IID_IUnknown || riid == IID_IExplorerBrowserEvents) {
            *ppvObject = static_cast<IExplorerBrowserEvents *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return static_cast<ULONG>(InterlockedIncrement(&refCount_));
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG count = static_cast<ULONG>(InterlockedDecrement(&refCount_));
        if (count == 0)
            delete this;
        return count;
    }

    HRESULT STDMETHODCALLTYPE OnNavigationPending(PCIDLIST_ABSOLUTE) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnViewCreated(IShellView *) override
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnNavigationComplete(PCIDLIST_ABSOLUTE pidlFolder) override
    {
        if (owner_)
            owner_->handleNavigationComplete(pidlFolder);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnNavigationFailed(PCIDLIST_ABSOLUTE pidlFolder) override
    {
        if (owner_)
            owner_->handleNavigationFailed(pidlFolder);
        return S_OK;
    }

private:
    ~EventSink() = default;

    LONG refCount_ = 1;
    ExplorerBrowserWidget *owner_ = nullptr;
};

ExplorerBrowserWidget::ExplorerBrowserWidget(const QString &initialPath, QWidget *parent)
    : QWidget(parent), currentPath_(initialPath)
{
    setContentsMargins(0, 0, 0, 0);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(4);

    auto *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(4, 4, 4, 0);
    toolbarLayout->setSpacing(3);

    backButton_ = new QPushButton(QStringLiteral("←"), this);
    forwardButton_ = new QPushButton(QStringLiteral("→"), this);
    upButton_ = new QPushButton(QStringLiteral("↑"), this);
    refreshButton_ = new QPushButton(QStringLiteral("⟳"), this);

    for (auto *button : {backButton_, forwardButton_, upButton_, refreshButton_}) {
        button->setFixedWidth(30);
        toolbarLayout->addWidget(button);
    }

    backButton_->setToolTip(text("Dock.Back"));
    forwardButton_->setToolTip(text("Dock.Forward"));
    upButton_->setToolTip(text("Dock.Up"));
    refreshButton_->setToolTip(text("Dock.Refresh"));

    pathEdit_ = new QLineEdit(this);
    pathEdit_->setPlaceholderText(text("Dock.PathPlaceholder"));
    toolbarLayout->addWidget(pathEdit_, 1);

    auto *goButton = new QPushButton(text("Dock.Go"), this);
    goButton->setFixedWidth(44);
    toolbarLayout->addWidget(goButton);

    rootLayout->addLayout(toolbarLayout);

    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->hide();
    rootLayout->addWidget(statusLabel_);

    nativeHost_ = new QWidget(this);
    nativeHost_->setAttribute(Qt::WA_NativeWindow, true);
    nativeHost_->setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    nativeHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    nativeHost_->installEventFilter(this);
    rootLayout->addWidget(nativeHost_, 1);

    connect(backButton_, &QPushButton::clicked, this, [this]() { navigateBack(); });
    connect(forwardButton_, &QPushButton::clicked, this, [this]() { navigateForward(); });
    connect(upButton_, &QPushButton::clicked, this, [this]() { navigateUp(); });
    connect(refreshButton_, &QPushButton::clicked, this, [this]() { refreshView(); });
    connect(goButton, &QPushButton::clicked, this, [this]() { navigateTo(pathEdit_->text()); });
    connect(pathEdit_, &QLineEdit::returnPressed, this, [this]() { navigateTo(pathEdit_->text()); });

    (void)nativeHost_->winId();

    if (!initializeBrowser()) {
        setStatusText(text("Dock.InitFailed"), true);
        return;
    }

    QString firstPath = initialPath.trimmed();
    if (firstPath.isEmpty())
        firstPath = QDir::homePath();

    pathEdit_->setText(firstPath);
    navigateTo(firstPath);
}

ExplorerBrowserWidget::~ExplorerBrowserWidget()
{
    shutdownBrowser();
}

void ExplorerBrowserWidget::setPathChangedCallback(std::function<void(const QString &)> callback)
{
    pathChangedCallback_ = std::move(callback);
}

QString ExplorerBrowserWidget::currentPath() const
{
    return currentPath_;
}

bool ExplorerBrowserWidget::initializeBrowser()
{
    if (browser_)
        return true;

    HRESULT hr = CoCreateInstance(CLSID_ExplorerBrowser, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&browser_));
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[obs-explorer-dock] CoCreateInstance(CLSID_ExplorerBrowser) failed: 0x%08lX", static_cast<unsigned long>(hr));
        return false;
    }

    RECT rect{};
    GetClientRect(reinterpret_cast<HWND>(nativeHost_->winId()), &rect);

    FOLDERSETTINGS folderSettings{};
    folderSettings.ViewMode = FVM_AUTO;
    folderSettings.fFlags = FWF_AUTOARRANGE | FWF_SHOWSELALWAYS;

    hr = browser_->Initialize(reinterpret_cast<HWND>(nativeHost_->winId()), &rect, &folderSettings);
    if (FAILED(hr)) {
        blog(LOG_ERROR, "[obs-explorer-dock] IExplorerBrowser::Initialize failed: 0x%08lX", static_cast<unsigned long>(hr));
        browser_->Release();
        browser_ = nullptr;
        return false;
    }

    browser_->SetOptions(EBO_ALWAYSNAVIGATE);
    browser_->SetPropertyBag(L"OBSExplorerDockViewState");

    eventSink_ = new EventSink(this);
    hr = browser_->Advise(eventSink_, &adviseCookie_);
    if (FAILED(hr)) {
        blog(LOG_WARNING, "[obs-explorer-dock] IExplorerBrowser::Advise failed: 0x%08lX", static_cast<unsigned long>(hr));
        eventSink_->Release();
        eventSink_ = nullptr;
        adviseCookie_ = 0;
    }

    return true;
}

void ExplorerBrowserWidget::shutdownBrowser()
{
    if (eventSink_)
        eventSink_->detachOwner();

    if (browser_) {
        if (adviseCookie_ != 0)
            browser_->Unadvise(adviseCookie_);

        browser_->Destroy();
        browser_->Release();
        browser_ = nullptr;
    }

    adviseCookie_ = 0;

    if (eventSink_) {
        eventSink_->Release();
        eventSink_ = nullptr;
    }
}

void ExplorerBrowserWidget::navigateTo(const QString &path)
{
    if (!browser_)
        return;

    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return;

    PIDLIST_ABSOLUTE pidl = nullptr;
    SFGAOF attrs = 0;
    const std::wstring widePath = trimmed.toStdWString();
    HRESULT hr = SHParseDisplayName(widePath.c_str(), nullptr, &pidl, 0, &attrs);
    if (FAILED(hr) || !pidl) {
        setStatusText(text("Dock.InvalidPath").arg(trimmed).arg(hresultText(hr)), true);
        return;
    }

    hr = browser_->BrowseToIDList(pidl, SBSP_ABSOLUTE);
    CoTaskMemFree(pidl);

    if (FAILED(hr)) {
        setStatusText(text("Dock.NavigationFailed").arg(trimmed).arg(hresultText(hr)), true);
        return;
    }

    setStatusText(QString());
}

void ExplorerBrowserWidget::navigateBack()
{
    if (!browser_)
        return;
    const HRESULT hr = browser_->BrowseToIDList(nullptr, SBSP_NAVIGATEBACK);
    if (FAILED(hr))
        setStatusText(text("Dock.NoBackHistory"), false);
}

void ExplorerBrowserWidget::navigateForward()
{
    if (!browser_)
        return;
    const HRESULT hr = browser_->BrowseToIDList(nullptr, SBSP_NAVIGATEFORWARD);
    if (FAILED(hr))
        setStatusText(text("Dock.NoForwardHistory"), false);
}

void ExplorerBrowserWidget::navigateUp()
{
    if (!browser_)
        return;
    const HRESULT hr = browser_->BrowseToIDList(nullptr, SBSP_PARENT);
    if (FAILED(hr))
        setStatusText(text("Dock.NoParent"), false);
}

void ExplorerBrowserWidget::refreshView()
{
    if (!browser_)
        return;

    IShellView *view = nullptr;
    const HRESULT hr = browser_->GetCurrentView(IID_PPV_ARGS(&view));
    if (SUCCEEDED(hr) && view) {
        view->Refresh();
        view->Release();
    }
}

void ExplorerBrowserWidget::handleNavigationComplete(const void *absolutePidl)
{
    const QString path = pidlToDisplayPath(absolutePidl);
    if (!path.isEmpty()) {
        currentPath_ = path;
        pathEdit_->setText(path);
        setStatusText(QString());
        if (pathChangedCallback_)
            pathChangedCallback_(path);
    }
}

void ExplorerBrowserWidget::handleNavigationFailed(const void *absolutePidl)
{
    const QString path = pidlToDisplayPath(absolutePidl);
    setStatusText(text("Dock.NavigationFailedSimple").arg(path), true);
}

QString ExplorerBrowserWidget::pidlToDisplayPath(const void *absolutePidl) const
{
    if (!absolutePidl)
        return QString();

    PWSTR rawPath = nullptr;
    const HRESULT hr = SHGetNameFromIDList(static_cast<PCIDLIST_ABSOLUTE>(absolutePidl), SIGDN_DESKTOPABSOLUTEPARSING, &rawPath);
    if (FAILED(hr) || !rawPath)
        return QString();

    QString result = QString::fromWCharArray(rawPath);
    CoTaskMemFree(rawPath);
    return result;
}

void ExplorerBrowserWidget::setStatusText(const QString &textValue, bool error)
{
    if (textValue.isEmpty()) {
        statusLabel_->clear();
        statusLabel_->hide();
        return;
    }

    statusLabel_->setText(textValue);
    statusLabel_->setProperty("error", error);
    statusLabel_->show();
}

void ExplorerBrowserWidget::updateBrowserRect()
{
    if (!browser_ || !nativeHost_)
        return;

    RECT rect{};
    GetClientRect(reinterpret_cast<HWND>(nativeHost_->winId()), &rect);
    browser_->SetRect(nullptr, rect);
}

bool ExplorerBrowserWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == nativeHost_ && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
        QTimer::singleShot(0, this, [this]() { updateBrowserRect(); });
    }
    return QWidget::eventFilter(watched, event);
}
