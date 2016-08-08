#ifndef RTREADINGTHREAD_H
#define RTREADINGTHREAD_H

#include <QObject>
#include <QThread>
#include <QDir>
#include <QVector>
#include <QMetaType>
#include <QMutex>
#include <QTimer>
#include <QProcess>

typedef struct
{
    QVector <QString> signal;
    int bw[4];
    float freq[4];
    int numBits[4];
    bool isSetupMode;
}Attributes;

typedef struct {qint32 valuesI[32768];qint32 valuesQ[32768];int chan;}FFTSamples;
typedef QList <FFTSamples> FFTSampleBuffer;
typedef enum {ST_SETUP, ST_RECORD, ST_REPLAY,ST_NULL} Strategy;

class RTReadingThread : public QThread
{
    Q_OBJECT
public:
    explicit RTReadingThread(QObject *parent);
    ~RTReadingThread();
    void run();
    int fileSeqA,fileSeqB;
    QString fileName;
    Attributes attr;
    void setRamDiskPath(QString path);
    void setGNSFilePath(QString path);
    void setChannel(int chan);
    void setFPGASel(int sel);
    void resetSync();
    bool isAlive;
    Strategy strategy;
private:
    QFileInfo currentFileA,currentFileB,endFileA,endFileB;
    void toNextFile(QFileInfo *currentFile,int *seq);
    int getHeader();
    void getData(QFileInfo targetFile,int *syncMode,int *skipWordCount,int *readWordCount,
                 int numBits,FFTSampleBuffer *ptr2Buffer,bool readSel,bool channelSel,int *currentChan);
    int getInitialData(QFileInfo targetFile,int *syncMode,int *skipWordCount,int *readWordCount,
                       int numBits,FFTSampleBuffer *ptr2Buffer,bool readSel,bool channelSel,int *currentChan);
    int getGNSData(QFile* currentFilePtr_A,QDataStream* inputStreamPtr_A, int *syncMode_A, int *skipWordCount_A, int *readWordCount_A,
                   int numBits_A, FFTSampleBuffer *ptr2Buffer_A, int *currentChan_A,bool* isFirst_A,
                  QFile* currentFilePtr_B,QDataStream* inputStreamPtr_B, int *syncMode_B, int *skipWordCount_B, int *readWordCount_B,
                  int numBits_B, FFTSampleBuffer *ptr2Buffer_B, int *currentChan_B,bool* isFirst_B,bool hasB);
    QDir ramDiskPathA,ramDiskPathB;
    int fpgaSel;
    QDir gnsFile;
    int channelSel;
    int syncModeA,syncModeB;
    int skipWordCountA[2],skipWordCountB[2];
    int readWordCountA[2],readWordCountB[2];
    FFTSampleBuffer fftBufferA,fftBufferB;
    FFTSamples currentFFTSamples;
    void reorderWord(quint32 *word);
    int word2sample(quint32 *word,int numBits,qint32 *outI,qint32 *outQ);
    qint32 sampleIFromWord[8],sampleQFromWord[8];
    QMutex mutex;
    QTimer *timer;
    //int readOneWord(int syncMode);
    int currentChanA,currentChanB;
    int readWords(QDataStream * inputStream,int syncMode,bool chanSel,quint32 *bufferWord,int *currentChan);
    int skipWords(QDataStream * inputStream,int syncMode,int *skipWordCount,int *currentChan);
    int readWords12(QDataStream * inputStream,int syncMode,bool chanSel,quint32 *bufferWord,int *currentChan);
    int skipWords12(QDataStream * inputStream,int syncMode,int *skipWordCount,int *currentChan);
    int read3Words(QDataStream * inputStream,quint32 *bufferWord);
    int skip3Words(QDataStream * inputStream,int skipLen=1);
    void subWordRead(int times);
    int currentSubWord;
    int isRecording();
    int isReplaying();
    QProcess *pollingProc;
    void clearRamDisk();
    bool removeDir(const QString & dirName);
    int isSetupMode();
    int isStopped();
    int saveToTxt(FFTSamples samples,int numOfBits);
signals:
    void dispHeader(QString header);
    void dispAttr(Attributes attr);
    void threadTerminated();
    void sendFFTSamples(FFTSamples samples,int numOfBits,bool isSetup);
    void signalError();
private slots:
    void onTimeOut();
};

#endif // RTREADINGTHREAD_H
