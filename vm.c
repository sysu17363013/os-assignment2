#include"string.h"
#include"stdio.h"
#include <stdlib.h>
#include<unistd.h>
#define tlbSIZE 16

struct logicalAddress    
{
	int lad;         //�߼���ַ 
	int pagenum;     //ҳ�� 
	int offset;      //ƫ���� 
	int physicalAddress;      //�����ַ 
	int key;         //���� 
}laddress[1000];

struct pageTable     
{
	int page;       //ҳ�� 
	int frame;      //֡�� 
}pagetable[256],tlb[16];

struct tlb_lru      //lru�㷨��������ʵ�ֵ�tlb 
{
    int page;       
    int frame;
    struct tlb_lru *next;
}*tlb_head, *tlb_tail, *tlb_p;

struct pmemory_lru   //lru�㷨��������ʵ�ֵ������ڴ�ռ� 
{
	int index;       //֡�� 
	char page[256];  //ҳ���� 
	struct pmemory_lru *next;   //ָ����һҳ 
}*pm_head, *pm_tail, *pm_p;

void init(struct pageTable* pt)   //ҳ��tlb��ʼ�� 
{
  	int i,j;
  	for(i = 0; i < 256; i++)
	{
    	pt[i].page = -1;
    	pt[i].frame = -1;
	}	
    for(i = 0; i < 16; i++)
	{
    	tlb[i].page = -1;
    	tlb[i].frame = -1;
	}
}

/*��tlb����β����֡��Ϊa��tlb�� */
void tlb_Pushback(int a)     
{
    tlb_p->next = NULL;
    tlb_tail->next = tlb_p;
    tlb_tail = tlb_p;
    tlb_tail->frame = a;
}

/*�������ڴ�����β����֡��Ϊa��ҳ */
void pm_PushBack(int a)
{
    pm_p->next = NULL;
    pm_tail->next = pm_p;
    pm_tail = pm_p;
    pm_tail->index = a;
}

/*�ͷ������ڴ������ͷ���*/ 
void pm_releaseHead(int a)
{
    pm_PushBack(a);
    pm_p = pm_head->next;
    pm_head->next = pm_p->next;
    free(pm_p);
}

/*��֡��Ϊa��tlb���������β�������ͷ�tlb�����ͷ���*/
void releaseHead(int a)
{
    //printf("����ȱҳ�ж� %d\n",p->id);
    tlb_Pushback(a);
    tlb_p = tlb_head->next;
    tlb_head->next = tlb_p->next;
    free(tlb_p);
}

/*��֡��Ϊa��tlb�����tlb����*/ 
void insert(int* num, int a)
{
	if (*num < tlbSIZE) //cacheδ������������
    {
        tlb_Pushback(a);
        *num += 1; //����cache+1
    }
    else
    {
    	releaseHead(a);//��֡��Ϊa��tlb���������β�������ͷ�tlb�����ͷ���
	}
}

/*���߼���ַת��Ϊҳ���ƫ����*/ 
void advert(int a, int index, struct logicalAddress* num)
{
	num[index].lad = a;
	num[index].pagenum = a / 256;
	num[index].offset = a % 256; 
}

/*���ҳ��Ϊpage�����Ƿ���ҳ����*/ 
int in_mem_item(int page,struct pageTable* pt)
{
  int a;
  int b = -1;
  for(a = 0;a < 256;a++){
    	if(pt[a].page == page){   //���� 
    		b = a;
    		return b;             //����ҳ�����±� 
      	}
  }
  return b;    //����-1 
}

/* ���ҳ��Ϊpage�����Ƿ���tlb����*/
int in_mem_tlb(int page,struct pageTable* pt)
{
  int a;
  int b = -1;
  for(a = 0;a < 16;a++){
    	if(pt[a].page == page){
    		b = a;
    		return b;
      	}
  }
  return b;
}

