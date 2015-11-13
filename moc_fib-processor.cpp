/****************************************************************************
** Meta object code from reading C++ file 'fib-processor.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "includes/backend/fib-processor.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fib-processor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_fib_processor[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      14,   40,   42,   42, 0x05,
      43,   42,   42,   42, 0x05,
      66,   40,   42,   42, 0x05,
      94,   42,   42,   42, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_fib_processor[] = {
    "fib_processor\0addEnsembleChar(char,int)\0"
    ",\0\0addtoEnsemble(QString)\0"
    "nameofEnsemble(int,QString)\0"
    "changeinConfiguration()\0"
};

void fib_processor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        fib_processor *_t = static_cast<fib_processor *>(_o);
        switch (_id) {
        case 0: _t->addEnsembleChar((*reinterpret_cast< char(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->addtoEnsemble((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->nameofEnsemble((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->changeinConfiguration(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData fib_processor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject fib_processor::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_fib_processor,
      qt_meta_data_fib_processor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &fib_processor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *fib_processor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *fib_processor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_fib_processor))
        return static_cast<void*>(const_cast< fib_processor*>(this));
    return QObject::qt_metacast(_clname);
}

int fib_processor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void fib_processor::addEnsembleChar(char _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void fib_processor::addtoEnsemble(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void fib_processor::nameofEnsemble(int _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void fib_processor::changeinConfiguration()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
