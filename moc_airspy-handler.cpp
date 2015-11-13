/****************************************************************************
** Meta object code from reading C++ file 'airspy-handler.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "src/input/airspy/airspy-handler.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'airspy-handler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_airspyHandler[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      14,   32,   38,   38, 0x08,
      39,   32,   38,   38, 0x08,
      59,   32,   38,   38, 0x08,
      77,   38,   38,   38, 0x08,
      91,   38,   38,   38, 0x08,
     107,   38,   38,   38, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_airspyHandler[] = {
    "airspyHandler\0set_lna_gain(int)\0value\0"
    "\0set_mixer_gain(int)\0set_vga_gain(int)\0"
    "set_lna_agc()\0set_mixer_agc()\0"
    "set_rf_bias()\0"
};

void airspyHandler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        airspyHandler *_t = static_cast<airspyHandler *>(_o);
        switch (_id) {
        case 0: _t->set_lna_gain((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->set_mixer_gain((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->set_vga_gain((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->set_lna_agc(); break;
        case 4: _t->set_mixer_agc(); break;
        case 5: _t->set_rf_bias(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData airspyHandler::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject airspyHandler::staticMetaObject = {
    { &virtualInput::staticMetaObject, qt_meta_stringdata_airspyHandler,
      qt_meta_data_airspyHandler, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &airspyHandler::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *airspyHandler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *airspyHandler::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_airspyHandler))
        return static_cast<void*>(const_cast< airspyHandler*>(this));
    if (!strcmp(_clname, "Ui_airspyWidget"))
        return static_cast< Ui_airspyWidget*>(const_cast< airspyHandler*>(this));
    return virtualInput::qt_metacast(_clname);
}

int airspyHandler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = virtualInput::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
