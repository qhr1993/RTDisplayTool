#include "rtreadingthread.h"
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QProcess>
#include <unistd.h>
#include <sys/file.h>

#define BUFFER_SIZE 50
#define TIMER_MSEC 200
#define PRELOAD_SIZE 25
#define REVSTR "RECORDING"
#define RPLSTR "PLAYING"
#define STPSTR "STOPPED"

RTReadingThread::RTReadingThread(QObject *parent):QThread(parent)
{
    fileSeqA=0;
    fileSeqB=0;
    syncModeA=-1;
    syncModeB=-1;
    skipWordCountA[0]=-1;
    skipWordCountB[0]=-1;
    readWordCountA[0]=-1;
    readWordCountB[0]=-1;
    skipWordCountA[1]=-1;
    skipWordCountB[1]=-1;
    readWordCountA[1]=-1;
    readWordCountB[1]=-1;
    fftBufferA.reserve(BUFFER_SIZE);
    fftBufferB.reserve(BUFFER_SIZE);
    channelSel=-1;
    currentChanA=-1;
    currentChanB=-1;
    strategy = ST_SETUP;
    startFlag = false;

    attr.signal.resize(4);
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeOut()));
}
RTReadingThread::~RTReadingThread()
{
    delete timer;
    this->isAlive=false;
    this->terminate();
    this->wait();
}

