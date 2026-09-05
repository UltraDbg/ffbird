#ifndef BIONIC_STRESS_BIONIC_STRESS_H
#define BIONIC_STRESS_BIONIC_STRESS_H
#ifdef __cplusplus
extern "C" {
#endif
int bionic_stress_run();
double bionic_stress_math(double x);
void bionic_stress_libc();
int bionic_stress_tls();
#ifdef __cplusplus
}
#endif
#endif
