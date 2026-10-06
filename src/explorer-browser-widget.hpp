#pragma once

#include <QWidget>
#include <QString>
#include <functional>

struct IExplorerBrowser;
class QLineEdit;
class QLabel;
class QPushButton;
class QEvent;

class ExplorerBrowserWidget final : public QWidget {
public:
    explicit ExplorerBrowserWidget(const QString &initialPath, QWidget *parent = nullptr);
    ~ExplorerBrowserWidget() override;

    void setPathChangedCallback(std::function<void(const QString &)> callback);
    QString currentPath() const;

    void navigateTo(const QString &path);
    void navigateBack();
    void navigateForward();
    void navigateUp();
    void refreshView();

    void handleNavigationComplete(const void *absolutePidl);
    void handleNavigationFailed(const void *absolutePidl);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    class EventSink;

    bool initializeBrowser();
    void shutdownBrowser();
    void updateBrowserRect();
    void setStatusText(const QString &text, bool error = false);
    QString pidlToDisplayPath(const void *absolutePidl) const;

    QWidget *nativeHost_ = nullptr;
    QLineEdit *pathEdit_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QPushButton *backButton_ = nullptr;
    QPushButton *forwardButton_ = nullptr;
    QPushButton *upButton_ = nullptr;
    QPushButton *refreshButton_ = nullptr;

    IExplorerBrowser *browser_ = nullptr;
    EventSink *eventSink_ = nullptr;
    unsigned long adviseCookie_ = 0;
    QString currentPath_;
    std::function<void(const QString &)> pathChangedCallback_;
};