//FIFO���� 
void FIFO(FILE* backingstore, int ptSIZE)
{
	int i, j, k, tlb_hit, pagefault, index_t, index_p;
	char pmemory[ptSIZE][256]; 
	i = 0;    //��¼�߼���ַ���� 
	j = 0, k = 0;   //jҳ��������kΪtlb������ 
	tlb_hit = 0;   
	pagefault = 0;
	while(i < 1000)
	{
		index_t = in_mem_tlb(laddress[i].pagenum, tlb);
		if(index_t >= 0)//��ҳ������tlb��    
		{
			tlb_hit++;
			laddress[i].physicalAddress = tlb[index_t].frame * 256 + laddress[i].offset;
			laddress[i].key = pmemory[tlb[index_t].frame][laddress[i].offset];
			//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
      printf("%d\n", laddress[i].key);
		}
		else if(in_mem_item(laddress[i].pagenum, pagetable) >= 0)//��ҳ������ҳ���� 
		{
			index_p = in_mem_item(laddress[i].pagenum, pagetable);
			laddress[i].physicalAddress = pagetable[index_p].frame * 256 + laddress[i].offset;
			laddress[i].key = pmemory[pagetable[index_p].frame][laddress[i].offset];
			tlb[k].page = laddress[i].pagenum;    //����ҳ���뵽tlb�� 
			tlb[k].frame = pagetable[index_p].frame;
			k = (k + 1) % 16;      //tlb���󸲸�tlb��һ�� 
			//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
      printf("%d\n", laddress[i].key);
		}
		else//��ҳ����ҳ���� 
		{
			pagefault++;
			fseek(backingstore, laddress[i].pagenum * 256, SEEK_SET);//����ҳ���ݴ�bin�ļ����������ڴ� 
			fread(pmemory[j], 1, 256, backingstore);  
			pagetable[j].page = laddress[i].pagenum;//���ҳ���� 
			pagetable[j].frame = j;
			laddress[i].physicalAddress = j * 256 + laddress[i].offset;
			laddress[i].key = pmemory[j][laddress[i].offset];
			tlb[k].page = laddress[i].pagenum;//���tlb�� 
			tlb[k].frame = j;
			j = (j + 1) % ptSIZE;//ҳ�����󸲸�ҳ���1�� 
			k = (k + 1) % 16;
			//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
      printf("%d\n", laddress[i].key);
		}
		i++;
	}
	printf("Page Faults = %d\n", pagefault);
  printf("TLB Hits= %d\n",tlb_hit);
}

