#pragma once

#include <obs-module.h>

#include <QString>
#include <cstring>

inline QString uiText(const char *key)
{
    const char *translated = obs_module_text(key);
    if (translated && std::strcmp(translated, key) != 0)
        return QString::fromUtf8(translated);

    struct Entry {
        const char *key;
        const char *value;
    };

    static constexpr Entry fallback[] = {
        {"Menu.ManageExplorerDocks", "Windows-Explorer-Docks..."},

        {"Manager.Title", "Windows-Explorer-Docks verwalten"},
        {"Manager.Description", "Hier kannst du Ordner als eigene Docks direkt in OBS einbinden. Jedes Dock merkt sich automatisch den zuletzt geöffneten Ordner."},
        {"Manager.DockName", "Dock-Name"},
        {"Manager.StartFolder", "Startordner"},
        {"Manager.Browse", "Ordner auswählen..."},
        {"Manager.Delete", "Dock entfernen"},
        {"Manager.Add", "Neues Explorer-Dock"},
        {"Manager.Apply", "Speichern & übernehmen"},
        {"Manager.Close", "Schließen"},

        {"DefaultDockName", "Windows Explorer"},

        {"Dock.Back", "Zurück"},
        {"Dock.Forward", "Vor"},
        {"Dock.Up", "Zum übergeordneten Ordner"},
        {"Dock.Refresh", "Aktualisieren"},
        {"Dock.Go", "Öffnen"},
        {"Dock.PathPlaceholder", "Ordnerpfad eingeben..."},
        {"Dock.InitFailed", "Der Windows-Explorer konnte nicht im OBS-Dock gestartet werden. Weitere Details findest du im OBS-Log."},
        {"Dock.InvalidPath", "Der Ordner konnte nicht geöffnet werden: %1 (%2)"},
        {"Dock.NavigationFailed", "Der Ordner konnte nicht geöffnet werden: %1 (%2)"},
        {"Dock.NavigationFailedSimple", "Dieser Speicherort konnte nicht geöffnet werden: %1"},
        {"Dock.NoBackHistory", "Im Verlauf gibt es keinen vorherigen Ordner."},
        {"Dock.NoForwardHistory", "Im Verlauf gibt es keinen nächsten Ordner."},
        {"Dock.NoParent", "Dieser Speicherort hat keinen übergeordneten Ordner."},
    };

    for (const Entry &entry : fallback) {
        if (std::strcmp(entry.key, key) == 0)
            return QString::fromUtf8(entry.value);
    }

    return QString::fromUtf8(key);
}
