#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct OneLinkedList OneLinkedList;
typedef struct Thread Thread;

// cpuIsUsed равен true, если на данный момент на процессоре исполняется поток
// timeSlice - максимальное время, которое может провести на процессоре поток
// cntTicks - количество времени, которое текущий поток исполняется на процессоре.
// Если cntTicks == timeSplice, то scheduler принудительно снимет поток с процесса и поставит первый из очереди readyThreads
// readyThreads - очередь тех потоков, которые готовы к исполнению. Новые потоки отправляются в конец очереди,
// а смене потока новый берется из начала очереди
// waitTreads - список заблокированных потоков. После разблокировки поток попадает в конец очереди readyThreads
typedef struct Scheduler {
    bool cpuIsUsed;
    int timeSlice;
    int cntTicks;
    Thread *currThread;
    OneLinkedList *readyThreads;
    OneLinkedList *waitThreads;
} Scheduler;

// threadID - id потока
// next - указатель на следующий поток в односвязном списке
struct Thread {
    int threadID;
    Thread *next;
};

// head - указатель на начало односвязного списка
// tail - указатель на конец односвязного списка
struct OneLinkedList {
    Thread* head;
    Thread* tail;
};

Scheduler *scheduler = NULL;

// Инициализирует одно связный список после его создания
void initList(OneLinkedList *list) {
    list->head = NULL;
    list->tail = NULL;
}

// Инициализирует поток после его создания
void initThread(int threadID, Thread *thread) {
    thread->threadID = threadID;
    thread->next = NULL;
}

// Добавляет в конец одно связного списка элемент
void pushBack(Thread *thread, OneLinkedList *list) {
    if (list->tail == NULL) {
        list->head = thread;
    } else {
        Thread *listTail = list->tail;
        listTail->next = thread;
    }
    list->tail = thread;
}

// Удаляет из начала односвязного списка элемент
Thread* popFront(OneLinkedList *list) {
    if (list->head == NULL)
        return NULL;
    Thread *popNode = list->head;
    if (list->head == list->tail) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->head = popNode->next;
    }
    return popNode;
}

// Удаляет из односвязного списка элемент, значение которого равно threadID
Thread* pop(int threadID, OneLinkedList *list) {
    if (list->head == NULL)
        return NULL;
    Thread *headNode = list->head;
    if (headNode->threadID == threadID)
        return popFront(list);
    Thread *prevNode = headNode;
    Thread *currNode = headNode->next;
    while (currNode != NULL) {
        if (currNode->threadID == threadID) {
            prevNode->next = currNode->next;
            return currNode;
        }
        prevNode = currNode;
        currNode = currNode->next;
    }
    return NULL;
}

// Возвращает true если в односвязном списке есть хотя бы один элемент
bool isListNotEmpty(OneLinkedList *list) {
    return list->head != NULL;
}

// Если в очереди готовых к исполнению потоков есть хотя бы один элемент, то функция запустит его.
// Если потоков в очереди нет, то переведет процессор в состояние простоя
void startNextThread() {
    if (scheduler != NULL) {
        scheduler->cntTicks = 0;
        if (isListNotEmpty(scheduler->readyThreads)) {
            scheduler->currThread = popFront(scheduler->readyThreads);
            scheduler->cpuIsUsed = true;
        } else {
            scheduler->currThread = NULL;
            scheduler->cpuIsUsed = false;
        }
    }
}

// Инициализирует начальное состояние scheduler.
// timeslice - это размер кванта времени. Он определяет сколько один поток может исполняться на процессоре
void scheduler_setup(int timeslice) {
    if (scheduler == NULL) {
        scheduler = malloc(sizeof(Scheduler));
        scheduler->readyThreads = malloc(sizeof(OneLinkedList));
        scheduler->waitThreads = malloc(sizeof(OneLinkedList));
    }
    scheduler->cpuIsUsed = false;
    scheduler->timeSlice = timeslice;
    scheduler->cntTicks = 0;
    scheduler->currThread = NULL;

    initList(scheduler->readyThreads);
    initList(scheduler->waitThreads);
}

// Функция оповещает scheduler о создании нового потока, айди которого thread_id.
// Если процессор находится в состоянии простоя, то запускает новый поток
void new_thread(int thread_id) {
    if (scheduler != NULL) {
        Thread *newThread = malloc(sizeof(Thread));
        initThread(thread_id, newThread);
        pushBack(newThread, scheduler->readyThreads);
        if (scheduler->cpuIsUsed == false)
            startNextThread();
    }
}

// Функция оповещает scheduler, что поток, который на данный момент находится на процессоре
// закончил свою работу
void exit_thread() {
    if (scheduler != NULL && scheduler->cpuIsUsed == true) {
        free(scheduler->currThread);
        startNextThread();
    }
}

// Данная функция снимает с процессора поток, который исполняется на данный момент
// и перемещает поток в список weitThreads
void block_thread() {
    if (scheduler != NULL && scheduler->cpuIsUsed == true) {
        pushBack(scheduler->currThread, scheduler->waitThreads);
        startNextThread();
    }
}

// Данная функция перемещает поток с thread_id из weitThreads в readyThreads
void wake_thread(int thread_id) {
    if (scheduler != NULL) {
        Thread *wakeThread = pop(thread_id, scheduler->waitThreads);
        if (wakeThread != NULL) {
            pushBack(wakeThread, scheduler->readyThreads);
            if (scheduler->cpuIsUsed == false)
                startNextThread();
        }
    }
}

// Данная функция эмулирует работу таймера.
// Т.е. если при инциализации scheduler timesplice был равен 3, то
// на третий вызов этой функции, она снимет текущий поток с процессора,
// переместит его в конец readyThreads, возьмет с начала очереди readyThreads новый поток
// и поместит его на процессор
void timer_tick() {
    if (scheduler != NULL && scheduler->cpuIsUsed == true) {
        scheduler->cntTicks += 1;
        if (scheduler->cntTicks == scheduler->timeSlice) {
            pushBack(scheduler->currThread, scheduler->readyThreads);
            startNextThread();
        }
    }
}

// Функция возвращает id потока, который на момент вызова функции исполнялся на процессоре.
// Если процессор на момент вызова простаивал, то функция вернет -1
int current_thread() {
    if (scheduler == NULL || scheduler->cpuIsUsed == false)
        return -1;
    Thread *currThread = scheduler->currThread;
    return currThread->threadID;
}

// Пример использования:
// int main() {
//     int timeslice = 3;
//     int firstThreadID = 0;
//     int secondThreadID = 1;
//     scheduler_setup(timeslice);
//     new_thread(firstThreadID);
//     new_thread(secondThreadID);
//     printf("%i\n", current_thread()); 0
//     timer_tick();
//     timer_tick();
//     timer_tick();
//     printf("%i\n", current_thread()); 1
//     timer_tick();
//     timer_tick();
//     timer_tick();
//     printf("%i\n", current_thread()); 0
//     timer_tick();
//     timer_tick();
//     timer_tick();
//     printf("%i\n", current_thread()); 1
//     timer_tick();
//     timer_tick();
//     timer_tick();
//     printf("%i\n", current_thread()); 0

//     return 0;
// }
