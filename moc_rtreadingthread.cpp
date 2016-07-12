/****************************************************************************
** Meta object code from reading C++ file 'rtreadingthread.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "rtreadingthread.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'rtreadingthread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_RTReadingThread[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   17,   16,   16, 0x05,
      49,   44,   16,   16, 0x05,
      70,   16,   16,   16, 0x05,
     115,   89,   16,   16, 0x05,
     151,   16,   16,   16, 0x05,

 // slots: signature, parameters, type, tag, flags
     165,   16,   16,   16, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_RTReadingThread[] = {
    "RTReadingThread\0\0header\0dispHeader(QString)\0"
    "attr\0dispAttr(Attributes)\0threadTerminated()\0"
    "samples,numOfBits,isSetup\0"
    "sendFFTSamples(FFTSamples,int,bool)\0"
    "signalError()\0onTimeOut()\0"
};

void RTReadingThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        RTReadingThread *_t = static_cast<RTReadingThread *>(_o);
        switch (_id) {
        case 0: _t->dispHeader((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->dispAttr((*reinterpret_cast< Attributes(*)>(_a[1]))); break;
        case 2: _t->threadTerminated(); break;
        case 3: _t->sendFFTSamples((*reinterpret_cast< FFTSamples(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 4: _t->signalError(); break;
        case 5: _t->onTimeOut(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData RTReadingThread::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject RTReadingThread::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_RTReadingThread,
      qt_meta_data_RTReadingThread, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &RTReadingThread::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *RTReadingThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *RTReadingThread::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RTReadingThread))
        return static_cast<void*>(const_cast< RTReadingThread*>(this));
    return QThread::qt_metacast(_clname);
}

int RTReadingThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void RTReadingThread::dispHeader(QString _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void RTReadingThread::dispAttr(Attributes _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RTReadingThread::threadTerminated()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void RTReadingThread::sendFFTSamples(FFTSamples _t1, int _t2, bool _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void RTReadingThread::signalError()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
