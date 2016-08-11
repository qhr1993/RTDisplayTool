#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSharedMemory>
#include "rtreadingthread.h"
#include "qcustomplot.h"
#include "sharedcontrol.h"

namespace Ui {
class MainWindow;
}

typedef QVector<double> SpectrumSamples;
typedef QList<SpectrumSamples> SpectrumSampleBuffer;

typedef QVector<double> HistoSamples;
typedef QList<SpectrumSamples> HistoSampleBuffer;

typedef struct {
    QCPItemLine *vLine;
} QCPCursor;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    RTReadingThread *readingThread;
    QCPBars* bars1;
    SharedControl *ptrMem;
    QSharedMemory *sharedMem;
    void syncUiToShared();
    QTimer *timer;
private:
    int frameCount;
    SpectrumSamples tmpSpectrumSamples;
    SpectrumSampleBuffer sampleSpectrumBuffer;
    HistoSamples tmpHistoSamples;
    HistoSampleBuffer sampleHistoBuffer;
    int currentChan,currentSpecBd,currentHistoBd;
    int spectrumNumAvg,histoNumAvg;
    template <typename T,typename P> QList<double> avg(T* out,P* in);
    bool isHistoShown,isSpectrumShown;
    void manageCursor(QCustomPlot *customPlot, QCPCursor *cursor, double x, double y, QPen pen);
    bool cursorEnabled;
    double xLimit,xCentre,y2Max,yMax,yMin;
    bool removeDir(const QString & dirName);
    Ui::MainWindow *ui;
    Attributes attri;
    bool isShmSuccess;
private slots:
    void onHeaderRcvd(QString header);
    void onAttrRcvd(Attributes attr);

    void onThreadTerminated();
    void onChannelSelChanged(int id);
    void onFFTSampleRcvd(FFTSamples,int,bool);

    void on_pushButton_6_clicked();

    void on_pushButton_9_clicked();
    void onXAxisRangeChanged(QCPRange,QCPRange);
    void onYAxisRangeChanged(QCPRange,QCPRange);
    void on2YAxisRangeChanged(QCPRange,QCPRange);

    void on_comboBox_2_currentIndexChanged(const QString &arg1);

    void on_comboBox_currentIndexChanged(const QString &arg1);

    void on_pushButton_5_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();
    void onSignalError();
    void on_pushButton_3_clicked();
    void on_pushButton_10_clicked();
    void onInitialisation();
    void onOverflow(int num);
    void onUiSelectionUpdated();
};

#endif // MAINWINDOW_H
