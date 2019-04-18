; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S < %s -slp-vectorizer -slp-max-reg-size=128 -slp-min-reg-size=128 | FileCheck %s

declare void @llvm.sideeffect()

; SLP vectorization across a @llvm.sideeffect.

define void @test(float* %p) {
; CHECK-LABEL: @test(
; CHECK-NEXT:    [[P0:%.*]] = getelementptr float, float* [[P:%.*]], i64 0
; CHECK-NEXT:    [[P1:%.*]] = getelementptr float, float* [[P]], i64 1
; CHECK-NEXT:    [[P2:%.*]] = getelementptr float, float* [[P]], i64 2
; CHECK-NEXT:    [[P3:%.*]] = getelementptr float, float* [[P]], i64 3
; CHECK-NEXT:    call void @llvm.sideeffect()
; CHECK-NEXT:    [[TMP1:%.*]] = bitcast float* [[P0]] to <4 x float>*
; CHECK-NEXT:    [[TMP2:%.*]] = load <4 x float>, <4 x float>* [[TMP1]], align 4
; CHECK-NEXT:    call void @llvm.sideeffect()
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast float* [[P0]] to <4 x float>*
; CHECK-NEXT:    store <4 x float> [[TMP2]], <4 x float>* [[TMP3]], align 4
; CHECK-NEXT:    ret void
;
  %p0 = getelementptr float, float* %p, i64 0
  %p1 = getelementptr float, float* %p, i64 1
  %p2 = getelementptr float, float* %p, i64 2
  %p3 = getelementptr float, float* %p, i64 3
  %l0 = load float, float* %p0
  %l1 = load float, float* %p1
  %l2 = load float, float* %p2
  call void @llvm.sideeffect()
  %l3 = load float, float* %p3
  store float %l0, float* %p0
  call void @llvm.sideeffect()
  store float %l1, float* %p1
  store float %l2, float* %p2
  store float %l3, float* %p3
  ret void
}
