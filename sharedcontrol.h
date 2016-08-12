#ifndef SHAREDCONTROL_H
#define SHAREDCONTROL_H

#endif // SHAREDCONTROL_H

typedef enum {AV_1, AV_8, AV_16,AV_32, AV_N} Avrg;

typedef struct {
    bool isValid;
    int chanSel;
    int fpgaSel;
    int fftPoints;
    Avrg avrgSpec;
    Avrg avrgHisto;
    Avrg resDiv;
}SharedControl;