void RTReadingThread::run()
{
    while (1)
    {
        pollingProc = new QProcess(this);
        fileSeqA=0;
        fileSeqB=0;
        fftBufferA.clear();
        fftBufferB.clear();
        fftBufferA.reserve(BUFFER_SIZE);
        fftBufferB.reserve(BUFFER_SIZE);
        while (isStopped())
        {
            //qWarning()<<"mode: STOPPED";
            msleep(100);
        }

        isAlive=true;
        timer->start(TIMER_MSEC);
        startFlag = false;

        pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-m");
        if (!pollingProc->waitForFinished())
            return;
        QString mode = pollingProc->readAll();
        mode = mode.split(" ").at(1);
        if (mode==RPLSTR)
            strategy=ST_REPLAY;
        else if ((mode==REVSTR) && (isSetupMode()))
            strategy=ST_SETUP;
        else
            strategy=ST_RECORD;
        emit initToUi();

        while (isAlive)
        {
            if (strategy==ST_SETUP)
            {
                if (!((isRecording())&&(isSetupMode())))
                    qWarning()<<"Not in setup mode" <<isRecording()<<isSetupMode();
                else
                {
                    QString fileNameRaw;
                    pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-f");
                    if (!pollingProc->waitForFinished())
                        return;
                    fileNameRaw = pollingProc->readAll();
                    fileName = fileNameRaw.mid(10).left(12);
                    currentFileA = QFileInfo(ramDiskPathA.absolutePath()+"/"+fileName+".000");
                    qWarning()<<"current file"<<currentFileA.absoluteFilePath();
                    qWarning()<<currentFileA.fileName();
                    currentFileB = QFileInfo(ramDiskPathB.absolutePath()+"/"+fileName+".000");
                    endFileA=QFileInfo(ramDiskPathA.absolutePath()+"/"+fileName+".end");
                    endFileB=QFileInfo(ramDiskPathB.absolutePath()+"/"+fileName+".end");
                    break;
                }
            }
            else if (strategy==ST_RECORD)
            {
                if (!((isRecording())&&(!(isSetupMode()))))
                    qWarning()<<"Not in recording mode" ;
                else
                {
                    QString fileNameRaw;
                    pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-f");
                    if (!pollingProc->waitForFinished())
                        return;
                    fileNameRaw = pollingProc->readAll();
                    fileName = fileNameRaw.mid(10).left(12);
                    currentFileA = QFileInfo(gnsFile.absolutePath()+"/"+fileName+".A.gns");
                    qWarning()<<"current file"<<currentFileA.absoluteFilePath();
                    qWarning()<<currentFileA.fileName();
                    currentFileB = QFileInfo(gnsFile.absolutePath()+"/"+fileName+".B.gns");
                    break;
                }
            }
            else if (strategy==ST_REPLAY)
            {
                if (!(isReplaying()))
                    qWarning()<<"Not in replaying mode" ;
                else
                {
                    QString fileNameRaw;
                    pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-f");
                    if (!pollingProc->waitForFinished())
                        return;
                    fileNameRaw = pollingProc->readAll();
                    fileName = fileNameRaw.mid(10).left(12);
                    currentFileA = QFileInfo(gnsFile.absolutePath()+"/"+fileName+".A.gns");
                    qWarning()<<"current file"<<currentFileA.absoluteFilePath();
                    qWarning()<<currentFileA.fileName();
                    currentFileB = QFileInfo(gnsFile.absolutePath()+"/"+fileName+".B.gns");
                    break;
                }
            }
            else
            {
                qWarning()<<"Error: State Error" ;
            }
        }

        QFile gnsFileA(currentFileA.absoluteFilePath());
        QFile gnsFileB(currentFileB.absoluteFilePath());
        QDataStream inputStreamA(&gnsFileA);
        QDataStream inputStreamB(&gnsFileB);
        bool isFirstGNSA = true,isFirstGNSB = true;
        while(isAlive)
        {
            if (isStopped()) break;
            if (strategy==ST_SETUP)
            {
                bool isRecordingReg = ((isRecording()) && (isSetupMode()));
                // qWarning()<<currentFileA.absoluteFilePath()<<currentFileA.exists()
                //<<isRecordingReg;
                //qWarning()<<currentFileA.absoluteFilePath().toLatin1().data()<<access(currentFileA.absoluteFilePath().toLatin1().data(),F_OK)<<isRecordingReg;
                if ((access(currentFileA.absoluteFilePath().toLatin1().data(),F_OK)+1)   &&  isRecordingReg)
                {
                    if (fileSeqA==0)
                    {
                        if (!getHeader())
                        {
                            qWarning()<<"Cannot open header.";
                            break;
                        }
                        if (attr.signal[(int)fpgaSel*2+(int)channelSel].isEmpty())
                        {
                            emit signalError();
                            break;
                        }

                        if (!getInitialData(currentFileA,&syncModeA,skipWordCountA,
                                            readWordCountA,attr.numBits[0],&fftBufferA,fpgaSel==0,channelSel,&currentChanA))
                        {
                            qWarning()<<"Cannot open .000.";
                            break;
                        }
                    }
                    else
                        getData(currentFileA,&syncModeA,skipWordCountA,
                                readWordCountA,attr.numBits[0],&fftBufferA,fpgaSel==0,channelSel,&currentChanA);

                    toNextFile(&currentFileA,&fileSeqA);
                }

                if ((!attr.signal[2].isEmpty()))
                {
                    // qWarning()<<currentFileB.absoluteFilePath();
                    if ((access(currentFileB.absoluteFilePath().toLatin1().data(),F_OK)+1) && isRecordingReg)
                    {
                        if (fileSeqB==0)
                        {
                            if (!getInitialData(currentFileB,&syncModeB,skipWordCountB,
                                                readWordCountB,attr.numBits[2],&fftBufferB,fpgaSel==1,channelSel,&currentChanB))
                            {
                                qWarning()<<"Cannot open .000B.";
                                break;
                            }
                        }
                        else
                            getData(currentFileB,&syncModeB,skipWordCountB,
                                    readWordCountB,attr.numBits[2],&fftBufferB,fpgaSel==1,channelSel,&currentChanB);
                        toNextFile(&currentFileB,&fileSeqB);
                    }
                }
            }
            else if (strategy==ST_RECORD)
            {
                bool isRecordingReg = ((isRecording()) && (!isSetupMode()));
                //qWarning()<<currentFileA.absoluteFilePath()<<currentFileA.exists()
                //<<isRecordingReg;
                //qWarning()<<currentFileA.absoluteFilePath().toLatin1().data()<<access(currentFileA.absoluteFilePath().toLatin1().data(),F_OK)<<isRecordingReg;
                if ((access(currentFileA.absoluteFilePath().toLatin1().data(),F_OK)+1)   &&  isRecordingReg)
                {
                    if (isFirstGNSA)
                    {
                        if (!getHeader())
                        {
                            qWarning()<<"Cannot open header.";
                            break;
                        }
                    }
                    if (attr.signal[(int)fpgaSel*2+(int)channelSel].isEmpty())
                    {
                        emit signalError();
                        break;
                    }
                    if (!gnsFileA.isOpen())
                    {
                        qWarning()<<"access: "<<(access(currentFileA.absoluteFilePath().toLatin1().data(),R_OK)+1);
                        qWarning()<<"open: "<<gnsFileA.open(QIODevice::ReadOnly);
                    }
                    if ((!gnsFileB.isOpen()) && (!attr.signal[2].isEmpty()))
                    {
                        qWarning()<<"access: "<<(access(currentFileB.absoluteFilePath().toLatin1().data(),R_OK)+1);
                        qWarning()<<"open: "<<gnsFileB.open(QIODevice::ReadOnly);
                    }
                    if (!getGNSData(&gnsFileA,&inputStreamA,&syncModeA,skipWordCountA,
                                    readWordCountA,attr.numBits[0],&fftBufferA,&currentChanA,&isFirstGNSA,
                                    &gnsFileB,&inputStreamB,&syncModeB,skipWordCountB,
                                    readWordCountB,attr.numBits[2],&fftBufferB,&currentChanB,&isFirstGNSB,(!attr.signal[2].isEmpty())))
                    {
                        qWarning()<<"Error";
                        break;
                    }
                }
            }
            else if (strategy==ST_REPLAY)
            {
                bool isReplayingReg = isReplaying();
                if (!isReplayingReg)
                {
                    fftBufferB.clear();
                    fftBufferA.clear();
                }
                //qWarning()<<currentFileA.absoluteFilePath()<<currentFileA.exists()
                //<<isRecordingReg;
                //qWarning()<<currentFileA.absoluteFilePath().toLatin1().data()<<access(currentFileA.absoluteFilePath().toLatin1().data(),F_OK)<<isReplayingReg;
                if ((access(currentFileA.absoluteFilePath().toLatin1().data(),F_OK)+1)   &&  isReplayingReg)
                {
                    if (isFirstGNSA)
                    {
                        if (!getHeader())
                        {
                            qWarning()<<"Cannot open header.";
                            break;
                        }
                    }
                    if (attr.signal[(int)fpgaSel*2+(int)channelSel].isEmpty())
                    {
                        emit signalError();
                        break;
                    }
                    if (!gnsFileA.isOpen())
                    {
                        qWarning()<<"accessA: "<<(access(currentFileA.absoluteFilePath().toLatin1().data(),R_OK)+1);
                        qWarning()<<"openA: "<<gnsFileA.open(QIODevice::ReadOnly);
                    }
                    if ((!gnsFileB.isOpen()) && (!attr.signal[2].isEmpty()))
                    {
                        qWarning()<<"accessB: "<<(access(currentFileB.absoluteFilePath().toLatin1().data(),R_OK)+1);
                        qWarning()<<"openB: "<<gnsFileB.open(QIODevice::ReadOnly);
                    }
                    if (!getGNSData(&gnsFileA,&inputStreamA,&syncModeA,skipWordCountA,
                                    readWordCountA,attr.numBits[0],&fftBufferA,&currentChanA,&isFirstGNSA,
                                    &gnsFileB,&inputStreamB,&syncModeB,skipWordCountB,
                                    readWordCountB,attr.numBits[2],&fftBufferB,&currentChanB,&isFirstGNSB,(!attr.signal[2].isEmpty())))
                    {
                        qWarning()<<"Cannot open .A.gns.";
                        break;
                    }
                }
            }
            else
            {
                qWarning()<<"Error: State Error" ;
            }
        }

        if (gnsFileA.isOpen())
            gnsFileA.close();
        if (gnsFileB.isOpen())
            gnsFileB.close();
        timer->stop();
        isAlive=false;
        delete pollingProc;
        if (strategy==ST_SETUP)
            clearRamDisk();
        emit threadTerminated();
    }
}

void RTReadingThread::toNextFile(QFileInfo* currentFile,int *seq)
{
    (*seq)++;
    QString suffix = QString::number((*seq)%1000);
    suffix = suffix.rightJustified(3,'0',true);
    *currentFile = QFileInfo(currentFile->absoluteDir().absolutePath()+"/"+fileName+"."+suffix);
}

