// Oliver Leontiev
// DSA, Zadanie 1
// 2020
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct freeblock {
	unsigned int header;
	struct freeblock *next;
}FREEBLOCK;

typedef struct fullblock {
	unsigned short header;
}FULLBLOCK;

typedef struct root {
	FREEBLOCK *next;
	char* end;

}ROOT;

ROOT * master_header;

void memory_init(void* ptr, unsigned int size) {
	master_header = ptr;
	FREEBLOCK* first_free;

	first_free = (FREEBLOCK*)(master_header + 1);
	master_header->next = first_free; // prvy volny blok je za master_headerom
	master_header->end = ((char*)master_header + size - 1);
	first_free->header = size - sizeof(ROOT);
	first_free->next = NULL;

}

void addblock(FREEBLOCK *p, FREEBLOCK* prev, unsigned short size) {
	unsigned int newsize = size;
	unsigned int oldsize = p->header;
	FREEBLOCK* next = p->next;
	char* pom = (char*)p;
	FREEBLOCK* remainder;

	if (newsize <= oldsize-8) { // najmensi mozny blok je 8B + hlavicka = 10B
		pom = pom + newsize;
		remainder = (FREEBLOCK*)pom;
		remainder->header = oldsize - newsize;
		((FULLBLOCK*)p)->header = newsize;
		remainder->next = next;
		next=remainder;
	}
	if (prev == (FREEBLOCK*)master_header)
		master_header->next = next;
	else
		prev->next = next;	
}

void* memory_alloc(unsigned int size) { 
	FREEBLOCK* p;
	FREEBLOCK* prev; // zachovavame predka aby sme ho vedeli nasmerovat na dalsi volny blok po alokacii
	FREEBLOCK* best = NULL;
	FREEBLOCK* best_prev = NULL;

	if (master_header->next == NULL) // plna pamat
		return NULL;

	prev = (FREEBLOCK*)master_header; 
	p = (FREEBLOCK*)master_header->next;
	if (p->header >= size + sizeof(FULLBLOCK)) { // overujeme ci je prvy volny blok dostatocne velky
		best = p;
		best_prev = (FREEBLOCK*)prev;
	}
	while (p->next != NULL) { // hladame volny blok s velkostou najblizsou size
		prev = p;
		p = p->next;
		if ((p->header >= size + sizeof(FULLBLOCK))
			&& ( (best == NULL) || (best->header > p->header ) ))
		{
			best = p;
			best_prev = prev;
		}
	}
	if (best == NULL) // ziadny volny blok nie je dost velky
		return NULL;

	addblock(best, best_prev, size + sizeof(FULLBLOCK));
	return ((FULLBLOCK*)best + 1);
}


void freeblock(FULLBLOCK* p, FREEBLOCK* prev) {
	FREEBLOCK* next;
	unsigned int next_size;
	unsigned int p_size = (unsigned int)((unsigned short)p->header);
	// case |mh|p|... kde mh je master_header
	if (prev == (FREEBLOCK*)master_header) {
		//case |mh|p|full...|end|
		if (master_header->next == NULL) {
			master_header->next = (FREEBLOCK*)p;
			((FREEBLOCK*)p)->header = p_size;
			((FREEBLOCK*)p)->next = NULL;
		}
		else {
			next = master_header->next;
			next_size = next->header;
			master_header->next = (FREEBLOCK*)p;
			//case |mh|p|free|
			if ((char*)next == (char*)p + p_size) {
				((FREEBLOCK*)p)->header = p_size + next_size;
				((FREEBLOCK*)p)->next = next->next;
			}//case |mh|p|full...|free
			else {
				((FREEBLOCK*)p)->header = p_size;
				((FREEBLOCK*)p)->next = next;
			}
		}
		return;
	}
	//case |free|p|end|
	if (prev->next == NULL) {
		if ((char*)prev + prev->header == (char*)p) {
			prev->header += p->header;
			return;
		}
		//case |free|p|full...|end|
		prev->next = (FREEBLOCK*)p;
		((FREEBLOCK*)p)->header = p_size;
		((FREEBLOCK*)p)->next = NULL;
		return;
	}
	next = prev->next;
	next_size = next->header;
	//case |free|p|free|
	if ((char*)prev + prev->header == (char*)p && next == (FREEBLOCK*)((char*)p + p_size)) {
		prev->header += p->header + next_size;
		prev->next = next->next;
		return;
	}
	//case |full|p|free|
	if (next == (FREEBLOCK*)((char*)p + p_size)) {
		prev->next = (FREEBLOCK*)p;
		((FREEBLOCK*)p)->header = p_size + next_size;
		((FREEBLOCK*)p)->next = next->next;
		return;
	}
	//case |free|p|full|
	if ((char*)prev + prev->header == (char*)p) {
		prev->header += p->header;
		return;
	}
	// case |full|p|full|
	else {
		prev->next = (FREEBLOCK*)p;
		((FREEBLOCK*)p)->header = p_size;
		((FREEBLOCK*)p)->next = next;
	}
}

