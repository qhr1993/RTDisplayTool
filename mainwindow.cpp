#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fftw3.h"
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QFileDialog>
#include <QSharedMemory>
#include "math.h"

#define YMAX 40*1.023
#define BITDEPTH_MAX 8

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    readingThread = new RTReadingThread(this);
    qRegisterMetaType<Attributes>("Attributes");
    connect(readingThread, SIGNAL(dispAttr(Attributes)), this, SLOT(onAttrRcvd(Attributes)));
    connect(readingThread, SIGNAL(dispHeader(QString)), this, SLOT(onHeaderRcvd(QString)));
    connect(readingThread, SIGNAL(threadTerminated()),this,SLOT(onThreadTerminated()));
    connect(ui->buttonGroup,SIGNAL(buttonClicked(int)),this, SLOT(onChannelSelChanged(int)));
    connect(readingThread,SIGNAL(sendFFTSamples(FFTSamples,int,bool)),this,SLOT(onFFTSampleRcvd(FFTSamples,int,bool)));
    connect(ui->widget->xAxis,SIGNAL(rangeChanged(QCPRange,QCPRange)),this,SLOT(onXAxisRangeChanged(QCPRange,QCPRange)));
    connect(ui->widget->yAxis,SIGNAL(rangeChanged(QCPRange,QCPRange)),this,SLOT(onYAxisRangeChanged(QCPRange,QCPRange)));
    connect(ui->widget_2->yAxis,SIGNAL(rangeChanged(QCPRange,QCPRange)),this,SLOT(on2YAxisRangeChanged(QCPRange,QCPRange)));
    connect(readingThread,SIGNAL(signalError()),this,SLOT(onSignalError()));

    ui->buttonGroup->setId(ui->radioButton,0);
    ui->buttonGroup->setId(ui->radioButton_2,1);
    ui->buttonGroup->setId(ui->radioButton_3,2);
    ui->buttonGroup->setId(ui->radioButton_4,3);

    ui->tabWidget->setCurrentIndex(0);
    ui->comboBox_3->setDisabled(true);
    ui->comboBox_4->setDisabled(true);


    xLimit=40.92;
    xCentre=1575.43;
    y2Max=32768;
    yMax=50;
    yMin=0;
    ui->widget->clearGraphs();
    ui->widget->addGraph();
    ui->widget->graph(0)->setPen(QPen(Qt::blue));
    ui->widget->axisRect()->setupFullAxesBox(true);
    ui->widget->xAxis->setRange(-40.92,40.92);
    ui->widget->yAxis->setRange(0,40);
    ui->widget->setInteraction(QCP::iRangeDrag, true);
    ui->widget->setInteraction(QCP::iRangeZoom, true);
    ui->widget->axisRect()->setRangeZoomAxes(ui->widget->xAxis,0);
    ui->widget->xAxis->setLabel("Frequency (MHz)");


    ui->widget_2->clearGraphs();
    ui->widget_2->xAxis2->setVisible(true);
    ui->widget_2->xAxis2->setTickLabels(false);
    ui->widget_2->yAxis2->setVisible(true);
    ui->widget_2->yAxis2->setTickLabels(false);
    bars1 = new QCPBars(ui->widget_2->xAxis, ui->widget_2->yAxis);
    ui->widget_2->addPlottable(bars1);
    bars1->setPen(Qt::NoPen);
    bars1->setBrush(QColor(10, 140, 70, 160));
    ui->widget_2->setInteraction(QCP::iRangeDrag, false);
    ui->widget_2->setInteraction(QCP::iRangeZoom, true);
    ui->widget_2->axisRect()->setRangeZoomAxes(ui->widget_2->yAxis,0);
    ui->widget_2->xAxis->setAutoTickStep(true);
    //ui->widget_2->xAxis->setTickStep(1);
    ui->widget_2->xAxis->setAutoSubTicks(false);
    ui->widget_2->xAxis->setSubTickCount(0);
    ui->widget_2->xAxis->grid()->setVisible(false);
    ui->widget_2->yAxis->setRange(0,32767);

