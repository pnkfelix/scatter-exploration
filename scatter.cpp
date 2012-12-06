#include <iostream>
#include <sstream>
#include <string>
#include <math.h>

#include <pthread.h>

using namespace std;

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

class WorkerContext : public PrintingHelpers
{
    static pthread_mutex_t cout_mutex;
public:
    static void init() { pthread_mutex_init(&cout_mutex, NULL); }
    static void wait_and_exit() { pthread_exit(NULL); }
private:
    static void *Run_Worker(void *data) {
        WorkerContext *w = (WorkerContext*)data;
        w->run();
        pthread_exit(NULL);
    }

public:
    WorkerContext(int a) : threadid_( a) { }
public:
    virtual void run() { }
    void spawn() {
        int rc = pthread_create(&thread, NULL, Run_Worker, (void*)this);
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
    virtual void prior_printing() { cout_acq(); }
    virtual void after_printing() { cout_rel(); }

    void cout_acq() { pthread_mutex_lock(&cout_mutex); }
    void cout_rel() { pthread_mutex_unlock(&cout_mutex); }
};

pthread_mutex_t WorkerContext::cout_mutex;

void *Worker_1_Hello(void *data);
class HelloWorker : public WorkerContext {
    void run() { Worker_1_Hello(this); }
public: HelloWorker(int i) : WorkerContext(i) { }
};

void *Worker_1_IterFib(void *data);
class IterFibWorker : public WorkerContext {
    void run() { Worker_1_IterFib(this); }
public: IterFibWorker(int i) : WorkerContext(i) { }
};

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
    WorkerContext **contexts = new WorkerContext*[args.num_threads];
    for (int i=0; i < args.num_threads; i++) {
        contexts[i] = new HelloWorker(i);
    }
    for (int i=0; i < args.num_threads; i++) {
        contexts[i]->println("main() : creating thread, ", i);
        contexts[i]->spawn();
    }

    WorkerContext::wait_and_exit();
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

void *Worker_1_IterFib(void *data)
{
    int tid;
    WorkerContext *w = (WorkerContext*)data;
    tid = (intptr_t)w->threadid();
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

void *Worker_1_Hello(void *data)
{
    WorkerContext *w = (WorkerContext*)data;
    intptr_t i = (intptr_t)w->threadid();
    w->println("Hello World ", i);
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
