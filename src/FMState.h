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

class FMRds {
    private:
        unsigned short pi;
        char ps[9];
        char rt[65];

        unsigned char pty;

        bool tp = false;
        bool ta = false;

        bool update = false;

        void trim_rt(void);
    public:
        FMRds(void);

        char *get_ps(void);
        char *get_rt(void);
        unsigned char get_pty(void);
        unsigned short get_pi(void);
        bool get_tp(void);
        bool get_ta(void);

        bool has_rds(void);
        bool has_ps(void);
        bool needs_update(void);

        void has_updated(void);
        void reset(void);

        void set_ps(char *ps);
        void set_rt(char *rt);
        void set_pty(unsigned char pty);
        void set_pi(unsigned short pi);
        void set_tp(bool tp);
        void set_ta(bool ta);
};

struct FMState {
    private:
        bool stereoChanged = false;
        bool signalChanged = false;

        int8_t volume;
        uint16_t frequency;
        uint8_t signalStrength;

        bool stereo = false;
    public:
        FMRds rds;
        
        FMState(uint16_t, uint8_t);

        bool getSignalStateChanged();
        bool getStereoStateChanged();

        void allStatesChanged();

        void reset();
        
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

        void setRdsPS(const char* ps);

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
        FMStationItem *get(FMStationItem freq);

        FMStationItem *operator[](int);
        void operator+(FMStationItem*);
        bool operator==(const FMStationList& r);
};

#endif