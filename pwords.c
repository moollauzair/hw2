#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define numthreads 4
#define MAXWORD 1024

typedef struct sharedobject{
  FILE *infile;
  struct dict *sd;
  char *line;
  pthread_mutex_t lockfile;
} so_t;

typedef struct targ {
  long tid;      
  so_t *soptr;
  char *wordbuf;
} targ_t;

typedef struct dict {
  char *word;
  int count;
  struct dict *next;
} dict_t;

char *
make_word( char *word ) {
  return strcpy( malloc( strlen( word )+1 ), word );
}

dict_t *
make_dict(char *word) {
  dict_t *nd = (dict_t *) malloc( sizeof(dict_t) );
  nd->word = make_word( word );
  nd->count = 1;
  nd->next = NULL;
  return nd;
}

dict_t *
insert_word( dict_t *d, char *word ) {
  
  dict_t *nd;
  dict_t *pd = NULL;		 
  dict_t *di = d;		
  
  while(di && ( strcmp(word, di->word ) >= 0) ) { 
    if( strcmp( word, di->word ) == 0 ) { 
      di->count++;		 
      return d;			 
    }
    pd = di;			
    di = di->next;
  }
  nd = make_dict(word);		 
  nd->next = di;		 
  if (pd) {
    pd->next = nd;
    return d;			 
  }
  return nd;
}
void print_dict(dict_t *d) {
  while (d) {
    printf("[%d] %s\n", d->count, d->word);
    d = d->next;
  }
}

int
get_word( char *buf, FILE *infile) {
  int inword = 0;
  int c;  
  while( (c = fgetc(infile)) != EOF ) {
    if (inword && !isalpha(c)) {
      buf[inword] = '\0';	
      return 1;
    } 
    if (isalpha(c)) {
      buf[inword++] = c;
    }
  }
  return 0;			
}

void
*insertwords(void *arg){
  targ_t *targ = (targ_t *) arg;
  long currtid = targ->tid;   
  so_t *so = targ->soptr;
  char *wbuf = targ->wordbuf;
  int nt = 0;
  while(true){
  pthread_mutex_lock(&so->lockfile);
    if(get_word(wbuf,so->infile) == 1 ){
      so->sd = insert_word(so->sd,wbuf);
      pthread_mutex_unlock(&so->lockfile);
      nt++;
    }else{  
      pthread_mutex_unlock(&so->lockfile);
      printf("%i\n",nt);
      return;
    }
  
  }
}

int
main( int argc, char *argv[] ){
  dict_t *d = NULL;
  pthread_t insert[numthreads];
  so_t *share = malloc( sizeof(so_t) );
  FILE *infile = stdin;
  int check;
  targ_t insar[numthreads];
  char buffer[MAXWORD];
  
  if (argc >= 2) {
    infile = fopen (argv[1],"r");
  }
  if( !infile ) {
    printf("Unable to open file  %s\n",argv[1]);
    exit( EXIT_FAILURE );
  }

  share->infile = infile;
  share->sd = d;
  
  if( pthread_mutex_init( &share->lockfile, NULL ) != 0 ){
    perror("mutex init failed" );
    exit(EXIT_FAILURE);
  }

  for(int i = 0; i<numthreads; ++i){
    insar[i].tid = i;
    insar[i].soptr = share;
    insar[i].wordbuf = buffer;
    if(pthread_create(&insert[i], NULL, insertwords, &insar[i]) != 0){
    perror("Failed to create threads" );
    exit(EXIT_FAILURE);
    }
  }

  for(int j = 0; j<numthreads; j++){
    if(pthread_join(insert[j],NULL) != 0){
    perror("Failed to join threads" );
    exit(EXIT_FAILURE);
    }
  }

  print_dict(share->sd);
  fclose(infile);

  if(pthread_mutex_destroy(&share->lockfile) != 0){
    perror("Failed to destroy mutex" );
    exit(EXIT_FAILURE);
  }  

  free(share);
  
}
