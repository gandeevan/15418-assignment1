

export void sum(uniform int a[], uniform int b[], uniform int c[], uniform int n) {
  for(int i=0; i<n; i+=programCount) {
    int idx = i + programIndex;
    c[idx] = a[idx] + b[idx];
  }
}