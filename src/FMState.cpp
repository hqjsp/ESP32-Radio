
#include "FMState.h"

FMState::FMState(uint16_t frequency, uint8_t volume){
    this->frequency = frequency;
    this->volume = volume;

    this->reset();
}

void FMState::reset(){
    this->ps = "[No PS]";
    this->pty = -1;
    this->rt = "[No RadioText]";
    this->hasRds = false;
    this->hasStereo = false;
    this->signalStrength = 0;
}


FMStationItem::FMStationItem(uint16_t frequency){
    this->frequency = frequency;
    this->ps[0] = '\0';
}
FMStationItem::FMStationItem(uint16_t frequency, char *ps){
    this->frequency = frequency;
    memcpy(this->ps, ps, 9);
    this->ps[8] = '\0';
    this->rds = true;
}
String FMStationItem::getRdsPS(){
    return this->ps;
}
uint16_t FMStationItem::getFrequency(){
    return this->frequency;
}
bool FMStationItem::hasRds(){
    return this->rds;
}
const char *FMStationItem::toString(){
    String s = String(((float)this->frequency / 100.0f));
    s.concat(": ");
    s.concat(this->ps);
    return s.c_str();
}
bool FMStationItem::operator==(const FMStationItem& r){
    if (this->rds){
        if (strncmp(this->ps, r.ps, 8) == 0)
            return true;
    }

    return this->frequency == r.frequency;
}


FMStationList::FMStationList(){
    
}
int FMStationList::size(){
    return this->items.size();
}
void FMStationList::add(FMStationItem* item){
    this->items.push_back(item);
}
void FMStationList::remove(int index){
    if (this->size() > index || index < 0)
        return;
    
    this->items.erase(std::find(this->items.begin(), this->items.end(), this->items[index]));
}
void FMStationList::remove(FMStationItem *item){
    this->items.erase(std::find(this->items.begin(), this->items.end(), item));
}
FMStationItem **FMStationList::getAll(){
    FMStationItem **list = (FMStationItem **)malloc(this->size() * sizeof(FMStationItem*));
    for(int i = 0; i < this->size(); i++)
        list[i] = this->items[i];
    return list;
}
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
