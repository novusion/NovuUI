/****************************************************************************
** Meta object code from reading C++ file 'FeedbackForm.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../include/novu/ui/FeedbackForm.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FeedbackForm.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN4novu2ui12FeedbackFormE_t {};
} // unnamed namespace

template <> constexpr inline auto novu::ui::FeedbackForm::qt_create_metaobjectdata<qt_meta_tag_ZN4novu2ui12FeedbackFormE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "novu::ui::FeedbackForm",
        "submitted",
        "",
        "novu::ui::FeedbackRequest",
        "request",
        "cancelled",
        "stateChanged",
        "novu::ui::FeedbackForm::State",
        "state",
        "State",
        "Empty",
        "Editing",
        "Invalid",
        "Sending",
        "Sent",
        "Failed"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'submitted'
        QtMocHelpers::SignalData<void(const novu::ui::FeedbackRequest &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'cancelled'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stateChanged'
        QtMocHelpers::SignalData<void(novu::ui::FeedbackForm::State)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'State'
        QtMocHelpers::EnumData<enum State>(9, 9, QMC::EnumIsScoped).add({
            {   10, State::Empty },
            {   11, State::Editing },
            {   12, State::Invalid },
            {   13, State::Sending },
            {   14, State::Sent },
            {   15, State::Failed },
        }),
    };
    return QtMocHelpers::metaObjectData<FeedbackForm, qt_meta_tag_ZN4novu2ui12FeedbackFormE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject novu::ui::FeedbackForm::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui12FeedbackFormE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui12FeedbackFormE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN4novu2ui12FeedbackFormE_t>.metaTypes,
    nullptr
} };

void novu::ui::FeedbackForm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FeedbackForm *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->submitted((*reinterpret_cast<std::add_pointer_t<novu::ui::FeedbackRequest>>(_a[1]))); break;
        case 1: _t->cancelled(); break;
        case 2: _t->stateChanged((*reinterpret_cast<std::add_pointer_t<novu::ui::FeedbackForm::State>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (FeedbackForm::*)(const novu::ui::FeedbackRequest & )>(_a, &FeedbackForm::submitted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (FeedbackForm::*)()>(_a, &FeedbackForm::cancelled, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (FeedbackForm::*)(novu::ui::FeedbackForm::State )>(_a, &FeedbackForm::stateChanged, 2))
            return;
    }
}

const QMetaObject *novu::ui::FeedbackForm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *novu::ui::FeedbackForm::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui12FeedbackFormE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int novu::ui::FeedbackForm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void novu::ui::FeedbackForm::submitted(const novu::ui::FeedbackRequest & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void novu::ui::FeedbackForm::cancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void novu::ui::FeedbackForm::stateChanged(novu::ui::FeedbackForm::State _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
namespace {
struct qt_meta_tag_ZN4novu2ui14FeedbackDialogE_t {};
} // unnamed namespace

template <> constexpr inline auto novu::ui::FeedbackDialog::qt_create_metaobjectdata<qt_meta_tag_ZN4novu2ui14FeedbackDialogE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "novu::ui::FeedbackDialog"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<FeedbackDialog, qt_meta_tag_ZN4novu2ui14FeedbackDialogE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject novu::ui::FeedbackDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<Dialog::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui14FeedbackDialogE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui14FeedbackDialogE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN4novu2ui14FeedbackDialogE_t>.metaTypes,
    nullptr
} };

void novu::ui::FeedbackDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<FeedbackDialog *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *novu::ui::FeedbackDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *novu::ui::FeedbackDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN4novu2ui14FeedbackDialogE_t>.strings))
        return static_cast<void*>(this);
    return Dialog::qt_metacast(_clname);
}

int novu::ui::FeedbackDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Dialog::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
