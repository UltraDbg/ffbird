#include <android/log.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <locale.h>
#include <wchar.h>

// Bionic-specific headers that force Bionic ABI
#include <sys/cdefs.h>

#define LOG_TAG "bionic_stress"

// TLS test — Bionic TLS via __thread / pthread_key
static __thread int tls_counter = 0;
static pthread_key_t tls_key;
static pthread_once_t tls_once = PTHREAD_ONCE_INIT;

static void tls_destructor(void* p) {
    free(p);
}

static void tls_init_once() {
    pthread_key_create(&tls_key, tls_destructor);
}

// Mutex/cond shared test — hybris must translate Android futex layout
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t g_recursive_mutex;
static int g_shared_value = 0;
static volatile int g_threads_done = 0;

// Math heavy — forces many libm symbols (298 map) to be needed
static double math_heavy(double x) {
    // Use ~40 libm symbols to force DT_NEEDED libm.so completeness
    double a = sin(x);
    double b = cos(x);
    double c = tan(x);
    double d = asin(fmod(x, 1.0));
    double e = acos(fmod(x, 1.0));
    double f = atan2(a, b);
    double g = sinh(x);
    double h = cosh(x);
    double i = tanh(x);
    double j = exp(x);
    double k = exp2(x);
    double l = expm1(x);
    double m = log(fabs(x) + 1.0);
    double n = log10(fabs(x) + 1.0);
    double o = log2(fabs(x) + 1.0);
    double p = log1p(fabs(x));
    double q = pow(fabs(x) + 1.0, 0.5);
    double r = sqrt(fabs(x) + 1.0);
    double s = cbrt(x);
    double t = hypot(x, 1.0);
    double u = erf(x);
    double v = erfc(x);
    double w = tgamma(fabs(x) + 1.0);
    double x1 = lgamma(fabs(x) + 1.0);
    double y = fma(x, 2.0, 1.0);
    double z = fmax(x, 0.5);
    double ab = fmin(x, 0.5);
    double ac = ceil(x);
    double ad = floor(x);
    double ae = round(x);
    double af = trunc(x);
    double ag = nearbyint(x);
    double ah = rint(x);
    double ai = fabs(x);
    double aj = copysign(x, -1.0);
    // Complex gap: __signbit / significand already via signbit
    volatile double sink = a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p+q+r+s+t+u+v+w+x1+y+z+ab+ac+ad+ae+af+ag+ah+ai+aj;
    (void)sink;
    return a+b;
}

static float mathf_heavy(float x) {
    float a = sinf(x);
    float b = cosf(x);
    float c = powf(x, 2.0f);
    float d = expf(x);
    float e = logf(fabsf(x)+1);
    float f = sqrtf(fabsf(x)+1);
    volatile float sink = a+b+c+d+e+f;
    (void)sink;
    return a;
}

// String/memory heavy — forces libc symbols (strlcpy, mem* etc. that libc-shim must cover)
static void libc_heavy() {
    char* p = (char*)malloc(256);
    char* q = (char*)calloc(1, 256);
    char buf[128];
    strcpy(p, "bionic");
    strcat(p, "_stress");
    strncpy(buf, p, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    memcmp(p, q, 10);
    memset(q, 0xAB, 64);
    memcpy(buf, p, strlen(p)+1);
    strlcpy(buf, p, sizeof(buf));
    strlcat(buf, "_ok", sizeof(buf));
    volatile size_t l = strlen(buf);
    (void)l;
    free(p);
    free(q);
    // file I/O
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd >= 0) {
        char tmp[64];
        ssize_t n = read(fd, tmp, sizeof(tmp)-1);
        if (n > 0) tmp[n]='\0';
        close(fd);
    }
    // time
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    struct timeval tv;
    gettimeofday(&tv, nullptr);
}

// Thread entry — exercises Bionic pthread + TLS + math + log
static void* thread_entry(void* arg) {
    int id = *(int*)arg;
    tls_counter = id * 10;
    pthread_once(&tls_once, tls_init_once);
    int* tls_data = (int*)pthread_getspecific(tls_key);
    if (!tls_data) {
        tls_data = (int*)malloc(sizeof(int));
        *tls_data = id;
        pthread_setspecific(tls_key, tls_data);
    }

    for (int i=0;i<100;i++) {
        math_heavy((double)id + i*0.01);
        mathf_heavy((float)id + i*0.01f);
        if (i % 25 == 0) {
            __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "thread %d iter %d tls=%d", id, i, tls_counter);
        }
    }

    libc_heavy();

    pthread_mutex_lock(&g_mutex);
    g_shared_value += id;
    if (g_shared_value >= 6) { // 0+1+2+3 =6
        pthread_cond_signal(&g_cond);
    }
    pthread_mutex_unlock(&g_mutex);

    __sync_fetch_and_add(&g_threads_done, 1);
    return nullptr;
}

// Global recursive mutex init test — Bionic pthread_mutexattr
static void init_recursive_mutex() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_recursive_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    // Test recursive lock
    pthread_mutex_lock(&g_recursive_mutex);
    pthread_mutex_lock(&g_recursive_mutex);
    pthread_mutex_unlock(&g_recursive_mutex);
    pthread_mutex_unlock(&g_recursive_mutex);
}

extern "C" {

// Main entry called by host via ulinker::dlsym
int bionic_stress_run() {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "bionic_stress_run: start pid=%d", getpid());
    init_recursive_mutex();
    pthread_once(&tls_once, tls_init_once);

    // locale — Bionic locale bug workaround in mcpelauncher
    setlocale(LC_ALL, "C");

    const int N = 4;
    pthread_t th[N];
    int ids[N] = {0,1,2,3};
    g_shared_value = 0;
    g_threads_done = 0;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i=0;i<N;i++) {
        int r = pthread_create(&th[i], nullptr, thread_entry, &ids[i]);
        if (r != 0) {
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "pthread_create %d failed: %d %s", i, r, strerror(r));
            return r;
        }
    }

    // Wait with cond + timeout (tests pthread_cond_timedwait Bionic layout via hybris hooks.c)
    pthread_mutex_lock(&g_mutex);
    while (g_shared_value < 6) {
        struct timespec to;
        clock_gettime(CLOCK_REALTIME, &to);
        to.tv_sec += 2;
        int rc = pthread_cond_timedwait(&g_cond, &g_mutex, &to);
        if (rc != 0 && rc != ETIMEDOUT) break;
        if (g_shared_value >= 6) break;
    }
    pthread_mutex_unlock(&g_mutex);

    for (int i=0;i<N;i++) pthread_join(th[i], nullptr);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long ms = (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1000000;

    // Final math to force long double path (fake_long_double.c)
    volatile long double ld = fabsl(-1.5L);
    (void)ld;

    // dlopen self test — Bionic libdl
    void* self = dlopen("libbionic_stress.so", RTLD_NOW);
    if (self) dlclose(self);

    // Check threads done
    int done = __sync_fetch_and_add(&g_threads_done, 0);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "bionic_stress_run: done=%d shared=%d ms=%ld ld=%Lf", done, g_shared_value, ms, ld);
    return (done == N && g_shared_value == 6) ? 0 : -1;
}

// Also expose individual probes for granular tests
double bionic_stress_math(double x) { return math_heavy(x); }
void bionic_stress_libc() { libc_heavy(); }
int bionic_stress_tls() {
    tls_counter = 42;
    return tls_counter;
}

} // extern "C"