//    QCPItemText *textTime1 = new QCPItemText(ui->widget);
//    QCPItemText *textChan1 = new QCPItemText(ui->widget);
//    QCPItemText *textTime2 = new QCPItemText(ui->widget_2);
//    QCPItemText *textChan2 = new QCPItemText(ui->widget_2);
//    ui->widget->addItem(textTime1);
//    ui->widget->addItem(textChan1);
//    ui->widget_2->addItem(textTime2);
//    ui->widget_2->addItem(textChan2);
//    textTime1->position->setCoords(1575,100);
//    textChan1->position->setCoords(1600,300);
//    textChan1->setText("chan");
//    textTime1->setText("time");



    frameCount=0;
    currentChan=-1;
    currentHistoBd=-1;
    currentSpecBd=-1;
    spectrumNumAvg=1;
    histoNumAvg=1;
    isSpectrumShown=true;
    isHistoShown=true;
    cursorEnabled=true;

    readingThread->setGNSFilePath("/home/spirent/Data");
    QDir tmpSpe("/tmp/img_buffer_spe");
    if (!tmpSpe.exists())
        QDir("/tmp").mkdir("/tmp/img_buffer_spe");
    QDir tmpHis("/tmp/img_buffer_his");
    if (!tmpHis.exists())
        QDir("/tmp").mkdir("/tmp/img_buffer_his");
}

MainWindow::~MainWindow()
{
    delete ui;
    readingThread->isAlive=false;
    readingThread->wait();
    delete readingThread;
}


