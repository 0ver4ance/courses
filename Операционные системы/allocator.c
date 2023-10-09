#include <stdbool.h>
#define MIN_USER_SEG_SIZE 16

void *HEAD_PTR;
void *MEM_START;
void *MEM_END;
size_t META_INFO_SIZE;


typedef struct header {
    void *next;
    void *prev;
    size_t size;
    bool isFree;
} header;

typedef struct footer {
    size_t size;
    bool isFree;
} footer;


void setHeader(void *headStart, size_t size, bool isFree);
void setFooter(void *headStart, size_t size, bool isFree);
void setHeaderAndFooter(void *headStart, size_t size, bool isFree);
void popSegment(header *head);

void mysetup(void *buf, size_t size);
void* myalloc(size_t size);
void myfree(void *p);

// int main() {
//   void *buf = malloc(BUF_SIZE);        Выделение памяти, которую будет распределять аллокатор
//   mysetup(buf, BUF_SIZE);              Начальная инциализация
//   void *ptr = myalloc(USER_SEG_SIZE);  Выделение памяти
//   myfree(ptr);                         Освобождение памяти
//   return 0;
// }

// По нужному адресу вставляет в участок памяти header
void setHeader(void *headStart, size_t size, bool isFree) {
    header *head = headStart;
    head->size = size;
    head->next = NULL;
    head->prev = NULL;
    head->isFree = isFree;
}

// По нужному адресу вставляет в участок памяти footer
void setFooter(void *headStart, size_t size, bool isFree) {
    footer *tail = (footer *)((char *)headStart + size - sizeof(footer));
    tail->size = size;
    tail->isFree = isFree;
}

// Объединение функций setHeader и setFooter
void setHeaderAndFooter(void *headStart, size_t size, bool isFree) {
    setHeader(headStart, size, isFree);
    setFooter(headStart, size, isFree);
}

// Удаляет сегмент памяти из списка свободных сегментов
void popSegment(header *head) {
    header *headNext = head->next;
    header *headPrev = head->prev;
    headPrev->next = headNext;
    if (headNext != NULL) {
        headNext->prev = headPrev;
    }
}

// Устанавливает значение констант и первично инициализирует память
void mysetup(void *buf, size_t size) {
    HEAD_PTR = buf;
    MEM_START = buf;
    MEM_END = (void *)((char *)buf + size);
    META_INFO_SIZE = sizeof(footer) + sizeof(header);
    setHeaderAndFooter(buf, size, true);
}

// Функция аллокации
void* myalloc(size_t size) {
    int newSegSize = size + META_INFO_SIZE;
    header *currFreeSeg = HEAD_PTR;
    while (currFreeSeg != NULL) {
        if (currFreeSeg->size >= MIN_USER_SEG_SIZE + META_INFO_SIZE + newSegSize) {
            // Сюда переходим только если после того, как отдадим пользователю память,
            // осташейся памяти в текущем сегменте хватит на то, чтобы отдать в будущем сегмент
            // не меньше MIN_USER_SEG_SIZE
            currFreeSeg->size -= newSegSize;
            setFooter(currFreeSeg, currFreeSeg->size, true);
            void *newHeadStart = (char *)currFreeSeg + currFreeSeg->size;
            setHeaderAndFooter(newHeadStart, newSegSize, false);
            void *userSpaceStart = (char *)newHeadStart + sizeof(header);
            return userSpaceStart;
        } if (currFreeSeg->size >= newSegSize) {
            // Сюда переходим если верхний if не сработал, но памяти в текущем сегменте
            // хватит для пользователя. В таком случае отдаем пользователю весь текущий семент,
            // то есть его размер может быть больше на META_INFO_SIZE + MIN_USER_SEG_SIZE - 1, чем просил пользователь
            header *currFreeSegNext = currFreeSeg->next;
            header *currFreeSegPrev = currFreeSeg->prev;
            if (currFreeSegPrev == NULL)
                HEAD_PTR = currFreeSegNext;
            else
                currFreeSegPrev->next = currFreeSegNext;
            if (currFreeSegNext != NULL)
                currFreeSegNext->prev = currFreeSegPrev;
            setHeaderAndFooter(currFreeSeg, currFreeSeg->size, false);
            void *userSpaceStart = (char *)currFreeSeg + sizeof(header);
            return userSpaceStart;
        }
        currFreeSeg = currFreeSeg->next;
    }
    // Если память кончилась, то возвращаем NULL
    return NULL;
}

