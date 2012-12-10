#include <iostream>
#include <sstream>
#include <string>
#include <math.h>
#include <algorithm>
#include <sys/time.h>

#include <pthread.h>

using namespace std;

// FINIS HEADER INCLUDES

#define NO_INLINE __attribute__ ((noinline)) 

// START CLASS AND DATA DEFINITIONS

class ParseArgs {
public:
    static int max_nthreads;
    static int max_rep;
    static int max_program;
    int num_threads;
    int rep;
    int program;
    int seed_u;
    int seed_z;
    int64_t input_length;
    int domain_size;
    int verbosity;

    ParseArgs()
    : num_threads(1)
    , rep(1)
    , program(1)
    , seed_u(1)
    , seed_z(1)
    , input_length(20)
    , domain_size(10)
    , verbosity(1)
    { }

public:
    int parse_all(int argc, const char **argv);
    void print_values();
private:
    template<typename INT_T>
    bool arg(const int argc, const char **argv,
             int *pi, const char* option,
             INT_T *result, INT_T min_value, INT_T max_value);
};

int ParseArgs::max_nthreads = 100;
int ParseArgs::max_rep = 10;
int ParseArgs::max_program = 10;

class PrintingHelpers
{
    virtual void prior_printing() = 0;
    virtual void after_printing() = 0;
public:
    template<typename T> void println(T arg) { println(arg, ""); }

    template<typename T, typename U>
    void println(T arg1, U arg2) { println(arg1, arg2, "");  }

    template<typename T, typename U, typename V>
    void println(T arg1, U arg2, V arg3) {
        println(arg1, arg2, arg3, "");
    }

    template<typename T, typename U, typename V, typename W>
    void println(T arg1, U arg2, V arg3, W arg4) {
        println(arg1, arg2, arg3, arg4, "");
    }

    template<class T, class U, class V, class W, class X>
    void println(T arg1, U arg2, V arg3, W arg4, X arg5) {
        println(arg1, arg2, arg3, arg4, arg5, "");
    }

    template<class T, class U, class V, class W, class X, class Y>
    void println(T arg1, U arg2, V arg3, W arg4, X arg5, Y arg6) {
        println(arg1, arg2, arg3, arg4, arg5, arg6, "");
    }

    template<class T, class U, class V, class W, class X, class Y, class Z>
    void println(T a1, U a2, V a3, W a4, X a5, Y a6, Z a7) {
        prior_printing();
        cout << a1 << a2 << a3 << a4 << a5 << a6 << a7 << endl;
        after_printing();
    }

    void start_println() { prior_printing(); }
    template<class T>
    void start_println(T a1) { prior_printing(); cout << a1; }
    template<class T>
    void inter_println(T a1) { cout << a1; }

    void finis_println() { finis_println(""); }
    template<class T>
    void finis_println(T a1) { finis_println(a1, ""); }
    template<class T, class U>
    void finis_println(T a1, U a2) { finis_println(a1, a2, ""); }
    template<class T, class U, class V>
    void finis_println(T a1, U a2, V a3) { finis_println(a1, a2, a3, ""); }
    template<class T, class U, class V, class W>
    void finis_println(T a1, U a2, V a3, W a4) {
        finis_println(a1, a2, a3, a4, "");
    }
    template<class T, class U, class V, class W, class X>
    void finis_println(T a1, U a2, V a3, W a4, X a5) {
        finis_println(a1, a2, a3, a4, a5, "");
    }
    template<class T, class U, class V, class W, class X, class Y>
    void finis_println(T a1, U a2, V a3, W a4, X a5, Y a6) {
        finis_println(a1, a2, a3, a4, a5, a6, "");
    }
    template<class T, class U, class V, class W, class X, class Y, class Z>
    void finis_println(T a1, U a2, V a3, W a4, X a5, Y a6, Z a7) {
        cout << a1 << a2 << a3 << a4 << a5 << a6 << a7 << endl;
        after_printing();
    }
};

