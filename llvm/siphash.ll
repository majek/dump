target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define private hidden {<2 x i64>, <2 x i64>}
       @half_round_13(<2 x i64> %ac0, <2 x i64> %bd0)
       alwaysinline readonly nounwind {
       %ac2 = add <2 x i64> %ac0, %bd0

       %b1  = extractelement <2 x i64> %bd0, i32 0
       %d1  = extractelement <2 x i64> %bd0, i32 1
       %b2l = shl i64 %b1, 13
       %b2r = lshr i64 %b1, 51
       %b2  = xor i64 %b2l, %b2r
       %d2l = shl i64 %d1, 16
       %d2r = lshr i64 %d1, 48
       %d2  = xor i64 %d2l, %d2r

       %bd2a = insertelement <2 x i64> undef, i64 %b2, i32 0
       %bd2  = insertelement <2 x i64> %bd2a, i64 %d2, i32 1

       %bd3 = xor <2 x i64> %bd2, %ac2

       %ac2_4i = bitcast <2 x i64> %ac2 to <4 x i32>
       %ca0_4i = shufflevector <4 x i32> %ac2_4i, <4 x i32> undef,
                               <4 x i32> <i32 2, i32 3, i32 1, i32 0>
       %ca0 = bitcast <4 x i32> %ca0_4i to <2 x i64>

       ; %bd3
       ; %ca0

       %r0 = insertvalue {<2 x i64>, <2 x i64>} undef, <2 x i64> %bd3, 0
       %r1 = insertvalue {<2 x i64>, <2 x i64>} %r0, <2 x i64> %ca0, 1
       ret {<2 x i64>, <2 x i64>} %r1
}

define private hidden {<2 x i64>, <2 x i64>}
       @half_round_17(<2 x i64> %ac0, <2 x i64> %bd0)
       alwaysinline readonly nounwind {
       %ac2 = add <2 x i64> %ac0, %bd0

       %b1  = extractelement <2 x i64> %bd0, i32 0
       %d1  = extractelement <2 x i64> %bd0, i32 1
       %b2l = shl i64 %b1, 17
       %b2r = lshr i64 %b1, 47
       %b2  = xor i64 %b2l, %b2r
       %d2l = shl i64 %d1, 21
       %d2r = lshr i64 %d1, 43
       %d2  = xor i64 %d2l, %d2r

       %bd2a = insertelement <2 x i64> undef, i64 %b2, i32 0
       %bd2  = insertelement <2 x i64> %bd2a, i64 %d2, i32 1

       %bd3 = xor <2 x i64> %bd2, %ac2

       %ac2_4i = bitcast <2 x i64> %ac2 to <4 x i32>
       %ca0_4i = shufflevector <4 x i32> %ac2_4i, <4 x i32> undef,
                               <4 x i32> <i32 2, i32 3, i32 1, i32 0>
       %ca0 = bitcast <4 x i32> %ca0_4i to <2 x i64>

       %r0 = insertvalue {<2 x i64>, <2 x i64>} undef, <2 x i64> %bd3, 0
       %r1 = insertvalue {<2 x i64>, <2 x i64>} %r0, <2 x i64> %ca0, 1
       ret {<2 x i64>, <2 x i64>} %r1
}



define <4 x i64> @siphash_round(i64 %a, i64 %b, i64 %c, i64 %d)
       readnone nounwind uwtable {
       %ac0 = insertelement <2 x i64> undef, i64 %a, i32 0
       %ac1 = insertelement <2 x i64> %ac0,  i64 %c, i32 1
       %bd0 = insertelement <2 x i64> undef, i64 %b, i32 0
       %bd1 = insertelement <2 x i64> %bd0,  i64 %d, i32 1

       %r0 = call {<2 x i64>, <2 x i64>} @half_round_13(<2 x i64> %ac1, <2 x i64> %bd1)

       %bd2  = extractvalue {<2 x i64>, <2 x i64>} %r0, 0
       %ac2  = extractvalue {<2 x i64>, <2 x i64>} %r0, 1

       %r1 = call {<2 x i64>, <2 x i64>} @half_round_17(<2 x i64> %ac2, <2 x i64> %bd2)

       %bd3  = extractvalue {<2 x i64>, <2 x i64>} %r1, 0
       %ac3  = extractvalue {<2 x i64>, <2 x i64>} %r1, 1

       %b4  = extractelement <2 x i64> %bd3, i32 0
       %d4  = extractelement <2 x i64> %bd3, i32 1

       %a4  = extractelement <2 x i64> %ac3, i32 0
       %c4  = extractelement <2 x i64> %ac3, i32 1

       %x0  = insertelement <4 x i64> undef, i64 %a4, i32 0
       %x1  = insertelement <4 x i64> %x0, i64 %b4, i32 1
       %x2  = insertelement <4 x i64> %x1, i64 %c4, i32 2
       %x3  = insertelement <4 x i64> %x2, i64 %d4, i32 3
       ret <4 x i64> %x3
}