// Освобождает ранее выделенную пользователю память
void myfree(void *p) {
    header *currHead = (header *)((char *)p - sizeof(header));
    footer *currTail = (footer *)((char *)currHead + currHead->size - sizeof(footer));
    header *leftHead = NULL;
    footer *leftTail = (footer *)((char *)currHead - sizeof(footer));
    header *rightHead = (header *)((char *)currTail + sizeof(footer));
    footer *rightTail = NULL;

    currHead->isFree = true;
    currTail->isFree = true;

    if (currHead != MEM_START && leftTail->isFree == true) {
        // Сюда переходим если слева от освобождаемого сегмента свободный сегмент.
        // В таком случае необходимо увеличить размер headder свободного сегмента слева и
        // footer текущего освобождаемого сегмента
        leftHead = (header *)((char *)currHead - leftTail->size);
        leftHead->size += currHead->size;
        currTail->size += leftTail->size;
        currHead = leftHead;
    }
    if (MEM_END != rightHead && rightHead->isFree == true) {
        // Сюда переходим если справа от освобождаемого сегмента свободный сегмент.
        // В таком случае необходимо увеличить размер header текущего особождаемого сегмента либо
        // свободного сегмента слева и footer свободного сегмента справа
        rightTail = (footer *)((char *)rightHead + rightHead->size - sizeof(footer));
        rightTail->size += currHead->size;
        currHead->size += rightHead->size;
    }
    bool leftMerge = leftHead != NULL;
    bool rightMerge = rightTail != NULL;
    if (!(leftMerge || rightMerge)) {
        // Сюда переходим если ни слева, ни справа нет свободных сегментов.
        // В таком случае необходимо текущий освобождаемый сегмент поставить в начало связного списка
        header *oldFirstHead = HEAD_PTR;
        HEAD_PTR = currHead;
        currHead->next = oldFirstHead;
        if (oldFirstHead != NULL)
            oldFirstHead->prev = currHead;
    } else if (leftMerge == false && rightMerge == true) {
        // Сюда переходим если слева не было свободного сегмента, а справа был.
        // В таком случае необходимо все мета данные с правого сегмента перекинуть на текущий освобождаемый
        header *rightHeadPrev = rightHead->prev;
        header *rightHeadNext = rightHead->next;
        if (rightHeadPrev != NULL)
            rightHeadPrev->next = currHead;
        else
            HEAD_PTR = currHead;
        if (rightHeadNext != NULL)
            rightHeadNext->prev = currHead;
        currHead->prev = rightHead->prev;
        currHead->next = rightHead->next;
    } else if (leftMerge && rightMerge) {
        // Сюда попадаем если и справа были свободные сегменты.
        // В таком случае необходимо сначала левый сегмент поставить в начало связного списка.
        // Потом правый сегмент поставить после левого
        if (leftHead->prev != NULL) {
            popSegment(leftHead);
            header *leftHeadNewNext = HEAD_PTR;
            leftHead->prev = NULL;
            leftHead->next = leftHeadNewNext;
            leftHeadNewNext->prev = leftHead;
            HEAD_PTR = leftHead;
        }
        if (rightHead->prev != leftHead) {
            popSegment(rightHead);
            header *leftHeadNext = leftHead->next;
            leftHead->next = rightHead;
            rightHead->prev = leftHead;
            rightHead->next = leftHeadNext;
            leftHeadNext->prev = rightHead;
        }
        header *rightHeadNext = rightHead->next;
        leftHead->next = rightHeadNext;
        if (rightHeadNext != NULL)
            rightHeadNext->prev = leftHead;
    }
}