class LockingPrintingHelper : public PrintingHelpers {
    static pthread_mutex_t cout_mutex;
public:
    static void init() { pthread_mutex_init(&cout_mutex, NULL); }
private:
    virtual void prior_printing() { cout_acq(); }
    virtual void after_printing() { cout_rel(); }

    void cout_acq() { pthread_mutex_lock(&cout_mutex); }
    void cout_rel() { pthread_mutex_unlock(&cout_mutex); }
};

class SnapshotResourceUsage
{
    struct rusage r_usage;
    struct timeval real_time;
public:
    SnapshotResourceUsage() { }
    void take_snapshot() {
        getrusage(RUSAGE_SELF, &r_usage);
        gettimeofday(&real_time, NULL);
    }
private:
    void timeDuration(time_t start_s, suseconds_t start_us,
                      time_t finis_s, suseconds_t finis_us,
                      time_t *d_s_recv,
                      suseconds_t *d_us_recv) const {
        time_t ds;
        suseconds_t dms;
        if (finis_us >= start_us) {
            ds = finis_s - start_s;
            dms = finis_us - start_us;
        } else {
            ds = (finis_s - 1) - start_s;
            dms = finis_us + 1000000 - start_us;
        }
        *d_s_recv = ds;
        *d_us_recv = dms;
    }

public:
    void userTimeDuration(SnapshotResourceUsage const &end,
                          time_t *dseconds_recv,
                          suseconds_t *dmicroseconds_recv) const {
        timeDuration(r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec,
                     end.r_usage.ru_utime.tv_sec, end.r_usage.ru_utime.tv_usec,
                     dseconds_recv, dmicroseconds_recv);
    }

    void systemTimeDuration(SnapshotResourceUsage const &end,
                            time_t *dseconds_recv,
                            suseconds_t *dmicroseconds_recv) const {
        timeDuration(r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec,
                     end.r_usage.ru_stime.tv_sec, end.r_usage.ru_stime.tv_usec,
                     dseconds_recv, dmicroseconds_recv);
    }

    void wallclockTimeDuration(SnapshotResourceUsage const &end,
                               time_t *dseconds_recv,
                               suseconds_t *dmicroseconds_recv) const {
        timeDuration(real_time.tv_sec, real_time.tv_usec,
                     end.real_time.tv_sec, end.real_time.tv_usec,
                     dseconds_recv, dmicroseconds_recv);
    }
};

class WorkerContext : protected LockingPrintingHelper
{
public:
    static void init() { LockingPrintingHelper::init(); }
    static void die(int i) { exit(i); }
private:
    friend class WorkerBuilder;
    static void wait_and_exit() { pthread_exit(NULL); }
    static void *run_worker(void *data) {
        WorkerContext *w = (WorkerContext*)data;
        w->run();
        // pthread_exit(NULL);
        return NULL;
    }

public:
    WorkerContext(int a) : threadid_( a) { }
public:
    virtual void run() { }
    void spawn() {
        int rc = pthread_create(&thread, NULL, run_worker, (void*)this);
        if (rc) {
            cout << "Error: unable to create thread," << rc << endl;
            exit(3);
        }
    }
private:
    int threadid_;
private:
    pthread_t thread;
public:
    int threadid() { return threadid_; }
private:
    void join() {
        void *retval;
        int err = pthread_join(this->thread, &retval);
        if (err != 0) {
            println("join error on thread ", threadid(), " error code ", err);
        }
    }
};

class WorkerBuilder : protected LockingPrintingHelper
{
private:
    virtual WorkerContext *newWorker(int i) = 0;

    // callback for when all workers have finished.
    // No specific computation specified (no-op is fine).
    virtual void onExit() = 0;
    // callback after workers created but before any are spawned.
    // No specific computation specified (no-op is fine).
    virtual void onStart() = 0;

    // checksum to describe result output of computation.
    virtual intptr_t resultSummary() = 0;

public:
    int num_threads() const {
        return m_num_threads;
    }