//LRU���� 
void LRU(FILE* backingstore, int ptSIZE)
{
	int i, j, k, tlb_hit, pagefault, index_p, item_num, page_num; 
	i = 0;//��¼�߼���ַ����
	j = 0;//��¼ҳ����Ŀ�� 
	tlb_hit = 0;
	pagefault = 0;
	item_num = 0;//tlb������
	while(i < 1000)
	{
		tlb_p = (struct tlb_lru *)malloc(sizeof(struct tlb_lru));
		tlb_p->page = laddress[i].pagenum;//Ϊ��i���߼���ַ����tlb��ҳ�ڵ� 
		pm_p = (struct pmemory_lru *)malloc(sizeof(struct pmemory_lru));
		struct tlb_lru *q; //���ڱ���tlb���� 
	    q = tlb_head;
	    struct pmemory_lru *v;  //���ڱ��������ڴ����� 
	    v = pm_head;
	    while (q->next != NULL)//����tlb���� 
	    {
	        if (q->next->page == tlb_p->page)//tlb hit
	        {
	        	tlb_hit++;
	            tlb_Pushback(q->next->frame);
	            tlb_p = q->next;
	            q->next = tlb_p->next;
	            free(tlb_p);//�ͷ�ԭ��ͬtlb�� 
	            laddress[i].physicalAddress = tlb_tail->frame * 256 + laddress[i].offset;
	            while(v->next->index != tlb_tail->frame)
	            {
	            	v = v->next;
				}
				pm_PushBack(v->next->index);//����tlb������β 
	            /*ͬ���������ڴ�ҳ����β��*/ 
				for(k=0;k<256;k++)
	            {
	            	pm_tail->page[k] = v->next->page[k];
				}
	            pm_p = v->next;
	            v->next = pm_p->next;
	            free(pm_p);
	            laddress[i].key = pm_tail->page[laddress[i].offset];
	            //printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
                printf("%d\n", laddress[i].key);
	            break;
	        }
	        q = q->next;
	    }
	    if(!q->next)//tlb miss
	    {
	    	if(in_mem_item(laddress[i].pagenum, pagetable) >= 0) //ҳ���д��� 
	    	{
	    		index_p = in_mem_item(laddress[i].pagenum, pagetable);
				laddress[i].physicalAddress = pagetable[index_p].frame * 256 + laddress[i].offset;
				insert(&item_num, pagetable[index_p].frame);  //����ҳ�� 
				while(v->next->index != tlb_tail->frame)
	            {
	            	v = v->next;
				}
	            pm_PushBack(v->next->index);//����ҳ���������ڴ�����β 
	            for(k=0;k<256;k++)
	            {
	            	pm_tail->page[k] = v->next->page[k];
				}
	            pm_p = v->next;
	            v->next = pm_p->next;
	            free(pm_p);//�ͷ�ԭ����ҳ�� 
	            laddress[i].key = pm_tail->page[laddress[i].offset];
				//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
        		printf("%d\n", laddress[i].key);
			}
	    	else//ҳ���в����ڸ�ҳ�� 
	    	{
	    		pagefault++;
	    		if(j < ptSIZE)//ҳ�������ڴ棩δ�� 
	    		{
	    			pagetable[j].page = laddress[i].pagenum;
	    			pm_PushBack(j);//����ҳ���������ڴ� 
        			pagetable[j].frame = pm_tail->index;
        			insert(&item_num, pm_tail->index);
				}
				else
				{
					int m = 0;//ҳ�������ڴ棩���� 
					while(pagetable[m].frame != pm_head->next->index)//�ҵ���Ҫ�ͷŵ�ҳ���� 
					{
						m++;
					}
					pagetable[m].page = laddress[i].pagenum;//�ڸ�ҳ������������ҳ�� 
					pm_releaseHead(pm_head->next->index);//����ҳ���������ڴ� 
					insert(&item_num, pm_tail->index);//����tlb�����tlb�� 
				}
	    		fseek(backingstore, laddress[i].pagenum * 256, SEEK_SET);
	    		fread(pm_tail->page, 1, 256, backingstore);//����ҳ���ݶ��������ڴ� 
				laddress[i].physicalAddress = pm_tail->index * 256 + laddress[i].offset;
				laddress[i].key = pm_tail->page[laddress[i].offset];
				//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
        		printf("%d\n", laddress[i].key);
				j = j + 1;
			}
		}
		i++;
	}
	printf("Page Faults = %d\n", pagefault);
  printf("TLB Hits = %d\n",tlb_hit);
}

void Usage()
{
	printf("Wrong parameters\n");
}

int main(int argc, char * const argv[])
{
	int ptSIZE;
	void (*pfun)(FILE*, int);
  int opt;
  /*-n���������ڴ�ռ��С��-t�����滻���� fifo��lru*/ 
  while((opt = getopt(argc, argv, "n:t:")) != -1) 
	{
		switch(opt)
		{
			case 'n':
				ptSIZE = atoi(optarg);
        		break;
			case 't':
				if(strcmp(optarg, "fifo") == 0)
					pfun = FIFO;
				else if(strcmp(optarg, "lru") == 0)
					pfun = LRU;
        		break;
			default:
				Usage();
				exit(-1);
		}
  }
	int i,j;
    init(pagetable);//ҳ���ʼ�� 
    tlb_head = (struct tlb_lru *)malloc(sizeof(struct tlb_lru));
    tlb_head->next = NULL;
    tlb_tail = tlb_head;
    pm_head = (struct pmemory_lru *)malloc(sizeof(struct pmemory_lru));
    pm_head->next = NULL;
    pm_tail = pm_head;
    FILE* address;
    address = fopen("./addresses.txt", "r+");
    char* ad = (char*)malloc(sizeof(char)*7);
    i = 0;
    while(i < 1000)/*���߼���ַ����laddress����*/ 
    {
    	fgets(ad, 7, address);
    	j = atoi(ad);
    	advert(j, i, laddress);
    	i++;
	}
  FILE* backingstore;
  backingstore = fopen("./BACKING_STORE.bin","r+");
  (*pfun)(backingstore, ptSIZE);
  return 0;
}
