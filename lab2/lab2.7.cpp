#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <regex.h>
#include <queue>
#include <unordered_set>
#include <string>
#include <iostream>
#include <chrono>

#define BUFSIZE 16384
#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }
using namespace std;
pthread_mutex_t mutex;
pthread_cond_t cond;
size_t lr = 0;
queue<char *> sq;
unordered_set<char*> check;
bool state=true;
size_t filterit(void *ptr, size_t size, size_t nmemb, char *stream)
{
  if ( (lr + size*nmemb) > BUFSIZE ) return BUFSIZE;
  memcpy(stream+lr, ptr, size*nmemb);
  lr += size*nmemb;
  return size*nmemb;
}
void write(char* cur)
{

  //cout<<cur<<" "<<pthread_self()<<endl;
  char buffer[BUFSIZE];
  CURL *curlHandle;
  curlHandle = curl_easy_init();
  curl_easy_setopt(curlHandle, CURLOPT_URL, cur);
  curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, filterit);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, buffer);
  int success = curl_easy_perform(curlHandle);
  curl_easy_cleanup(curlHandle);
  buffer[lr] = 0;
  regmatch_t amatch;
  regex_t cregex;
  regmatch_t amatch2;
  regex_t cregex2;
  regcomp(&cregex, "<a href=\"*", REG_NEWLINE);
  regcomp(&cregex2, "http*", REG_NEWLINE);
  int eflag = 0;
  char *ps=buffer;
  while(regexec(&cregex, ps, 1, &amatch, eflag)==0)
  { 

    int bi = amatch.rm_eo;
    char buf[20]="";

    for (int i=0;i<50;i++)
    {
      if(ps[bi]=='\"') break;
      buf[i]=ps[bi];
      bi++;
    }
    ps+= amatch.rm_eo+bi;
    char b[80]="";
    
    char* curr=cur;
    if(regexec(&cregex2, buf, 1, &amatch2, eflag)!=0)
    {
      int c=0;
      for(c;curr[c]!='\0';++c)
      {
        b[c]=curr[c];
      }
      b[c]='/';
      c++;

      for(int i=0;buf[i]!='\0';++i)
      {
        b[c]=buf[i];
        c++;
      }
      b[c]='\0';
    }  
    bool flag=false;

   for (auto it = check.begin(); it != check.end(); it++) 
    {

      if( strcmp(*it,b) ==0)
      {
        flag=true;
        break;
      }
    }
    if(!flag&&strcmp("",b) !=0)
     {

       sq.push(strdup(b));
    int err = pthread_cond_signal(&cond);
    if(err != 0)
      err_exit(err, "Cannot send signal");
     }

  }
    regfree(&cregex);
    check.insert(cur);
    int err = pthread_cond_signal(&cond);
    if(err != 0)
      err_exit(err, "Cannot send signal");
     

}

void* thread_job(void* arg)
{

  int err;
  int stop=0;
    while(!sq.empty())
    {
      
      err = pthread_mutex_lock(&mutex);
      if(err != 0)
        err_exit(err, "Cannot lock mutex");
      if(sq.empty())
      {
        err = pthread_cond_wait(&cond,&mutex);
        if(err != 0)
          err_exit(err, "Cannot wait");
      }

      char* cur=sq.front();
      stop++;
      sq.pop();
      err = pthread_mutex_unlock(&mutex);
      if(err != 0)
        err_exit(err, "Cannot unlock mutex");
      write(cur);
      if(stop>20)
      break;
    
  }
  return (NULL);
}
int main()
{
  for(int j=2;j<10;++j)
    {
  int err;
  pthread_t* threads = new pthread_t[j];
  err = pthread_mutex_init(&mutex, NULL);
  if(err != 0)
  {   
    std::cout<< "Cannot initialize mutex";
    return 0;
  }
  err = pthread_cond_init(&cond, NULL);
  if(err != 0)
      err_exit(err, "Cannot initialize condition variable");

      string url2 = "https://www.stroustrup.com";

      //sq.push("https://www.stroustrup.com");
      sq.push(const_cast<char*>(url2.c_str()));

        auto begin= chrono::steady_clock::now();
  for(int i = 0; i < j; i++)
    {
      
      
        // Создание потока
        err = pthread_create(&threads[i], NULL,thread_job , NULL);
        if(err != 0)
        {
            std::cout << "Cannot create a thread: " << strerror(err) << endl;
        exit(-1);
        }

    }  
    
    
    for(int i = 0; i < j; ++i)
    {
        pthread_join(threads[i], NULL);
    }
     auto end= chrono::steady_clock::now();
    auto time = chrono::duration_cast<std::chrono::microseconds>(end - begin);
    std::cout<<"потоков "<<j-1<<" time "<<time.count()<<endl;
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  }
  return 0;
}
