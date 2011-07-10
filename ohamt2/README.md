
The data structure is based on a 64-elements wide trie. We may assume
that the architecture has a fast 64-bit popcnt command, although it's
not a requirement.

User supplied values are 40-bit wide, and must be resolvable by a user
supplied callback function to 128-bit wide key. The last of 40 bits
must be clear, so it's actually 39 user-supplied bits.

The hard part in this data structure is memory allocation. Values,
with last bit set, are treated as 'pointers' or actually references to
a pointer. Memory allocation code resolves this 39-bit value to a
proper pointer.

In order to resolve this value, memory allocator understands the value
as:
  6 bits - how wide is the object (8+5*x bytes wide, x is <1, 64>)
 23 bits - 'page' number
 10 bits - index of element in a page
  1 bit - reserved