/********
int RTReadingThread::getHeader(QDir targetDir)
{
    QFile tmpFile(targetDir.absolutePath()+"/"+fileName+".scn");
    qWarning()<<tmpFile.fileName();
    if (!tmpFile.open(QIODevice::ReadOnly))
        return 0;
    QTextStream inputStream(&tmpFile);
    bool stopFlag = false;
    QString header;
    while (!stopFlag)
    {
            QString line = inputStream.readLine();
            header.append(line);
            header.append("\r\n");
            if (line.split(">").first()=="<Signal Recorded")
            {
                line = line.remove("<Signal Recorded>");
                line = line.remove(QChar(' '));
                int index=0;
                index = line.split(",").at(0).toInt()-1;
                qWarning()<<"index: "<<index;
                attr.signal[index]= line.split(",").at(1);
                QString tmp;
                tmp = line.split(",").at(2);
                attr.bw[index]=tmp.remove(QRegExp("[A-z]")).toInt();
                tmp = line.split(",").at(3);
                attr.freq[index]=tmp.remove(QRegExp("[A-z]")).toFloat();
                tmp = line.split(",").at(4);
                attr.numBits[index]=tmp.remove(QRegExp("[A-z]")).toInt();
            }
            else if (line.split(">").first()=="<Local Start Time")
            {
                stopFlag = true;
            }
    }
    tmpFile.close();
    skipWordCountA[0]=attr.bw[0]*1023000*(2*attr.numBits[0])/32/5;
    skipWordCountA[1]=skipWordCountA[0];
    readWordCountA[0]=32768*(2*attr.numBits[0])/32;
    readWordCountA[1]=readWordCountA[0];
    if ((!attr.signal[2].isEmpty()))
    {
        skipWordCountB[0]=attr.bw[2]*1023000*(2*attr.numBits[2])/32/5;
        skipWordCountB[1]=skipWordCountB[0];
        readWordCountB[0]=32768*(2*attr.numBits[2])/32;
        readWordCountB[1]=readWordCountB[0];
    }
    emit dispAttr(attr);
    emit dispHeader(header);
    return 1;
}
*******/

int RTReadingThread::getHeader()
{
    if (strategy!=ST_REPLAY)
        pollingProc->start("/home/spirent/Projects/App/shm_rec_channels_get",QStringList()<<"-a");
    else
        pollingProc->start("/home/spirent/Projects/App/shm_play_channels_get",QStringList()<<"-a");
    if (!pollingProc->waitForFinished())
        return 0;
    QString mode = pollingProc->readAll();
    QStringList lines = mode.split("\r\n");
    lines.removeLast();
    attr.isSetupMode = isSetupMode();
    for (int i=0;i<lines.length();i++)
    {
        qWarning()<<i;
        int sig=0;
        QStringList args=lines.at(i).split(" ");
        for (int j=0;j<args.length();j++)
        {
            QString str = args.at(j);
            if (str.startsWith("-c"))
            {
                sig=str.mid(2).toInt()-1;
                qWarning()<<"sig: "<<sig;
            }
            else if (str.startsWith("-N"))
                attr.signal[sig]=str.mid(2);
            else if (str.startsWith("-S"))
                attr.bw[sig]=str.mid(2).toInt();
            else if (str.startsWith("-b"))
            {
                if (attr.isSetupMode) attr.numBits[sig]=16;
                else attr.numBits[sig]=str.mid(2).toInt();
            }
            else if (str.startsWith("-M"))
                attr.freq[sig]=str.mid(2).toFloat();
        }
    }

    skipWordCountA[0]=attr.bw[0]*1023000*(2*attr.numBits[0])/32/5;
    skipWordCountA[1]=skipWordCountA[0];
    readWordCountA[0]=32768*(2*attr.numBits[0])/32;
    readWordCountA[1]=readWordCountA[0];
    if ((!attr.signal[2].isEmpty()))
    {
        skipWordCountB[0]=attr.bw[2]*1023000*(2*attr.numBits[2])/32/5;
        skipWordCountB[1]=skipWordCountB[0];
        readWordCountB[0]=32768*(2*attr.numBits[2])/32;
        readWordCountB[1]=readWordCountB[0];
    }
    emit dispAttr(attr);
    emit dispHeader(mode);
    return 1;
}

void RTReadingThread::getData(QFileInfo targetFile,int *syncMode,int *skipWordCount,int *readWordCount,
                              int numBits,FFTSampleBuffer *ptr2Buffer,bool readSel,bool chanSel,int *currentChan)
{
    QFile tmpFile(targetFile.absoluteFilePath());
    QDataStream inputStream(&tmpFile);
    quint32 bufferWord[3];
    //qWarning()<<tmpFile.fileName();
    if (!tmpFile.open(QIODevice::ReadOnly))
        return;
    bool endOfFile=false;
    bool is12Bits=(numBits==12);
    if (!readSel)
    {
        ptr2Buffer->clear();
        ptr2Buffer->reserve(BUFFER_SIZE);
    }
    while ((!endOfFile) && isAlive)
    {
        if (readSel&&(skipWordCount[0]==skipWordCount[1]))
        {
            while (readWordCount[0])
            {
                //if (!inputStream.readRawData((char *)&bufferWord,4))
                if (is12Bits)
                {
                    if (!readWords12(&inputStream,*syncMode,chanSel,bufferWord,currentChan))
                    {
                        endOfFile=true;
                        tmpFile.close();
                        return;
                    }
                }
                else
                {
                    if (!readWords(&inputStream,*syncMode,chanSel,bufferWord,currentChan))
                    {
                        endOfFile=true;
                        tmpFile.close();
                        return;
                    }
                }
                reorderWord(bufferWord);
                if (is12Bits)
                {
                    reorderWord(bufferWord+1);
                    reorderWord(bufferWord+2);
                }
                int samplePerWord;
                samplePerWord=word2sample(bufferWord,numBits,sampleIFromWord,sampleQFromWord);
                for (int i=0;i<samplePerWord;i++)
                {
                    currentFFTSamples.valuesI[32768-(readWordCount[0])*32/(2*numBits)+i]=sampleIFromWord[i];
                    currentFFTSamples.valuesQ[32768-(readWordCount[0])*32/(2*numBits)+i]=sampleQFromWord[i];
                    //qWarning()<<32768-(readWordCount[0])*32/numBits+i;
                }
                currentFFTSamples.chan=(int)fpgaSel*2+(int)chanSel+1;
                //qWarning()<<(int)readSel*2+(int)chanSel+1;
                //qWarning()<<readWordCount[0]<<skipWordCount[0];
                (readWordCount[0])--;
                (skipWordCount[0])--;
            }
            mutex.lock();
            if (ptr2Buffer->length()==BUFFER_SIZE)
                ptr2Buffer->removeFirst();
            ptr2Buffer->append(currentFFTSamples);
            //qWarning()<<"new appended";
            mutex.unlock();
        }
        //skipWordCount[0]=skipWordCount[0]-inputStream.skipRawData(4*skipWordCount[0])/4;
        if (is12Bits)
            skipWordCount[0]=skipWordCount[0]-skipWords12(&inputStream,*syncMode,skipWordCount,currentChan)/4;
        else
            skipWordCount[0]=skipWordCount[0]-skipWords(&inputStream,*syncMode,skipWordCount,currentChan)/4;
        if (skipWordCount[0]==0)
        {
            skipWordCount[0]=skipWordCount[1];
            readWordCount[0]=readWordCount[1];
            endOfFile=false;
        }
        else
        {
            endOfFile=true;
        }
    }
    tmpFile.close();
    tmpFile.remove();
    return;
}

