static inline void bitmap_set(uint64_t *map, int n)
{
	map[ n / 64 ] |= 0x1ULL << (n % 64);
}

static inline void bitmap_clear(uint64_t *map, int n)
{
	map[ n / 64 ] &= ~(0x1ULL << (n % 64));
}

static inline int bitmap_find_set(uint64_t *map_start, int map_sizeof)
{
	uint64_t *map = map_start;
	uint64_t *map_end = map_start + (map_sizeof / sizeof(uint64_t));
	for (; map < map_end; map++) {
		if (*map) {
			return (map - map_start) * 64 +
				__builtin_ffsll(*map) - 1;
		}
	}
	return -1;
}

