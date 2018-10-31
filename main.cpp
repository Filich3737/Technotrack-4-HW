#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>

using namespace std;

#define UNITTEST(what, op, ref)                                                                                     \
{                                                                                                                   \
    auto result = (what);                                                                                           \
    auto r = (ref);                                                                                                 \
    if (result op r)                                                                                                \
    cout << #what << " \x1b[32m[passed]\x1b[0m: result is " << result << " and should be " #op " " << r <<endl;     \
    else                                                                                                            \
    cout << #what << " \x1b[31m[failed]\x1b[0m: result is " << result << " but should be " #op " " << r <<endl;     \
    }                                                                                                                   \

#define Stack_dump(inpSt, reason)\
{\
    auto addr = (inpSt);\
    auto res = Stack_OK(inpSt);\
    cout << "-------------------------------------------------------" << endl;\
    cout << (reason) << endl;\
    cout << "Stack " << #inpSt << " " << addr << " Stack_OK:" << res << endl;\
    cout << "Capacity: " << inpSt->capacity << endl;\
    cout << "Count: " << inpSt->count << endl;\
    cout << "Data: " << inpSt->data << endl;\
    cout << "\x1b[033mCanary1:\x1b[0m ";\
    if(*inpSt->dat_can1 == 0xBEDABEDA) cout << "\x1b[32m"; else cout << "\x1b[31m";\
    cout << *inpSt->dat_can1 << "\x1b[0m" << endl;\
    \
    for (unsigned i = 0; i < inpSt->capacity; i++)\
{\
    printf("[%d] = ", i);\
    if(i >= inpSt->count)\
    printf("\x1b[33m");\
    if(inpSt->data[i] == 666)\
    printf("\x1b[31m");\
    printf("%d\n", inpSt->data[i]);\
    printf("\x1b[0m");\
    }\
    \
    cout << "\x1b[033mCanary2:\x1b[0m ";\
    if(*inpSt->dat_can2 == 0xBEDABEDA) cout << "\x1b[32m"; else cout << "\x1b[31m";\
    cout << *inpSt->dat_can2 << "\x1b[0m" << endl;\
    cout << "-------------------------------------------------------" << "\n" <<endl;\
    }

#define ASSERT_OK(inpSt, reason) if(Stack_OK(inpSt) != 0) {Stack_dump(inpSt, reason); assert(!reason);}

struct Stack
{
    uint16_t canary1 = 0xBEDA;
    unsigned count = 0;
    unsigned capacity = 0;
    long unsigned stk_sum = 0;
    long unsigned dat_sum = 0;
    unsigned *data = static_cast<unsigned*>(nullptr);
    unsigned *dat_can1 = static_cast<unsigned*>(nullptr);
    unsigned *dat_can2 = static_cast<unsigned*>(nullptr);
    uint16_t canary2 = 0xBEDA;
};

long unsigned calc_stk_sum(Stack *inpSt)
{
    assert(inpSt != nullptr);
    unsigned long tmp = inpSt->stk_sum;
    inpSt->stk_sum = 0;
    long unsigned sum = 0;
    uint8_t *ptr = (uint8_t*)inpSt;
    for (unsigned long i = 1; i < sizeof(Stack) - 2; i++)
        sum += i * ptr[i];
    inpSt->stk_sum = tmp;
    return sum;
}

long unsigned calc_dat_sum(Stack *inpSt)
{
    assert(inpSt != nullptr);
    long unsigned sum = 0;
    unsigned *ptr = inpSt->dat_can1;
    for (unsigned long i = 0; i <= inpSt->capacity; i++)
        sum += i * ptr[i];
    return sum;
}

void write_stk_sum(Stack *inpSt)
{
    assert(inpSt != nullptr);
    inpSt->stk_sum = calc_stk_sum(inpSt);
}

void write_dat_sum(Stack *inpSt)
{
    assert(inpSt != nullptr);
    inpSt->dat_sum = calc_dat_sum(inpSt);
}

void write_control_sums(Stack *inpSt)
{
    assert(inpSt != nullptr);
    write_dat_sum(inpSt);
    write_stk_sum(inpSt);
}

int Stack_OK(Stack *inpSt)
{
    assert(inpSt != nullptr);
    if (inpSt->canary1 != 0xBEDA)
        return 1;
    else if (inpSt->canary2 != 0xBEDA)
        return 2;
    else if (*inpSt->dat_can1 != 0xBEDABEDA)
        return 3;
    else if (*inpSt->dat_can2 != 0xBEDABEDA)
        return 4;
    else if (inpSt->stk_sum != calc_stk_sum(inpSt))
        return 5;
    else if (inpSt->dat_sum != calc_dat_sum(inpSt))
        return 6;
    else if ( inpSt->count > inpSt->capacity )
        return 7;
    else if (inpSt->data == nullptr)
        return 8;
    else
        return 0;
}

