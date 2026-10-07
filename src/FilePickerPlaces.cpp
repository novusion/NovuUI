#include "novu/ui/FilePickerPlaces.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace novu::ui {
namespace {

const QString kGroup = QStringLiteral("FilePicker");
const QString kFavourites = QStringLiteral("favourites");
const QString kPath = QStringLiteral("path");
const QString kLabel = QStringLiteral("label");
const QString kLastDirectory = QStringLiteral("lastDirectory");

// Named explicitly rather than taken from the application, because the
// favourites are shared by every application that shows a picker - see the
// header. IniFormat so QSettings::setPath() can move it for a test.
QSettings store()
{
    return QSettings(QSettings::IniFormat, QSettings::UserScope,
                     QStringLiteral("Novusion"), QStringLiteral("NovuUI"));
}

// Windows paths name one folder whatever their case, and a favourite that
// could be added twice by spelling it "j:/builds" is two pins on one place.
Qt::CaseSensitivity pathCase()
{
#if defined(Q_OS_WIN)
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

bool samePath(const QString& a, const QString& b)
{
    return a.compare(b, pathCase()) == 0;
}

QString folderName(const QString& path)
{
    const QString name = QDir(path).dirName();
    // A root has no name of its own ("C:/" -> ""), and is shown as itself.
    return name.isEmpty() ? QDir::toNativeSeparators(path) : name;
}

void write(QSettings& settings, const QVector<FilePickerPlaces::Favourite>& favourites)
{
    settings.beginGroup(kGroup);
    // Rewritten whole. An array with a hole in it - what removing one entry
    // in place leaves - reads back with a blank favourite where the hole was.
    settings.remove(kFavourites);
    settings.beginWriteArray(kFavourites, favourites.size());
    for (int i = 0; i < favourites.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(kPath, favourites.at(i).path);
        settings.setValue(kLabel, favourites.at(i).label);
    }
    settings.endArray();
    settings.endGroup();
    settings.sync();
}

// Per application, so one product's last folder is not another's.
QString lastDirectoryKey()
{
    QString app = QCoreApplication::applicationName();
    if (app.isEmpty()) {
        app = QStringLiteral("default");
    }
    return kLastDirectory + QLatin1Char('/') + app;
}

} // namespace

QString FilePickerPlaces::normalised(const QString& path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    const QString slashed = QDir::fromNativeSeparators(trimmed);
    const QString absolute = QDir::isAbsolutePath(slashed)
        ? slashed : QDir::current().absoluteFilePath(slashed);
    // cleanPath folds "a/./b", "a/../b" and a doubled separator, and drops a
    // trailing one - everywhere except a root, where "C:/" stays "C:/".
    return QDir::cleanPath(absolute);
}

QVector<FilePickerPlaces::Favourite> FilePickerPlaces::favourites()
{
    QSettings settings = store();
    QVector<Favourite> result;
    settings.beginGroup(kGroup);
    const int count = settings.beginReadArray(kFavourites);
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        Favourite favourite;
        favourite.path = normalised(settings.value(kPath).toString());
        if (favourite.path.isEmpty()) {
            continue;   // a hand-edited file, or a hole; nothing to show
        }
        favourite.label = settings.value(kLabel).toString().trimmed();
        if (favourite.label.isEmpty()) {
            favourite.label = folderName(favourite.path);
        }
        bool duplicate = false;
        for (const Favourite& seen : result) {
            duplicate = duplicate || samePath(seen.path, favourite.path);
        }
        if (!duplicate) {
            result.append(favourite);
        }
    }
    settings.endArray();
    settings.endGroup();
    return result;
}

bool FilePickerPlaces::isFavourite(const QString& path)
{
    const QString key = normalised(path);
    if (key.isEmpty()) {
        return false;
    }
    for (const Favourite& favourite : favourites()) {
        if (samePath(favourite.path, key)) {
            return true;
        }
    }
    return false;
}

bool FilePickerPlaces::addFavourite(const QString& path, const QString& label)
{
    const QString key = normalised(path);
    if (key.isEmpty() || !QFileInfo(key).isDir() || isFavourite(key)) {
        return false;
    }
    QVector<Favourite> list = favourites();
    const QString name = label.trimmed();
    list.append({key, name.isEmpty() ? folderName(key) : name});
    QSettings settings = store();
    write(settings, list);
    return true;
}

bool FilePickerPlaces::renameFavourite(const QString& path, const QString& label)
{
    const QString key = normalised(path);
    const QString name = label.trimmed();
    if (key.isEmpty() || name.isEmpty()) {
        return false;
    }
    QVector<Favourite> list = favourites();
    for (Favourite& favourite : list) {
        if (samePath(favourite.path, key)) {
            favourite.label = name;
            QSettings settings = store();
            write(settings, list);
            return true;
        }
    }
    return false;
}

bool FilePickerPlaces::removeFavourite(const QString& path)
{
    const QString key = normalised(path);
    if (key.isEmpty()) {
        return false;
    }
    QVector<Favourite> list = favourites();
    for (int i = 0; i < list.size(); ++i) {
        if (samePath(list.at(i).path, key)) {
            list.removeAt(i);
            QSettings settings = store();
            write(settings, list);
            return true;
        }
    }
    return false;
}

QString FilePickerPlaces::lastDirectory()
{
    QSettings settings = store();
    settings.beginGroup(kGroup);
    const QString directory = normalised(settings.value(lastDirectoryKey()).toString());
    settings.endGroup();
    // A folder that has gone away is not somewhere to open. Reported as
    // "nowhere remembered" so the caller falls back the way it already does.
    return !directory.isEmpty() && QFileInfo(directory).isDir() ? directory : QString();
}

void FilePickerPlaces::setLastDirectory(const QString& directory)
{
    const QString key = normalised(directory);
    if (key.isEmpty()) {
        return;
    }
    QSettings settings = store();
    settings.beginGroup(kGroup);
    settings.setValue(lastDirectoryKey(), key);
    settings.endGroup();
    settings.sync();
}

} // namespace novu::ui
