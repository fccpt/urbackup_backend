#include "LRUMemCache.h"


LRUMemCache::LRUMemCache( size_t buffersize, size_t nbuffers )
	: buffersize(buffersize), nbuffers(nbuffers), callback(NULL)
{
}

char* LRUMemCache::get( __int64 offset, size_t& bsize )
{
	for(size_t i=0;i<lruItems.size();++i)
	{
		if(lruItems[i].offset<=offset &&
			lruItems[i].offset+static_cast<__int64>(buffersize)<offset)
		{
			size_t innerOffset = static_cast<size_t>(offset-lruItems[i].offset);
			bsize = buffersize - innerOffset;
			return lruItems[i].buffer + innerOffset;
		}
	}

	return NULL;
}

bool LRUMemCache::put( __int64 offset, const char* buffer, size_t bsize )
{
	for(size_t i=0;i<lruItems.size();++i)
	{
		if(lruItems[i].offset<=offset &&
			lruItems[i].offset+static_cast<__int64>(buffersize)<offset)
		{
			size_t innerOffset = static_cast<size_t>(offset-lruItems[i].offset);

			if( buffersize - innerOffset < bsize)
			{
				return false;
			}

			memcpy(lruItems[i].buffer + innerOffset, buffer, bsize);

			putBack(i);

			return true;
		}
	}

	SCacheItem newItem = createInt(offset);

	size_t innerOffset = static_cast<size_t>(offset-newItem.offset);

	if( buffersize - innerOffset < bsize)
	{
		return false;
	}

	memcpy(newItem.buffer + innerOffset, buffer, bsize);

	return true;
}

void LRUMemCache::putBack( size_t idx )
{
	SCacheItem item = lruItems[idx];
	lruItems.erase(lruItems.begin()+idx);
	lruItems.push_back(item);
}

void LRUMemCache::setCacheEvictionCallback( ICacheEvictionCallback* cacheEvictionCallback )
{
	callback=cacheEvictionCallback;
}

void LRUMemCache::clear()
{
	for(size_t i=0;i<lruItems.size();++i)
	{
		evict(lruItems[i]);
	}
	lruItems.clear();
}

void LRUMemCache::evict( SCacheItem& item )
{
	if(callback!=NULL)
	{
		callback->evictFromLruCache(item);
	}
	delete item.buffer;
}

LRUMemCache::~LRUMemCache()
{
	clear();
}

SCacheItem LRUMemCache::createInt( __int64 offset )
{
	if(lruItems.size()==nbuffers)
	{
		SCacheItem& toremove = lruItems[0];
		evict(toremove);
		lruItems.erase(lruItems.begin());
	}

	SCacheItem newItem;
	newItem.buffer=new char[buffersize];
	newItem.offset=offset - offset % buffersize;

	lruItems.push_back(newItem);

	return newItem;
}

char* LRUMemCache::create( __int64 offset )
{
	size_t bsize;
	char* buf = get(offset, bsize);

	if(buf!=NULL)
	{
		return buf;
	}

	return createInt(offset).buffer;
}