void create_new_data(Stack *inpSt)
{
    assert(inpSt != nullptr);

    unsigned *ptr = static_cast<unsigned*>(calloc(3, sizeof(unsigned)));
    assert(ptr != nullptr);

    ptr[0] = 0xBEDABEDA;
    ptr[1] = 444;
    ptr[2] = 0xBEDABEDA;
    inpSt->dat_can1 = &(ptr[0]);
    inpSt->data = &(ptr[1]);
    inpSt->dat_can2 = &(ptr[2]);
}

void StackCtor(Stack *inpSt)
{
    assert(inpSt != nullptr);
    inpSt->capacity = 1;

    create_new_data(inpSt);

    write_control_sums(inpSt);
}

Stack StackNew()
{
    Stack outSt;
    StackCtor(&outSt);
    return outSt;
}

void poison_dat(Stack *inpSt)
{
    assert(inpSt != nullptr);
    for (unsigned i = 0; i < inpSt->capacity + 2; i++)
        inpSt->dat_can1[i] = 666;
}

void poison_stk(Stack *inpSt)
{
    assert(inpSt != nullptr);
    uint32_t *ptr = (uint32_t*)inpSt;
    for (unsigned i = 0; i < sizeof(Stack)/4; i++)
        ptr[i] = 666;
}

void StackDtor(Stack *inpSt)
{
    poison_dat(inpSt);
    free(inpSt->dat_can1);
    poison_stk(inpSt);
}

void expand_stk(Stack *inpSt, uint8_t isExp)
{
    assert(inpSt != nullptr);

    if(isExp)
        inpSt->capacity *= 2;
    else
        inpSt->capacity /= 2;

    void *tmp = realloc(inpSt->dat_can1, (inpSt->capacity + 2) * sizeof(unsigned));
    assert(tmp != nullptr);

    unsigned *ptr = static_cast<unsigned*>(tmp);
    if(isExp)
    {
        for (unsigned i = 0; i < inpSt->capacity/2; i++)
            *(ptr + inpSt->capacity/2 + 1 +i) = 444;
    }
    ptr[inpSt->capacity + 1] = 0xBEDABEDA;

    inpSt->dat_can1 = ptr;
    inpSt->data = ptr+1;
    inpSt->dat_can2 = &ptr[inpSt->capacity + 1];
}

void Push_back(Stack *inpSt, unsigned val)
{
    assert(inpSt != nullptr);
    ASSERT_OK(inpSt, "Push_back_before");
    if(inpSt->count == inpSt->capacity)
        expand_stk(inpSt, 1);
    inpSt->data[inpSt->count] = val;
    inpSt->count++;
    write_control_sums(inpSt);
    ASSERT_OK(inpSt, "Push_back_after");
}

unsigned Pop_back(Stack *inpSt)
{
    assert(inpSt != nullptr);
    assert(inpSt->count != 0);
    ASSERT_OK(inpSt, "Pop_back_before");

    unsigned tmp = inpSt->data[--inpSt->count];
    inpSt->data[inpSt->count] = 555;
    if( (inpSt->capacity != 1) && (inpSt->count == inpSt->capacity/2) )
        expand_stk(inpSt, 0);

    write_control_sums(inpSt);
    ASSERT_OK(inpSt, "Pop_back_after");
    return tmp;
}

void My_dump(Stack *inpSt)
{
    printf("cap= %d,    count= %d\n", inpSt->capacity, inpSt->count);
    for (unsigned i = 0; i <= inpSt->capacity+1; i++)
        printf("%d ", inpSt->dat_can1[i]);
    puts("");
    puts("");
}

unsigned StackSize(Stack *inpSt)
{
    assert(inpSt != nullptr);
    return inpSt->count;
}

unsigned StackCapacity(Stack *inpSt)
{
    assert(inpSt != nullptr);
    return inpSt->capacity;
}

double StackTop(Stack *inpSt)
{
    assert(inpSt != nullptr);
    return inpSt->data[inpSt->count-1];
}

int main()
{
    Stack st = StackNew();
    Push_back(&st, 1);
    Push_back(&st, 2);
    Push_back(&st, 3);
    Push_back(&st, 4);
    Push_back(&st, 5);
    Push_back(&st, 6);
    Push_back(&st, 7);
    UNITTEST(Pop_back(&st), ==, 7);
    UNITTEST(Pop_back(&st), ==, 6);
    UNITTEST(Pop_back(&st), ==, 5);
    UNITTEST(Pop_back(&st), ==, 4);
    UNITTEST(Pop_back(&st), ==, 3);
    UNITTEST(Pop_back(&st), ==, 2);
    UNITTEST(Pop_back(&st), ==, 6);
    UNITTEST(Pop_back(&st), ==, 0);
    return 0;
}