    NO_INLINE void wait_and_exit() {
        for (int i=0; i < m_num_threads; i++) {
            if (contexts[i] != NULL)
                contexts[i]->join();
        }
        takeSnapshotFinis();
        onExit();
        // WorkerContext::wait_and_exit();
    }
public:
    WorkerBuilder(int num_threads) : m_num_threads(num_threads) {
        contexts = new WorkerContext*[num_threads];
        for (int i=0; i < num_threads; i++) {
            contexts[i] = NULL;
        }
    }
    void start_computation() {
        onStart();
        createWorkers();
        // all workers constructed; spawn each in its own thread.
        takeSnapshotStart();
        spawnWorkers();
    }
private:
    void createWorkers() {
        for (int i=0; i < m_num_threads; i++) {
            contexts[i] = newWorker(i);
        }
    }
    void spawnWorkers() {
        for (int i=0; i < m_num_threads; i++) {
            contexts[i]->println("main() : creating thread, ", i);
            contexts[i]->spawn();
        }
    }
public:
    virtual ~WorkerBuilder() {
        for (int i=0; i < m_num_threads; i++) {
            contexts[i] = NULL;
        }
        delete[] contexts;
    }
protected:
    void takeSnapshotStart() { start_resource_usage.take_snapshot(); }
    void takeSnapshotFinis() { finis_resource_usage.take_snapshot(); }
    void userTimeDuration(time_t *dseconds, suseconds_t *duseconds) {
        start_resource_usage.userTimeDuration(finis_resource_usage,
                                              dseconds,
                                              duseconds);
    }
    void wallclockTimeDuration(time_t *dseconds, suseconds_t *duseconds) {
        start_resource_usage.wallclockTimeDuration(finis_resource_usage,
                                                   dseconds,
                                                   duseconds);
    }
private:
    const int m_num_threads;
    WorkerContext **contexts;
    SnapshotResourceUsage start_resource_usage;
    SnapshotResourceUsage finis_resource_usage;
};

pthread_mutex_t LockingPrintingHelper::cout_mutex;

class HelloWorker;
class HelloWorker : public WorkerContext {
    void run();
public: HelloWorker(int i) : WorkerContext(i) { }
};
class HelloBuilder : public WorkerBuilder {
    WorkerContext *newWorker(int i) { return new HelloWorker(i); }
public:
    HelloBuilder(int num_threads) : WorkerBuilder(num_threads) { }
    void onExit() { println("Hello World done"); }
    void onStart() { println("Hello World start"); }
    intptr_t resultSummary() { return 0; }
};

class IterFibWorker;
class IterFibWorker : public WorkerContext {
    void run();
public: IterFibWorker(int i) : WorkerContext(i) { }
};
class IterFibBuilder : public WorkerBuilder {
    WorkerContext *newWorker(int i) { return new IterFibWorker(i); }
public:
    IterFibBuilder(int num_threads) : WorkerBuilder(num_threads) { }
    void onExit() { println("Iterated Fibonacci done"); }
    void onStart() { println("Iterated Fibonacci start"); }
    intptr_t resultSummary() { return 0; }
};

class MarsagliaRNG
{
public:
    uint32_t getUint32() {
        z = 36969 * (z & 65535) + (z >> 16);
        u = 18000 * (u & 65535) + (u >> 16);
        return (z << 16) + u;
    }
    uint64_t getUint64() {
        uint64_t h = getUint32();
        uint64_t l = getUint32();
        return (h << 32) + l;
    }
    int32_t getInt32() { return (int32_t)getUint32(); }
    int64_t getInt64() { return (int64_t)getUint64(); }
    uint32_t get(uint32_t max_value) {
        uint32_t u = getUint32();
        // 0 <= u < 2^32

        // The magic value below is 1/(2^32 + 1), to ensure
        // the result R obeys 0 <= R < 1.
        return uint32_t(double(max_value) *
                        (double(u) * 2.3283064359965952e-10));
    }

