#include "explorer-dock-manager.hpp"
#include "explorer-browser-widget.hpp"
#include "plugin-main.hpp"

#include <obs-frontend-api.h>

#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidget>
#include <QUuid>

namespace {
QString text(const char *key)
{
    return QString::fromUtf8(obs_module_text(key));
}

QString newDockId()
{
    return QStringLiteral("obs_explorer_dock_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
}
}

ExplorerDockManager::~ExplorerDockManager()
{
    shutdown();
}

QString ExplorerDockManager::configFilePath() const
{
    QString obsDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (obsDataPath.isEmpty())
        obsDataPath = QDir::homePath() + QStringLiteral("/AppData/Roaming/obs-studio");

    QDir dir(obsDataPath + QStringLiteral("/plugin_config/obs-explorer-dock"));
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    return dir.filePath(QStringLiteral("config.json"));
}

QList<ExplorerDockConfig> ExplorerDockManager::loadConfig() const
{
    QList<ExplorerDockConfig> result;
    QFile file(configFilePath());
    if (!file.exists())
        return result;

    if (!file.open(QIODevice::ReadOnly)) {
        blog(LOG_WARNING, "[obs-explorer-dock] Could not open config for reading");
        return result;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        blog(LOG_WARNING, "[obs-explorer-dock] Invalid config JSON");
        return result;
    }

    for (const QJsonValue &value : doc.array()) {
        const QJsonObject obj = value.toObject();
        ExplorerDockConfig config;
        config.id = obj.value(QStringLiteral("id")).toString();
        config.name = obj.value(QStringLiteral("name")).toString();
        config.path = obj.value(QStringLiteral("path")).toString();

        if (config.id.isEmpty())
            config.id = newDockId();
        if (config.name.isEmpty())
            config.name = text("DefaultDockName");
        if (config.path.isEmpty())
            config.path = QDir::homePath();

        result.push_back(config);
    }

    return result;
}

void ExplorerDockManager::saveConfig(const QList<ExplorerDockConfig> &configs) const
{
    QJsonArray array;
    for (const ExplorerDockConfig &config : configs) {
        QJsonObject obj;
        obj.insert(QStringLiteral("id"), config.id);
        obj.insert(QStringLiteral("name"), config.name);
        obj.insert(QStringLiteral("path"), config.path);
        array.push_back(obj);
    }

    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        blog(LOG_ERROR, "[obs-explorer-dock] Could not open config for writing");
        return;
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
}

void ExplorerDockManager::loadAndRestore()
{
    configs_ = loadConfig();
    rebuildAllDocks();
}

void ExplorerDockManager::removeAllDocks()
{
    const auto ids = activeDocks_.keys();
    for (const QString &id : ids)
        obs_frontend_remove_dock(id.toUtf8().constData());
    activeDocks_.clear();
}

void ExplorerDockManager::rebuildAllDocks()
{
    removeAllDocks();
    for (const ExplorerDockConfig &config : configs_)
        createDock(config);
}

void ExplorerDockManager::createDock(const ExplorerDockConfig &config)
{
    auto *widget = new ExplorerBrowserWidget(config.path);
    widget->setPathChangedCallback([this, id = config.id](const QString &path) {
        updateSavedPath(id, path);
    });

    const QByteArray idUtf8 = config.id.toUtf8();
    const QByteArray nameUtf8 = config.name.toUtf8();
    if (!obs_frontend_add_dock_by_id(idUtf8.constData(), nameUtf8.constData(), widget)) {
        blog(LOG_WARNING, "[obs-explorer-dock] Failed to add dock '%s'", idUtf8.constData());
        widget->deleteLater();
        return;
    }

    activeDocks_.insert(config.id, widget);
}

void ExplorerDockManager::updateSavedPath(const QString &id, const QString &path)
{
    if (path.isEmpty())
        return;

    bool changed = false;
    for (ExplorerDockConfig &config : configs_) {
        if (config.id == id && config.path != path) {
            config.path = path;
            changed = true;
            break;
        }
    }

    if (changed)
        saveConfig(configs_);
}

void ExplorerDockManager::appendRow(QTableWidget *table, const ExplorerDockConfig &config)
{
    const int row = table->rowCount();
    table->insertRow(row);

    auto *nameEdit = new QLineEdit(config.name, table);
    nameEdit->setProperty("dockId", config.id.isEmpty() ? newDockId() : config.id);
    table->setCellWidget(row, 0, nameEdit);

    auto *pathEdit = new QLineEdit(config.path.isEmpty() ? QDir::homePath() : config.path, table);
    table->setCellWidget(row, 1, pathEdit);

    auto *browseButton = new QPushButton(text("Manager.Browse"), table);
    table->setCellWidget(row, 2, browseButton);
    connect(browseButton, &QPushButton::clicked, table, [table, browseButton]() {
        const int currentRow = table->indexAt(browseButton->pos()).row();
        if (currentRow < 0)
            return;
        auto *edit = qobject_cast<QLineEdit *>(table->cellWidget(currentRow, 1));
        if (!edit)
            return;
        const QString selected = QFileDialog::getExistingDirectory(table, QString(), edit->text());
        if (!selected.isEmpty())
            edit->setText(QDir::toNativeSeparators(selected));
    });

    auto *deleteButton = new QPushButton(QStringLiteral("×"), table);
    deleteButton->setToolTip(text("Manager.Delete"));
    deleteButton->setFixedWidth(32);
    table->setCellWidget(row, 3, deleteButton);
    connect(deleteButton, &QPushButton::clicked, table, [table, deleteButton]() {
        const int currentRow = table->indexAt(deleteButton->pos()).row();
        if (currentRow >= 0)
            table->removeRow(currentRow);
    });
}

QList<ExplorerDockConfig> ExplorerDockManager::readRows(QTableWidget *table) const
{
    QList<ExplorerDockConfig> result;
    for (int row = 0; row < table->rowCount(); ++row) {
        auto *nameEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 0));
        auto *pathEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 1));
        if (!nameEdit || !pathEdit)
            continue;

        const QString name = nameEdit->text().trimmed();
        const QString path = pathEdit->text().trimmed();
        if (name.isEmpty() || path.isEmpty())
            continue;

        ExplorerDockConfig config;
        config.id = nameEdit->property("dockId").toString();
        if (config.id.isEmpty())
            config.id = newDockId();
        config.name = name;
        config.path = path;
        result.push_back(config);
    }
    return result;
}