void MainWindow::on_pushButton_clicked()
{
    int stra = ui->comboBox_5->currentIndex();
    readingThread->setRamDiskPath("/run/shm");
    readingThread->setChannel(ui->buttonGroup->checkedId());
    readingThread->resetSync();
    switch (stra)
    {
        case 0:   readingThread->strategy=ST_SETUP;   break;
        case 1:  readingThread->strategy=ST_RECORD;  break;
        case 2:  readingThread->strategy=ST_REPLAY; break;
        default: readingThread->strategy=ST_NULL;break;
    }
    if (readingThread->strategy!=ST_SETUP)
    {
        ui->comboBox_3->setDisabled(true);
        ui->comboBox_4->setDisabled(true);
    }
    else
    {
        ui->comboBox_3->setDisabled(false);
        ui->comboBox_4->setDisabled(false);
    }

    if (ui->buttonGroup->checkedId()<2)
        readingThread->setFPGASel(0);
    else
        readingThread->setFPGASel(1);
    readingThread->isAlive=true;
    readingThread->start(QThread::NormalPriority);
    frameCount=0;
    if (readingThread->isRunning())
    {
        ui->pushButton_2->setEnabled(true);
        ui->pushButton->setDisabled(true);
        ui->comboBox_5->setDisabled(true);
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    readingThread->isAlive=false;
}

void MainWindow::onHeaderRcvd(QString header)
{
    ui->textBrowser->setText(header);
}

void MainWindow::onAttrRcvd(Attributes attr)
{
    ui->treeWidget->clear();
    attri = attr;
    for (int i=0;i<4;i++)
    {
        if (!attr.signal[i].isEmpty())
        {
            QTreeWidgetItem *signal = new QTreeWidgetItem(ui->treeWidget);
            signal->setText(0, "Signal "+QString::number(i+1));
            QTreeWidgetItem *nameItem = new QTreeWidgetItem(signal);
            nameItem->setText(0, tr("Name"));
            nameItem->setText(1, attr.signal[i]);
            QTreeWidgetItem *bwItem = new QTreeWidgetItem(signal);
            bwItem->setText(0, tr("Bandwidth (MHz)"));
            bwItem->setText(1, QString::number(attr.bw[i]));
            QTreeWidgetItem *freqItem = new QTreeWidgetItem(signal);
            freqItem->setText(0, tr("Centre Freq (MHz)"));
            freqItem->setText(1, QString::number(attr.freq[i],'f',3));
            QTreeWidgetItem *numBitItem = new QTreeWidgetItem(signal);
            numBitItem->setText(0, tr("No. of Bits"));
            numBitItem->setText(1,QString::number(attr.numBits[i]));
            //qWarning()<<"singals: "<<ui->buttonGroup->buttons().length();
            ui->buttonGroup->button(i)->setEnabled(true);
        }
        else
        {
            ui->buttonGroup->button(i)->setEnabled(false);
        }
    }
}

void MainWindow::onThreadTerminated()
{
    readingThread->wait();
    if (!readingThread->isRunning())
    {
        ui->pushButton->setEnabled(true);
        ui->pushButton_2->setDisabled(true);
        ui->radioButton->setEnabled(false);
        ui->radioButton_2->setEnabled(false);
        ui->radioButton_3->setEnabled(false);
        ui->radioButton_4->setEnabled(false);
        ui->radioButton->setChecked(true);
        ui->comboBox_5->setEnabled(true);
        ui->comboBox_4->setDisabled(true);
        ui->comboBox_3->setDisabled(true);
    }
}

void MainWindow::onChannelSelChanged(int id)
{
    qWarning()<<"Checked Id: "<< id;
    readingThread->setChannel(ui->buttonGroup->checkedId());
    readingThread->setFPGASel((ui->buttonGroup->checkedId())/2);
}

void MainWindow::onFFTSampleRcvd(FFTSamples samples,int numOfBits,bool isSetupMode)
{
    bool newFlag=(samples.chan!=currentChan);
    currentChan=samples.chan;
    bool newSpecBdFlag=(ui->comboBox_3->currentText().toInt()!=currentSpecBd);
    bool newHistoBdFlag=(ui->comboBox_4->currentText().toInt()!=currentHistoBd);
    qint32 specBdFactor,histoBdFactor ;
    if(!isSetupMode)
    {
        currentSpecBd=numOfBits;
        currentHistoBd=numOfBits;
        //make sure no manual bit reduction if not in setup mode
        specBdFactor= 1;
        histoBdFactor = 1;
    }
    else
    {
        currentHistoBd=ui->comboBox_4->currentText().toInt();
        currentSpecBd=ui->comboBox_3->currentText().toInt();
        //setup mode
        specBdFactor= pow(2,16-currentSpecBd);
        histoBdFactor = pow(2,16-currentHistoBd);
    }
    //qWarning()<<samples.valuesI[0]<<samples.valuesI[1]<<samples.valuesI[2]<<samples.valuesI[3]<<samples.valuesI[4];

    if (newFlag)
    {
        sampleSpectrumBuffer.clear();
        sampleHistoBuffer.clear();
        xLimit=(double)(readingThread->attr.bw[currentChan-1])/2*1.023;
        xCentre=(double) (readingThread->attr.freq[currentChan-1])*1.023;
        ui->widget->xAxis->setRange((-1)*xLimit+xCentre ,xLimit+xCentre);
        qWarning()<<"new flag;";
    }
    if (newSpecBdFlag   &&  isSetupMode)
    {
        sampleSpectrumBuffer.clear();
        qWarning()<<"new spec flag;";
    }
    if (newHistoBdFlag  && isSetupMode)
    {
        sampleHistoBuffer.clear();
        qWarning()<<"new histo flag;";
    }
    //qWarning()<<"sample received";
    fftw_complex *in, *out;
    fftw_plan p;
    QVector <double> outMagLog(32768),xScale(32768);
    QVector <double>   distribution(64),xScaleDist(64);
    qint32 delta,scale=0;
    if (currentHistoBd<=BITDEPTH_MAX)
    {
        if (!isSetupMode)   delta=pow(2,currentHistoBd-1);
        else    delta = pow(2,15);
        distribution.resize(pow(2,currentHistoBd));
        xScaleDist.resize(distribution.size());
    }
    else
    {
        if (!isSetupMode)   delta=pow(2,currentHistoBd-1);
        else    delta = pow(2,15);
        scale=pow(2,currentHistoBd-BITDEPTH_MAX);
        distribution.resize(pow(2,currentHistoBd)/scale);
        xScaleDist.resize(distribution.size());
    }
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 32768);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 32768);
    p = fftw_plan_dft_1d(32768, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    for (int i=0;i<32768;i++)
    {
        in[i][0]=(double)((samples.valuesI[i]/specBdFactor)*2+1);
        in[i][1]=(double)((samples.valuesQ[i]/specBdFactor)*2+1);

        if (currentHistoBd<=BITDEPTH_MAX)
        {
            //qWarning()<<samples.valuesI[i]+delta;
            //qWarning()<<(samples.valuesQ[i]/histoBdFactor+delta)<<distribution.size();
            distribution[(samples.valuesQ[i]+delta)/histoBdFactor]++;
        }
        else
        {
            //qWarning()<<samples.valuesI[i]+delta;
            //qWarning()<<(samples.valuesQ[i]/histoBdFactor+delta)/scale<<distribution.size();
            distribution[(samples.valuesQ[i]+delta)/histoBdFactor/scale]++;
        }
    }
    fftw_execute(p); /* repeat as needed */
    for (int i=0;i<32768;i++)
    {
        if (i<16384)
            xScale[i]=((double)i)/16384*xLimit + xCentre;
        else
            xScale[i]=((double)i-32768)/16384*xLimit + xCentre;
        //outMagLog[i]=10*log10(sqrt(((out[i+1][0])*(out[i+1][0])+(out[i+1][1])*(out[i+1][1]))/2));
        outMagLog[i]=(sqrt(((out[i][1])*(out[i][1]))/2));
    }
    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);

    for (int i=0;i<xScaleDist.size();i++)
    {
        xScaleDist[i]=i;
    }

    while (sampleSpectrumBuffer.length()>=spectrumNumAvg)
        sampleSpectrumBuffer.removeFirst();
    sampleSpectrumBuffer.append(outMagLog);
    while (sampleHistoBuffer.length()>=histoNumAvg)
        sampleHistoBuffer.removeFirst();
    sampleHistoBuffer.append(distribution);
    QList <double> scaleList2 = avg(&tmpHistoSamples,&sampleHistoBuffer);
    y2Max = scaleList2.first();
    //qWarning()<<tmpHistoSamples;
    //qWarning()<<sampleHistoBuffer.last();
    QList <double> scaleList = avg(&tmpSpectrumSamples,&sampleSpectrumBuffer);
    yMax = 10*log10(scaleList.first());
    yMin = 10*log10(scaleList.last());
    for (int i=0;i<32768;i++)
        tmpSpectrumSamples[i]=10*log10(tmpSpectrumSamples[i]);
    //qWarning()<<tmpSpectrumSamples.at(1024);
    //qWarning()<<sampleSpectrumBuffer.last().at(1024);
    frameCount++;
    //qWarning()<<frameCount;
    if (isSpectrumShown)
    {
        //ui->widget->graph(0)->setData(xScale, outMagLog);
        ui->widget->graph(0)->setData(xScale, tmpSpectrumSamples);
        ui->widget->replot();
        ui->label_3->setText(QString::number((double)frameCount/5.0,'f',1));
        //----
    }
    if (isHistoShown)
    {
        //bars1->setData(xScaleDist,distribution);
        bars1->setData(xScaleDist,tmpHistoSamples);
        ui->widget_2->replot();
        ui->label_10->setText(QString::number((double)frameCount/5.0,'f',1));
    }
    bars1->keyAxis()->setRange(-0.5,xScaleDist.size()-0.5);

    //qWarning()<<distribution;
    ui->label_4->setText(QString::number(samples.chan));
    ui->label_9->setText(QString::number(samples.chan));
    if (newFlag | (frameCount%30==0))
    {
        ui->widget->yAxis->setRangeLower(-5+yMin);
        ui->widget->yAxis->setRangeUpper(5+yMax);
        ui->widget_2->yAxis->setRangeLower(0);
        ui->widget_2->yAxis->setRangeUpper(1.2*y2Max);
        removeDir("/tmp/img_buffer_spe");
        removeDir("/tmp/img_buffer_his");
    }

    QDir::setCurrent("/tmp/img_buffer_spe");
    ui->widget->saveJpg(QTime::currentTime().toString("hhmmsszzz-SPE")+".jpg",400,300);
    QDir::setCurrent("/tmp/img_buffer_his");
    ui->widget_2->saveJpg(QTime::currentTime().toString("hhmmsszzz-HIS")+".jpg",400,300);
    QDir::setCurrent("/tmp");
    QFile statusInfo("statusInfo.txt");
    statusInfo.open(QIODevice::WriteOnly|QIODevice::Truncate);
    QTextStream statusInfoStream(&statusInfo);
    statusInfoStream<<"<signal>"<<readingThread->attr.signal[currentChan-1]<<"</signal>";
    statusInfoStream<<"<bandwidth>"<<QString::number(readingThread->attr.bw[currentChan-1])<<"</bandwidth>";
    statusInfoStream<<"<bitdepth>"<<QString::number(readingThread->attr.numBits[currentChan-1])<<"</bitdepth>";
    statusInfoStream<<"<freq>"<<QString::number((double)readingThread->attr.freq[currentChan-1]*1.023,'f',2)<<"</freq>";
    statusInfoStream<<"<time>"<<QString::number((double)frameCount/5.0,'f',1)<<"</time>";
    statusInfo.close();
    QDir::setCurrent("/home/spirent/Projects/RTDisplayTool");
}

