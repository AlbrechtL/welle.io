/****************************************************************************
** Meta object code from reading C++ file 'sdrplay.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "src/input/sdrplay/sdrplay.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sdrplay.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_sdrplay[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
       8,   29,   29,   29, 0x08,
      30,   29,   29,   29, 0x08,
      51,   29,   29,   29, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_sdrplay[] = {
    "sdrplay\0setExternalGain(int)\0\0"
    "set_fCorrection(int)\0set_KhzOffset(int)\0"
};

void sdrplay::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        sdrplay *_t = static_cast<sdrplay *>(_o);
        switch (_id) {
        case 0: _t->setExternalGain((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->set_fCorrection((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->set_KhzOffset((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData sdrplay::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject sdrplay::staticMetaObject = {
    { &virtualInput::staticMetaObject, qt_meta_stringdata_sdrplay,
      qt_meta_data_sdrplay, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &sdrplay::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *sdrplay::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *sdrplay::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_sdrplay))
        return static_cast<void*>(const_cast< sdrplay*>(this));
    if (!strcmp(_clname, "Ui_sdrplayWidget"))
        return static_cast< Ui_sdrplayWidget*>(const_cast< sdrplay*>(this));
    return virtualInput::qt_metacast(_clname);
}

int sdrplay::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = virtualInput::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
