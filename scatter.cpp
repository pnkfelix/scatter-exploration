#include <iostream>
#include <sstream>
#include <string>
#include <math.h>

#include <pthread.h>

using namespace std;

// FINIS HEADER INCLUDES

// START CLASS AND DATA DEFINITIONS

class ParseArgs {
public:
    static int max_nthreads;
    static int max_rep;
    static int max_program;

    int num_threads;
    int rep;
    int program;

    ParseArgs() : num_threads(1), rep(1), program(1) { }

public:
    int parse_all(int argc, const char **argv);
    void print_values();
private:
    bool arg(const int argc, const char **argv,
             int *pi, const char* option, int *result, int max_value);
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
        pthread_exit(NULL);
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
        pthread_join(this->thread, &retval);
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
public:
    void wait_and_exit() {
        for (int i=0; i < num_threads; i++) {
            if (contexts[i] != NULL)
                contexts[i]->join();
        }
        onExit();
        WorkerContext::wait_and_exit();
    }
public:
    WorkerBuilder(int num_threads_) : num_threads(num_threads_) {
        contexts = new WorkerContext*[num_threads];
        for (int i=0; i < num_threads; i++) {
            contexts[i] = NULL;
        }
    }
    void start_computation() {
        onStart();
        createWorkers();
        // all workers constructed; spawn each in its own thread.
        spawnWorkers();
    }
private:
    void createWorkers() {
        for (int i=0; i < num_threads; i++) {
            contexts[i] = newWorker(i);
        }
    }
    void spawnWorkers() {
        for (int i=0; i < num_threads; i++) {
            contexts[i]->println("main() : creating thread, ", i);
            contexts[i]->spawn();
        }
    }
public:
    virtual ~WorkerBuilder() {
        for (int i=0; i < num_threads; i++) {
            contexts[i] = NULL;
        }
        delete[] contexts;
    }
private:
    int num_threads;
    WorkerContext **contexts;
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
    intptr_t input_length() { return input_len; }
    uint32_t* input_data() { return input; }
    intptr_t domain_length() { return domain_len; }
    uintptr_t* output_data() { return output; }
    HistogramBuilder(intptr_t input_length, int32_t domain_length)
        : input_len(input_length), domain_len(domain_length) {
        input = new uint32_t[input_len];
        output = new uintptr_t[domain_len];
        for (intptr_t i = 0; i < input_len; i++) {
            input[i] = get(domain_length);
        }
        for (intptr_t i = 0; i < domain_len; i++) {
            output[i] = -1;
        }
    }
    virtual ~HistogramBuilder() {
        delete[] input;
        delete[] output;
    }
private:
    intptr_t input_len;
    uint32_t* input;
    intptr_t domain_len;
    uintptr_t *output;
};

class SeqHistogramBuilder;
class HistogramBuilder;
class SeqHistogramWorker : public WorkerContext {
    void run();
    friend class SeqHistogramBuilder;
    SeqHistogramWorker(HistogramBuilder *builder_, int i)
        : WorkerContext(i), commonBuilder(builder_) { }
protected:
    HistogramBuilder *commonBuilder;
};
class SeqHistogramBuilder : public WorkerBuilder, public HistogramBuilder {
    WorkerContext *newWorker(int i) { return new SeqHistogramWorker(this, i); }
public:
    SeqHistogramBuilder(int num_threads, int input_len, int domain_size)
        : WorkerBuilder(num_threads)
        , HistogramBuilder(input_len, domain_size) { }
    void onStart();
    void onExit();
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
    case 3: builder = new SeqHistogramBuilder(nt, 20, 10); break;
    default: out << "unmatched program number: " << args.program << endl;
        WorkerContext::die(3);
    }

    builder->start_computation();
    builder->wait_and_exit();
}

void SeqHistogramWorker::run()
{
    HistogramBuilder *sb = commonBuilder;
    if (threadid() == 1) {
        intptr_t l = sb->input_length();
        uint32_t *d = sb->input_data();
        intptr_t m = sb->domain_length();
        uintptr_t *o = sb->output_data();
        for (int i = 0; i < m; i++) {
            o[i] = 0;
        }
        for (int i = 0; i < l; i++) {
            uint32_t x = d[i];
            if (x >= m) { println("whoops", x); exit(99); }
            o[x] += 1;
        }
    } else {
        // no-op
    }
}

void SeqHistogramBuilder::onStart()
{
    HistogramBuilder *sb = this;
    intptr_t l = sb->input_length();
    uint32_t *d = sb->input_data();

    println("Histogram input (length ", l, "):");
    start_println("    [");
    for (int i=0; i < l; i++) {
        inter_println(d[i]);
        if (i+1 < l)
            inter_println(", ");
    }
    finis_println("]");
    println("Histogram start");
}
void SeqHistogramBuilder::onExit()
{
    println("Histogram done");

    HistogramBuilder *sb = this;
    intptr_t l = sb->domain_length();
    uintptr_t *d = sb->output_data();
    intptr_t tot = 0;
    for (int i=0; i < l; i++) {
        if (d[i] != 0) {
            println(i, " : ", d[i]);
            tot += d[i];
        }
    }
    println("total: ", tot);
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
    printf("num_threads: %d\n", num_threads);
    printf("rep:         %d\n", rep);
    printf("program:     %d\n", program);
}

int ParseArgs::parse_all(int argc, const char **argv)
{
    for (int i=1; i<argc; i++) {
        // printf("Argument %d: %s\n", i, argv[i]);
        if (false) {
        } else if (arg(argc, argv, &i, "-nthreads", &num_threads, max_nthreads)) {
        } else if (arg(argc, argv, &i, "-rep", &rep, max_rep)) {
        } else if (arg(argc, argv, &i, "-program", &program, max_program)) {
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
bool ParseArgs::arg(const int argc, const char **argv,
                    int *pi, const char* option, int *result, int max_value)
{
    int i = *pi;
    string arg(argv[i]);
    if (arg.compare(option) == 0) {
        if (i+1 < argc) {
            arg = string(argv[i+1]);
            int temp;
            if ((stringstream(arg) >> temp) && temp > 0 && temp <= max_value) {
                *result = temp;
            }
            (*pi)++;
        }
        return true;
    } else {
        return false;
    }
}
