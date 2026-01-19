/**
 * ESP-32 Radio
 * 
 * FMState.cpp
 * 
 */

#include "FMState.h"

FMRds::FMRds(){
    this->reset();
}

/**
 * Returns the 8 character program service string for the current station.
 */
char * FMRds::get_ps(void){ return this->ps; }

/**
 * Returns the 65 character radio text string for the current station.
 */
char * FMRds::get_rt(void){ return this->rt; }

/**
 * Returns the program type integer for the current station.
 */
unsigned char FMRds::get_pty(void){ return this->pty; }

/**
 * Returns the 16 bit program identifier for the current station.
 */
unsigned short FMRds::get_pi(void){ return this->pi; }

/**
 * Returns the traffic program flag for the current station 
 */
bool FMRds::get_tp(void) { return this->tp; }

/**
 * Returns the traffic announcement flag for the current station.
 */
bool FMRds::get_ta(void) { return this->ta; }

/**
 * Returns true if the current station has RDS data.
 */
bool FMRds::has_rds(void) { return this->pi != 0; }

bool FMRds::has_ps(void) { return strlen(this->ps) != 0; }

/**
 * Call this function after the rds information has been updated. This resets
 * the update flag.
 */
void FMRds::has_updated(void){ this->update = false; }

/**
 * Returns true if the RDS information has been updated and the display must be
 * updated.
 */
bool FMRds::needs_update(void) { return this->update; }

/**
 * Resets the RDS data. This is useful when the frequency has been changed.
 */
void FMRds::reset(void) {
    this->pi = 0;
    memset(this->ps, 0, 8);
    memset(this->rt, 0, 64);
    this->pty = -1;
    this->tp = false;
    this->ta = false;
    this->update = true;
}

/**
 * Sets the program service field for this station. This can only be
 * up to 8 characters.
 * @param ps the program service value
 */
void FMRds::set_ps(char *ps){
    memset(this->ps, 0, 8);
    if (ps == NULL){
        this->update = true;
        return;
    }
    if (strncmp(this->ps, ps, 8) == 0) return;
    strncpy(this->ps, ps, 8);
    this->ps[8] = 0;
    this->update = true;

    // replace all corrupted characters with a space.
    for (int i = 0; i < 8; i++){
        if (this->ps[i] < ' ' || this->ps[i] > '~')
            this->ps[i] = ' ';
    }
}

/**
 * Sets the radio text field for this station. This can only be
 * up to 64 characters.
 * @param rt the station's radiotext field.
 */
void FMRds::set_rt(char *rt){
    memset(this->rt, 0, 64);
    if (rt == NULL){
        this->update = true;
        return;
    }
    strncpy(this->rt, rt, 64);
    this->rt[64] = 0;
    this->trim_rt();
    this->update = true;

    // replace all corrupted characters with a space.
    for (int i = 0; i < strlen(this->rt); i++){
        if (this->rt[i] < ' ' || this->rt[i] > '~')
            this->rt[i] = ' ';
    }
}

void FMRds::trim_rt(void){
    char *end = strchr(this->rt, '\r');

    if (end != NULL){
        // has a carriage return character to signify the end of the string
        for (int i = 0; i < strlen(end); i++)
            end[i] = 0;
        
    }else {
        for (int i = 64; i > 0; i--){
            if (this->rt[i] >= '!' && this->rt[i] <= '~')
                break;
            
            this->rt[i] = 0;
        }
    }
}

/**
 * Sets the current station's RDS program type field. This will also set the RDS flag.
 * This is a value between 0-31 representing the type of programming being broadcast on this channel.
 * @param pty the station's PTY value.
 */
void FMRds::set_pty(unsigned char pty){ 
    if (this->pty != pty) this->update = true; 
    this->pty = pty;
 }

/**
 * Sets the program identification field for the current station.
 */
void FMRds::set_pi(unsigned short pi){ 
    if (this->pi != pi) this->update = true; 
    this->pi = pi; 
}

/**
 * Sets the traffic program flag for the current station.
 */
void FMRds::set_tp(bool tp){ 
    if (tp != this->tp) this->update = true; 
    this->tp = tp; 
}

/**
 * Sets the traffic announcement flag for the current station.
 */
void FMRds::set_ta(bool ta){ 
    if (ta != this->ta) this->update = true;
    this->ta = ta; 
}

