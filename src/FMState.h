/**
 * ESP-32 Radio
 * 
 * FMState.h
 * 
 */


#ifndef FMSTATE_H
#define FMSTATE_H

#include <Arduino.h>
#include <vector>

// FM Band values
#define FM_STEP            10
#define FM_BAND_LRANGE   8750
#define FM_DEFAULT_FREQ  9990
#define FM_BAND_URANGE  10800

struct FMState {
    private:
        bool stateChanged = false;
        bool stereoChanged = false;
        bool signalChanged = false;

        int8_t volume;
        uint16_t frequency;
        uint8_t signalStrength;

        bool stereo = false;
        bool rds = false;

        String ps;
        String rt;
        int pty;
    public:
        
        FMState(uint16_t, uint8_t);

        bool getStateChanged();
        bool getSignalStateChanged();
        bool getStereoStateChanged();

        void allStatesChanged();

        void reset();

        void setRDS(bool);
        void setRdsPS(char*);
        void setRdsRT(char*);
        void setRdsPS(String);
        void setRdsRT(String);
        void setRdsPTY(int pty);
        
        void setStereo(bool);
        
        void setFrequency(uint16_t);
        void incrementFrequency(int step);
        void incrementFrequency();
        void decrementFrequency(int step);
        void decrementFrequency();
        void setSignalStrength(int signalStrength);

        void setVolume(int);
        void incrementVolume();
        void decrementVolume();

        int8_t getVolume();
        uint16_t getFrequency();
        uint8_t getSignalStrength();
        bool hasStereo();
        bool hasRDS();
        String getRdsPS();
        String getRdsRT();
        int getRdsPTY();
};

class FMStationItem {
    protected:
        uint16_t frequency;
        char ps[9];
        bool rds = false;
    
    public:
        FMStationItem(uint16_t frequency);
        FMStationItem(uint16_t frequency, char *ps);

        String getRdsPS();
        uint16_t getFrequency();
        bool hasRds();

        bool operator==(const FMStationItem& r);
};

struct FMStationList {
    protected:
        std::vector<FMStationItem*> items;
    public:
        FMStationList();
        
        void add(FMStationItem*);
        
        void remove(int);
        void remove(FMStationItem*);
        void removeAll();

        int size();

        FMStationItem **getAll();
        FMStationItem *get(int);

        FMStationItem *operator[](int);
        void operator+(FMStationItem*);
        bool operator==(const FMStationList& r);
};

#endif