    void reseed(uint32_t u, uint32_t z) { this->u = u; this->z = z; }
public:
    MarsagliaRNG() : u(1), z(1) { }
    MarsagliaRNG(uint32_t u_, uint32_t z_) : u(u_), z(z_) { }
private:
    uint32_t u;
    uint32_t z;
};

class HistogramBuilder : private MarsagliaRNG
{
public:
    int64_t input_length() const { return input_len; }
    uint32_t const* input_data() const { return input; }
    intptr_t domain_length() const { return domain_len; }
    uintptr_t* output_data() const { return output; }
    HistogramBuilder(int64_t input_length, int32_t domain_length, int verbosity_)
        : input_len(input_length)
        , domain_len(domain_length)
        , verbosity(verbosity_) {
        input = new uint32_t[input_len];
        output = new uintptr_t[domain_len];
    }
    void build_input() {
        for (intptr_t i = 0; i < input_len; i++) {
            input[i] = get(domain_len);
        }
    }
    void clear_output() {
        for (intptr_t i = 0; i < domain_len; i++) {
            output[i] = -1;
        }
    }
    virtual ~HistogramBuilder() {
        delete[] input;
        delete[] output;
    }
protected:
    void printInput();
    void printOutput();
private:
    intptr_t input_len;
    uint32_t* input;
    intptr_t domain_len;
    uintptr_t *output;
    LockingPrintingHelper p;
public:
    const int verbosity;
};

class CommonHistogramBuilder : public WorkerBuilder, public HistogramBuilder
{
protected:
    CommonHistogramBuilder(int num_threads, int64_t input_len, int domain_size, int verbosity)
        : WorkerBuilder(num_threads)
        , HistogramBuilder(input_len, domain_size, verbosity) { }
    void onStart();
    void onExit();
    intptr_t resultSummary();
private:
    time_t build_dseconds;
    suseconds_t build_dmicroseconds;
};

class SeqHistogramBuilder;
class SeqHistogramWorker : public WorkerContext {
    void run();
    friend class SeqHistogramBuilder;
    SeqHistogramWorker(CommonHistogramBuilder *builder_, int i)
        : WorkerContext(i), commonBuilder(builder_) { }
protected:
    CommonHistogramBuilder *commonBuilder;
};
class SeqHistogramBuilder : public CommonHistogramBuilder {
    WorkerContext *newWorker(int i) { return new SeqHistogramWorker(this, i); }
public:
    SeqHistogramBuilder(int num_threads, int64_t input_len, int domain_size, int verbosity)
        : CommonHistogramBuilder(num_threads, input_len, domain_size, verbosity) {}
};

class DivDomainHistogramBuilder;
class DivDomainHistogramWorker : public WorkerContext {
    void run();
    friend class DivDomainHistogramBuilder;
    DivDomainHistogramWorker(CommonHistogramBuilder *builder_, int i)
        : WorkerContext(i), commonBuilder(builder_) { }
protected:
    CommonHistogramBuilder *commonBuilder;
};
class DivDomainHistogramBuilder : public CommonHistogramBuilder {
    WorkerContext *newWorker(int i) { return new DivDomainHistogramWorker(this, i); }
public:
    DivDomainHistogramBuilder(int num_threads, int64_t input_len, int domain_size, int verbosity)
        : CommonHistogramBuilder(num_threads, input_len, domain_size, verbosity) {}
};

// FINIS CLASS AND DATA DEFINITIONS

int main(int argc, const char **argv)
{
    string s;
    stringstream out;

    ParseArgs args;

    if (args.parse_all(argc, argv)) {
        return 2;
    }
    args.print_values();

    WorkerContext::init();

    // set up workers
    WorkerBuilder *builder;
    int nt = args.num_threads;
    switch (args.program) {
    case 1: builder = new HelloBuilder(nt); break;
    case 2: builder = new IterFibBuilder(nt); break;
    case 3: builder = new SeqHistogramBuilder(nt, args.input_length, args.domain_size, args.verbosity); break;
    case 4: builder = new DivDomainHistogramBuilder(nt, args.input_length, args.domain_size, args.verbosity); break;
    default: out << "unmatched program number: " << args.program << endl;
        WorkerContext::die(3);
    }

    builder->start_computation();
    builder->wait_and_exit();
}

