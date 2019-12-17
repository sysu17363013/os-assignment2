#include"string.h"
#include"stdio.h"
#include <stdlib.h>
#include<unistd.h>
#define tlbSIZE 16

struct logicalAddress    
{
	int lad;         //逻辑地址 
	int pagenum;     //页码 
	int offset;      //偏移量 
	int physicalAddress;      //物理地址 
	int key;         //内容 
}laddress[1000];

struct pageTable     
{
	int page;       //页码 
	int frame;      //帧码 
}pagetable[256],tlb[16];

struct tlb_lru      //lru算法中用链表实现的tlb 
{
    int page;       
    int frame;
    struct tlb_lru *next;
}*tlb_head, *tlb_tail, *tlb_p;

struct pmemory_lru   //lru算法中用链表实现的物理内存空间 
{
	int index;       //帧码 
	char page[256];  //页内容 
	struct pmemory_lru *next;   //指向下一页 
}*pm_head, *pm_tail, *pm_p;

void init(struct pageTable* pt)   //页表、tlb初始化 
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

/*在tlb链表尾加上帧码为a的tlb项 */
void tlb_Pushback(int a)     
{
    tlb_p->next = NULL;
    tlb_tail->next = tlb_p;
    tlb_tail = tlb_p;
    tlb_tail->frame = a;
}

/*在物理内存链表尾加上帧码为a的页 */
void pm_PushBack(int a)
{
    pm_p->next = NULL;
    pm_tail->next = pm_p;
    pm_tail = pm_p;
    pm_tail->index = a;
}

/*释放物理内存链表的头结点*/ 
void pm_releaseHead(int a)
{
    pm_PushBack(a);
    pm_p = pm_head->next;
    pm_head->next = pm_p->next;
    free(pm_p);
}

/*将帧码为a的tlb项插入链表尾部，并释放tlb链表的头结点*/
void releaseHead(int a)
{
    //printf("发生缺页中断 %d\n",p->id);
    tlb_Pushback(a);
    tlb_p = tlb_head->next;
    tlb_head->next = tlb_p->next;
    free(tlb_p);
}

/*将帧码为a的tlb项插入tlb链表*/ 
void insert(int* num, int a)
{
	if (*num < tlbSIZE) //cache未满，放至后面
    {
        tlb_Pushback(a);
        *num += 1; //并对cache+1
    }
    else
    {
    	releaseHead(a);//将帧码为a的tlb项插入链表尾部，并释放tlb链表的头结点
	}
}

/*将逻辑地址转化为页码和偏移量*/ 
void advert(int a, int index, struct logicalAddress* num)
{
	num[index].lad = a;
	num[index].pagenum = a / 256;
	num[index].offset = a % 256; 
}

/*检测页码为page的项是否在页表中*/ 
int in_mem_item(int page,struct pageTable* pt)
{
  int a;
  int b = -1;
  for(a = 0;a < 256;a++){
    	if(pt[a].page == page){   //存在 
    		b = a;
    		return b;             //返回页表项下标 
      	}
  }
  return b;    //返回-1 
}

/* 检测页码为page的项是否在tlb表中*/
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

//FIFO策略 
void FIFO(FILE* backingstore, int ptSIZE)
{
	int i, j, k, tlb_hit, pagefault, index_t, index_p;
	char pmemory[ptSIZE][256]; 
	i = 0;    //记录逻辑地址项数 
	j = 0, k = 0;   //j页表条数，k为tlb表条数 
	tlb_hit = 0;   
	pagefault = 0;
	while(i < 1000)
	{
		index_t = in_mem_tlb(laddress[i].pagenum, tlb);
		if(index_t >= 0)//该页存在于tlb中    
		{
			tlb_hit++;
			laddress[i].physicalAddress = tlb[index_t].frame * 256 + laddress[i].offset;
			laddress[i].key = pmemory[tlb[index_t].frame][laddress[i].offset];
			//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
      printf("%d\n", laddress[i].key);
		}
		else if(in_mem_item(laddress[i].pagenum, pagetable) >= 0)//该页存在于页表中 
		{
			index_p = in_mem_item(laddress[i].pagenum, pagetable);
			laddress[i].physicalAddress = pagetable[index_p].frame * 256 + laddress[i].offset;
			laddress[i].key = pmemory[pagetable[index_p].frame][laddress[i].offset];
			tlb[k].page = laddress[i].pagenum;    //将该页加入到tlb中 
			tlb[k].frame = pagetable[index_p].frame;
			k = (k + 1) % 16;      //tlb满后覆盖tlb第一项 
			//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
      printf("%d\n", laddress[i].key);
		}
		else//该页不在页表中 
		{
			pagefault++;
			fseek(backingstore, laddress[i].pagenum * 256, SEEK_SET);//将该页内容从bin文件读入物理内存 
			fread(pmemory[j], 1, 256, backingstore);  
			pagetable[j].page = laddress[i].pagenum;//添加页表项 
			pagetable[j].frame = j;
			laddress[i].physicalAddress = j * 256 + laddress[i].offset;
			laddress[i].key = pmemory[j][laddress[i].offset];
			tlb[k].page = laddress[i].pagenum;//添加tlb项 
			tlb[k].frame = j;
			j = (j + 1) % ptSIZE;//页表满后覆盖页表第1项 
			k = (k + 1) % 16;
			//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
      printf("%d\n", laddress[i].key);
		}
		i++;
	}
	printf("Page Faults = %d\n", pagefault);
  printf("TLB Hits= %d\n",tlb_hit);
}

