; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -instcombine < %s | FileCheck %s

define float @exact_inverse(float %x) {
; CHECK-LABEL: @exact_inverse(
; CHECK-NEXT:    [[DIV:%.*]] = fmul float [[X:%.*]], 1.250000e-01
; CHECK-NEXT:    ret float [[DIV]]
;
  %div = fdiv float %x, 8.0
  ret float %div
}

; Min normal float = 1.17549435E-38

define float @exact_inverse2(float %x) {
; CHECK-LABEL: @exact_inverse2(
; CHECK-NEXT:    [[DIV:%.*]] = fmul float [[X:%.*]], 0x47D0000000000000
; CHECK-NEXT:    ret float [[DIV]]
;
  %div = fdiv float %x, 0x3810000000000000
  ret float %div
}

; Max exponent = 1.70141183E+38; don't transform to multiply with denormal.

define float @exact_inverse_but_denorm(float %x) {
; CHECK-LABEL: @exact_inverse_but_denorm(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv float [[X:%.*]], 0x47E0000000000000
; CHECK-NEXT:    ret float [[DIV]]
;
  %div = fdiv float %x, 0x47E0000000000000
  ret float %div
}

; Denormal = float 1.40129846E-45; inverse can't be represented.

define float @not_exact_inverse2(float %x) {
; CHECK-LABEL: @not_exact_inverse2(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv float [[X:%.*]], 0x36A0000000000000
; CHECK-NEXT:    ret float [[DIV]]
;
  %div = fdiv float %x, 0x36A0000000000000
  ret float %div
}

; Fast math allows us to replace this fdiv.

define float @not_exact_but_allow_recip(float %x) {
; CHECK-LABEL: @not_exact_but_allow_recip(
; CHECK-NEXT:    [[DIV:%.*]] = fmul arcp float [[X:%.*]], 0x3FD5555560000000
; CHECK-NEXT:    ret float [[DIV]]
;
  %div = fdiv arcp float %x, 3.0
  ret float %div
}

; Fast math allows us to replace this fdiv, but we don't to avoid a denormal.
; TODO: What if the function attributes tell us that denormals are flushed?

define float @not_exact_but_allow_recip_but_denorm(float %x) {
; CHECK-LABEL: @not_exact_but_allow_recip_but_denorm(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv arcp float [[X:%.*]], 0x47E0000100000000
; CHECK-NEXT:    ret float [[DIV]]
;
  %div = fdiv arcp float %x, 0x47E0000100000000
  ret float %div
}

define <2 x float> @exact_inverse_splat(<2 x float> %x) {
; CHECK-LABEL: @exact_inverse_splat(
; CHECK-NEXT:    [[DIV:%.*]] = fmul <2 x float> [[X:%.*]], <float 2.500000e-01, float 2.500000e-01>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %div = fdiv <2 x float> %x, <float 4.0, float 4.0>
  ret <2 x float> %div
}

; Fast math allows us to replace this fdiv.

define <2 x float> @not_exact_but_allow_recip_splat(<2 x float> %x) {
; CHECK-LABEL: @not_exact_but_allow_recip_splat(
; CHECK-NEXT:    [[DIV:%.*]] = fmul arcp <2 x float> [[X:%.*]], <float 0x3FD5555560000000, float 0x3FD5555560000000>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %div = fdiv arcp <2 x float> %x, <float 3.0, float 3.0>
  ret <2 x float> %div
}

define <2 x float> @exact_inverse_vec(<2 x float> %x) {
; CHECK-LABEL: @exact_inverse_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fmul <2 x float> [[X:%.*]], <float 2.500000e-01, float 1.250000e-01>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %div = fdiv <2 x float> %x, <float 4.0, float 8.0>
  ret <2 x float> %div
}

define <2 x float> @not_exact_inverse_splat(<2 x float> %x) {
; CHECK-LABEL: @not_exact_inverse_splat(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv <2 x float> [[X:%.*]], <float 3.000000e+00, float 3.000000e+00>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %div = fdiv <2 x float> %x, <float 3.0, float 3.0>
  ret <2 x float> %div
}

define <2 x float> @not_exact_inverse_vec(<2 x float> %x) {
; CHECK-LABEL: @not_exact_inverse_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv <2 x float> [[X:%.*]], <float 4.000000e+00, float 3.000000e+00>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %div = fdiv <2 x float> %x, <float 4.0, float 3.0>
  ret <2 x float> %div
}

define <2 x float> @not_exact_inverse_vec_arcp(<2 x float> %x) {
; CHECK-LABEL: @not_exact_inverse_vec_arcp(
; CHECK-NEXT:    [[DIV:%.*]] = fmul arcp <2 x float> [[X:%.*]], <float 2.500000e-01, float 0x3FD5555560000000>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %div = fdiv arcp <2 x float> %x, <float 4.0, float 3.0>
  ret <2 x float> %div
}

define <2 x float> @not_exact_inverse_vec_arcp_with_undef_elt(<2 x float> %x) {
; CHECK-LABEL: @not_exact_inverse_vec_arcp_with_undef_elt(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv arcp <2 x float> [[X:%.*]], <float undef, float 3.000000e+00>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %div = fdiv arcp <2 x float> %x, <float undef, float 3.0>
  ret <2 x float> %div
}

; (X / Y) / Z --> X / (Y * Z)

define float @div_with_div_numerator(float %x, float %y, float %z) {
; CHECK-LABEL: @div_with_div_numerator(
; CHECK-NEXT:    [[TMP1:%.*]] = fmul reassoc arcp float [[Y:%.*]], [[Z:%.*]]
; CHECK-NEXT:    [[DIV2:%.*]] = fdiv reassoc arcp float [[X:%.*]], [[TMP1]]
; CHECK-NEXT:    ret float [[DIV2]]
;
  %div1 = fdiv ninf float %x, %y
  %div2 = fdiv arcp reassoc float %div1, %z
  ret float %div2
}

; Z / (X / Y) --> (Z * Y) / X

define <2 x float> @div_with_div_denominator(<2 x float> %x, <2 x float> %y, <2 x float> %z) {
; CHECK-LABEL: @div_with_div_denominator(
; CHECK-NEXT:    [[TMP1:%.*]] = fmul reassoc arcp <2 x float> [[Y:%.*]], [[Z:%.*]]
; CHECK-NEXT:    [[DIV2:%.*]] = fdiv reassoc arcp <2 x float> [[TMP1]], [[X:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV2]]
;
  %div1 = fdiv nnan <2 x float> %x, %y
  %div2 = fdiv arcp reassoc <2 x float> %z, %div1
  ret <2 x float> %div2
}

; Don't create an extra multiply if we can't eliminate the first div.

declare void @use_f32(float)

define float @div_with_div_numerator_extra_use(float %x, float %y, float %z) {
; CHECK-LABEL: @div_with_div_numerator_extra_use(
; CHECK-NEXT:    [[DIV1:%.*]] = fdiv float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[DIV2:%.*]] = fdiv fast float [[DIV1]], [[Z:%.*]]
; CHECK-NEXT:    call void @use_f32(float [[DIV1]])
; CHECK-NEXT:    ret float [[DIV2]]
;
  %div1 = fdiv float %x, %y
  %div2 = fdiv fast float %div1, %z
  call void @use_f32(float %div1)
  ret float %div2
}

define float @div_with_div_denominator_extra_use(float %x, float %y, float %z) {
; CHECK-LABEL: @div_with_div_denominator_extra_use(
; CHECK-NEXT:    [[DIV1:%.*]] = fdiv float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[DIV2:%.*]] = fdiv fast float [[Z:%.*]], [[DIV1]]
; CHECK-NEXT:    call void @use_f32(float [[DIV1]])
; CHECK-NEXT:    ret float [[DIV2]]
;
  %div1 = fdiv float %x, %y
  %div2 = fdiv fast float %z, %div1
  call void @use_f32(float %div1)
  ret float %div2
}

define float @fneg_fneg(float %x, float %y) {
; CHECK-LABEL: @fneg_fneg(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %x.fneg = fsub float -0.0, %x
  %y.fneg = fsub float -0.0, %y
  %div = fdiv float %x.fneg, %y.fneg
  ret float %div
}

define float @unary_fneg_unary_fneg(float %x, float %y) {
; CHECK-LABEL: @unary_fneg_unary_fneg(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %x.fneg = fneg float %x
  %y.fneg = fneg float %y
  %div = fdiv float %x.fneg, %y.fneg
  ret float %div
}

define float @unary_fneg_fneg(float %x, float %y) {
; CHECK-LABEL: @unary_fneg_fneg(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %x.fneg = fneg float %x
  %y.fneg = fsub float -0.0, %y
  %div = fdiv float %x.fneg, %y.fneg
  ret float %div
}

define float @fneg_unary_fneg(float %x, float %y) {
; CHECK-LABEL: @fneg_unary_fneg(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %x.fneg = fsub float -0.0, %x
  %y.fneg = fneg float %y
  %div = fdiv float %x.fneg, %y.fneg
  ret float %div
}

; The test above shows that no FMF are needed, but show that we are not dropping FMF.

define float @fneg_fneg_fast(float %x, float %y) {
; CHECK-LABEL: @fneg_fneg_fast(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv fast float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %x.fneg = fsub float -0.0, %x
  %y.fneg = fsub float -0.0, %y
  %div = fdiv fast float %x.fneg, %y.fneg
  ret float %div
}

define float @unary_fneg_unary_fneg_fast(float %x, float %y) {
; CHECK-LABEL: @unary_fneg_unary_fneg_fast(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv fast float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %x.fneg = fneg float %x
  %y.fneg = fneg float %y
  %div = fdiv fast float %x.fneg, %y.fneg
  ret float %div
}

define <2 x float> @fneg_fneg_vec(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @fneg_fneg_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv <2 x float> [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %xneg = fsub <2 x float> <float -0.0, float -0.0>, %x
  %yneg = fsub <2 x float> <float -0.0, float -0.0>, %y
  %div = fdiv <2 x float> %xneg, %yneg
  ret <2 x float> %div
}

define <2 x float> @unary_fneg_unary_fneg_vec(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @unary_fneg_unary_fneg_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv <2 x float> [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %xneg = fneg <2 x float> %x
  %yneg = fneg <2 x float> %y
  %div = fdiv <2 x float> %xneg, %yneg
  ret <2 x float> %div
}

define <2 x float> @fneg_unary_fneg_vec(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @fneg_unary_fneg_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv <2 x float> [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %xneg = fsub <2 x float> <float -0.0, float -0.0>, %x
  %yneg = fneg <2 x float> %y
  %div = fdiv <2 x float> %xneg, %yneg
  ret <2 x float> %div
}

define <2 x float> @unary_fneg_fneg_vec(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @unary_fneg_fneg_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv <2 x float> [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %xneg = fneg <2 x float> %x
  %yneg = fsub <2 x float> <float -0.0, float -0.0>, %y
  %div = fdiv <2 x float> %xneg, %yneg
  ret <2 x float> %div
}

define <2 x float> @fneg_fneg_vec_undef_elts(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @fneg_fneg_vec_undef_elts(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv <2 x float> [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %xneg = fsub <2 x float> <float undef, float -0.0>, %x
  %yneg = fsub <2 x float> <float -0.0, float undef>, %y
  %div = fdiv <2 x float> %xneg, %yneg
  ret <2 x float> %div
}

define float @fneg_dividend_constant_divisor(float %x) {
; CHECK-LABEL: @fneg_dividend_constant_divisor(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv nsz float [[X:%.*]], -3.000000e+00
; CHECK-NEXT:    ret float [[DIV]]
;
  %neg = fsub float -0.0, %x
  %div = fdiv nsz float %neg, 3.0
  ret  float %div
}

define float @unary_fneg_dividend_constant_divisor(float %x) {
; CHECK-LABEL: @unary_fneg_dividend_constant_divisor(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv nsz float [[X:%.*]], -3.000000e+00
; CHECK-NEXT:    ret float [[DIV]]
;
  %neg = fneg float %x
  %div = fdiv nsz float %neg, 3.0
  ret  float %div
}

define float @fneg_divisor_constant_dividend(float %x) {
; CHECK-LABEL: @fneg_divisor_constant_dividend(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv nnan float 3.000000e+00, [[X:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %neg = fsub float -0.0, %x
  %div = fdiv nnan float -3.0, %neg
  ret float %div
}

define float @unary_fneg_divisor_constant_dividend(float %x) {
; CHECK-LABEL: @unary_fneg_divisor_constant_dividend(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv nnan float 3.000000e+00, [[X:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %neg = fneg float %x
  %div = fdiv nnan float -3.0, %neg
  ret float %div
}

define <2 x float> @fneg_dividend_constant_divisor_vec(<2 x float> %x) {
; CHECK-LABEL: @fneg_dividend_constant_divisor_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv ninf <2 x float> [[X:%.*]], <float -3.000000e+00, float 8.000000e+00>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %neg = fsub <2 x float> <float -0.0, float -0.0>, %x
  %div = fdiv ninf <2 x float> %neg, <float 3.0, float -8.0>
  ret <2 x float> %div
}

define <2 x float> @unary_fneg_dividend_constant_divisor_vec(<2 x float> %x) {
; CHECK-LABEL: @unary_fneg_dividend_constant_divisor_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv ninf <2 x float> [[X:%.*]], <float -3.000000e+00, float 8.000000e+00>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %neg = fneg <2 x float> %x
  %div = fdiv ninf <2 x float> %neg, <float 3.0, float -8.0>
  ret <2 x float> %div
}

define <2 x float> @fneg_dividend_constant_divisor_vec_undef_elt(<2 x float> %x) {
; CHECK-LABEL: @fneg_dividend_constant_divisor_vec_undef_elt(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv ninf <2 x float> [[X:%.*]], <float -3.000000e+00, float 8.000000e+00>
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %neg = fsub <2 x float> <float undef, float -0.0>, %x
  %div = fdiv ninf <2 x float> %neg, <float 3.0, float -8.0>
  ret <2 x float> %div
}

define <2 x float> @fneg_divisor_constant_dividend_vec(<2 x float> %x) {
; CHECK-LABEL: @fneg_divisor_constant_dividend_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv afn <2 x float> <float 3.000000e+00, float -5.000000e+00>, [[X:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %neg = fsub <2 x float> <float -0.0, float -0.0>, %x
  %div = fdiv afn <2 x float> <float -3.0, float 5.0>, %neg
  ret <2 x float> %div
}

define <2 x float> @unary_fneg_divisor_constant_dividend_vec(<2 x float> %x) {
; CHECK-LABEL: @unary_fneg_divisor_constant_dividend_vec(
; CHECK-NEXT:    [[DIV:%.*]] = fdiv afn <2 x float> <float 3.000000e+00, float -5.000000e+00>, [[X:%.*]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %neg = fneg <2 x float> %x
  %div = fdiv afn <2 x float> <float -3.0, float 5.0>, %neg
  ret <2 x float> %div
}


; X / (X * Y) --> 1.0 / Y

define float @div_factor(float %x, float %y) {
; CHECK-LABEL: @div_factor(
; CHECK-NEXT:    [[D:%.*]] = fdiv reassoc nnan float 1.000000e+00, [[Y:%.*]]
; CHECK-NEXT:    ret float [[D]]
;
  %m = fmul float %x, %y
  %d = fdiv nnan reassoc float %x, %m
  ret float %d;
}

; We can't do the transform without 'nnan' because if x is NAN and y is a number, this should return NAN.

define float @div_factor_too_strict(float %x, float %y) {
; CHECK-LABEL: @div_factor_too_strict(
; CHECK-NEXT:    [[M:%.*]] = fmul float [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[D:%.*]] = fdiv reassoc float [[X]], [[M]]
; CHECK-NEXT:    ret float [[D]]
;
  %m = fmul float %x, %y
  %d = fdiv reassoc float %x, %m
  ret float %d
}

; Commute, verify vector types, and show that we are not dropping extra FMF.
; X / (Y * X) --> 1.0 / Y

define <2 x float> @div_factor_commute(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @div_factor_commute(
; CHECK-NEXT:    [[D:%.*]] = fdiv reassoc nnan ninf nsz <2 x float> <float 1.000000e+00, float 1.000000e+00>, [[Y:%.*]]
; CHECK-NEXT:    ret <2 x float> [[D]]
;
  %m = fmul <2 x float> %y, %x
  %d = fdiv nnan ninf nsz reassoc <2 x float> %x, %m
  ret <2 x float> %d
}

; C1/(X*C2) => (C1/C2) / X

define <2 x float> @div_constant_dividend1(<2 x float> %x) {
; CHECK-LABEL: @div_constant_dividend1(
; CHECK-NEXT:    [[T2:%.*]] = fdiv reassoc arcp <2 x float> <float 5.000000e+00, float 1.000000e+00>, [[X:%.*]]
; CHECK-NEXT:    ret <2 x float> [[T2]]
;
  %t1 = fmul <2 x float> %x, <float 3.0e0, float 7.0e0>
  %t2 = fdiv arcp reassoc <2 x float> <float 15.0e0, float 7.0e0>, %t1
  ret <2 x float> %t2
}

define <2 x float> @div_constant_dividend1_arcp_only(<2 x float> %x) {
; CHECK-LABEL: @div_constant_dividend1_arcp_only(
; CHECK-NEXT:    [[T1:%.*]] = fmul <2 x float> [[X:%.*]], <float 3.000000e+00, float 7.000000e+00>
; CHECK-NEXT:    [[T2:%.*]] = fdiv arcp <2 x float> <float 1.500000e+01, float 7.000000e+00>, [[T1]]
; CHECK-NEXT:    ret <2 x float> [[T2]]
;
  %t1 = fmul <2 x float> %x, <float 3.0e0, float 7.0e0>
  %t2 = fdiv arcp <2 x float> <float 15.0e0, float 7.0e0>, %t1
  ret <2 x float> %t2
}

; C1/(X/C2) => (C1*C2) / X

define <2 x float> @div_constant_dividend2(<2 x float> %x) {
; CHECK-LABEL: @div_constant_dividend2(
; CHECK-NEXT:    [[T2:%.*]] = fdiv reassoc arcp <2 x float> <float 4.500000e+01, float 4.900000e+01>, [[X:%.*]]
; CHECK-NEXT:    ret <2 x float> [[T2]]
;
  %t1 = fdiv <2 x float> %x, <float 3.0e0, float -7.0e0>
  %t2 = fdiv arcp reassoc <2 x float> <float 15.0e0, float -7.0e0>, %t1
  ret <2 x float> %t2
}

define <2 x float> @div_constant_dividend2_reassoc_only(<2 x float> %x) {
; CHECK-LABEL: @div_constant_dividend2_reassoc_only(
; CHECK-NEXT:    [[T1:%.*]] = fdiv <2 x float> [[X:%.*]], <float 3.000000e+00, float -7.000000e+00>
; CHECK-NEXT:    [[T2:%.*]] = fdiv reassoc <2 x float> <float 1.500000e+01, float -7.000000e+00>, [[T1]]
; CHECK-NEXT:    ret <2 x float> [[T2]]
;
  %t1 = fdiv <2 x float> %x, <float 3.0e0, float -7.0e0>
  %t2 = fdiv reassoc <2 x float> <float 15.0e0, float -7.0e0>, %t1
  ret <2 x float> %t2
}

; C1/(C2/X) => (C1/C2) * X
; This tests the combination of 2 folds: (C1 * X) / C2 --> (C1 / C2) * X

define <2 x float> @div_constant_dividend3(<2 x float> %x) {
; CHECK-LABEL: @div_constant_dividend3(
; CHECK-NEXT:    [[TMP1:%.*]] = fmul reassoc arcp <2 x float> [[X:%.*]], <float 1.500000e+01, float -7.000000e+00>
; CHECK-NEXT:    [[T2:%.*]] = fmul reassoc arcp <2 x float> [[TMP1]], <float 0x3FD5555560000000, float 0x3FC24924A0000000>
; CHECK-NEXT:    ret <2 x float> [[T2]]
;
  %t1 = fdiv <2 x float> <float 3.0e0, float 7.0e0>, %x
  %t2 = fdiv arcp reassoc <2 x float> <float 15.0e0, float -7.0e0>, %t1
  ret <2 x float> %t2
}

define double @fdiv_fneg1(double %x, double %y) {
; CHECK-LABEL: @fdiv_fneg1(
; CHECK-NEXT:    [[TMP1:%.*]] = fdiv double [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[DIV:%.*]] = fsub double -0.000000e+00, [[TMP1]]
; CHECK-NEXT:    ret double [[DIV]]
;
  %neg = fsub double -0.0, %x
  %div = fdiv double %neg, %y
  ret double %div
}

define double @fdiv_unary_fneg1(double %x, double %y) {
; CHECK-LABEL: @fdiv_unary_fneg1(
; CHECK-NEXT:    [[TMP1:%.*]] = fdiv double [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[DIV:%.*]] = fsub double -0.000000e+00, [[TMP1]]
; CHECK-NEXT:    ret double [[DIV]]
;
  %neg = fneg double %x
  %div = fdiv double %neg, %y
  ret double %div
}

define <2 x float> @fdiv_fneg2(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @fdiv_fneg2(
; CHECK-NEXT:    [[TMP1:%.*]] = fdiv <2 x float> [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[DIV:%.*]] = fsub <2 x float> <float -0.000000e+00, float -0.000000e+00>, [[TMP1]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %neg = fsub <2 x float> <float -0.0, float -0.0>, %x
  %div = fdiv <2 x float> %y, %neg
  ret <2 x float> %div
}

define <2 x float> @fdiv_unary_fneg2(<2 x float> %x, <2 x float> %y) {
; CHECK-LABEL: @fdiv_unary_fneg2(
; CHECK-NEXT:    [[TMP1:%.*]] = fdiv <2 x float> [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[DIV:%.*]] = fsub <2 x float> <float -0.000000e+00, float -0.000000e+00>, [[TMP1]]
; CHECK-NEXT:    ret <2 x float> [[DIV]]
;
  %neg = fneg <2 x float> %x
  %div = fdiv <2 x float> %y, %neg
  ret <2 x float> %div
}

define float @fdiv_fneg1_extra_use(float %x, float %y) {
; CHECK-LABEL: @fdiv_fneg1_extra_use(
; CHECK-NEXT:    [[NEG:%.*]] = fsub float -0.000000e+00, [[X:%.*]]
; CHECK-NEXT:    call void @use_f32(float [[NEG]])
; CHECK-NEXT:    [[DIV:%.*]] = fdiv float [[NEG]], [[Y:%.*]]
; CHECK-NEXT:    ret float [[DIV]]
;
  %neg = fsub float -0.0, %x
  call void @use_f32(float %neg)
  %div = fdiv float %neg, %y
  ret float %div
}