void SeqHistogramWorker::run()
{
    HistogramBuilder *sb = commonBuilder;
    if (threadid() == 0) {
        intptr_t l = sb->input_length();
        uint32_t const* const d = sb->input_data();
        intptr_t m = sb->domain_length();
        uintptr_t *o = sb->output_data();
        for (intptr_t i = 0; i < m; i++) {
            o[i] = 0;
        }
        for (intptr_t i = 0; i < l; i++) {
            uint32_t x = d[i];
            if (x >= m) { println("whoops", x); exit(99); }
            o[x] += 1;
        }
    } else {
        // no-op
    }
}

void DivDomainHistogramWorker::run()
{
    CommonHistogramBuilder const* sb = commonBuilder;
    int nt = sb->num_threads();
    int64_t dom = sb->domain_length();
    int64_t subdom_size = (dom + (nt - 1)) / nt;
    int id = threadid();
    int64_t subdom_start = id * subdom_size;
    int64_t subdom_finis = subdom_start + subdom_size;

    if (0 && sb->verbosity > 1)
        println("worker ", id, " does domain [",
                subdom_start, " -- ", subdom_finis, ").");

    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();
    int64_t m = sb->domain_length();
    uintptr_t *o = sb->output_data();
    for (intptr_t i = subdom_start; i < min(m, subdom_finis); i++) {
        o[i] = 0;
    }
    for (intptr_t i = 0; i < l; i++) {
        uint32_t x = d[i];
        if (x >= m) { println("whoops", x); exit(99); }
        if (subdom_start <= x && x < subdom_finis)
            o[x] += 1;
    }
}

void HistogramBuilder::printInput()
{
    HistogramBuilder *sb = this;
    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();

    p.println("Histogram input (length ", l, "):");
    p.start_println("    [");
    for (int i=0; i < l; i++) {
        p.inter_println(d[i]);
        if (i+1 < l)
            p.inter_println(", ");
    }
    p.finis_println("]");
}

void HistogramBuilder::printOutput()
{
    HistogramBuilder *sb = this;
    intptr_t l = sb->domain_length();
    uintptr_t *d = sb->output_data();
    intptr_t tot = 0;
    for (int i=0; i < l; i++) {
        if (d[i] != 0) {
            p.println(i, " : ", d[i]);
            tot += d[i];
        }
    }
    p.println("total: ", tot);
}

void CommonHistogramBuilder::onStart()
{
    takeSnapshotStart();
    build_input();
    clear_output();
    takeSnapshotFinis();
    userTimeDuration(&build_dseconds, &build_dmicroseconds);
    if (verbosity > 2) printInput();
    WorkerBuilder::println("Histogram start");
}

void CommonHistogramBuilder::onExit()
{
    println("Histogram done");

    time_t dseconds;
    suseconds_t dmicroseconds;

    userTimeDuration(&dseconds, &dmicroseconds);

    println("seconds: ", dseconds, " microseconds: ", dmicroseconds);

    if (verbosity > 1) printOutput();

    stringstream ss;
    ss << std::hex << resultSummary();
    println("summary: 0x", ss.str());

    println("inp build time seconds: ", build_dseconds, " microseconds: ", build_dmicroseconds);
    println("histogram time seconds: ", dseconds, " microseconds: ", dmicroseconds);
}
intptr_t CommonHistogramBuilder::resultSummary() {
    HistogramBuilder *sb = this;
    intptr_t l = sb->domain_length();
    uintptr_t *d = sb->output_data();
    intptr_t accum = 0;
    for (intptr_t i=0; i < l; i++) {
        accum = (accum << 1) | ((accum >> sizeof(accum)) & 0x1);
        accum ^= intptr_t(d[i]);
    }
    return accum;
}

