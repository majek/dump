#define ENTRY(name) \
  .globl name; \
  .globl _##name; \
  .align 4,0x90; \
  name: ; \
  _##name: 

/*
 * void expand_key128(struct key128_ctx *ctx, u8 *key);
 */
ENTRY(expand_key128)
    # %rdi - ctx pointer
    # %rsi - key pointer
    movups (%rsi), %xmm0    # move key to xmm0
        
    movups %xmm0, (%rdi)   # save key as the first round key
    add $0x10, %rdi        # move to next slot
        
    aeskeygenassist $0x1, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x2, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x4, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x8, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x10, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x20, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x40, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x80, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x1b, %xmm0, %xmm1
    call _key_expansion_128
    aeskeygenassist $0x36, %xmm0, %xmm1
    call _key_expansion_128
    ret

.align 4,0x90
_key_expansion_128:
    # xmm0 - previous key, xmm1 - assist result
    # magical bitshifting
    pshufd $0b11111111, %xmm1, %xmm1
    shufps $0b00010000, %xmm0, %xmm4
    pxor %xmm4, %xmm0
    shufps $0b10001100, %xmm0, %xmm4
    pxor %xmm4, %xmm0
    pxor %xmm1, %xmm0
    movaps %xmm0, (%rdi) # save the key
    add $0x10, %rdi      # move to next slot
    ret


/*
 * void ctr_stream(struct key128_ctx *ctx, u8 *out, u32 len, u8 *iv);
 */
ENTRY(ctr_stream)
    # %rdi - ctx
    # %rsi - out
    # %rdx - len
    # %rcx - iv

    # Load round keys from CTX
    movaps (%rdi),     %xmm5
    movaps 0x10(%rdi), %xmm6
    movaps 0x20(%rdi), %xmm7
    movaps 0x30(%rdi), %xmm8
    movaps 0x40(%rdi), %xmm9
    movaps 0x50(%rdi), %xmm10
    movaps 0x60(%rdi), %xmm11
    movaps 0x70(%rdi), %xmm12
    movaps 0x80(%rdi), %xmm13
    movaps 0x90(%rdi), %xmm14
    movaps 0xa0(%rdi), %xmm15

    # Load IV to xmm0
    movups (%rcx), %xmm0

.loop:
    # Exit loop if done
    cmp $16, %rdx
    jb .loopexit

    # Encrypt xmm0 put result to xmm1
    movdqa     %xmm0,  %xmm1
    pxor       %xmm5,  %xmm1  # Whitening step (Round 0)
    aesenc     %xmm6,  %xmm1  # Round 1
    aesenc     %xmm7,  %xmm1  # Round 2
    aesenc     %xmm8,  %xmm1  # Round 3
    aesenc     %xmm9,  %xmm1  # Round 4
    aesenc     %xmm10, %xmm1  # Round 5
    aesenc     %xmm11, %xmm1  # Round 6
    aesenc     %xmm12, %xmm1  # Round 7
    aesenc     %xmm13, %xmm1  # Round 8
    aesenc     %xmm14, %xmm1  # Round 9
    aesenclast %xmm15, %xmm1  # Round 10
    
    movups %xmm1, (%rsi)      # Save generated stream
    add $16, %rsi             # Move pointer
    sub $16, %rdx             # Reduce len

    call _inc_xmm0            # Increase IV
    jmp .loop

.loopexit:
    ret

# Increment IV in %xmm0
.align 4,0x90
_inc_xmm0:
    # Use 16 bytes from the red zone
    lea     -0x10(%rsp), %r8 # Load the address to rbx
    movups  %xmm0, (%r8)      # Save xmm0 there
    mov     0x8(%r8), %rax    # Load bottom 8 bytes to rax
    bswap   %rax
    inc     %rax
    bswap   %rax
    mov     %rax, 0x8(%r8)    # Save it back
    movups  (%r8), %xmm0      # Reload xmm0
    ret
