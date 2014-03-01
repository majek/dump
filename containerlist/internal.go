package main

type ilruitem struct {
	key         string
	value       interface{}
	listElement element
}

type iLRUCache struct {
	table    map[string]*ilruitem
	lruList  list
	freeList list
	capacity uint
}

func (b *iLRUCache) Init(capacity uint) {
	b.table = make(map[string]*ilruitem, capacity)
	b.lruList.Init()
	b.freeList.Init()

	// Allocate 'capacity' items, add them to 'freeList'
	items := make([]ilruitem, capacity)
	for i := uint(0); i < capacity; i++ {
		item := &items[i]
		item.listElement.Value = item
		b.freeList.PushElementBack(&item.listElement)
	}
}

func (b *iLRUCache) Set(key string, value interface{}) {
	var item *ilruitem
	if listElement := b.freeList.Back(); listElement != nil {
		item = listElement.Value.(*ilruitem)
		b.freeList.Remove(listElement)
	} else {
		// No free items. Is our already present?
		item = b.table[key]
		if item == nil {
			// nope, remove the least used one
			item = b.lruList.Back().Value.(*ilruitem)
		}
		b.lruList.Remove(&item.listElement)
		delete(b.table, item.key)
	}
	item.key = key
	item.value = value
	b.lruList.PushElementFront(&item.listElement)
	b.table[key] = item
}

func (b *iLRUCache) Get(key string) (v interface{}, ok bool) {
	item := b.table[key]
	if item == nil {
		return nil, false
	}

	b.lruList.Remove(&item.listElement)
	b.lruList.PushElementFront(&item.listElement)

	return item.value, true
}
