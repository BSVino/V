#pragma once

#include "v.h"

#define VHASH_OCCUPIED (1<<0)

template <typename Value>
class vhash
{
public:
	typedef size_t hash_t;

	struct entry_s {
		unsigned char flags;
		hash_t        hash;
		const char*   key;
	};

public:
	vhash()
	{
		entries = NULL;
		values = NULL;
		size = 1024;
	}

	vhash(size_t size_)
	{
		entries = NULL;
		values = NULL;
		size = size_;
	}

	~vhash()
	{
		free(entries);
		free(values);
	}

public:
	void init(size_t size_)
	{
		// Force to use the next highest power of 2.
		size_--;
		size_ |= size_ >> 1;
		size_ |= size_ >> 2;
		size_ |= size_ >> 4;
		size_ |= size_ >> 8;
		size_ |= size_ >> 16;
		size_++;

		size = size_;

		free(entries);
		free(values);

		entries = (entry_s*)malloc(size_ * sizeof(entry_s));
		memset(entries, 0, size_ * sizeof(entry_s));

		values = (Value*)malloc(size_ * sizeof(Value));
	}

	void clear()
	{
		init(size);
	}

	size_t hash(const char* k)
	{
		hash_t hash = 0;

		size_t k_len = strlen(k);

		size_t shift = (size_t)0x12345678abcdef01;
		for (size_t i = 0; i < k_len; i++)
			hash ^= k[i] * (shift+i);

		return (hash_t)hash;
	}

	size_t find(const char* k, hash_t* key_hash = NULL, bool* item_found = NULL)
	{
		bool item_found_ = false;

		hash_t hash_ = hash(k);
		if (key_hash)
			*key_hash = hash_;

		size_t original_index = hash_&(size-1); // This amounts to a % because the size is always a power of 2.
		size_t index = original_index;
		entry_s* entry = &entries[index];

		while (entry->flags & VHASH_OCCUPIED)
		{
			if (entry->hash == hash_ && strcmp(entry->key, k) == 0)
			{
				item_found_ = true;
				break;
			}

			index = (index + 1)&(size-1);
			entry = &entries[index];
			if (index == original_index)
			{
				Assert(false);
				return ~0; // Force a crash is probably the best option.
			}
		}

		if (item_found)
			*item_found = item_found_;

		return index;
	}

	void set(const char* k, size_t index, hash_t key_hash, Value v)
	{
		entry_s* entry = &entries[index];
		entry->flags |= VHASH_OCCUPIED;
		entry->hash = key_hash;
		entry->key = k;

		values[index] = v;
	}

	void set(const char* k, Value v)
	{
		hash_t key_hash;
		size_t index = find(k, &key_hash);
		set(k, index, key_hash, v);
	}

	Value* get(size_t index)
	{
		return &values[index];
	}

	bool entry_exists(const char* key)
	{
		bool found;
		entry_s* entry = &entries[find(key, NULL, &found)];
		return found;
	}

private:
	size_t   size;
	entry_s* entries;
	Value*   values;
};

