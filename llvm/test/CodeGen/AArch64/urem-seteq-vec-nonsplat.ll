; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=aarch64-unknown-linux-gnu < %s | FileCheck %s

; At the moment, BuildUREMEqFold does not handle nonsplat vectors.

define <4 x i32> @test_urem_odd_div(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_odd_div:
; CHECK:       // %bb.0:
; CHECK-NEXT:    adrp x8, .LCPI0_0
; CHECK-NEXT:    ldr q1, [x8, :lo12:.LCPI0_0]
; CHECK-NEXT:    adrp x8, .LCPI0_1
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI0_1]
; CHECK-NEXT:    adrp x8, .LCPI0_2
; CHECK-NEXT:    umull2 v3.2d, v0.4s, v1.4s
; CHECK-NEXT:    umull v1.2d, v0.2s, v1.2s
; CHECK-NEXT:    uzp2 v1.4s, v1.4s, v3.4s
; CHECK-NEXT:    sub v3.4s, v0.4s, v1.4s
; CHECK-NEXT:    umull2 v4.2d, v3.4s, v2.4s
; CHECK-NEXT:    umull v2.2d, v3.2s, v2.2s
; CHECK-NEXT:    ldr q3, [x8, :lo12:.LCPI0_2]
; CHECK-NEXT:    adrp x8, .LCPI0_3
; CHECK-NEXT:    uzp2 v2.4s, v2.4s, v4.4s
; CHECK-NEXT:    ldr q4, [x8, :lo12:.LCPI0_3]
; CHECK-NEXT:    neg v3.4s, v3.4s
; CHECK-NEXT:    add v1.4s, v2.4s, v1.4s
; CHECK-NEXT:    ushl v1.4s, v1.4s, v3.4s
; CHECK-NEXT:    mls v0.4s, v1.4s, v4.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, #0
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 3, i32 5, i32 7, i32 9>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 0, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_even_div(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_even_div:
; CHECK:       // %bb.0:
; CHECK-NEXT:    adrp x8, .LCPI1_0
; CHECK-NEXT:    ldr q1, [x8, :lo12:.LCPI1_0]
; CHECK-NEXT:    adrp x8, .LCPI1_1
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI1_1]
; CHECK-NEXT:    adrp x8, .LCPI1_2
; CHECK-NEXT:    ldr q3, [x8, :lo12:.LCPI1_2]
; CHECK-NEXT:    neg v1.4s, v1.4s
; CHECK-NEXT:    adrp x8, .LCPI1_3
; CHECK-NEXT:    ushl v1.4s, v0.4s, v1.4s
; CHECK-NEXT:    umull2 v4.2d, v1.4s, v2.4s
; CHECK-NEXT:    umull v1.2d, v1.2s, v2.2s
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI1_3]
; CHECK-NEXT:    uzp2 v1.4s, v1.4s, v4.4s
; CHECK-NEXT:    neg v3.4s, v3.4s
; CHECK-NEXT:    ushl v1.4s, v1.4s, v3.4s
; CHECK-NEXT:    mls v0.4s, v1.4s, v2.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, #0
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 6, i32 10, i32 12, i32 14>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 0, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_pow2(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_pow2:
; CHECK:       // %bb.0:
; CHECK-NEXT:    adrp x8, .LCPI2_0
; CHECK-NEXT:    ldr q1, [x8, :lo12:.LCPI2_0]
; CHECK-NEXT:    adrp x8, .LCPI2_1
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI2_1]
; CHECK-NEXT:    adrp x8, .LCPI2_2
; CHECK-NEXT:    ldr q3, [x8, :lo12:.LCPI2_2]
; CHECK-NEXT:    umull2 v4.2d, v0.4s, v1.4s
; CHECK-NEXT:    umull v1.2d, v0.2s, v1.2s
; CHECK-NEXT:    uzp2 v1.4s, v1.4s, v4.4s
; CHECK-NEXT:    neg v2.4s, v2.4s
; CHECK-NEXT:    ushl v1.4s, v1.4s, v2.4s
; CHECK-NEXT:    mls v0.4s, v1.4s, v3.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, #0
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 6, i32 10, i32 12, i32 16>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 0, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_one(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_one:
; CHECK:       // %bb.0:
; CHECK-NEXT:    adrp x8, .LCPI3_0
; CHECK-NEXT:    ldr q1, [x8, :lo12:.LCPI3_0]
; CHECK-NEXT:    adrp x8, .LCPI3_1
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI3_1]
; CHECK-NEXT:    adrp x8, .LCPI3_2
; CHECK-NEXT:    ldr q3, [x8, :lo12:.LCPI3_2]
; CHECK-NEXT:    neg v1.4s, v1.4s
; CHECK-NEXT:    adrp x8, .LCPI3_3
; CHECK-NEXT:    ushl v1.4s, v0.4s, v1.4s
; CHECK-NEXT:    umull2 v4.2d, v1.4s, v2.4s
; CHECK-NEXT:    umull v1.2d, v1.2s, v2.2s
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI3_3]
; CHECK-NEXT:    adrp x8, .LCPI3_4
; CHECK-NEXT:    uzp2 v1.4s, v1.4s, v4.4s
; CHECK-NEXT:    ldr q4, [x8, :lo12:.LCPI3_4]
; CHECK-NEXT:    neg v3.4s, v3.4s
; CHECK-NEXT:    ushl v1.4s, v1.4s, v3.4s
; CHECK-NEXT:    bsl v2.16b, v0.16b, v1.16b
; CHECK-NEXT:    mls v0.4s, v2.4s, v4.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, #0
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 6, i32 1, i32 12, i32 14>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 0, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_comp(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_comp:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, #52429
; CHECK-NEXT:    movk w8, #52428, lsl #16
; CHECK-NEXT:    adrp x9, .LCPI4_0
; CHECK-NEXT:    dup v2.4s, w8
; CHECK-NEXT:    ldr q3, [x9, :lo12:.LCPI4_0]
; CHECK-NEXT:    umull2 v4.2d, v0.4s, v2.4s
; CHECK-NEXT:    umull v2.2d, v0.2s, v2.2s
; CHECK-NEXT:    uzp2 v2.4s, v2.4s, v4.4s
; CHECK-NEXT:    movi v1.4s, #5
; CHECK-NEXT:    ushr v2.4s, v2.4s, #2
; CHECK-NEXT:    mls v0.4s, v2.4s, v1.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, v3.4s
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 5, i32 5, i32 5, i32 5>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 0, i32 1, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_both(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_both:
; CHECK:       // %bb.0:
; CHECK-NEXT:    adrp x8, .LCPI5_0
; CHECK-NEXT:    ldr q1, [x8, :lo12:.LCPI5_0]
; CHECK-NEXT:    adrp x8, .LCPI5_1
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI5_1]
; CHECK-NEXT:    adrp x8, .LCPI5_2
; CHECK-NEXT:    ldr q3, [x8, :lo12:.LCPI5_2]
; CHECK-NEXT:    umull2 v4.2d, v0.4s, v1.4s
; CHECK-NEXT:    umull v1.2d, v0.2s, v1.2s
; CHECK-NEXT:    uzp2 v1.4s, v1.4s, v4.4s
; CHECK-NEXT:    ushr v1.4s, v1.4s, #2
; CHECK-NEXT:    mls v0.4s, v1.4s, v2.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, v3.4s
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 6, i32 5, i32 6, i32 5>
  %cmp = icmp eq <4 x i32> %urem, <i32 1, i32 0, i32 1, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_div_undef(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_div_undef:
; CHECK:       // %bb.0:
; CHECK-NEXT:    movi v0.2d, #0000000000000000
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 5, i32 5, i32 undef, i32 5>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 0, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_comp_undef(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_comp_undef:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, #52429
; CHECK-NEXT:    movk w8, #52428, lsl #16
; CHECK-NEXT:    dup v2.4s, w8
; CHECK-NEXT:    umull2 v3.2d, v0.4s, v2.4s
; CHECK-NEXT:    umull v2.2d, v0.2s, v2.2s
; CHECK-NEXT:    uzp2 v2.4s, v2.4s, v3.4s
; CHECK-NEXT:    movi v1.4s, #5
; CHECK-NEXT:    ushr v2.4s, v2.4s, #2
; CHECK-NEXT:    mls v0.4s, v2.4s, v1.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, #0
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 5, i32 5, i32 5, i32 5>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 undef, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_both_undef(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_both_undef:
; CHECK:       // %bb.0:
; CHECK-NEXT:    movi v0.2d, #0000000000000000
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 5, i32 5, i32 undef, i32 5>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 undef, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}

define <4 x i32> @test_urem_div_even_odd(<4 x i32> %X) nounwind readnone {
; CHECK-LABEL: test_urem_div_even_odd:
; CHECK:       // %bb.0:
; CHECK-NEXT:    adrp x8, .LCPI9_0
; CHECK-NEXT:    ldr q1, [x8, :lo12:.LCPI9_0]
; CHECK-NEXT:    adrp x8, .LCPI9_1
; CHECK-NEXT:    ldr q2, [x8, :lo12:.LCPI9_1]
; CHECK-NEXT:    umull2 v3.2d, v0.4s, v1.4s
; CHECK-NEXT:    umull v1.2d, v0.2s, v1.2s
; CHECK-NEXT:    uzp2 v1.4s, v1.4s, v3.4s
; CHECK-NEXT:    ushr v1.4s, v1.4s, #2
; CHECK-NEXT:    mls v0.4s, v1.4s, v2.4s
; CHECK-NEXT:    cmeq v0.4s, v0.4s, #0
; CHECK-NEXT:    movi v1.4s, #1
; CHECK-NEXT:    and v0.16b, v0.16b, v1.16b
; CHECK-NEXT:    ret
  %urem = urem <4 x i32> %X, <i32 5, i32 5, i32 6, i32 6>
  %cmp = icmp eq <4 x i32> %urem, <i32 0, i32 0, i32 0, i32 0>
  %ret = zext <4 x i1> %cmp to <4 x i32>
  ret <4 x i32> %ret
}
