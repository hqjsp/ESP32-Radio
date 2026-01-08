/**
 * ESP-32 Radio
 * 
 * FMState.cpp
 * 
 */

#include "FMState.h"


FMState::FMState(uint16_t frequency, uint8_t volume){
    this->frequency = frequency;
    this->volume = volume;

    this->reset();
}

bool FMState::getStateChanged(){
    bool change = this->stateChanged;
    this->stateChanged = false;
    return change;
}

/**
 * This resets all RDS parameters to their default values.
 * This should be called anytime the frequency changes (except if the frequency changes due to AF).
 */
void FMState::reset(){
    this->ps = "[No PS]";
    this->pty = -1;
    this->rt = "[No RadioText]";
    this->hasRds = false;
    this->hasStereo = false;
    this->signalStrength = 0;
}
/**
 * Changes the frequency that the tuner is tuned to.
 */
void FMState::setFrequency(uint16_t currentFrequency){
    if (this->frequency != currentFrequency)
        this->stateChanged = true;
    this->frequency = currentFrequency;

    this->reset();
}

/**
 * Increments the frequency by the specified step. This is the step in 10kHz units.
 * @param step an integer specifing the step in multiples of 10kHz.
 */
void FMState::incrementFrequency(int step){
    this->stateChanged = true;
    this->frequency += step;
    if (this->frequency > FM_BAND_URANGE)
        this->frequency = FM_BAND_LRANGE;
    this->reset();
}
/**
 * Increments the frequency by the default step defined in FMState.h. The default is 100kHz.
 */
void FMState::incrementFrequency(){
    this->incrementFrequency(FM_STEP);
}

/**
 * Decrements the frequency by the specified step. This is the step in 10kHz units.
 * @param step an integer specifing the step in multiples of 10kHz.
 */
void FMState::decrementFrequency(int step){
    this->stateChanged = true;
    this->frequency -= step;
    if (this->frequency < FM_BAND_LRANGE)
        this->frequency = FM_BAND_URANGE;
    this->reset();
}
/**
 * Decrements the frequency by the default step defined in FMState.h. The default is 100kHz.
 */
void FMState::decrementFrequency(){
    this->decrementFrequency(FM_STEP);
}

/**
 * Sets whether the current station is being decoded in stereo, or not.
 * @param stereo a bool value representing whether the station is in stereo or not.
 */
void FMState::setStereo(bool stereo){
    this->stateChanged = this->hasStereo != stereo;
    this->hasStereo = stereo;
}

/**
 * Sets the signal strength between 0-3, where 0 is no signal whatsoever and 3 is max signal.
 * @param signalStrength the signal strength between 0-3.
 */
void FMState::setSignalStrength(int signalStrength){
    this->stateChanged = this->signalStrength != signalStrength;
    if (signalStrength >= 0 && signalStrength <= 3)
        this->signalStrength = signalStrength;
    else this->signalStrength = 0;
}

/**
 * Sets whether the current station has RDS data available.
 * @param rds a bool value representing whether the station has RDS data or not.
 */
void FMState::setRDS(bool rds){
    this->stateChanged = this->hasRds != rds;
    this->hasRds = rds;
}

/**
 * Sets the current station's RDS program service field. This will also set the RDS flag.
 * @param rds_ps the program service value
 */
void FMState::setRdsPS(char * rds_ps){
    String nps = String(rds_ps).substring(0, 8);
    this->stateChanged = !this->ps.equals(nps);
    this->ps = nps;
    this->hasRds = true;
}

/**
 * Sets the current station's RDS program type field. This will also set the RDS flag.
 * This is a value between 0-31 representing the type of programming being broadcast on this channel.
 * @param rds_pty the station's PTY value.
 */
void FMState::setRdsPTY(int rds_pty){
    this->stateChanged = this->pty != rds_pty;
    this->pty = rds_pty;
    this->hasRds = true;
}

/**
 * Sets the current station's RDS radiotext field. This will also set the RDS flag.
 * @param rds_rt the station's radiotext field.
 */
void FMState::setRdsRT(char * rds_rt) {
    String nrt = String(rds_rt).substring(0, 128);
    this->stateChanged = !this->rt.equals(nrt);
    this->rt = nrt;
    this->hasRds = true;
}

void FMState::setVolume(int vol){
    if (vol < 0)
        vol = 0;
    if (vol > 15)
        vol = 15;
    
    this->volume = vol;
}
void FMState::incrementVolume(){
    this->volume++;
    if (this->volume > 15)
        this->volume = 15;
}
void FMState::decrementVolume(){
    this->volume--;
    if (this->volume < 0)
        this->volume = 0;
}


/**
 * Creates a new station item. This constructor should only be used on stations that do not contain
 * RDS values. The FMStationItem(uint16_t frequency, char *ps) constructor should be used for stations
 * that do contain RDS values.
 * @param frequency the station's frequency (in multiples of 10kHz)
 */
FMStationItem::FMStationItem(uint16_t frequency){
    this->frequency = frequency;
    this->ps[0] = '\0';
}
/**
 * Creates a new station item with the station's program service field saved.
 * @param frequency the station's frequency (in multiples of 10kHz)
 * @param ps the station's program service field.
 */
FMStationItem::FMStationItem(uint16_t frequency, char *ps){
    this->frequency = frequency;
    memcpy(this->ps, ps, 9);
    this->ps[8] = '\0';
    this->rds = true;
}
/**
 * @return the station's program service field.
 */
String FMStationItem::getRdsPS(){
    return this->ps;
}
/**
 * @return the station's frequency.
 */
uint16_t FMStationItem::getFrequency(){
    return this->frequency;
}
/**
 * @return a boolean representing whether this station has RDS data (program service) information available or not.
 */
bool FMStationItem::hasRds(){
    return this->rds;
}

bool FMStationItem::operator==(const FMStationItem& r){
    if (this->rds){
        if (strncmp(this->ps, r.ps, 8) == 0)
            return true;
    }

    return this->frequency == r.frequency;
}


FMStationList::FMStationList(){}

/** @return the count of elements in this list. */
int FMStationList::size(){
    return this->items.size();
}

/**
 * Adds a new item to the list.
 * @param item a new FMStationItem object to append to the list.
 */
void FMStationList::add(FMStationItem* item){
    this->items.push_back(item);
}
/**
 * Removes the item at a certain index.
 * @param index the index of the item to remove.
 */
void FMStationList::remove(int index){
    if (this->size() > index || index < 0)
        return;
    
    this->items.erase(std::find(this->items.begin(), this->items.end(), this->items[index]));
}
/**
 * Removes the item in the list.
 * @param item the pointer to the element that needs to be removed.
 */
void FMStationList::remove(FMStationItem *item){
    this->items.erase(std::find(this->items.begin(), this->items.end(), item));
}
/**
 * @return all elements in the list. Stored on the heap; must be freed with malloc free().
 */
FMStationItem **FMStationList::getAll(){
    FMStationItem **list = (FMStationItem **)malloc(this->size() * sizeof(FMStationItem*));
    for(int i = 0; i < this->size(); i++)
        list[i] = this->items[i];
    return list;
}
/**
 * @param index the element to return.
 * @return the element at the specified index.
 */
FMStationItem *FMStationList::get(int index){
    if (this->size() < index || index < 0)
        return nullptr;
    return this->items[index];
}
FMStationItem *FMStationList::operator[](int index){
    return this->get(index);
}
void FMStationList::operator+(FMStationItem *item){
    this->add(item);
}
bool FMStationList::operator==(const FMStationList& r){
    return this->items == r.items;
}
