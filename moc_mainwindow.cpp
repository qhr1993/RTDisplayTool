/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "mainwindow.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MainWindow[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x08,
      43,   36,   11,   11, 0x08,
      70,   65,   11,   11, 0x08,
      93,   11,   11,   11, 0x08,
     119,   11,   11,   11, 0x08,
     143,  140,   11,   11, 0x08,
     171,  168,   11,   11, 0x08,
     208,   11,   11,   11, 0x08,
     234,   11,   11,   11, 0x08,
     262,  260,   11,   11, 0x08,
     301,  260,   11,   11, 0x08,
     340,  260,   11,   11, 0x08,
     385,  380,   11,   11, 0x08,
     428,  380,   11,   11, 0x08,
     469,   11,   11,   11, 0x08,
     495,   11,   11,   11, 0x08,
     521,   11,   11,   11, 0x08,
     547,   11,   11,   11, 0x08,
     573,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_MainWindow[] = {
    "MainWindow\0\0on_pushButton_clicked()\0"
    "header\0onHeaderRcvd(QString)\0attr\0"
    "onAttrRcvd(Attributes)\0on_pushButton_2_clicked()\0"
    "onThreadTerminated()\0id\0"
    "onChannelSelChanged(int)\0,,\0"
    "onFFTSampleRcvd(FFTSamples,int,bool)\0"
    "on_pushButton_6_clicked()\0"
    "on_pushButton_9_clicked()\0,\0"
    "onXAxisRangeChanged(QCPRange,QCPRange)\0"
    "onYAxisRangeChanged(QCPRange,QCPRange)\0"
    "on2YAxisRangeChanged(QCPRange,QCPRange)\0"
    "arg1\0on_comboBox_2_currentIndexChanged(QString)\0"
    "on_comboBox_currentIndexChanged(QString)\0"
    "on_pushButton_5_clicked()\0"
    "on_pushButton_4_clicked()\0"
    "on_pushButton_7_clicked()\0"
    "on_pushButton_8_clicked()\0onSignalError()\0"
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MainWindow *_t = static_cast<MainWindow *>(_o);
        switch (_id) {
        case 0: _t->on_pushButton_clicked(); break;
        case 1: _t->onHeaderRcvd((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->onAttrRcvd((*reinterpret_cast< Attributes(*)>(_a[1]))); break;
        case 3: _t->on_pushButton_2_clicked(); break;
        case 4: _t->onThreadTerminated(); break;
        case 5: _t->onChannelSelChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->onFFTSampleRcvd((*reinterpret_cast< FFTSamples(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 7: _t->on_pushButton_6_clicked(); break;
        case 8: _t->on_pushButton_9_clicked(); break;
        case 9: _t->onXAxisRangeChanged((*reinterpret_cast< QCPRange(*)>(_a[1])),(*reinterpret_cast< QCPRange(*)>(_a[2]))); break;
        case 10: _t->onYAxisRangeChanged((*reinterpret_cast< QCPRange(*)>(_a[1])),(*reinterpret_cast< QCPRange(*)>(_a[2]))); break;
        case 11: _t->on2YAxisRangeChanged((*reinterpret_cast< QCPRange(*)>(_a[1])),(*reinterpret_cast< QCPRange(*)>(_a[2]))); break;
        case 12: _t->on_comboBox_2_currentIndexChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->on_comboBox_currentIndexChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 14: _t->on_pushButton_5_clicked(); break;
        case 15: _t->on_pushButton_4_clicked(); break;
        case 16: _t->on_pushButton_7_clicked(); break;
        case 17: _t->on_pushButton_8_clicked(); break;
        case 18: _t->onSignalError(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MainWindow::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MainWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_MainWindow,
      qt_meta_data_MainWindow, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MainWindow::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow))
        return static_cast<void*>(const_cast< MainWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