int memory_free(void* valid_ptr) {
	FREEBLOCK* prev=(FREEBLOCK*)(master_header->next);
	if (master_header->next == NULL || prev > (FREEBLOCK*)valid_ptr) {
		freeblock((FULLBLOCK*)valid_ptr - 1, (FREEBLOCK*)master_header);
		return 0;
	}
	else {
		while (prev->next != NULL && prev->next < (FREEBLOCK*)valid_ptr ) {
				prev = prev->next;
			}
		freeblock((FULLBLOCK*)valid_ptr-1, (FREEBLOCK*)prev);
		return 0;
	}
	return 1;
}

int memory_check(void* ptr) {
	FREEBLOCK* free;
	FULLBLOCK* full;
	
	if ((char*)ptr<(char*)(master_header+1) || (char*)ptr>master_header->end) //je v celkovej pamati?
		return 0;
	ptr = (FULLBLOCK*)ptr - 1; // premiestnenie na hlavicku
	//hladam freeblocky medzi ktorymi je ptr, potom prehladavam hlavicky fullblockov
	if (master_header->next == NULL || master_header->next > (FREEBLOCK*)ptr)
		full = (FULLBLOCK*)(master_header+1);

	else {
		free = master_header->next;
		while (free->next != NULL && free->next <= (FREEBLOCK*)ptr) {
			free = free->next;
			if (free == (FREEBLOCK*)ptr) // 
				return 0;
		}
		full = (FULLBLOCK*)((char*)free + free->header);
	}
	while ((char*)full <= (char*)ptr && (char*)full< master_header->end) {
		if (full == ((FULLBLOCK*)ptr))
			return 1;
		full = (FULLBLOCK*)((char*)full + full->header);
	}
	return 0;
}