int RTReadingThread::getInitialData(QFileInfo targetFile,int *syncMode,int *skipWordCount,int *readWordCount,
                                    int numBits,FFTSampleBuffer *ptr2Buffer,bool readSel,bool chanSel,int *currentChan)
{
    QFile tmpFile(targetFile.absoluteFilePath());
    qWarning()<<tmpFile.fileName();
    QDataStream inputStream(&tmpFile);
    quint32 bufferWord[3];
    quint32 syncWord[2];
    if (!tmpFile.open(QIODevice::ReadOnly))
        return 0;
    while ((bufferWord[0]!=0x11111111)&&(bufferWord[0]!=0x22222222))
        inputStream.readRawData((char *)bufferWord,4);
    syncWord[0]=bufferWord[0];
    //qWarning()<<syncWord[0];
    inputStream.readRawData((char *)bufferWord,4);
    syncWord[1]=bufferWord[0];
    //qWarning()<<syncWord[1];
    if ((syncWord[1]==0x11111111)&&(syncWord[0]==0x11111111))
        *syncMode=0;
    else if ((syncWord[0]==0x11111111)&&(syncWord[1]==0x22222222))
        *syncMode=1;
    else if ((syncWord[1]==0x11111111)&&(syncWord[0]==0x22222222))
        *syncMode=2;
    else
        *syncMode=-1;
    while ((bufferWord[0]==0x11111111)|(bufferWord[0]==0x22222222))
        inputStream.readRawData((char *)&bufferWord,4);
    qWarning()<<"Sync Mode: "<<syncModeA<<syncModeB;

    for (int i=0;i<5;i++)
        inputStream.readRawData((char *)&bufferWord,4);

    (*currentChan)=0;

    bool endOfFile=false;
    bool is12Bits=(numBits==12);
    if (!readSel)
    {
        ptr2Buffer->clear();
        ptr2Buffer->reserve(BUFFER_SIZE);
    }
    while (!endOfFile)
    {
        if (readSel&&(skipWordCount[0]==skipWordCount[1]))
        {
            while (readWordCount[0])
            {
                //if (!inputStream.readRawData((char *)&bufferWord,4))
                if (is12Bits)
                {
                    if (!readWords12(&inputStream,*syncMode,chanSel,bufferWord,currentChan))
                    {
                        endOfFile=true;
                        tmpFile.close();
                        return 1;
                    }
                }
                else
                {
                    if (!readWords(&inputStream,*syncMode,chanSel,bufferWord,currentChan))
                    {
                        endOfFile=true;
                        tmpFile.close();
                        return 1;
                    }
                }
                reorderWord(bufferWord);
                if (is12Bits)
                {
                    reorderWord(bufferWord+1);
                    reorderWord(bufferWord+2);
                }
                int samplePerWord;
                samplePerWord=word2sample(bufferWord,numBits,sampleIFromWord,sampleQFromWord);
                for (int i=0;i<samplePerWord;i++)
                {
                    currentFFTSamples.valuesI[32768-(readWordCount[0])*32/(2*numBits)+i]=sampleIFromWord[i];
                    currentFFTSamples.valuesQ[32768-(readWordCount[0])*32/(2*numBits)+i]=sampleQFromWord[i];
                    //qWarning()<<32768-(readWordCount[0])*32/(2*numBits)+i;
                }
                currentFFTSamples.chan=(int)fpgaSel*2+(int)channelSel+1;
                //qWarning()<<(int)readSel*2+(int)chanSel+1;
                //qWarning()<<readWordCount[0]<<skipWordCount[0];
                (readWordCount[0])--;
                (skipWordCount[0])--;
            }
            mutex.lock();
            if (ptr2Buffer->length()==BUFFER_SIZE)
                ptr2Buffer->removeFirst();
            ptr2Buffer->append(currentFFTSamples);
            //qWarning()<<"new appeneded";
            mutex.unlock();
        }
        //skipWordCount[0]=skipWordCount[0]-inputStream.skipRawData(4*skipWordCount[0])/4;
        skipWordCount[0]=skipWordCount[0]-skipWords(&inputStream,*syncMode,skipWordCount,currentChan)/4;
        if (skipWordCount[0]==0)
        {
            skipWordCount[0]=skipWordCount[1];
            readWordCount[0]=readWordCount[1];
            endOfFile=false;
        }
        else
        {
            endOfFile=true;
            //qWarning()<<"end of file: @"<<skipWordCount[0]<<" of "<<skipWordCount[1];
        }
    }
    tmpFile.close();
    tmpFile.remove();
    return 1;
}

