/****************************************************************************
** Meta object code from reading C++ file 'GalleryWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../playground/include/novu/playground/GalleryWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'GalleryWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN4novu10playground13GalleryWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto novu::playground::GalleryWindow::qt_create_metaobjectdata<qt_meta_tag_ZN4novu10playground13GalleryWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "novu::playground::GalleryWindow",
        "onSearchChanged",
        "",
        "query",
        "onScenarioChanged",
        "index",
        "onApplicationFilterToggled",
        "on",
        "onProfileChanged",
        "onOpenSource",
        "onRebuild",
        "onSourcesChanged",
        "paths",
        "onDataChanged",
        "onBuildStateChanged",
        "BuildState",
        "state",
        "detail"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onSearchChanged'
        QtMocHelpers::SlotData<void(const QString &)>(1, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Slot 'onScenarioChanged'
        QtMocHelpers::SlotData<void(int)>(4, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'onApplicationFilterToggled'
        QtMocHelpers::SlotData<void(bool)>(6, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 7 },
        }}),
        // Slot 'onProfileChanged'
        QtMocHelpers::SlotData<void(int)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 5 },
        }}),
        // Slot 'onOpenSource'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRebuild'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSourcesChanged'
        QtMocHelpers::SlotData<void(const QStringList &)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QStringList, 12 },
        }}),
        // Slot 'onDataChanged'
        QtMocHelpers::SlotData<void(const QStringList &)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QStringList, 12 },
        }}),
        // Slot 'onBuildStateChanged'
        QtMocHelpers::SlotData<void(BuildState, const QString &)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 15, 16 }, { QMetaType::QString, 17 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GalleryWindow, qt_meta_tag_ZN4novu10playground13GalleryWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject novu::playground::GalleryWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<novu::ui::AppWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu10playground13GalleryWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu10playground13GalleryWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN4novu10playground13GalleryWindowE_t>.metaTypes,
    nullptr
} };

void novu::playground::GalleryWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GalleryWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onSearchChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->onScenarioChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->onApplicationFilterToggled((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->onProfileChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->onOpenSource(); break;
        case 5: _t->onRebuild(); break;
        case 6: _t->onSourcesChanged((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 7: _t->onDataChanged((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 8: _t->onBuildStateChanged((*reinterpret_cast<std::add_pointer_t<BuildState>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    }
}

const QMetaObject *novu::playground::GalleryWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *novu::playground::GalleryWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu10playground13GalleryWindowE_t>.strings))
        return static_cast<void*>(this);
    return novu::ui::AppWindow::qt_metacast(_clname);
}

int novu::playground::GalleryWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = novu::ui::AppWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
