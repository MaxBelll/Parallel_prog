#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>

using namespace std;

typedef struct someArgs {
    int number;
    const char *msg;
} someArgss;
void display_stack_related_attributes(pthread_attr_t *attr)
{
    int err, detach_state;
    size_t stack_size, guard_size;
    void *stack_addr;

    err = pthread_attr_getguardsize(attr, &guard_size);
    if (err != 0)
        cout << strerror(err) << endl;
    printf("\tGuard size = %zu bytes\n", guard_size);

    err = pthread_attr_getstack(attr, &stack_addr, &stack_size);
    if (err != 0)
        cout << strerror(err) << endl;
    printf("\tStack address = %p", stack_addr);
    if (stack_size > 0)
        printf(" (EOS = %p)\n", (char *) stack_addr + stack_size);
    printf("\tStack size = 0x%zx (%zu) bytes\n", stack_size, stack_size);
    
    err = pthread_attr_getdetachstate(attr, &detach_state);
    if (err != 0)
        cout << strerror(err) << endl;
    detach_state == PTHREAD_CREATE_JOINABLE ? cout << "\tThread is joinable\n" : 
                                              cout << "\tThread is detached\n";
}

void *thread_job(void *arg)
{
    int err;
    pthread_t id = pthread_self();
    someArgss *x = (someArgss*)arg;
    pthread_attr_t attr;
    err = pthread_getattr_np(id, &attr);
    if (err != 0)
        cout << strerror(err) << endl;
        
    cout << "1. id:\t" << id << "\n";
    cout << "2. atributes: \n";
    display_stack_related_attributes(&attr);
    printf("3. arguments: \tnumber %d, msg %s\n", x->number, x->msg);

    err = pthread_attr_destroy(&attr);
    if (err != 0)
        cout << strerror(err) << endl;
    return (NULL);
}
int main(int argc, char *argv[])
{
    int number_for_guardsize = atoi(argv[1]);
    int detachstate = atoi(argv[2]);
    int err;
    pthread_t thread; // 1.Идентификатор потока
    pthread_attr_t thread_attr; // 2.Атрибуты потока
    someArgss arg; // 3.Аргументы потока
    arg.number = 123;
    arg.msg = "asdf";

    err = pthread_attr_init(&thread_attr);
    if(err != 0) {
    cout << "Cannot create thread attribute: " << strerror(err) << endl;
    exit(-1);
    }
    // Устанавливаем размер охранной зоны (в байтах)
    err = pthread_attr_setguardsize(&thread_attr, number_for_guardsize*1024);
    if(err != 0) {
    cout << "Setting guard size attribute failed: " << strerror(err) << endl;
    exit(-1);
    }
    // Устанавливаем тип потока
    err = pthread_attr_setdetachstate(&thread_attr, detachstate);
    if(err != 0) {
    cout << "Setting detach state attribute failed: " << strerror(err) << endl;
    exit(-1);
    }
    err = pthread_create(&thread, &thread_attr, thread_job, (void*) &arg);
    if(err != 0) {
    cout << "Cannot create a thread: " << strerror(err) << endl;
    exit(-1);
    }
    pthread_exit(NULL);
}