int RTReadingThread::getGNSData(QFile* currentFilePtr_A,QDataStream* inputStreamPtr_A, int *syncMode_A, int *skipWordCount_A, int *readWordCount_A,
                                int numBits_A, FFTSampleBuffer *ptr2Buffer_A,int *currentChan_A,bool* isFirst_A,
                                QFile* currentFilePtr_B,QDataStream* inputStreamPtr_B, int *syncMode_B, int *skipWordCount_B, int *readWordCount_B,
                                int numBits_B, FFTSampleBuffer *ptr2Buffer_B, int *currentChan_B,bool* isFirst_B,bool hasB )
{
    quint32 bufferWord_A[3];
    quint32 syncWord_A[2];

    quint32 bufferWord_B[3];
    quint32 syncWord_B[2];

    bool endOfFile_A=false;
    bool is12Bits_A=(numBits_B==12);
    bool endOfFile_B=false;
    bool is12Bits_B=(numBits_B==12);

    if (*isFirst_A)
    {
        while ((bufferWord_A[0]!=0x11111111)&&(bufferWord_A[0]!=0x22222222))
        {
            if (inputStreamPtr_A->readRawData((char *)bufferWord_A,4)<4)
            {
                usleep(1000*1000);
                qWarning()<<"no data in A";
                return 1;
            }
        }
        syncWord_A[0]=bufferWord_A[0];
        inputStreamPtr_A->readRawData((char *)bufferWord_A,4);
        syncWord_A[1]=bufferWord_A[0];
        if ((syncWord_A[1]==0x11111111)&&(syncWord_A[0]==0x11111111))
            *syncMode_A=0;
        else if ((syncWord_A[0]==0x11111111)&&(syncWord_A[1]==0x22222222))
            *syncMode_A=1;
        else if ((syncWord_A[1]==0x11111111)&&(syncWord_A[0]==0x22222222))
            *syncMode_A=2;
        else
            *syncMode_A=-1;
        while ((bufferWord_A[0]==0x11111111)|(bufferWord_A[0]==0x22222222))
            inputStreamPtr_A->readRawData((char *)&bufferWord_A,4);
        qWarning()<<"Sync ModeA : "<<syncModeA<<syncModeB;

        for (int i=0;i<5;i++)
            inputStreamPtr_A->readRawData((char *)&bufferWord_A,4);
        (*currentChan_A)=0;
    }

    *isFirst_A=false;

    if (fpgaSel!=0)
    {
        ptr2Buffer_A->clear();
        ptr2Buffer_A->reserve(BUFFER_SIZE);
    }

    if (*isFirst_B && hasB)
    {
        while ((bufferWord_B[0]!=0x11111111)&&(bufferWord_B[0]!=0x22222222))
        {
            if (inputStreamPtr_B->readRawData((char *)bufferWord_B,4)<4)
            {
                usleep(1000*1000);
                qWarning()<<"no data in B";
                return 1;
            }
        }
        syncWord_B[0]=bufferWord_B[0];
        inputStreamPtr_B->readRawData((char *)bufferWord_B,4);
        syncWord_B[1]=bufferWord_B[0];
        if ((syncWord_B[1]==0x11111111)&&(syncWord_B[0]==0x11111111))
            *syncMode_B=0;
        else if ((syncWord_B[0]==0x11111111)&&(syncWord_B[1]==0x22222222))
            *syncMode_B=1;
        else if ((syncWord_B[1]==0x11111111)&&(syncWord_B[0]==0x22222222))
            *syncMode_B=2;
        else
            *syncMode_B=-1;
        while ((bufferWord_B[0]==0x11111111)|(bufferWord_B[0]==0x22222222))
            inputStreamPtr_B->readRawData((char *)&bufferWord_B,4);
        qWarning()<<"Sync Mode: "<<syncModeA<<syncModeB;

        for (int i=0;i<5;i++)
            inputStreamPtr_B->readRawData((char *)&bufferWord_B,4);
        (*currentChan_B)=0;
    }

    *isFirst_B=false;

    if (fpgaSel!=1)
    {
        ptr2Buffer_B->clear();
        ptr2Buffer_B->reserve(BUFFER_SIZE);
    }

    while ((!endOfFile_A)|((!endOfFile_B) && (hasB)))
    {
        if (!isAlive) break;
        if ((isStopped()) && (strategy!=ST_RECORD)) break;
        if (!(endOfFile_A))
        {
            if ((fpgaSel==0)&&(skipWordCount_A[0]==skipWordCount_A[1]))
            {
                int iChannelSel=channelSel;
                while (readWordCount_A[0])
                {
                    if (is12Bits_A)
                    {
                        if (!readWords12(inputStreamPtr_A,*syncMode_A,iChannelSel,bufferWord_A,currentChan_A))
                        {
                            endOfFile_A=true;
                            return 1;
                        }
                    }
                    else
                    {
                        if (!readWords(inputStreamPtr_A,*syncMode_A,iChannelSel,bufferWord_A,currentChan_A))
                        {
                            endOfFile_A=true;
                            return 1;
                        }
                    }
                    reorderWord(bufferWord_A);
                    if (is12Bits_A)
                    {
                        reorderWord(bufferWord_A+1);
                        reorderWord(bufferWord_A+2);
                    }
                    int samplePerWord_A;
                    samplePerWord_A=word2sample(bufferWord_A,numBits_A,sampleIFromWord,sampleQFromWord);
                    for (int i=0;i<samplePerWord_A;i++)
                    {
                        currentFFTSamples.valuesI[32768-(readWordCount_A[0])*32/(2*numBits_A)+i]=sampleIFromWord[i];
                        currentFFTSamples.valuesQ[32768-(readWordCount_A[0])*32/(2*numBits_A)+i]=sampleQFromWord[i];
                    }
                    currentFFTSamples.chan=(int)iChannelSel+1;
                    (readWordCount_A[0])--;
                    (skipWordCount_A[0])--;
                }

                while ((ptr2Buffer_A->length()==BUFFER_SIZE)&&(strategy==ST_REPLAY))
                    msleep(200);
                mutex.lock();
                if ((ptr2Buffer_A->length()==BUFFER_SIZE)&&(strategy==ST_RECORD))
                {
                    ptr2Buffer_A->removeFirst();
                    emit overflow();
                    qWarning()<<"A: overflow";
                }
                ptr2Buffer_A->append(currentFFTSamples);
                //qWarning()<<"new appeneded";
                mutex.unlock();
            }

            skipWordCount_A[0]=skipWordCount_A[0]-skipWords(inputStreamPtr_A,*syncMode_A,skipWordCount_A,currentChan_A)/4;
            if (skipWordCount_A[0]==0)
            {
                skipWordCount_A[0]=skipWordCount_A[1];
                readWordCount_A[0]=readWordCount_A[1];
                endOfFile_A=false;
                qWarning()<<"end of fileA: @"<<skipWordCount_A[0]<<" of "<<skipWordCount_A[1];
            }
            else
            {
                endOfFile_A=true;
                usleep(1000*10);
            }

        }
        /**********for .b.gns file********/
        if ((!endOfFile_B) && (hasB))
        {
            if ((fpgaSel==1)&&(skipWordCount_B[0]==skipWordCount_B[1]))
            {
                int iChannelSel=channelSel;
                while (readWordCount_B[0])
                {
                    if (is12Bits_B)
                    {
                        if (!readWords12(inputStreamPtr_B,*syncMode_B,iChannelSel,bufferWord_B,currentChan_B))
                        {
                            endOfFile_B=true;
                            return 1;
                        }
                    }
                    else
                    {
                        if (!readWords(inputStreamPtr_B,*syncMode_B,iChannelSel,bufferWord_B,currentChan_B))
                        {
                            endOfFile_B=true;
                            return 1;
                        }
                    }
                    reorderWord(bufferWord_B);
                    if (is12Bits_B)
                    {
                        reorderWord(bufferWord_B+1);
                        reorderWord(bufferWord_B+2);
                    }
                    int samplePerWord_B;
                    samplePerWord_B=word2sample(bufferWord_B,numBits_B,sampleIFromWord,sampleQFromWord);
                    for (int i=0;i<samplePerWord_B;i++)
                    {
                        currentFFTSamples.valuesI[32768-(readWordCount_B[0])*32/(2*numBits_B)+i]=sampleIFromWord[i];
                        currentFFTSamples.valuesQ[32768-(readWordCount_B[0])*32/(2*numBits_B)+i]=sampleQFromWord[i];
                    }
                    currentFFTSamples.chan=2+(int)iChannelSel+1;
                    (readWordCount_B[0])--;
                    (skipWordCount_B[0])--;
                }

                while ((ptr2Buffer_B->length()==BUFFER_SIZE)&&(strategy==ST_REPLAY))
                    msleep(200);
                mutex.lock();
                if ((ptr2Buffer_B->length()==BUFFER_SIZE)&&(strategy==ST_RECORD))
                    ptr2Buffer_B->removeFirst();
                ptr2Buffer_B->append(currentFFTSamples);
                //qWarning()<<"new appeneded";
                mutex.unlock();
            }
            skipWordCount_B[0]=skipWordCount_B[0]-skipWords(inputStreamPtr_B,*syncMode_B,skipWordCount_B,currentChan_B)/4;
            if (skipWordCount_B[0]==0)
            {
                skipWordCount_B[0]=skipWordCount_B[1];
                readWordCount_B[0]=readWordCount_B[1];
                endOfFile_B=false;
                //qWarning()<<"end of fileB: @"<<skipWordCount_B[0]<<" of "<<skipWordCount_B[1];
            }
            else
            {
                endOfFile_B=true;
                usleep(1000*10);
            }
        }
    }
    return 1;
}

