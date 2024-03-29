#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <math.h>
#include <algorithm>
#include <sys/time.h>

#include <pthread.h>

#include <libkern/OSAtomic.h>

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

class ResourceMeasure
{
public:
    void take_snapshot_start() { start_resource_usage.take_snapshot(); }
    void take_snapshot_finis() { finis_resource_usage.take_snapshot(); }

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
    SnapshotResourceUsage start_resource_usage;
    SnapshotResourceUsage finis_resource_usage;
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
    virtual ~WorkerContext() { }
public:
    virtual void run() { }
    void spawn() {
        int rc = pthread_create(&thread, NULL, run_worker, (void*)this);
        if (rc) {
            cout << "Error: unable to create thread," << rc << endl;
            WorkerContext::die(3);
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

class Computation
{
private:
    virtual void BeforeAllCompute() = 0;
    virtual void DoMyComputation() = 0;
    virtual void AfterAllCompute() = 0;
public:
    void before() { BeforeAllCompute(); }
    void go() { DoMyComputation(); }
    void after() { AfterAllCompute(); }
    virtual ~Computation() {}
};

class ChainedComputation : public Computation
{
private:
    void BeforeAllCompute() { c1->before(); c2->before(); }
    void DoMyComputation() { c1->go(); c2->go(); }
    void AfterAllCompute() { c1->after(); c2->after(); }
public:
    ChainedComputation(Computation *c1, Computation *c2) : c1(c1), c2(c2) {}
    ~ChainedComputation() { delete c1; delete c2; }
private:
    Computation *c1;
    Computation *c2;
};

class WorkerBuilder : public Computation, protected LockingPrintingHelper, public ResourceMeasure
{
private:
    void BeforeAllCompute() {}
    void DoMyComputation() { this->StartComputation(); this->WaitForFinish(); }
    void AfterAllCompute() { onFinish(); }

    virtual WorkerContext *newWorker(int i) = 0;

    // callback for when all workers have finished.
    // No specific computation specified (no-op is fine).
    virtual void onFinish() = 0;
    // callback after workers created but before any are spawned.
    // No specific computation specified (no-op is fine).
    virtual void onStart() = 0;

    // checksum to describe result output of computation.
    virtual uint64_t resultSummary() = 0;

public:
    int num_threads() const {
        return m_num_threads;
    }
private:
    void WaitForFinish() {
        if (!sequential) {
            for (int i=0; i < m_num_threads; i++) {
                if (contexts[i] != NULL)
                    contexts[i]->join();
            }
        }
        take_snapshot_finis();
    }
public:
    WorkerBuilder(ParseArgs const& args)
        : m_num_threads(args.num_threads)
        , m_args(args)
        , sequential(false)
        {
            contexts = new WorkerContext*[args.num_threads];
            for (int i=0; i < m_num_threads; i++) {
                contexts[i] = NULL;
            }
        }
private:
    void StartComputation() {
        onStart();
        createWorkers();
        // all workers constructed; spawn each in its own thread.
        take_snapshot_start();
        if (!sequential)
            spawnWorkers();
        else
            runWorkers();
    }
private:
    void createWorkers() {
        for (int i=0; i < m_num_threads; i++) {
            contexts[i] = newWorker(i);
        }
    }
    void spawnWorkers() {
        for (int i=0; i < m_num_threads; i++) {
            if (m_args.verbosity > 2) contexts[i]->println("main() : creating thread, ", i);
            contexts[i]->spawn();
        }
    }
    void runWorkers() {
        for (int i=0; i < m_num_threads; i++) {
            contexts[i]->run();
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
    void println_seconds(const char* prefix,
                         time_t dseconds, suseconds_t dmicroseconds);
private:
    int const m_num_threads;
    ParseArgs const& m_args;
    WorkerContext **contexts;
    bool sequential;
};

pthread_mutex_t LockingPrintingHelper::cout_mutex;

class HelloBuilder : public WorkerBuilder {
    class Worker : public WorkerContext {
        void run();
    public: Worker(int i) : WorkerContext(i) { }
    };
    WorkerContext *newWorker(int i) { return new Worker(i); }
public:
    HelloBuilder(ParseArgs const& args) : WorkerBuilder(args) { }
    void onFinish() { println("Hello World done"); }
    void onStart() { println("Hello World start"); }
    uint64_t resultSummary() { return 0; }
};

class IterFibBuilder : public WorkerBuilder {
    class Worker : public WorkerContext {
        void run();
    public: Worker(int i) : WorkerContext(i) { }
    };
    WorkerContext *newWorker(int i) { return new Worker(i); }
public:
    IterFibBuilder(ParseArgs const& args) : WorkerBuilder(args) { }
    void onFinish() { println("Iterated Fibonacci done"); }
    void onStart() { println("Iterated Fibonacci start"); }
    uint64_t resultSummary() { return 0; }
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

class InputOutputBuilder : private MarsagliaRNG
{
public:
    int64_t input_length() const { return input_len; }
    uint32_t const* input_data() const { return input; }
    intptr_t domain_length() const { return domain_len; }
    uint64_t* output_data() const { return output; }
    InputOutputBuilder(ParseArgs const& args)
        : input_len(args.input_length)
        , domain_len(args.domain_size)
        , args(args) {
        input = new uint32_t[input_len];
        output = new uint64_t[domain_len];
    }
    void build_input() {
        ResourceMeasure rm;
        rm.take_snapshot_start();
        for (intptr_t i = 0; i < input_len; i++)
            input[i] = get(domain_len);
        rm.take_snapshot_finis();
        rm.wallclockTimeDuration(&build_dseconds, &build_dmicroseconds);
    }
    virtual ~InputOutputBuilder() {
        delete[] input;
        delete[] output;
    }
protected:
    void printInput();
    void printOutput();
public:
    void print_input() { printInput(); }
    void print_output() { printOutput(); }
private:
    intptr_t input_len;
    uint32_t* input;
    intptr_t domain_len;
    uint64_t *output;
    LockingPrintingHelper p;
public:
    time_t build_dseconds;
    suseconds_t build_dmicroseconds;
public:
    ParseArgs const& args;
};

class CommonHistogramBuilder : public WorkerBuilder
{
protected:
    CommonHistogramBuilder(ParseArgs const& args, InputOutputBuilder *hb)
        : WorkerBuilder(args), hb(hb) { }
    void onStart();
    void onFinish();
    uint64_t resultSummary();
public:
    int64_t input_length() const { return hb->input_length(); }
    uint32_t const* input_data() const { return hb->input_data(); }
    intptr_t domain_length() const { return hb->domain_length(); }
    uint64_t* output_data() const { return hb->output_data(); }
    ParseArgs const& args() const { return hb->args; }
private:
    InputOutputBuilder *hb;
};

class SeqHistogramBuilder : public CommonHistogramBuilder {
    typedef SeqHistogramBuilder BLDR;
    class Worker : public WorkerContext {
        void run();
        friend class SeqHistogramBuilder;
        Worker(BLDR *builder_, int i)
            : WorkerContext(i), commonBuilder(builder_) { }
    protected:
        BLDR *commonBuilder;
    };
    WorkerContext *newWorker(int i) { return new Worker(this, i); }
public:
    SeqHistogramBuilder(ParseArgs const& args, InputOutputBuilder *hb)
        : CommonHistogramBuilder(args, hb) {}
};

class DivDomainHistogramBuilder : public CommonHistogramBuilder {
    typedef DivDomainHistogramBuilder BLDR;
    class Worker : public WorkerContext {
        void run();
        friend class DivDomainHistogramBuilder;
        Worker(BLDR *builder_, int i)
            : WorkerContext(i), commonBuilder(builder_) { }
    protected:
        BLDR *commonBuilder;
    };
    WorkerContext *newWorker(int i) { return new Worker(this, i); }
public:
    DivDomainHistogramBuilder(ParseArgs const& args, InputOutputBuilder *hb)
        : CommonHistogramBuilder(args, hb) {}
};

class DivRangeHistogramClearer : public WorkerBuilder
{
    WorkerContext *newWorker(int i) { return new Worker(this, i); }
    class Worker : public WorkerContext
    {
        friend class DivRangeHistogramClearer;
        void run();
        Worker(DivRangeHistogramClearer *builder_, int i)
            : WorkerContext(i), commonBuilder(builder_) { }
    protected:
        DivRangeHistogramClearer *commonBuilder;
    };

public:
    int64_t input_length() const { return hb->input_length(); }
    uint32_t const* input_data() const { return hb->input_data(); }
    intptr_t domain_length() const { return hb->domain_length(); }
    uint64_t* output_data() const { return hb->output_data(); }
    ParseArgs const& args() const { return hb->args; }
private:
    void onFinish();
    void onStart() { }
    uint64_t resultSummary() { return 0; }

public:
    DivRangeHistogramClearer(ParseArgs const& args, InputOutputBuilder *hb)
        : WorkerBuilder(args), hb(hb) {}
    InputOutputBuilder *hb;
};

class DivInputHistogramBuilderLocking : public CommonHistogramBuilder {
    typedef DivInputHistogramBuilderLocking BLDR;
    class Worker : public WorkerContext {
        void run();
        friend class DivInputHistogramBuilderLocking;
        Worker(BLDR *builder_, int i)
            : WorkerContext(i), commonBuilder(builder_) { }
    protected:
        BLDR *commonBuilder;
    };
    WorkerContext *newWorker(int i) { return new Worker(this, i); }
public:
    DivInputHistogramBuilderLocking(ParseArgs const& args, InputOutputBuilder *hb)
        : CommonHistogramBuilder(args, hb)
        { pthread_mutex_init(&output_mutex, NULL); }
    void increment(uint32_t index)
        {
            pthread_mutex_lock(&this->output_mutex);
            this->output_data()[index] += 1;
            pthread_mutex_unlock(&this->output_mutex);
        }
private:
    pthread_mutex_t output_mutex;
};

class DivInputHistogramBuilderCSWPing : public CommonHistogramBuilder {
    typedef DivInputHistogramBuilderCSWPing BLDR;
    class Worker : public WorkerContext {
        void run();
        friend class DivInputHistogramBuilderCSWPing;
        Worker(BLDR *builder_, int i)
            : WorkerContext(i), commonBuilder(builder_) { }
    protected:
        BLDR *commonBuilder;
    };
    WorkerContext *newWorker(int i) { return new Worker(this, i); }
public:
    DivInputHistogramBuilderCSWPing(ParseArgs const& args, InputOutputBuilder *hb)
        : CommonHistogramBuilder(args, hb) {}
    void increment(uint32_t index)
        {
            bool r;
            do {
                uint64_t x = this->output_data()[index];
                uint64_t xnew = x+1;
                r = OSAtomicCompareAndSwapPtr((void*)x, (void*)xnew,
                                              (void**)&this->output_data()[index]);
            } while (!r);
        }
};

class DivInputHistogramBuilderAADDing : public CommonHistogramBuilder {
    typedef DivInputHistogramBuilderAADDing BLDR;
    class Worker : public WorkerContext {
        void run();
        friend class DivInputHistogramBuilderAADDing;
        Worker(BLDR *builder_, int i)
            : WorkerContext(i), commonBuilder(builder_) { }
    protected:
        BLDR *commonBuilder;
    };
    WorkerContext *newWorker(int i) { return new Worker(this, i); }
public:
    DivInputHistogramBuilderAADDing(ParseArgs const& args, InputOutputBuilder *hb)
        : CommonHistogramBuilder(args, hb) {}
    void increment(uint32_t index)
        {
            OSAtomicIncrement64((int64_t*)&this->output_data()[index]);
        }
};

class DivInputHistogramBuilderDupeing : public CommonHistogramBuilder {
    typedef DivInputHistogramBuilderDupeing BLDR;
    class Worker : public WorkerContext {
        void run();
        friend class DivInputHistogramBuilderDupeing;
        Worker(BLDR *builder_, int i)
            : WorkerContext(i), commonBuilder(builder_) {
            myOutput = new uint64_t[commonBuilder->domain_length()];
        }
        ~Worker() { delete[] myOutput; }
        void increment(uint32_t index) {
            myOutput[index] += 1;
        }
    protected:
        BLDR *commonBuilder;
        uint64_t *myOutput;
    };
    WorkerContext *newWorker(int i) {
        Worker *w = new Worker(this, i);
        workers[i] = w;
        return w;
    }
public:
    DivInputHistogramBuilderDupeing(ParseArgs const& args, InputOutputBuilder *hb)
        : CommonHistogramBuilder(args, hb) {
        workers = new Worker*[args.num_threads];
    }
    ~DivInputHistogramBuilderDupeing() {
        delete[] workers;
    }
    void onFinish()
        {
            int64_t dom = domain_length();
            uint64_t *o = output_data();
            for (int i=0; i < num_threads(); i++) {
                for (int j = 0; j < dom; j++) {
                    o[j] += workers[i]->myOutput[j];
                }
            }
            this->CommonHistogramBuilder::onFinish();
        }
private:
    Worker **workers;
};

// FINIS CLASS AND DATA DEFINITIONS

Computation *makeBuilder(ParseArgs const& args, InputOutputBuilder *hist)
{
    stringstream out;
    Computation *builder;
    int nt = args.num_threads;
    switch (args.program) {
    case 1: builder = new HelloBuilder(args); break;
    case 2: builder = new IterFibBuilder(args); break;
    case 3: builder = new SeqHistogramBuilder(args, hist); break;
    case 4: builder = new DivDomainHistogramBuilder(args, hist); break;
    case 5:
        builder =
            new ChainedComputation(new DivRangeHistogramClearer(args, hist),
                                   new DivInputHistogramBuilderLocking(args, hist));
        break;
    case 6:
        builder =
            new ChainedComputation(new DivRangeHistogramClearer(args, hist),
                                   new DivInputHistogramBuilderCSWPing(args, hist));
        break;
    case 7:
        builder =
            new ChainedComputation(new DivRangeHistogramClearer(args, hist),
                                   new DivInputHistogramBuilderAADDing(args, hist));
        break;
    case 8:
        builder =
            new ChainedComputation(new DivRangeHistogramClearer(args, hist),
                                   new DivInputHistogramBuilderDupeing(args, hist));
        break;
    default: out << "unmatched program number: " << args.program << endl;
        WorkerContext::die(3);
    }

    return builder;
}

char const* describeProgramCode(int program)
{
    char const *ret;
    switch (program) {
    case 1: ret = "Hello"; break;
    case 2: ret = "IterFib"; break;
    case 3: ret = "Sequential Histogram"; break;
    case 4: ret = "Par Histogram Div-Domain"; break;
    case 5: ret = "Par Histogram Div-Input One-Domain via Locking"; break;
    case 6: ret = "Par Histogram Div-Input One-Domain via CMPXCHG"; break;
    case 7: ret = "Par Histogram Div-Input One-Domain via Atomic Add"; break;
    case 8: ret = "Par Histogram Div-Input Dup-Domain"; break;
    default:
        WorkerContext::die(4);
    }
    return ret;
}

int main(int argc, const char **argv)
{
    string s;

    ParseArgs args;

    if (args.parse_all(argc, argv)) {
        return 2;
    }
    args.print_values();

    WorkerContext::init();

    // Prepare histogram input
    InputOutputBuilder hist(args);

    hist.build_input();

    // set up workers
    Computation *builder = makeBuilder(args, &hist);

    builder->before();
    builder->go();
    builder->after();

    delete builder;
}

void SeqHistogramBuilder::Worker::run()
{
    CommonHistogramBuilder *sb = commonBuilder;
    if (threadid() == 0) {
        intptr_t l = sb->input_length();
        uint32_t const* const d = sb->input_data();
        intptr_t m = sb->domain_length();
        uint64_t *o = sb->output_data();

        for (intptr_t i = 0; i < m; i++) {
            o[i] = 0;
        }
        for (intptr_t i = 0; i < l; i++) {
            uint32_t x = d[i];
            if (x >= m) { println("whoops", x); WorkerContext::die(99); }
            o[x] += 1;
        }
    } else {
        // no-op
    }
}

void DivDomainHistogramBuilder::Worker::run()
{
    CommonHistogramBuilder const* sb = commonBuilder;
    int nt = sb->num_threads();
    int64_t dom = sb->domain_length();
    int64_t subdom_size = (dom + (nt - 1)) / nt;
    int id = threadid();
    int64_t subdom_start = id * subdom_size;
    int64_t subdom_finis = subdom_start + subdom_size;

    if (sb->args().verbosity > 2)
        println("worker ", id, " does domain [",
                subdom_start, " -- ", subdom_finis, ").");

    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();
    int64_t m = sb->domain_length();
    uint64_t *o = sb->output_data();
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

void DivRangeHistogramClearer::onFinish()
{
    time_t user_dseconds;
    suseconds_t user_dmicroseconds;

    time_t wall_dseconds;
    suseconds_t wall_dmicroseconds;

    userTimeDuration(&user_dseconds, &user_dmicroseconds);
    wallclockTimeDuration(&wall_dseconds, &wall_dmicroseconds);

    println_seconds("clearing user time", user_dseconds, user_dmicroseconds);

    println_seconds("clearing wallclock", wall_dseconds, wall_dmicroseconds);
}

void DivRangeHistogramClearer::Worker::run()
{
    DivRangeHistogramClearer const* sb = commonBuilder;
    int nt = sb->num_threads();
    int64_t dom = sb->domain_length();
    int64_t subdom_size = (dom + (nt - 1)) / nt;
    int id = threadid();
    int64_t subdom_start = id * subdom_size;
    int64_t subdom_finis = subdom_start + subdom_size;

    if (sb->args().verbosity > 2)
        println("worker ", id, " does domain [",
                subdom_start, " -- ", subdom_finis, ").");

    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();
    int64_t m = sb->domain_length();
    uint64_t *o = sb->output_data();
    for (intptr_t i = subdom_start; i < min(m, subdom_finis); i++) {
        // println("clearing ", i, " ", &o[i]);
        o[i] = 0;
    }
}

void DivInputHistogramBuilderLocking::Worker::run()
{
    DivInputHistogramBuilderLocking * sb = commonBuilder;
    int nt = sb->num_threads();
    int64_t len = sb->input_length();
    int64_t subinp_size = (len + (nt - 1)) / nt;
    int id = threadid();
    int64_t subinp_start = id * subinp_size;
    int64_t subinp_finis = min(len, subinp_start + subinp_size);

    if (sb->args().verbosity > 2)
        println("worker ", id, " does input [",
                subinp_start, " -- ", subinp_finis, ").");

    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();
    int64_t m = sb->domain_length();
    for (intptr_t i = subinp_start; i < subinp_finis; i++) {
        uint32_t x = d[i];
        if (x >= m) { println("whoops", x); exit(99); }
        sb->increment(x);
    }
}

void DivInputHistogramBuilderCSWPing::Worker::run()
{
    DivInputHistogramBuilderCSWPing * sb = commonBuilder;
    int nt = sb->num_threads();
    int64_t len = sb->input_length();
    int64_t subinp_size = (len + (nt - 1)) / nt;
    int id = threadid();
    int64_t subinp_start = id * subinp_size;
    int64_t subinp_finis = min(len, subinp_start + subinp_size);

    if (sb->args().verbosity > 2)
        println("worker ", id, " does input [",
                subinp_start, " -- ", subinp_finis, ").");

    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();
    int64_t m = sb->domain_length();
    for (intptr_t i = subinp_start; i < subinp_finis; i++) {
        uint32_t x = d[i];
        if (x >= m) { println("whoops", x); exit(99); }
        sb->increment(x);
    }
}

void DivInputHistogramBuilderAADDing::Worker::run()
{
    BLDR * sb = commonBuilder;
    int nt = sb->num_threads();
    int64_t len = sb->input_length();
    int64_t subinp_size = (len + (nt - 1)) / nt;
    int id = threadid();
    int64_t subinp_start = id * subinp_size;
    int64_t subinp_finis = min(len, subinp_start + subinp_size);

    if (sb->args().verbosity > 2)
        println("worker ", id, " does input [",
                subinp_start, " -- ", subinp_finis, ").");

    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();
    int64_t m = sb->domain_length();
    for (intptr_t i = subinp_start; i < subinp_finis; i++) {
        uint32_t x = d[i];
        if (x >= m) { println("whoops", x); exit(99); }
        sb->increment(x);
    }
}

void DivInputHistogramBuilderDupeing::Worker::run()
{
    BLDR * sb = commonBuilder;
    int nt = sb->num_threads();
    int64_t len = sb->input_length();
    int64_t subinp_size = (len + (nt - 1)) / nt;
    int id = threadid();
    int64_t subinp_start = id * subinp_size;
    int64_t subinp_finis = min(len, subinp_start + subinp_size);

    if (sb->args().verbosity > 2)
        println("worker ", id, " does input [",
                subinp_start, " -- ", subinp_finis, ").");

    intptr_t l = sb->input_length();
    uint32_t const* const d = sb->input_data();
    int64_t m = sb->domain_length();
    for (intptr_t i = subinp_start; i < subinp_finis; i++) {
        uint32_t x = d[i];
        if (x >= m) { println("whoops", x); exit(99); }
        increment(x); // note that this is on *my* array, not sb's
    }
}

void InputOutputBuilder::printInput()
{
    InputOutputBuilder *sb = this;
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

void InputOutputBuilder::printOutput()
{
    InputOutputBuilder *sb = this;
    intptr_t l = sb->domain_length();
    uint64_t *d = sb->output_data();
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
    if (args().verbosity > 2) hb->print_input();
    WorkerBuilder::println("Histogram start");
}

void WorkerBuilder::println_seconds(const char* prefix,
                                    time_t dseconds,
                                    suseconds_t dmicroseconds)
{
    // println(prefix, "seconds: ", dseconds, " microseconds: ", dmicroseconds);
    println(prefix, " seconds: ", dseconds, ".", setw(6), setfill('0'), dmicroseconds);
}

void CommonHistogramBuilder::onFinish()
{
    println("Histogram done");

    time_t user_dseconds;
    suseconds_t user_dmicroseconds;

    time_t wall_dseconds;
    suseconds_t wall_dmicroseconds;

    userTimeDuration(&user_dseconds, &user_dmicroseconds);
    wallclockTimeDuration(&wall_dseconds, &wall_dmicroseconds);

    if (args().verbosity > 2) hb->print_output();

    stringstream ss;
    ss << std::hex << resultSummary();
    println("summary: 0x", ss.str());

    println_seconds("initializing input", hb->build_dseconds, hb->build_dmicroseconds);

    println_seconds("building user time", user_dseconds, user_dmicroseconds);

    println_seconds("building wallclock", wall_dseconds, wall_dmicroseconds);
}
uint64_t CommonHistogramBuilder::resultSummary() {
    CommonHistogramBuilder *sb = this;
    intptr_t l = sb->domain_length();
    uint64_t *d = sb->output_data();
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

void IterFibBuilder::Worker::run()
{
    Worker *w = this;
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
}

void HelloBuilder::Worker::run()
{
    println("Hello World ", threadid());
}

void ParseArgs::print_values()
{
    printf("num_threads:  %d\n", num_threads);
    if (verbosity > 1) printf("rep:          %d\n", rep);
    printf("program:      %d (%s)\n", program, describeProgramCode(program));
    if (verbosity > 2) printf("seed_u:       %d\n", seed_u);
    if (verbosity > 2) printf("seed_z:       %d\n", seed_z);
    printf("input_length: %lld\n", input_length);
    printf("domain_size:  %d\n", domain_size);
    if (verbosity > 1) printf("verbosity:    %d\n", verbosity);
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
