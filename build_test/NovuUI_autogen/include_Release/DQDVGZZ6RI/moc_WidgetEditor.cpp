/****************************************************************************
** Meta object code from reading C++ file 'WidgetEditor.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/novu/ui/WidgetEditor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WidgetEditor.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN4novu2ui12WidgetEditorE_t {};
} // unnamed namespace

template <> constexpr inline auto novu::ui::WidgetEditor::qt_create_metaobjectdata<qt_meta_tag_ZN4novu2ui12WidgetEditorE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "novu::ui::WidgetEditor",
        "picked",
        "",
        "QWidget*",
        "widget",
        "sizeEdited",
        "QSize",
        "size",
        "Qt::Orientations",
        "axes",
        "finished",
        "dragMoved",
        "QPoint",
        "globalPosition",
        "dropRequested"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'picked'
        QtMocHelpers::SignalData<void(QWidget *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'sizeEdited'
        QtMocHelpers::SignalData<void(QWidget *, QSize, Qt::Orientations, bool)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 6, 7 }, { 0x80000000 | 8, 9 }, { QMetaType::Bool, 10 },
        }}),
        // Signal 'dragMoved'
        QtMocHelpers::SignalData<void(QWidget *, QPoint)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 12, 13 },
        }}),
        // Signal 'dropRequested'
        QtMocHelpers::SignalData<void(QWidget *, QPoint)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 12, 13 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<WidgetEditor, qt_meta_tag_ZN4novu2ui12WidgetEditorE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject novu::ui::WidgetEditor::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui12WidgetEditorE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui12WidgetEditorE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN4novu2ui12WidgetEditorE_t>.metaTypes,
    nullptr
} };

void novu::ui::WidgetEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<WidgetEditor *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->picked((*reinterpret_cast<std::add_pointer_t<QWidget*>>(_a[1]))); break;
        case 1: _t->sizeEdited((*reinterpret_cast<std::add_pointer_t<QWidget*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QSize>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<Qt::Orientations>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4]))); break;
        case 2: _t->dragMoved((*reinterpret_cast<std::add_pointer_t<QWidget*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 3: _t->dropRequested((*reinterpret_cast<std::add_pointer_t<QWidget*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (WidgetEditor::*)(QWidget * )>(_a, &WidgetEditor::picked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (WidgetEditor::*)(QWidget * , QSize , Qt::Orientations , bool )>(_a, &WidgetEditor::sizeEdited, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (WidgetEditor::*)(QWidget * , QPoint )>(_a, &WidgetEditor::dragMoved, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (WidgetEditor::*)(QWidget * , QPoint )>(_a, &WidgetEditor::dropRequested, 3))
            return;
    }
}

const QMetaObject *novu::ui::WidgetEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *novu::ui::WidgetEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui12WidgetEditorE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int novu::ui::WidgetEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void novu::ui::WidgetEditor::picked(QWidget * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void novu::ui::WidgetEditor::sizeEdited(QWidget * _t1, QSize _t2, Qt::Orientations _t3, bool _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2, _t3, _t4);
}

// SIGNAL 2
void novu::ui::WidgetEditor::dragMoved(QWidget * _t1, QPoint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void novu::ui::WidgetEditor::dropRequested(QWidget * _t1, QPoint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}
QT_WARNING_POP