void RTReadingThread::setRamDiskPath(QString path)
{
    ramDiskPathA=QDir(path+"/FPGA_A");
    ramDiskPathB=QDir(path+"/FPGA_B");
}

void RTReadingThread::setFPGASel(int sel)
{
    fpgaSel=sel;//0-FPGAA 1-FPGAB
}

void RTReadingThread::setGNSFilePath(QString path)
{
    gnsFile=QDir(path);
}

void RTReadingThread::setChannel(int chan)
{
    channelSel=chan%2;
}

void RTReadingThread::resetSync()
{
    syncModeA=-1;
    syncModeB=-1;
}

void RTReadingThread::reorderWord(quint32 *word)
{
    unsigned char tmp[4];
    unsigned char reg;
    //(*(quint32 *)tmp)=(*word);
    memcpy(tmp, word, sizeof(*word));
    //    reg = tmp[0];
    //    tmp[0]=tmp[3];
    //    tmp[3]=reg;
    //    reg = tmp[1];
    //    tmp[1]=tmp[2];
    //    tmp[2]=reg;
    //    //(*word)=(*(quint32 *)tmp);
    //    memcpy(word, tmp, sizeof(*word));
}

int RTReadingThread::word2sample(quint32 *word, int numBits, qint32 *outI,qint32 *outQ)
{
    int numSample;
    int mask = 0xFFFFFFFF;
    //qWarning()<<"word: "<<QString::number(*word,2).rightJustified(32,'0',true);
    if (numBits==12)
    {
        quint32 tmp;
        numSample= 4;

        tmp=(*word)>>20;
        outI[0]=tmp&(~(mask<<12));
        if (tmp>>11)
            outI[0]=outI[0]|(mask<<12);
        tmp=(*word)>>8;
        outQ[0]=tmp&(~(mask<<12));
        if (tmp>>11)
            outQ[0]=outQ[0]|(mask<<12);
        //
        tmp=((*word)<<4)&((*(word+1))>>28);
        outI[1]=tmp&(~(mask<<12));
        if (tmp>>11)
            outI[1]=outI[1]|(mask<<12);
        tmp=(*(word+1))>>16;
        outQ[1]=tmp&(~(mask<<12));
        if (tmp>>11)
            outQ[1]=outQ[1]|(mask<<12);
        //
        tmp=(*(word+1))>>4;
        outI[2]=tmp&(~(mask<<12));
        if (tmp>>11)
            outI[2]=outI[2]|(mask<<12);
        tmp=((*(word+1))<<8)&((*(word+2))>>24);
        outQ[2]=tmp&(~(mask<<12));
        if (tmp>>11)
            outQ[2]=outQ[2]|(mask<<12);
        //
        tmp=(*(word+2))>>12;
        outI[3]=tmp&(~(mask<<12));
        if (tmp>>11)
            outI[3]=outI[3]|(mask<<12);
        tmp=*(word+1);
        outQ[3]=tmp&(~(mask<<12));
        if (tmp>>11)
            outQ[3]=outQ[3]|(mask<<12);
        //qWarning()<<"I: "<<QString::number(outI[i],2).rightJustified(numBits,'0',true);
        //qWarning()<<QString::number(outQ[i],2).rightJustified(numBits,'0',true);
    }
    else
    {
        numSample=16/numBits;
        int mask = 0xFFFFFFFF;
        //qWarning()<<"word: "<<QString::number(*word,2).rightJustified(32,'0',true);
        for (int i=0;i<numSample;i++)
        {
            outI[i]=((*word)>>(32-2*numBits*(i+1)))&(~(mask<<(numBits)));
            if ((((*word)>>(32-2*numBits*(i+1)))&(~(mask<<(numBits))))>>(numBits-1))
                outI[i]=outI[i]|(mask<<numBits);
            outQ[i]=((*word)>>(32-2*numBits*i-numBits))&(~(mask<<(numBits)));
            if ((((*word)>>(32-2*numBits*i-numBits))&(~(mask<<(numBits))))>>(numBits-1))
            {
                //qWarning()<<"I: "<<QString::number(outI[i],2).rightJustified(numBits,'0',true);
                outQ[i]=outQ[i]|(mask<<numBits);
                //qWarning()<<outI[i];
            }
            //qWarning()<<"I: "<<QString::number(outI[i],2).rightJustified(numBits,'0',true);
            //qWarning()<<QString::number(outQ[i],2).rightJustified(numBits,'0',true);
        }
    }
    return numSample;
}

