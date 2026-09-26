/****************************************************************************
** Meta object code from reading C++ file 'ModuleBar.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/novu/ui/ModuleBar.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ModuleBar.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN4novu2ui9ModuleBarE_t {};
} // unnamed namespace

template <> constexpr inline auto novu::ui::ModuleBar::qt_create_metaobjectdata<qt_meta_tag_ZN4novu2ui9ModuleBarE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "novu::ui::ModuleBar",
        "moduleChosen",
        "",
        "key"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'moduleChosen'
        QtMocHelpers::SignalData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ModuleBar, qt_meta_tag_ZN4novu2ui9ModuleBarE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject novu::ui::ModuleBar::staticMetaObject = { {
    QMetaObject::SuperData::link<ItemStrip::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui9ModuleBarE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui9ModuleBarE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN4novu2ui9ModuleBarE_t>.metaTypes,
    nullptr
} };

void novu::ui::ModuleBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ModuleBar *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->moduleChosen((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ModuleBar::*)(const QString & )>(_a, &ModuleBar::moduleChosen, 0))
            return;
    }
}

const QMetaObject *novu::ui::ModuleBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *novu::ui::ModuleBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui9ModuleBarE_t>.strings))
        return static_cast<void*>(this);
    return ItemStrip::qt_metacast(_clname);
}

int novu::ui::ModuleBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ItemStrip::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void novu::ui::ModuleBar::moduleChosen(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}
QT_WARNING_POP
