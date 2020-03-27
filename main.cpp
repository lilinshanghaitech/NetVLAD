#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>

int     memfd;
void    *mapped_convreg_base;
void    *mapped_vladreg_base;
void    *mapped_ddr_base;
void    *mapped_inst_base;

#undef readl
#define readl(addr) \
    ({ unsigned int __v = (*(volatile unsigned int *) (addr)); __v; })

#undef writel
#define writel(addr,b) (void)((*(volatile unsigned int *) (addr)) = (b))

#undef readf
#define readf(addr) \
    ({ float __v = (*(volatile float *) (addr)); __v; })
#undef writef
#define writef(addr,b) (void)((*(volatile float *) (addr)) = (b))


#define CONVREG_BASE_ADDRESS     0xA0010000
#define VLADREG_BASE_ADDRESS    0xA0000000
#define DDR_BASE_ADDRESS     0x01000000


void *memory_map(unsigned int map_size, off_t base_addr) //map_size = n MByte
{
    void *mapped_base;
    mapped_base = mmap(0, map_size*1024*1024, PROT_READ | PROT_WRITE, MAP_SHARED
, memfd, base_addr);
    if (mapped_base == (void *) -1) {
        printf("Can't map memory to user space.\n");
        exit(0);
    }
#ifdef DEBUG
    printf("Memory mapped at address %p.\n", mapped_base);
#endif
    return mapped_base;
}


void memory_unmap(unsigned int map_size, void *mapped_base)
{
    if (munmap(mapped_base, map_size*1024*1024) == -1) {
        printf("Can't unmap memory from user space.\n");
        exit(0);
    }
}


int load_bin(char *path, void* offset, size_t size)
{
    FILE *pb_in=fopen(path,"r");
    printf("3\n");
    unsigned int t=0;
    int addr=0;
    int numread=0;
    printf("3\n");

    if(pb_in == NULL){
        printf("Err: open %s: %s\n",__func__, path);
        return -1;
    }
    printf("3\n");

#ifdef DEBUG
    printf("loading %s @0x%x size %d, expected %d, file size:%d\n", path, (int)offset,
        (int)(size),(int)get_file_size(path) *4/9, (int)get_file_size(path));
#endif
    printf("3\n");

    if (fread(offset, sizeof(char), size, pb_in) != size){
        printf("Err: fread error, actually read: %d\n",numread);
        //mem_show(offset, 64);
        }

    printf("3\n");
    fclose(pb_in);
    printf("3\n");
    return addr;
}

static void timespec_sub(struct timespec *t1, const struct timespec *t2)
{
  assert(t1->tv_nsec >= 0);
  assert(t1->tv_nsec < 1000000000);
  assert(t2->tv_nsec >= 0);
  assert(t2->tv_nsec < 1000000000);
  t1->tv_sec -= t2->tv_sec;
  t1->tv_nsec -= t2->tv_nsec;
  if (t1->tv_nsec >= 1000000000)
  {
    t1->tv_sec++;
    t1->tv_nsec -= 1000000000;
  }
  else if (t1->tv_nsec < 0)
  {
    t1->tv_sec--;
    t1->tv_nsec += 1000000000;
  }
}


int dump(char *path, void* from, size_t size) // legacy
{
    FILE *pb_out;
    unsigned int t=0;
    int addr=0;
    pb_out=fopen(path,"wb");

    if(pb_out==NULL){
        printf("dump_ddr:open file error\n");
            return 1;
        }
    
    fwrite(from, 1, size, pb_out);


    fclose(pb_out);
    return addr;
}


int init_fpga(){

   for (int i = 0; i < 294912; ++i)
       {
         writef(mapped_ddr_base + i*4, (float)i);
       }
   for (int i = 0; i < 32768; ++i)
         {
           writef(mapped_ddr_base+ 0x200000 + i*4, (float)1 );         
         }
   for (int i = 0; i < 36864; ++i)
         {
           writef(mapped_ddr_base+ 0x600000 + i*4, (float)i/1000);
            // float res = readf(mapped_ddr_base+ 0x600000 + 4*i);
           // printf("%f\n", res);  
          
         }
   for (int i = 0; i < 32768; ++i)
         {
           writef(mapped_ddr_base+ 0x800000 + i*4, (float)i/1000);
         }



    writel(mapped_convreg_base+0x10,DDR_BASE_ADDRESS);
    writel(mapped_convreg_base+0x20,DDR_BASE_ADDRESS + 0x200000);
    writel(mapped_convreg_base+0x18,DDR_BASE_ADDRESS + 0x400000);

    if (!((readl(mapped_convreg_base) & 0x1)))
      printf("conv peripheral is ready.  Starting... \n\r");
   else {
      printf("!!! conv peripheral is not ready! Exiting...\n\r");
      exit(-1);
   }


    ////// RUN !!!!/////

    writel(mapped_convreg_base,1);


    ////// WAIT //////s
// usleep(50000);
//     while(! ((readl(mapped_reg_base) >> 1) & 0x1)) {
//         // std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     // sleep(0.5);
//       printf("22\n");
//     // if((readl(mapped_reg_base) >> 1) & 0x1){printf("11111111111\n");}
//     }
  do{
    usleep(10000);
    printf("22\n");
      }while (! ((readl(mapped_convreg_base) >> 1) & 0x1));

  for (int i = 0; i < 10; ++i)
         {  
           float res = readf(mapped_ddr_base+ 0x400000 + 4*i);
           printf("%f\n", res);         
         }



    writel(mapped_vladreg_base+0x10,DDR_BASE_ADDRESS);
    writel(mapped_vladreg_base+0x18,DDR_BASE_ADDRESS + 0x600000);
    writel(mapped_vladreg_base+0x28,DDR_BASE_ADDRESS + 0x800000);
    writel(mapped_vladreg_base+0x20,DDR_BASE_ADDRESS + 0xa00000);

    if (!((readl(mapped_vladreg_base) & 0x1)))
      printf("conv peripheral is ready.  Starting... \n\r");
   else {
      printf("!!! conv peripheral is not ready! Exiting...\n\r");
      exit(-1);
   }
    

    ////// RUN !!!!/////

    writel(mapped_vladreg_base,1);


    ////// WAIT //////s
  do{
    usleep(10000);
    printf("1111\n");
      }while (! ((readl(mapped_vladreg_base) >> 1) & 0x1));

  for (int i = 0; i < 10; ++i)
         {  
           float res = readf(mapped_ddr_base+ 0xa00000 + 4*i);
           printf("%f\n", res);         
         }


	return 0;
}

int main(){
  off_t   convreg_base = CONVREG_BASE_ADDRESS;
  off_t   vladreg_base = VLADREG_BASE_ADDRESS;
  off_t   ddr_base = DDR_BASE_ADDRESS;
    
    
  struct timespec ts_start, ts_end;

  int rc;
  memfd = open("/dev/mem", O_RDWR | O_SYNC);
  mapped_convreg_base = memory_map(1,convreg_base);
  mapped_vladreg_base = memory_map(1,vladreg_base);
  mapped_ddr_base = memory_map(1024,ddr_base);

  init_fpga();

//   rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
//   while(!readl(mapped_reg_base+0x0)){
//     usleep(1000);
//   }
//   rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);

//   writel(mapped_reg_base+0x120,0x00000002);
//   timespec_sub(&ts_end, &ts_start);
//   printf("CLOCK_MONOTONIC reports %ld.%09ld seconds t\n",
//     ts_end.tv_sec, ts_end.tv_nsec);
  

  return 0;
}