void RTReadingThread::onTimeOut()
{
    if (fftBufferA.length()>PRELOAD_SIZE)             startFlag = true;
    if (!startFlag) return;
    if (!fpgaSel)
    {
        mutex.lock();
        if (fftBufferA.length()>0)
        {
            emit sendFFTSamples(fftBufferA.first(),attr.numBits[0],strategy==ST_SETUP);
            //saveToTxt(fftBufferA.first(),attr.numBits[0]);
            fftBufferA.removeFirst();
            fftBufferA.reserve(BUFFER_SIZE);
            //qWarning()<<"sample sent A";
        }
        else
        {
            qWarning()<<"A: underflow";
        }
        mutex.unlock();
    }
    else
    {
        mutex.lock();
        if (fftBufferB.length()>0)
        {
            emit sendFFTSamples(fftBufferB.first(),attr.numBits[2],strategy==ST_SETUP);
            //saveToTxt(fftBufferA.first(),attr.numBits[0]);
            fftBufferB.removeFirst();
            fftBufferB.reserve(BUFFER_SIZE);
            //qWarning()<<"sample sent B";
        }
        mutex.unlock();
    }
}

int RTReadingThread::saveToTxt(FFTSamples samples, int numOfBits)
{
    //    QFile samplePool("/tmp/IFsample.txt");
    //    if (!samplePool.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    //             return 0;
    //    QTextStream writeStream(&samplePool);
    //    writeStream.setCodec("UTF-16");
    //    writeStream.setGenerateByteOrderMark(true);
    //    writeStream<<"_C"<<QString::number(samples.chan);
    //    writeStream<<"_B"<<QString::number(numOfBits);
    //    writeStream<<"\r\n";
    //    writeStream<<"_V";
    //    for (int i=0;i<32768;i++)
    //    {
    //        if ((samples.valuesI[i]+32768)<0x0021)
    //            writeStream<<QChar(samples.valuesI[i]+32768+67328);
    //        else if ((samples.valuesI[i]+32768)>0xFFFD)
    //            writeStream<<QChar(samples.valuesI[i]+32768+2);
    //        else if (((samples.valuesI[i]+32768)>=0xFDD0) && (((samples.valuesI[i]+32768)<=0xFDEF) ))
    //            writeStream<<QChar(samples.valuesI[i]+32768+12288);
    //        else if (((samples.valuesI[i]+32768)>0xDFFF) | (((samples.valuesI[i]+32768)<0xD800) ))
    //            writeStream<<QChar(samples.valuesI[i]+32768);
    //        else
    //            writeStream<<QChar(samples.valuesI[i]+32768+12288);

    //        if ((samples.valuesQ[i]+32768)<0x0021)
    //            writeStream<<QChar(samples.valuesQ[i]+32768+67328);
    //        else if ((samples.valuesQ[i]+32768)>0xFFFD)
    //            writeStream<<QChar(samples.valuesQ[i]+32768+2);
    //        else if (((samples.valuesQ[i]+32768)>=0xFDD0) && (((samples.valuesQ[i]+32768)<=0xFDEF) ))
    //            writeStream<<QChar(samples.valuesQ[i]+32768+12288);
    //        else if (((samples.valuesQ[i]+32768)>0xDFFF) | (((samples.valuesQ[i]+32768)<0xD800) ))
    //            writeStream<<QChar(samples.valuesQ[i]+32768);
    //        else
    //            writeStream<<QChar(samples.valuesQ[i]+32768+12288);
    //    }
    //    samplePool.close();
    //    return 1;
}

int RTReadingThread::readWords(QDataStream * inputStream,int syncMode,bool chanSel,quint32 *bufferWord,int *currentChan)
{
    if (syncMode==0)
    {
        return (inputStream->readRawData((char *)bufferWord,4));
    }
    else if ((syncMode+(int)chanSel)==2)
    {
        if (*currentChan==0)
        {
            if (!inputStream->skipRawData(4))
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
            if (!inputStream->readRawData((char *)bufferWord,4))
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
        }
        else if (*currentChan==1)
        {
            if (!inputStream->skipRawData(4))
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
            if (!inputStream->readRawData((char *)bufferWord,4))
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
        }
    }
    else if (((syncMode+(int)chanSel)==1)|((syncMode+(int)chanSel)==3))
    {
        if (*currentChan==1)
        {
            if (!inputStream->skipRawData(4))
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
            if (!inputStream->readRawData((char *)bufferWord,4))
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
        }
        else if (*currentChan==0)
        {
            if (!inputStream->readRawData((char *)bufferWord,4))
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
            if (!inputStream->skipRawData(4))
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
        }
    }
    return 1;
}

