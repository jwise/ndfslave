void main() {
  unsigned long long cs[256] = {0};
  unsigned long long pops[256] = {0};
  unsigned long long expops[256] = {0};
  unsigned long long sum = 0;
  unsigned char c[2048];
  int rv;
  int i;
  
  while ((rv = read(0, c, 2048)))
  {
    for (i = 0; i < rv; i++) {
      cs[c[i]]++;
      c[i] = ((c[i] & 0xAA) >> 1) + (c[i] & 0x55);
      c[i] = ((c[i] & 0xCC) >> 2) + (c[i] & 0x33);
      c[i] = ((c[i] & 0xF0) >> 4) + (c[i] & 0x0F);
      pops[c[i]]++;
    }
    sum += rv;
  }
  
  printf("total bytes: %llu\n", sum);
  
  for (i = 0; i < 256; i++)
  {
    float f = (double)cs[i] / (double)sum;
    printf("%02x: %1.3f\n", i, f);
  }
  
  for (i = 0; i < 256; i++) {
    c[i] = i;
    c[i] = ((c[i] & 0xAA) >> 1) + (c[i] & 0x55);
    c[i] = ((c[i] & 0xCC) >> 2) + (c[i] & 0x33);
    c[i] = ((c[i] & 0xF0) >> 4) + (c[i] & 0x0F);
    expops[c[i]]++;
  }
  
  for (i = 0; i < 9; i++)
  {
    float f = (double)pops[i] / (double)sum;
    float f2 = (double)expops[i] / 256.0;
    
    printf("pop%d: %1.3f, ex %1.3f, norm %1.3f\n", i, f, f2, f/f2);
    pops[i] = 0;
  }
}