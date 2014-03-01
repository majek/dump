package main

import (
	clist "container/list"
)

type elruitem struct {
	key         string
	value       interface{}
	listElement *clist.Element
}

type eLRUCache struct {
	table    map[string]*elruitem
	lruList  clist.List
	freeList clist.List
	capacity uint
}

func (b *eLRUCache) Init(capacity uint) {
	b.table = make(map[string]*elruitem, capacity)
	b.lruList.Init()
	b.freeList.Init()

	// Allocate 'capacity' items, add them to 'freeList'
	items := make([]elruitem, capacity)
	for i := uint(0); i < capacity; i++ {
		item := &items[i]
		item.listElement = b.freeList.PushBack(item)
	}
}

func (b *eLRUCache) Set(key string, value interface{}) {
	var item *elruitem
	if listElement := b.freeList.Back(); listElement != nil {
		item = listElement.Value.(*elruitem)
		b.freeList.Remove(listElement)
	} else {
		// No free items. Is our already present?
		item = b.table[key]
		if item == nil {
			// nope, remove the last used one
			item = b.lruList.Back().Value.(*elruitem)
		}
		b.lruList.Remove(item.listElement)
		delete(b.table, item.key)
	}
	item.key = key
	item.value = value
	item.listElement = b.lruList.PushFront(item)
	b.table[key] = item
}

func (b *eLRUCache) Get(key string) (v interface{}, ok bool) {
	item := b.table[key]
	if item == nil {
		return nil, false
	}

	b.lruList.Remove(item.listElement)
	item.listElement = b.lruList.PushFront(item)

	return item.value, true
}
