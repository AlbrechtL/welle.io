/****************************************************************************
** Meta object code from reading C++ file 'gui.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "gui.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RadioInterface[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      25,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   26,   26,   26, 0x08,
      27,   26,   26,   26, 0x08,
      47,   26,   26,   26, 0x08,
      73,   26,   26,   26, 0x08,
      93,   26,   26,   26, 0x08,
     112,   26,   26,   26, 0x08,
     129,   26,   26,   26, 0x08,
     148,   26,   26,   26, 0x08,
     172,   26,   26,   26, 0x08,
     199,   26,   26,   26, 0x08,
     218,   26,   26,   26, 0x08,
     245,   26,   26,   26, 0x08,
     259,   26,   26,   26, 0x08,
     275,   26,   26,   26, 0x0a,
     305,   26,   26,   26, 0x0a,
     337,   26,   26,   26, 0x0a,
     353,   26,   26,   26, 0x0a,
     376,  404,   26,   26, 0x0a,
     406,  404,   26,   26, 0x0a,
     432,   26,   26,   26, 0x0a,
     454,   26,   26,   26, 0x0a,
     473,   26,   26,   26, 0x0a,
     487,   26,   26,   26, 0x0a,
     503,   26,   26,   26, 0x0a,
     522,   26,   26,   26, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_RadioInterface[] = {
    "RadioInterface\0setStart()\0\0"
    "updateTimeDisplay()\0setStreamOutSelector(int)\0"
    "selectMode(QString)\0autoCorrector_on()\0"
    "abortSystem(int)\0TerminateProcess()\0"
    "set_bandSelect(QString)\0"
    "set_channelSelect(QString)\0"
    "setDevice(QString)\0selectService(QModelIndex)\0"
    "set_dumping()\0set_audioDump()\0"
    "set_fineCorrectorDisplay(int)\0"
    "set_coarseCorrectorDisplay(int)\0"
    "clearEnsemble()\0addtoEnsemble(QString)\0"
    "nameofEnsemble(int,QString)\0,\0"
    "addEnsembleChar(char,int)\0"
    "show_successRate(int)\0show_ficRatio(int)\0"
    "show_snr(int)\0setSynced(char)\0"
    "showLabel(QString)\0changeinConfiguration()\0"
};

void RadioInterface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        RadioInterface *_t = static_cast<RadioInterface *>(_o);
        switch (_id) {
        case 0: _t->setStart(); break;
        case 1: _t->updateTimeDisplay(); break;
        case 2: _t->setStreamOutSelector((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->selectMode((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->autoCorrector_on(); break;
        case 5: _t->abortSystem((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->TerminateProcess(); break;
        case 7: _t->set_bandSelect((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->set_channelSelect((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 9: _t->setDevice((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 10: _t->selectService((*reinterpret_cast< QModelIndex(*)>(_a[1]))); break;
        case 11: _t->set_dumping(); break;
        case 12: _t->set_audioDump(); break;
        case 13: _t->set_fineCorrectorDisplay((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->set_coarseCorrectorDisplay((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 15: _t->clearEnsemble(); break;
        case 16: _t->addtoEnsemble((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->nameofEnsemble((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 18: _t->addEnsembleChar((*reinterpret_cast< char(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 19: _t->show_successRate((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 20: _t->show_ficRatio((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 21: _t->show_snr((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 22: _t->setSynced((*reinterpret_cast< char(*)>(_a[1]))); break;
        case 23: _t->showLabel((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 24: _t->changeinConfiguration(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RadioInterface::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RadioInterface::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_RadioInterface,
      qt_meta_data_RadioInterface, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RadioInterface::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RadioInterface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RadioInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RadioInterface))
        return static_cast<void*>(const_cast< RadioInterface*>(this));
    return QDialog::qt_metacast(_clname);
}

int RadioInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 25)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 25;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
