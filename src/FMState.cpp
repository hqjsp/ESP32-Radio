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
    this->ps = "[No Name]";
    this->pty = -1;
    this->rt = "[No RadioText]";
    this->pi = 0x0;
    this->tp = false;
    this->rds = false;
    this->stereo = false;
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
 * Sets whether the current station has RDS data available.
 * @param rds a bool value representing whether the station has RDS data or not.
 */
void FMState::setRDS(bool rds){
    if (!this->stateChanged) this->stateChanged = this->rds != rds;
    this->rds = rds;
}

/**
 * Sets the current station's RDS program service field. This will also set the RDS flag.
 * @param rds_ps the program service value
 */
void FMState::setRdsPS(char * rds_ps){
    this->setRdsPS(String(rds_ps));
}

/**
 * Sets the current station's RDS program service field. This will also set the RDS flag.
 * @param rds_ps the program service value
 */
void FMState::setRdsPS(String rds_ps){
    rds_ps = String(rds_ps).substring(0, 8);
    if (!this->stateChanged) this->stateChanged = !this->ps.equals(rds_ps);
    this->ps = rds_ps;
    this->rds = true;
}

/**
 * Sets the current station's RDS program type field. This will also set the RDS flag.
 * This is a value between 0-31 representing the type of programming being broadcast on this channel.
 * @param rds_pty the station's PTY value.
 */
void FMState::setRdsPTY(int rds_pty){
    if (!this->stateChanged) this->stateChanged = this->pty != rds_pty;
    this->pty = rds_pty;
    this->rds = true;
}

void FMState::setRdsPI(uint16_t rds_pi){
    if (!this->stateChanged) this->stateChanged = (this->pi != rds_pi);
    this->pi = rds_pi;
    this->rds = true;
}

void FMState::setRdsTP(bool tp){
    if (!this->stateChanged) this->stateChanged = (this->tp != tp);
    this->tp = tp;
    this->rds = true;
}

uint16_t FMState::getRdsPI(){
    return this->pi;
}

/**
 * Sets the current station's RDS radiotext field. This will also set the RDS flag.
 * @param rds_rt the station's radiotext field.
 */
void FMState::setRdsRT(char * rds_rt) {
    this->setRdsRT(String(rds_rt));
}

void FMState::setRdsRT(String rds_rt){
    int null_index = rds_rt.indexOf(13);
    rds_rt = String(rds_rt).substring(0, null_index != -1 ? null_index : 64);
    rds_rt.trim();
    if (!this->stateChanged) this->stateChanged = !this->rt.equals(rds_rt);
    this->rt = rds_rt;
    this->rds = true;
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

int8_t FMState::getVolume(){
    return this->volume;
}
uint16_t FMState::getFrequency(){
    return this->frequency;
}
uint8_t FMState::getSignalStrength(){
    return this->signalStrength;
}
bool FMState::hasStereo(){
    return this->stereo;
}
bool FMState::hasRDS(){
    return this->rds;
}
String FMState::getRdsPS(){
    return this->ps;
}
String FMState::getRdsRT(){
    return this->rt;
}
int FMState::getRdsPTY(){
    return this->pty;
}
bool FMState::getRdsTP(){
    return this->tp;
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