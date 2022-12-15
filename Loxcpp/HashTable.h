#ifndef lox_tablecpp_h
#define lox_tablecpp_h

#include "Common.h"
#include "Value.h"

struct ObjString;

struct Entry
{
    ObjString* key = nullptr;
    Value value = Value();
};

struct Table
{
    Table();

    bool set(ObjString* key, const Value& value);
    void adjustCapacity(size_t capacity);
    bool get(ObjString* key, Value* value);
    bool remove(ObjString* key);
    ObjString* findString(const char* chars, int length, uint32_t hash);

    void copyTo(Table& to) const;

    void Clear();

    size_t count;
    size_t capacity;
    std::vector<Entry> entries;
};

#endif