//LRU策略 
void LRU(FILE* backingstore, int ptSIZE)
{
	int i, j, k, tlb_hit, pagefault, index_p, item_num, page_num; 
	i = 0;//记录逻辑地址项数
	j = 0;//记录页表条目数 
	tlb_hit = 0;
	pagefault = 0;
	item_num = 0;//tlb表项数
	while(i < 1000)
	{
		tlb_p = (struct tlb_lru *)malloc(sizeof(struct tlb_lru));
		tlb_p->page = laddress[i].pagenum;//为第i项逻辑地址创建tlb项页节点 
		pm_p = (struct pmemory_lru *)malloc(sizeof(struct pmemory_lru));
		struct tlb_lru *q; //用于遍历tlb链表 
	    q = tlb_head;
	    struct pmemory_lru *v;  //用于遍历物理内存链表 
	    v = pm_head;
	    while (q->next != NULL)//遍历tlb链表 
	    {
	        if (q->next->page == tlb_p->page)//tlb hit
	        {
	        	tlb_hit++;
	            tlb_Pushback(q->next->frame);
	            tlb_p = q->next;
	            q->next = tlb_p->next;
	            free(tlb_p);//释放原相同tlb项 
	            laddress[i].physicalAddress = tlb_tail->frame * 256 + laddress[i].offset;
	            while(v->next->index != tlb_tail->frame)
	            {
	            	v = v->next;
				}
				pm_PushBack(v->next->index);//将新tlb项插入表尾 
	            /*同理将该物理内存页调入尾部*/ 
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
	    	if(in_mem_item(laddress[i].pagenum, pagetable) >= 0) //页表中存在 
	    	{
	    		index_p = in_mem_item(laddress[i].pagenum, pagetable);
				laddress[i].physicalAddress = pagetable[index_p].frame * 256 + laddress[i].offset;
				insert(&item_num, pagetable[index_p].frame);  //插入页表 
				while(v->next->index != tlb_tail->frame)
	            {
	            	v = v->next;
				}
	            pm_PushBack(v->next->index);//将该页调至物理内存链表尾 
	            for(k=0;k<256;k++)
	            {
	            	pm_tail->page[k] = v->next->page[k];
				}
	            pm_p = v->next;
	            v->next = pm_p->next;
	            free(pm_p);//释放原物理页项 
	            laddress[i].key = pm_tail->page[laddress[i].offset];
				//printf("Virtual address: %d Physical address: %d Value: %d\n", laddress[i].lad, laddress[i].physicalAddress, laddress[i].key);
        		printf("%d\n", laddress[i].key);
			}
	    	else//页表中不存在该页码 
	    	{
	    		pagefault++;
	    		if(j < ptSIZE)//页表（物理内存）未满 
	    		{
	    			pagetable[j].page = laddress[i].pagenum;
	    			pm_PushBack(j);//将该页调入物理内存 
        			pagetable[j].frame = pm_tail->index;
        			insert(&item_num, pm_tail->index);
				}
				else
				{
					int m = 0;//页表（物理内存）已满 
					while(pagetable[m].frame != pm_head->next->index)//找到需要释放的页表项 
					{
						m++;
					}
					pagetable[m].page = laddress[i].pagenum;//在该页表项中填入新页码 
					pm_releaseHead(pm_head->next->index);//将新页插入物理内存 
					insert(&item_num, pm_tail->index);//将新tlb项插入tlb表 
				}
	    		fseek(backingstore, laddress[i].pagenum * 256, SEEK_SET);
	    		fread(pm_tail->page, 1, 256, backingstore);//将该页内容读入物理内存 
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
  /*-n传入物理内存空间大小，-t传入替换策略 fifo或lru*/ 
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
    init(pagetable);//页表初始化 
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
    while(i < 1000)/*将逻辑地址存入laddress数组*/ 
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