FMState::FMState(uint16_t frequency, uint8_t volume){
    this->frequency = frequency;
    this->volume = volume;

    this->reset();
}

bool FMState::getSignalStateChanged(){
    bool change = this->signalChanged;
    this->signalChanged = false;
    return change;
}

bool FMState::getStereoStateChanged(){
    bool change = this->stereoChanged;
    this->stereoChanged = false;
    return change;
}

void FMState::allStatesChanged(){
    this->stereoChanged = true;
    this->signalChanged = true;
}

/**
 * This resets all RDS parameters to their default values.
 * This should be called anytime the frequency changes (except if the frequency changes due to AF).
 */
void FMState::reset(){
    /*this->ps = "[No Name]";
    this->pty = -1;
    this->rt = "[No RadioText]";
    this->pi = 0x0;
    this->tp = false;
    this->rds = false;*/
    this->rds.reset();
    this->stereo = false;
    this->signalStrength = 0;
}
/**
 * Changes the frequency that the tuner is tuned to.
 */
void FMState::setFrequency(uint16_t currentFrequency){
    this->frequency = currentFrequency;

    this->reset();
}

/**
 * Increments the frequency by the specified step. This is the step in 10kHz units.
 * @param step an integer specifing the step in multiples of 10kHz.
 */
void FMState::incrementFrequency(int step){
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
    this->stereoChanged = this->stereo != stereo;
    this->stereo = stereo;
}

/**
 * Sets the signal strength between 0-3, where 0 is no signal whatsoever and 3 is max signal.
 * @param signalStrength the signal strength between 0-3.
 */
void FMState::setSignalStrength(int signalStrength){
    this->signalChanged = this->signalStrength != signalStrength;
    if (signalStrength >= 0 && signalStrength <= 3)
        this->signalStrength = signalStrength;
    else this->signalStrength = 0;
}

/**
 * Sets the volume level. This does not adjust the volume level for the SI470x library.
 * @param int the volume level between 0 and 15.
 */
void FMState::setVolume(int vol){
    if (vol < 0)
        vol = 0;
    if (vol > 15)
        vol = 15;
    
    this->volume = vol;
}

/**
 * Increments the volume level by 1. If the volume is 15, this has no effect.
 */
void FMState::incrementVolume(){
    this->volume++;
    if (this->volume > 15)
        this->volume = 15;
}
/**
 * Decrements the volume level by 1. If the volume is 0, this has no effect.
 */
void FMState::decrementVolume(){
    this->volume--;
    if (this->volume < 0)
        this->volume = 0;
}

/**
 * Returns the volume level.
 */
int8_t FMState::getVolume(){
    return this->volume;
}
/**
 * Returns the frequency in 10kHz units.
 */
uint16_t FMState::getFrequency(){
    return this->frequency;
}
/**
 * Returns the signal strength between 0 and 3.
 */
uint8_t FMState::getSignalStrength(){
    return this->signalStrength;
}
/**
 * Returns if the station is in stereo or not.
 */
bool FMState::hasStereo(){
    return this->stereo;
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

void FMStationItem::setRdsPS(const char* ps){
    memcpy(this->ps, ps, 9);
    this->ps[8] = '\0';
    this->rds = true;
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
    for (int i = 0; i < this->items.size(); i++){
        if (this->items[i]->getFrequency() == item->getFrequency())
            return;
    }
    this->items.push_back(item);
}
/**
 * Removes the item at a certain index.
 * @param index the index of the item to remove.
 */
void FMStationList::remove(int index){
    if (this->size() > index || index < 0)
        return;
    
        delete this->items[index];
    this->items.erase(std::find(this->items.begin(), this->items.end(), this->items[index]));
}
/**
 * Removes the item in the list.
 * @param item the pointer to the element that needs to be removed.
 */
void FMStationList::remove(FMStationItem *item){
    delete item;
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
FMStationItem *FMStationList::get(FMStationItem freq){
    for (int i = 0; i < this->items.size(); i++){
        if (this->items[i]->getFrequency() == freq.getFrequency())
            return this->items[i];
    }
    return nullptr;
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
void FMStationList::removeAll(){
    for (int i = 0; i < this->items.size(); i++){
        delete this->items[i];
    }
    this->items.clear();
}