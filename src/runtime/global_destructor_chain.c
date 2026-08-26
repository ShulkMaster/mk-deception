typedef void (*Destructor)(void* object, int complete);

typedef struct DestructorChain {
    struct DestructorChain* next;
    Destructor destructor;
    void* object;
} DestructorChain;

DestructorChain* __global_destructor_chain;

void __destroy_global_chain(void)
{
    DestructorChain* current;

    while ((current = __global_destructor_chain) != 0) {
        __global_destructor_chain = current->next;
        current->destructor(current->object, -1);
    }
}

void* __register_global_object(void* object, Destructor destructor, DestructorChain* registration)
{
    registration->next = __global_destructor_chain;
    registration->destructor = destructor;
    registration->object = object;
    __global_destructor_chain = registration;
    return object;
}
