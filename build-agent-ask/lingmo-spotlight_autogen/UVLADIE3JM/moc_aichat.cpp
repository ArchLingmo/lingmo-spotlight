/****************************************************************************
** Meta object code from reading C++ file 'aichat.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/aichat.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'aichat.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
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
struct qt_meta_tag_ZN6AiChatE_t {};
} // unnamed namespace

template <> constexpr inline auto AiChat::qt_create_metaobjectdata<qt_meta_tag_ZN6AiChatE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AiChat",
        "busyChanged",
        "",
        "modelChanged",
        "errorChanged",
        "countChanged",
        "pendingConfirmationChanged",
        "usingAssistantChanged",
        "ask",
        "question",
        "stop",
        "clear",
        "copy",
        "row",
        "confirm",
        "requestId",
        "allow",
        "openInAssistant",
        "busy",
        "model",
        "error",
        "count",
        "providerIcon",
        "pendingConfirmation",
        "usingAssistant"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'busyChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'modelChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'errorChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'countChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'pendingConfirmationChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'usingAssistantChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'ask'
        QtMocHelpers::MethodData<void(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Method 'stop'
        QtMocHelpers::MethodData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'clear'
        QtMocHelpers::MethodData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'copy'
        QtMocHelpers::MethodData<void(int) const>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Method 'confirm'
        QtMocHelpers::MethodData<void(const QString &, bool)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 }, { QMetaType::Bool, 16 },
        }}),
        // Method 'openInAssistant'
        QtMocHelpers::MethodData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'busy'
        QtMocHelpers::PropertyData<bool>(18, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'model'
        QtMocHelpers::PropertyData<QString>(19, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'error'
        QtMocHelpers::PropertyData<QString>(20, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'count'
        QtMocHelpers::PropertyData<int>(21, QMetaType::Int, QMC::DefaultPropertyFlags, 3),
        // property 'providerIcon'
        QtMocHelpers::PropertyData<QString>(22, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'pendingConfirmation'
        QtMocHelpers::PropertyData<bool>(23, QMetaType::Bool, QMC::DefaultPropertyFlags, 4),
        // property 'usingAssistant'
        QtMocHelpers::PropertyData<bool>(24, QMetaType::Bool, QMC::DefaultPropertyFlags, 5),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AiChat, qt_meta_tag_ZN6AiChatE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AiChat::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6AiChatE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6AiChatE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN6AiChatE_t>.metaTypes,
    nullptr
} };

void AiChat::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AiChat *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->busyChanged(); break;
        case 1: _t->modelChanged(); break;
        case 2: _t->errorChanged(); break;
        case 3: _t->countChanged(); break;
        case 4: _t->pendingConfirmationChanged(); break;
        case 5: _t->usingAssistantChanged(); break;
        case 6: _t->ask((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->stop(); break;
        case 8: _t->clear(); break;
        case 9: _t->copy((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->confirm((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 11: _t->openInAssistant(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AiChat::*)()>(_a, &AiChat::busyChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AiChat::*)()>(_a, &AiChat::modelChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AiChat::*)()>(_a, &AiChat::errorChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AiChat::*)()>(_a, &AiChat::countChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AiChat::*)()>(_a, &AiChat::pendingConfirmationChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AiChat::*)()>(_a, &AiChat::usingAssistantChanged, 5))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->busy(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->model(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->error(); break;
        case 3: *reinterpret_cast<int*>(_v) = _t->count(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->providerIcon(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->pendingConfirmation(); break;
        case 6: *reinterpret_cast<bool*>(_v) = _t->usingAssistant(); break;
        default: break;
        }
    }
}

const QMetaObject *AiChat::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AiChat::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN6AiChatE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int AiChat::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void AiChat::busyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AiChat::modelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AiChat::errorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AiChat::countChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AiChat::pendingConfirmationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AiChat::usingAssistantChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
