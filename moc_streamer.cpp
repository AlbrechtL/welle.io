/****************************************************************************
** Meta object code from reading C++ file 'streamer.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "includes/output/streamer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'streamer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_streamerServer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      15,   31,   31,   31, 0x05,

 // slots: signature, parameters, type, tag, flags
      32,   31,   31,   31, 0x0a,
      51,   31,   31,   31, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_streamerServer[] = {
    "streamerServer\0handleSamples()\0\0"
    "acceptConnection()\0processSamples()\0"
};

void streamerServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        streamerServer *_t = static_cast<streamerServer *>(_o);
        switch (_id) {
        case 0: _t->handleSamples(); break;
        case 1: _t->acceptConnection(); break;
        case 2: _t->processSamples(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData streamerServer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject streamerServer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_streamerServer,
      qt_meta_data_streamerServer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &streamerServer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *streamerServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *streamerServer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_streamerServer))
        return static_cast<void*>(const_cast< streamerServer*>(this));
    return QObject::qt_metacast(_clname);
}

int streamerServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void streamerServer::handleSamples()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