void MainWindow::on_pushButton_6_clicked()
{
    ui->widget->saveJpg(QTime::currentTime().toString("hhmmsszzz-SPE")+".jpg",800,480);
}

void MainWindow::on_pushButton_9_clicked()
{
    ui->widget_2->saveJpg(QTime::currentTime().toString("hhmmsszzz-HIS")+".jpg",800,480);
}

void MainWindow::onXAxisRangeChanged(QCPRange newRange,QCPRange oldRange)
{
    if(newRange.lower<(-1)*xLimit+xCentre)
    {
        ui->widget->xAxis->setRangeLower(-1*xLimit+xCentre);
    }
    if(newRange.upper>xLimit+xCentre)
    {
        ui->widget->xAxis->setRangeUpper(xLimit+xCentre);
    }
}

void MainWindow::onYAxisRangeChanged(QCPRange newRange,QCPRange oldRange)
{
    if(newRange.lower<=(-1)*150)
    {
        ui->widget->yAxis->setRangeLower(oldRange.lower);
        ui->widget->yAxis->setRangeUpper(oldRange.upper);
    }
    if(newRange.upper>=150)
    {
        ui->widget->yAxis->setRangeUpper(oldRange.upper);
        ui->widget->yAxis->setRangeLower(oldRange.lower);
    }
}

