#ifndef FMSTATE_H
#define FMSTATE_H

#include <Arduino.h>
#include <vector>

struct FMState {
    public:
        uint8_t volume;
        uint16_t frequency;
        uint8_t signalStrength;

        bool hasStereo = false;
        bool hasRds = false;

        String ps;
        String rt;
        int pty;

        FMState(uint16_t, uint8_t);

        void reset();
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

        const char *toString();

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