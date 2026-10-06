#pragma once

#include <QMap>
#include <QList>
#include <QString>

class QDialog;
class QTableWidget;
class ExplorerBrowserWidget;

struct ExplorerDockConfig {
    QString id;
    QString name;
    QString path;
};

class ExplorerDockManager final {
public:
    ExplorerDockManager() = default;
    ~ExplorerDockManager();

    void loadAndRestore();
    void openManager();
    void shutdown();

private:
    QList<ExplorerDockConfig> loadConfig() const;
    void saveConfig(const QList<ExplorerDockConfig> &configs) const;
    QString configFilePath() const;

    void rebuildAllDocks();
    void removeAllDocks();
    void createDock(const ExplorerDockConfig &config);
    void updateSavedPath(const QString &id, const QString &path);

    void appendRow(QTableWidget *table, const ExplorerDockConfig &config);
    QList<ExplorerDockConfig> readRows(QTableWidget *table) const;

    QList<ExplorerDockConfig> configs_;
    QMap<QString, ExplorerBrowserWidget *> activeDocks_;
    QDialog *managerDialog_ = nullptr;
};