void MainWindow::on2YAxisRangeChanged(QCPRange newRange,QCPRange oldRange)
{
    ui->widget_2->yAxis->setRangeLower(0);
    if(newRange.upper>=32768)
    {
        ui->widget_2->yAxis->setRangeUpper(oldRange.upper);
        ui->widget_2->yAxis->setRangeLower(0);
    }
}

template <typename T,typename P> QList<double> MainWindow::avg(T* out,P* in)
{
    double tmp=0,max=-32768,min=32768;
    QList <double> list;
    int sampleLength=in->first().size();
    out->clear();
    for (int i=0;i<sampleLength;i++)
    {
        for (int j=0;j<in->length();j++)
            tmp=tmp+in->at(j).at(i);
        (*out).append(tmp/(in->length()));
        if ((*out).last()>max)    max = (*out).last();
        if ((*out).last()<min)    min = (*out).last();
        tmp=0;
    }
    return (list<<max<<min);
}

void MainWindow::on_comboBox_2_currentIndexChanged(const QString &arg1)
{
    QString tmpStr(ui->comboBox_2->currentText());
    histoNumAvg = tmpStr.toInt();
}

void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    QString tmpStr(ui->comboBox->currentText());
    spectrumNumAvg = tmpStr.toInt();
}

void MainWindow::on_pushButton_5_clicked()
{
    isSpectrumShown=false;
    ui->pushButton_4->setEnabled(true);
    ui->pushButton_5->setDisabled(true);
}

