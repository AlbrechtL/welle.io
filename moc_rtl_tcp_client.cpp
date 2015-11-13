/****************************************************************************
** Meta object code from reading C++ file 'rtl_tcp_client.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "src/input/rtl_tcp/rtl_tcp_client.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'rtl_tcp_client.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_rtl_tcp_client[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   29,   29,   29, 0x08,
      30,   29,   29,   29, 0x08,
      46,   29,   29,   29, 0x08,
      67,   29,   29,   29, 0x08,
      78,   29,   29,   29, 0x08,
      94,   29,   29,   29, 0x08,
     108,   29,   29,   29, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_rtl_tcp_client[] = {
    "rtl_tcp_client\0sendGain(int)\0\0"
    "set_Offset(int)\0set_fCorrection(int)\0"
    "readData()\0setConnection()\0wantConnect()\0"
    "setDisconnect()\0"
};

void rtl_tcp_client::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        rtl_tcp_client *_t = static_cast<rtl_tcp_client *>(_o);
        switch (_id) {
        case 0: _t->sendGain((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->set_Offset((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->set_fCorrection((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->readData(); break;
        case 4: _t->setConnection(); break;
        case 5: _t->wantConnect(); break;
        case 6: _t->setDisconnect(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData rtl_tcp_client::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject rtl_tcp_client::staticMetaObject = {
    { &virtualInput::staticMetaObject, qt_meta_stringdata_rtl_tcp_client,
      qt_meta_data_rtl_tcp_client, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &rtl_tcp_client::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *rtl_tcp_client::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *rtl_tcp_client::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_rtl_tcp_client))
        return static_cast<void*>(const_cast< rtl_tcp_client*>(this));
    if (!strcmp(_clname, "Ui_rtl_tcp_widget"))
        return static_cast< Ui_rtl_tcp_widget*>(const_cast< rtl_tcp_client*>(this));
    return virtualInput::qt_metacast(_clname);
}

int rtl_tcp_client::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = virtualInput::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