int RTReadingThread::skipWords(QDataStream * inputStream,int syncMode,int *skipWordCount,int *currentChan)
{
    int skipped;
    if (syncMode==0)
    {
        return inputStream->skipRawData(4*skipWordCount[0]);
    }
    else if (*currentChan==0)
    {
        skipped = inputStream->skipRawData(8*skipWordCount[0]);
        if (skipped/4%2==0)
        {
            *currentChan=0;
            return skipped/8*4;
        }
        else
        {
            *currentChan=1;
            return skipped/8*4;
        }
    }
    else if (*currentChan==1)
    {
        skipped = inputStream->skipRawData(8*skipWordCount[0]);
        if (skipped/4%2==0)
        {
            *currentChan=1;
            return skipped/8*4;
        }
        else
        {
            *currentChan=0;
            return (skipped+1)/8*4;
        }
    }
    return (0);
}

int RTReadingThread::readWords12(QDataStream *inputStream, int syncMode, bool chanSel, quint32 *bufferWord, int *currentChan)
{
    if (syncMode==0)
    {
        return read3Words(inputStream,bufferWord);
    }
    else if ((syncMode+(int)chanSel)==2)
    {
        if (*currentChan==0)
        {
            if (skip3Words(inputStream)<3)
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
            if (!read3Words(inputStream,bufferWord))
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
        }
        else if (*currentChan==1)
        {
            if (skip3Words(inputStream)<3)
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
            if (!read3Words(inputStream,bufferWord))
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
        }
    }
    else if (((syncMode+(int)chanSel)==1)|((syncMode+(int)chanSel)==3))
    {
        if (*currentChan==1)
        {
            if (skip3Words(inputStream)<3)
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
            if (!read3Words(inputStream,bufferWord))
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
        }
        else if (*currentChan==0)
        {
            if (!read3Words(inputStream,bufferWord))
            {
                //*currentChan=0;
                return (0);
            }
            else
                *currentChan=1;
            if (skip3Words(inputStream)<3)
            {
                //*currentChan=1;
                return (0);
            }
            else
                *currentChan=0;
        }
    }
    return 1;
}

int RTReadingThread::skipWords12(QDataStream *inputStream, int syncMode, int *skipWordCount, int *currentChan)
{
    int skipped;
    if (syncMode==0)
    {
        return skip3Words(inputStream,skipWordCount[0])*4;
    }
    else if (*currentChan==0)
    {
        skipped = skip3Words(inputStream,skipWordCount[0]);
        if (skipped%2==0)
        {
            *currentChan=0;
            return skipped/2*4;
        }
        else
        {
            *currentChan=1;
            return skipped/2*4;
        }
    }
    else if (*currentChan==1)
    {
        int skipped;
        skipped = skip3Words(inputStream,skipWordCount[0]);
        if (skipped/12%2==0)
        {
            *currentChan=1;
            return skipped/2*4;
        }
        else
        {
            *currentChan=0;
            return (skipped+1)/2*4;
        }
    }
    return (0);
}

int RTReadingThread::read3Words(QDataStream *inputStream, quint32 *bufferWord)
{
    if  (!inputStream->readRawData((char *)bufferWord,4))
        return (0);
    else
        subWordRead(1);
    if  (!inputStream->readRawData((char *)(bufferWord+1),4))
        return (0);
    else
        subWordRead(1);
    if  (!inputStream->readRawData((char *)(bufferWord+2),4))
        return (0);
    else
        subWordRead(1);
    return 1;
}

int RTReadingThread::skip3Words(QDataStream *inputStream,int skipLen)
{
    int skipped=0;
    if (currentSubWord)
        skipped=(int)(inputStream->skipRawData(4*(3-currentSubWord))>0);//this should alway succeed
    skipped = skipped + inputStream->skipRawData(3*4*skipLen)/4;
    subWordRead(skipped%3);
    return skipped;//this is how many THREE-WORD blocks are skipped
}

void RTReadingThread::subWordRead(int times)
{
    for (int i=0;i<times;i++)
    {
        if (currentSubWord==2)
            currentSubWord=0;
        else
            currentSubWord++;
    }
}

int RTReadingThread::isRecording()
{
    pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-m");
    if (!pollingProc->waitForFinished())
        return 0;
    QString mode = pollingProc->readAll();
    mode = mode.split(" ").at(1);
    if (mode==REVSTR)
        return 1;
    else
        return 0;
}

int RTReadingThread::isStopped()
{
    pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-m");
    if (!pollingProc->waitForFinished())
        return 0;
    QString mode = pollingProc->readAll();
    mode = mode.split(" ").at(1);
    if (mode==STPSTR)
        return 1;
    else
        return 0;
}

int RTReadingThread::isReplaying()
{
    pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-m");
    if (!pollingProc->waitForFinished())
        return 0;
    QString mode = pollingProc->readAll();
    mode = mode.split(" ").at(1);
    if (mode==RPLSTR)
        return 1;
    else
        return 0;
}

int RTReadingThread::isSetupMode()
{
    pollingProc->start("/home/spirent/Projects/App/shm_get",QStringList()<<"-K");
    if (!pollingProc->waitForFinished())
        return -1;
    QString mode = pollingProc->readAll();
    mode = mode.split(" ").at(1).at(0);
    if (mode=="1")
        return 1;
    else
        return 0;
}

void RTReadingThread::clearRamDisk()
{
    removeDir(ramDiskPathA.absolutePath());
    removeDir(ramDiskPathB.absolutePath());
}

bool RTReadingThread::removeDir(const QString & dirName)
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

void RTReadingThread::clearBuffer()
{
    mutex.lock();
    if (fpgaSel==0)
        emit overflow(fftBufferA.length());
    else
        emit overflow(fftBufferB.length());
    fftBufferA.clear();
    fftBufferA.reserve(BUFFER_SIZE);
    fftBufferB.clear();
    fftBufferB.reserve(BUFFER_SIZE);
    mutex.unlock();
}
