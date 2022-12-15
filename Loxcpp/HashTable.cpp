#include "HashTable.h"

#include "Object.h"

#define TABLE_MAX_LOAD 0.75

Entry* findEntry(std::vector<Entry>& entries, size_t capacity, const ObjString* key)
{
    uint32_t index = key->hash % capacity;
    Entry* tombstone = nullptr;

    for (;;)
    {
        Entry* entry = &entries[index];
        if (entry->key == nullptr)
        {
            if (isNil(entry->value))
            {
                // Empty entry.
                return tombstone != nullptr ? tombstone : entry;
            }
            else
            {
                // We found a tombstone.
                if (tombstone == nullptr) tombstone = entry;
            }
        }
        else if (entry->key == key)
        {
            // We found the key.
            return entry;
        }

        index = (index + 1) % capacity;
    }

    return nullptr;
}

Table::Table()
    : count(0)
    , capacity(0)
    , entries()
{}

bool Table::set(ObjString* key, const Value& value)
{
    if (count + 1 > capacity * TABLE_MAX_LOAD)
    {
        const size_t nextCapacity = capacity < 8 ? 8 : capacity * 2;
        adjustCapacity(nextCapacity);
    }

    Entry* entry = findEntry(entries, capacity, key);
    const bool isNewKey = entry->key == nullptr;
    if (isNewKey && isNil(entry->value)) count++;

    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void Table::adjustCapacity(size_t nextCapacity)
{
    // Instead of resizing the vector, we make a new one and swap with the old one
    // The vector would have to reallocate with the resizing anyway, and that way it's
    // easier to move the old values to the new hashtable
    std::vector<Entry> nextEntries(nextCapacity);

    count = 0;
    for (uint32_t i = 0; i < capacity; i++)
    {
        Entry* entry = &entries[i];
        if (entry->key == nullptr) continue;

        Entry* dest = findEntry(nextEntries, nextCapacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        count++;
    }

    std::swap(entries, nextEntries);

    capacity = nextCapacity;
}

bool Table::get(ObjString* key, Value* value)
{
    if (count == 0) return false;

    Entry* entry = findEntry(entries, capacity, key);
    if (entry->key == nullptr) return false;

    *value = entry->value;
    return true;
}

bool Table::remove(ObjString* key)
{
    if (count == 0) return false;

    // Find the entry.
    Entry* entry = findEntry(entries, capacity, key);
    if (entry->key == nullptr) return false;

    // Place a tombstone in the entry.
    entry->key = nullptr;
    entry->value = Value(true);
    return true;
}

ObjString* Table::findString(const char* chars, int length, uint32_t hash)
{
    if (count == 0) return nullptr;

    uint32_t index = hash % capacity;
    for (;;)
    {
        Entry* entry = &entries[index];
        if (entry->key == NULL)
        {
            // Stop if we find an empty non-tombstone entry.
            if (isNil(entry->value)) return NULL;
        }
        else if (entry->key->length == length &&
            entry->key->hash == hash &&
            memcmp(&entry->key->chars[0], chars, length) == 0)
        {
            // We found it.
            return entry->key;
        }

        index = (index + 1) % capacity;
    }
}

void Table::copyTo(Table& to) const
{
    for (uint32_t i = 0; i < capacity; i++)
    {
        const Entry* entry = &entries[i];
        if (entry->key != nullptr)
        {
            to.set(entry->key, entry->value);
        }
    }
}


void Table::Clear()
{
    entries.clear();
    count = 0;
    capacity = 0;
}