void test1() {
	char region[200];
	char* pointers[50];
	int sizes[50];
	int sum = 0;
	int size_mem, i = 0, j, k, l, a, ideal=0, ideal_pom;
	printf("TEST 1\n");
	
	for (l = 0; l < 3; l++) {
		ideal = 0;
		sum = 0;
		if (l == 0)
			size_mem = 50;
		else if (l==1)
			size_mem = 100;
		else if (l == 2)
			size_mem = 200;
		printf("PAMAT %d BYTOV:\n",size_mem);
		memory_init(region, size_mem);
		for (j = 8; j <= 24; j++) { //j predstavuje bloky od 8 do 24
			a = 0;
			ideal_pom = 0;
			while (ideal_pom + 8 <= size_mem) { // vypocet idealnej situacie
				if ((ideal_pom + j - a > size_mem) && (j - a >= 8))
					a++;
				else
					ideal_pom += j - a;
			}
			i = 0;
			a = 0;
			while (1) { // alokujeme najprv 'j' kym sa zmesti a potom postupne mensie bloky az po 8
				pointers[i] = memory_alloc(j - a);
				if (pointers[i] == NULL)
					a++;
				else {
					sizes[i] = j - a;
					memset(pointers[i], sizes[i], sizes[i]);
					sum += j - a;
					i++;
				}
				if (j - a < 8)
					break;
			}
			// test memory_free / fragmentacie
			for (k = 0; k < 2; k++) {
				a = rand() % i;
				if (memory_check(pointers[a])) {
					memory_free(pointers[a]);
					ideal_pom -= sizes[a];
					sum -= sizes[a];
				}
			}
			while (1) {
				pointers[i] = memory_alloc(8);
				if (pointers[i] == NULL)
					break;
				else {
					sizes[i] = 8;
					memset(pointers[i], sizes[i], sizes[i]);
					sum += 8;
					i++;
				}
			}
			while (ideal_pom + 8 <= size_mem) {
				ideal_pom += 8;
			}
			ideal += ideal_pom;
			for (k = 0; k < i; k++) { //uvolnenie vsetkych alokovanych blokov
				if (memory_check(pointers[k]))
					memory_free(pointers[k]);
			}
			i = 0;
		}
		printf("Percento alokovanej pamate oproti idealnemu pripadu:\t%.2f%%\n", (sum / (double)ideal) * 100);
	}
}
void test2() {
	char region[200];
	char* pointers[50];
	int sizes[50];
	int sum = 0;
	int size_mem, i = 0, j = 0, k, l,a, ideal = 0;
	printf("TEST 2\n");
	for (l = 0; l < 3; l++) {
		sum = 0;
		ideal = 0;
		if (l == 0)
			size_mem = 50;
		else if (l == 1)
			size_mem = 100;
		else if (l == 2)
			size_mem = 200;
		printf("PAMAT %d BYTOV:\n", size_mem);
		memory_init(region, size_mem);	
		while (1) { // generujeme nahodne bloky 8-24B
			j = (rand() % (24 - 8 + 1)) + 8;
			pointers[i] = memory_alloc(j);
			if (pointers[i] == NULL)
				break;
			else {
				sizes[i] = j;
				memset(pointers[i], sizes[i], sizes[i]);
				sum += j;
				ideal += j;
				i++;
			}
		}
		while (1) { // nezmestil sa posledny nahodny blok, snazim sa vyplnit pamat 8B blokmi
			pointers[i] = memory_alloc(8);
			if (pointers[i] == NULL)
				break;
			else {
				sizes[i] = 8;
				memset(pointers[i], sizes[i], sizes[i]);
				sum += 8;
				i++;
			}

		}
		while (ideal +8<=size_mem)
			ideal += 8;
		
		// test memory_free / fragmentacie
		for (k = 0; k < 2; k++) {
			a = rand() % i;
			if (memory_check(pointers[a])) {
				memory_free(pointers[a]);
				ideal -= sizes[a];
				sum -= sizes[a];
			}
		}
		while (1) {
			pointers[i] = memory_alloc(8);
			if (pointers[i] == NULL)
				break;
			else {
				sum += 8;
				memset(pointers[i], 8, 8);
				i++;
			}
		}
		while (ideal + 8 <= size_mem) {
			ideal += 8;
		}
		for (k = 0; k < i; k++) { //uvolnenie vsetkych alokovanych blokov
			if (memory_check(pointers[k]))
				memory_free(pointers[k]);
		}
		i = 0;
		printf("Percento alokovanej pamate oproti idealnemu pripadu:\t%.2f%%\n", (sum / (double)ideal) * 100);
	}
		
}
void test3() {
	char region[100000];
	char* pointers[201];
	int sizes[201] ;
	int sum = 0;
	int size_mem, i = 0, j = 0, k, l, ideal = 0,a;
	printf("TEST 3\n");
	sum = 0;
	ideal = 0;	
	for (l = 0; l < 2; l++) {
		if (l == 0)
			size_mem = 10000;
		else if (l==1)
			size_mem = 100000;
		sum = 0;
		ideal = 0;
		printf("PAMAT %d BYTOV:\n", size_mem);
		memory_init(region, size_mem);
		while (1) {
			j = (rand() % (5000 - 500  + 1)) + 500;
			pointers[i] = memory_alloc(j);
			if (pointers[i] == NULL)
				break;
			else {
				sizes[i] = j;
				memset(pointers[i], 1, j);
				sum += j;
				ideal += j;
				i++;
			}
		}
		while (1) {
			pointers[i] = memory_alloc(500);
			if (pointers[i] == NULL)
				break;
			else {
				sizes[i] = 500;
				memset(pointers[i], 2, 500);
				sum += 500;
				i++;
			}

		}
		while (ideal + 500 <= size_mem)
			ideal += 500;
		for (k = 0; k < 5; k++) {
			a = rand() % i; 
			if (memory_check(pointers[a])) {
				memory_free(pointers[a]);
				ideal -= sizes[a];
				sum -= sizes[a];
			}
		}
		while (1) {
			pointers[i] = memory_alloc(500);
			if (pointers[i] == NULL)
				break;
			else {
				sum += 500;
				memset(pointers[i], 3, 500);
				i++;
			}
		}
		while (ideal + 500 <= size_mem) {
			ideal += 500;
		}
		for (k = 0; k < i; k++) { //uvolnenie vsetkych alokovanych blokov
			if (memory_check(pointers[k]))
				memory_free(pointers[k]);
		}
		i = 0;
		printf("Percento alokovanej pamate oproti idealnemu pripadu:\t%.2f%%\n", (sum / (double)ideal) * 100);
	}
}
void test4() {
	char region[100000];
	char* pointers[20000];
	int sizes[20000];
	int sum = 0;
	int size_mem, i = 0, j = 0, k, ideal = 0, a;
	printf("TEST 4\n");
	sum = 0;
	ideal = 0;
		
	size_mem = 100000;
	sum = 0;
	ideal = 0;
	printf("PAMAT %d BYTOV:\n", size_mem);
	memory_init(region, size_mem);
	while (1) {
		j = (rand() % (50000 - 8 + 1)) + 8;
		pointers[i] = memory_alloc(j);
		if (pointers[i] == NULL)
			break;
		else {
			sizes[i] = j;
			memset(pointers[i], 1, j);
			sum += j;
			ideal += j;
			i++;
		}
	}
	while (1) {
		pointers[i] = memory_alloc(8);
		if (pointers[i] == NULL)
			break;
		else {
			sizes[i] = 8;
			memset(pointers[i], 8, 8);
			sum += 8;
			i++;
		}

	}
	while (ideal + 8 <= size_mem)
			ideal += 8;
	for (k = 0; k < 5; k++) {
		a = rand() % i;
		if (memory_check(pointers[a])) {
			memory_free(pointers[a]);
			ideal -= sizes[a];
			sum -= sizes[a];
		}
	}
	while (1) {
		pointers[i] = memory_alloc(8);
		if (pointers[i] == NULL)
			break;
		else {
			sum += 8;
			memset(pointers[i], 8, 8);
			i++;
		}
	}
	while (ideal + 8 <= size_mem) {
		ideal += 8;
	}
	for (k = 0; k < i; k++) { //uvolnenie vsetkych alokovanych blokov
		if (memory_check(pointers[k]))
			memory_free(pointers[k]);
	}
	i = 0;
	printf("Percento alokovanej pamate oproti idealnemu pripadu:\t%.2f%%\n", (sum / (double)ideal) * 100);
}

int main() {
	srand(time(0));
	test1();
	test2();
	test3();
	test4();
	return 0;
}