#ifndef lox_tablecpp_h
#define lox_tablecpp_h

#include "Common.h"
#include "Value.h"

#include <unordered_map>

struct ObjString;

struct Entry
{
    ObjString* key = nullptr;
    Value value = Value();
};

struct Hasher
{
    size_t operator()(ObjString* key) const;
};

struct TableCpp
{
    using EntriesMap = std::unordered_map<uint32_t, Entry>;

    bool set(ObjString* key, const Value& value);
    bool get(ObjString* key, Value* value);
    bool remove(ObjString* key);
    ObjString* findString(const char* chars, int length, uint32_t hash);
    void mark();
    void removeWhite();

private:

    EntriesMap entries;

};

struct TableLox
{
    TableLox();

    bool set(ObjString* key, const Value& value);
    bool get(ObjString* key, Value* value);
    bool remove(ObjString* key);
    ObjString* findString(const char* chars, int length, uint32_t hash);
    void mark();
    void removeWhite();

private:
    void adjustCapacity(size_t capacity);

    void copyTo(TableLox& to) const;

    void Clear();

    size_t count;
    size_t capacity;
    std::vector<Entry> entries;
};

using Table = TableLox;

#endif