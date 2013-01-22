```
SipHash is a family of pseudorandom functions (a.k.a. keyed hash
functions) optimized for speed on short messages.

Target applications include network traffic authentication and defense
against hash-flooding DoS attacks.

SipHash is secure, fast, and simple (for real):
* SipHash is simpler and faster than previous cryptographic algorithms
  (e.g. MACs based on universal hashing)
* SipHash is competitive in performance with insecure
  non-cryptographic algorithms (e.g. MurmurHash)
* We propose that hash tables switch to SipHash as a hash
  function. Users of SipHash already include OpenDNS, Perl 5, Ruby, or
  Rust.
```