void MainWindow::on_pushButton_4_clicked()
{
    isSpectrumShown=true;
    ui->pushButton_4->setEnabled(false);
    ui->pushButton_5->setDisabled(false);
}

void MainWindow::on_pushButton_7_clicked()
{
    isHistoShown=false;
    ui->pushButton_8->setEnabled(true);
    ui->pushButton_7->setDisabled(true);
}

void MainWindow::on_pushButton_8_clicked()
{
    isHistoShown=true;
    ui->pushButton_8->setEnabled(false);
    ui->pushButton_7->setDisabled(false);
}

void MainWindow::onSignalError()
{
    QMessageBox msgBox;
    msgBox.setText("Signal does not exist.");
    msgBox.exec();
}

//void MainWindow::manageCursor(QCustomPlot *customPlot, QCPCursor *cursor, double x, double y, QPen pen)
//{
//    if(cursorEnabled)
//    {
//        if(cursor->vLine) customPlot->removeItem(cursor->vLine);
//        cursor->vLine = new QCPItemLine(customPlot);
//        customPlot->addItem(cursor->vLine);
//        cursor->vLine->setPen(pen);
//        cursor->vLine->start->setCoords( x, QCPRange::minRange);
//        cursor->vLine->end->setCoords( x, QCPRange::maxRange);
//    }
//}

//void MainWindow::mouseRelease(QMouseEvent* event)
//{
//    QCustomPlot *customPlot=ui->widget;
//    static QCPCursor cursor1;
//    double x=customPlot->xAxis->pixelToCoord(event->pos().x());
//    double y=customPlot->yAxis->pixelToCoord(event->pos().y());

//    if(event->button() == Qt::LeftButton)
//        manageCursor(customPlot, &cursor1, x, y, QPen(Qt::black));
//    customPlot->replot();
//    cursorEnabled=true;
//}

//void MainWindow::on_pushButton_3_clicked()
//{
//    ui->widget->xAxis->setRangeLower(-1*xLimit+xCentre);
//    ui->widget->xAxis->setRangeUpper(xLimit+xCentre);
//}



//void MainWindow::on_pushButton_10_clicked()
//{
//    ui->widget_2->yAxis->setRangeLower(0);
//    ui->widget_2->yAxis->setRangeUpper(1.2*y2Max);
//}

void MainWindow::on_pushButton_3_clicked()
{
    ui->widget->xAxis->setRangeLower(-1*xLimit+xCentre);
    ui->widget->xAxis->setRangeUpper(xLimit+xCentre);
    ui->widget->yAxis->setRangeLower(-5+yMin);
    ui->widget->yAxis->setRangeUpper(5+yMax);
    qWarning()<<yMin<<yMax;
}

void MainWindow::on_pushButton_10_clicked()
{
    ui->widget_2->yAxis->setRangeLower(0);
    ui->widget_2->yAxis->setRangeUpper(1.2*y2Max);
}


bool MainWindow::removeDir(const QString & dirName)
{
    bool result = true;
    QDir dir(dirName);
    if (dir.exists())
    {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
        {
            if (info.isDir())
            {
                result = removeDir(info.absoluteFilePath());
            }
            else
            {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result)
            {
                return result;
            }
        }
    }
    return result;
}