long long fib(int x)
{
    long long i = 0, a = 0, b = 1, t;
    while (i < x) {
        t = a + b;
        a = b;
        b = t;
        i++;
    }
    return a;
}

void IterFibWorker::run()
{
    IterFibWorker *w = this;
    int tid = w->threadid();
    w->println("Hello World!  Thread ID: ", tid);

    for (int i=0; i < 10; i++) {
        long long f;
        long long r;
        long long j;
        int k;
        for (j=0; j < 5000; j++) {
            f = 92 - tid;
            r = fib(f);
            if (r < 0)
                w->println("neg fib f(", f, ") = ", r);

            for (k=0; k < 92-tid; k++) {
                f = r;
                while (f >= 93)
                    f = log(f);
                r = fib(f);
                if (r < 0)
                    w->println("neg fib f(", f, ") = ", r);
            }

            r = log(f);
        }

        w->start_println();
        for (int k=0; k < tid; k++)
            w->inter_println("  ");
        w->finis_println("ID(", tid, "): ", r, " ", "iteration ", i);
    }
    pthread_exit(NULL);
}

void HelloWorker::run()
{
    println("Hello World ", threadid());
    pthread_exit(NULL);
}

void ParseArgs::print_values()
{
    printf("num_threads:  %d\n", num_threads);
    printf("rep:          %d\n", rep);
    printf("program:      %d\n", program);
    printf("seed_u:       %d\n", seed_u);
    printf("seed_z:       %d\n", seed_z);
    printf("input_length: %lld\n", input_length);
    printf("domain_size:  %d\n", domain_size);
    printf("verbosity:    %d\n", verbosity);
}

int ParseArgs::parse_all(int argc, const char **argv)
{
    for (int i=1; i<argc; i++) {
        // printf("Argument %d: %s\n", i, argv[i]);
        if (false) {
        } else if (arg(argc, argv, &i, "-nthreads", &num_threads, 1, max_nthreads)) {
        } else if (arg(argc, argv, &i, "-num_threads", &num_threads, 1, max_nthreads)) {
        } else if (arg(argc, argv, &i, "-nt", &num_threads, 1, max_nthreads)) {
        } else if (arg(argc, argv, &i, "-rep", &rep, 1, max_rep)) {
        } else if (arg(argc, argv, &i, "-program", &program, 1, max_program)) {
        } else if (arg(argc, argv, &i, "-seed_u", &seed_u, 1, INT_MAX)) {
        } else if (arg(argc, argv, &i, "-seed_z", &seed_z, 1, INT_MAX)) {
        } else if (arg(argc, argv, &i, "-input_length", &input_length, 1LL, INT64_MAX)) {
        } else if (arg(argc, argv, &i, "-domain_size", &domain_size, 1, INT_MAX)) {
        } else if (arg(argc, argv, &i, "-verbosity", &verbosity, 0, 10)) {
        } else {
            printf("unknown argument: `%s', exiting\n", argv[i]);
            return 2;
        }
    }
    return 0;
}

// Let I denote *pi.  If argv[I] matches option and next arg is
// available (argv[I+1] < argc), then returns true and increments I;
// otherwise false.  Also, if returns true, next arg can be read as
// integer, and the resulting integer is <= max_value, then *result
// holds resulting integer; otherwise result is unchanged.
template<typename INT_T>
bool ParseArgs::arg(const int argc, const char **argv,
                    int *pi, const char* option, INT_T *result, INT_T min_value, INT_T max_value)
{
    int i = *pi;
    string arg(argv[i]);
    if (arg.compare(option) == 0) {
        if (i+1 < argc) {
            arg = string(argv[i+1]);
            INT_T temp;
            if ((stringstream(arg) >> temp) && temp >= min_value && temp <= max_value) {
                *result = temp;
            }
            (*pi)++;
        }
        return true;
    } else {
        return false;
    }
}
