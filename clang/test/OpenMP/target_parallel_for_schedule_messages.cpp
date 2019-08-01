// RUN: %clang_cc1 -verify -fopenmp %s -Wuninitialized

// RUN: %clang_cc1 -verify -fopenmp-simd %s -Wuninitialized

void foo() {
}

bool foobool(int argc) {
  return argc;
}

struct S1; // expected-note {{declared here}}

template <class T, typename S, int N, int ST> // expected-note {{declared here}}
T tmain(T argc, S **argv) {
  T z;
  #pragma omp target parallel for schedule // expected-error {{expected '(' after 'schedule'}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule ( // expected-error {{expected 'static', 'dynamic', 'guided', 'auto', 'runtime', 'monotonic', 'nonmonotonic' or 'simd' in OpenMP clause 'schedule'}} expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule () // expected-error {{expected 'static', 'dynamic', 'guided', 'auto', 'runtime', 'monotonic', 'nonmonotonic' or 'simd' in OpenMP clause 'schedule'}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (auto // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (auto_dynamic // expected-error {{expected 'static', 'dynamic', 'guided', 'auto', 'runtime', 'monotonic', 'nonmonotonic' or 'simd' in OpenMP clause 'schedule'}} expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (auto,  // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (runtime, 3)  // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  // expected-error@+1 {{expected ')'}} expected-note@+1 {{to match this '('}}
  #pragma omp target parallel for schedule (guided argc
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  // expected-error@+1 2 {{argument to 'schedule' clause must be a strictly positive integer value}}
  #pragma omp target parallel for schedule (static, ST // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (dynamic, 1)) // expected-warning {{extra tokens at the end of '#pragma omp target parallel for' are ignored}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (guided, (ST > 0) ? 1 + ST : 2)
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  // expected-error@+2 2 {{directive '#pragma omp target parallel for' cannot contain more than one 'schedule' clause}}
  // expected-error@+1 {{argument to 'schedule' clause must be a strictly positive integer value}}
  #pragma omp target parallel for schedule (static, foobool(argc)), schedule (dynamic, true), schedule (guided, -5)
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (static, S) // expected-error {{'S' does not refer to a value}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  // expected-error@+1 2 {{expression must have integral or unscoped enumeration type, not 'char *'}}
  #pragma omp target parallel for schedule (guided, argv[1]=2) // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (dynamic, 1 + z)
  for (int i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  #pragma omp target parallel for schedule (static, N) // expected-error {{argument to 'schedule' clause must be a strictly positive integer value}}
  for (T i = ST; i < N; i++) argv[0][i] = argv[0][i] - argv[0][i-ST];
  return argc;
}

int main(int argc, char **argv) {
  int z;
  #pragma omp target parallel for schedule // expected-error {{expected '(' after 'schedule'}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule ( // expected-error {{expected 'static', 'dynamic', 'guided', 'auto', 'runtime', 'monotonic', 'nonmonotonic' or 'simd' in OpenMP clause 'schedule'}} expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule () // expected-error {{expected 'static', 'dynamic', 'guided', 'auto', 'runtime', 'monotonic', 'nonmonotonic' or 'simd' in OpenMP clause 'schedule'}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (auto // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (auto_dynamic // expected-error {{expected 'static', 'dynamic', 'guided', 'auto', 'runtime', 'monotonic', 'nonmonotonic' or 'simd' in OpenMP clause 'schedule'}} expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (auto,  // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (runtime, 3)  // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (guided, 4 // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (static, 2+2)) // expected-warning {{extra tokens at the end of '#pragma omp target parallel for' are ignored}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (dynamic, foobool(1) > 0 ? 1 : 2 + z)
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  // expected-error@+2 2 {{directive '#pragma omp target parallel for' cannot contain more than one 'schedule' clause}}
  // expected-error@+1 {{argument to 'schedule' clause must be a strictly positive integer value}}
  #pragma omp target parallel for schedule (guided, foobool(argc)), schedule (static, true), schedule (dynamic, -5)
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  #pragma omp target parallel for schedule (guided, S1) // expected-error {{'S1' does not refer to a value}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  // expected-error@+1 {{expression must have integral or unscoped enumeration type, not 'char *'}}
  #pragma omp target parallel for schedule (static, argv[1]=2) // expected-error {{expected ')'}} expected-note {{to match this '('}}
  for (int i = 4; i < 12; i++) argv[0][i] = argv[0][i] - argv[0][i-4];
  // expected-error@+3 {{statement after '#pragma omp target parallel for' must be a for loop}}
  // expected-note@+1 {{in instantiation of function template specialization 'tmain<int, char, -1, -2>' requested here}}
  #pragma omp target parallel for schedule(dynamic, schedule(tmain<int, char, -1, -2>(argc, argv) // expected-error 2 {{expected ')'}} expected-note 2 {{to match this '('}}
  foo();
  // expected-note@+1 {{in instantiation of function template specialization 'tmain<int, char, 1, 0>' requested here}}
  return tmain<int, char, 1, 0>(argc, argv);
}