void ExplorerDockManager::openManager()
{
    if (managerDialog_) {
        managerDialog_->show();
        managerDialog_->raise();
        managerDialog_->activateWindow();
        return;
    }

    managerDialog_ = new QDialog();
    managerDialog_->setAttribute(Qt::WA_DeleteOnClose);
    managerDialog_->setWindowTitle(text("Manager.Title"));
    managerDialog_->resize(860, 430);

    auto *layout = new QVBoxLayout(managerDialog_);
    auto *description = new QLabel(text("Manager.Description"), managerDialog_);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *table = new QTableWidget(0, 4, managerDialog_);
    table->setHorizontalHeaderLabels({text("Manager.DockName"), text("Manager.StartFolder"), QString(), QString()});
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    layout->addWidget(table, 1);

    for (const ExplorerDockConfig &config : configs_)
        appendRow(table, config);

    auto *bottom = new QHBoxLayout();
    auto *addButton = new QPushButton(text("Manager.Add"), managerDialog_);
    bottom->addWidget(addButton);
    bottom->addStretch();

    auto *applyButton = new QPushButton(text("Manager.Apply"), managerDialog_);
    auto *closeButton = new QPushButton(text("Manager.Close"), managerDialog_);
    bottom->addWidget(applyButton);
    bottom->addWidget(closeButton);
    layout->addLayout(bottom);

    connect(addButton, &QPushButton::clicked, managerDialog_, [this, table]() {
        ExplorerDockConfig config;
        config.id = newDockId();
        config.name = text("DefaultDockName");
        config.path = QDir::homePath();
        appendRow(table, config);
    });

    connect(applyButton, &QPushButton::clicked, managerDialog_, [this, table]() {
        configs_ = readRows(table);
        saveConfig(configs_);
        rebuildAllDocks();
    });

    connect(closeButton, &QPushButton::clicked, managerDialog_, [this]() {
        if (managerDialog_)
            managerDialog_->close();
    });

    connect(managerDialog_, &QDialog::destroyed, [this]() { managerDialog_ = nullptr; });
    managerDialog_->show();
}

void ExplorerDockManager::shutdown()
{
    if (managerDialog_) {
        managerDialog_->close();
        managerDialog_ = nullptr;
    }
    removeAllDocks();
}
